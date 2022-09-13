//
// TWAIN source code:
// Copyright (C) '91-'92 TWAIN Working Group:
// Aldus, Caere, Eastman-Kodak, Logitech,
// Hewlett-Packard Corporations.
// All rights reserved.
//
//
/*	Copyright ?1998 TWAIN Working Group: Adobe Systems Incorporated, 
*	Canon Information Systems, Eastman Kodak Company, 
*	Fujitsu Computer Products of America, Genoa Technology, 
*	Hewlett-Packard Company, Intel Corporation, Kofax Image Products, 
*	JFL Peripheral Solutions Inc., Ricoh Corporation, and Xerox Corporation.  
*	All rights reserved.
*/
#define GRAY_MENUS	 	// gray's menu items to prevent a second call
                        // to Acquire or Select while the first call
                        // is still pending.
                        // Including the menu gray code will introduce
                        // a requirement in your application to support
                        // a similar menu scheme.


#include <windows.h>    // Note: twain.h also REQUIRES windows defs
#include <stdio.h>		// required for wsprintf
#include <string.h>		// required for strlen

#include "twacker.h"

#include "twndebug.h"
#include "dca_glue.h"   // for function prototypes
#include "dca_app.h"    // string .RC defines
#include "res_32.h"
#include "dca_acq.h"
#include "captest.h"

// Globals to this module ONLY
static BOOL TWDSMOpen = FALSE;    // glue code flag for an Open Source Manager
static HANDLE hDSMDLL = NULL;     // handle to Source Manager for explicit load
static HWND hWnd = NULL;          // global for window handle
static TW_USERINTERFACE  twUI;	  // Fix as per Courisimault: 940518 - bd
static BOOL TWDSOpen  = FALSE;    // glue code flag for an Open Source
static BOOL TWDSEnabled  = FALSE; // glue code flag for an Open Source

// Globals
	
TW_IDENTITY appID, dsID;				// storage for App and DS (Source) states
DSMENTRYPROC lpDSM_Entry;				// entry point to the SM
TW_STATUS gGlobalStatus = {0, 0};


int TWItemSize[]= 
{ 
	sizeof(TW_INT8),
	sizeof(TW_INT16),
	sizeof(TW_INT32),
	sizeof(TW_UINT8),
	sizeof(TW_UINT16),
	sizeof(TW_UINT32),

	sizeof(TW_BOOL),
	sizeof(TW_FIX32),
	sizeof(TW_FRAME),
	sizeof(TW_STR32),
	sizeof(TW_STR64),
	sizeof(TW_STR128),
	sizeof(TW_STR255),
};

////////////////////////////////////////////////////////////////////////////
// FUNCTION: TWInitialize
//
// ARGS:    pIdentity information about the App
//          hMainWnd  handle to App main window
//
// RETURNS: none
//
// NOTES:   This simple copy to a static structure, appID, does not support
//          multiple connections. TODO, upgrade.
//
VOID TWInitialize (pTW_IDENTITY pIdentity, HWND hMainWnd)
{
	ASSERT(pIdentity);
	ASSERT(hMainWnd);

	appID = *pIdentity;    // get copy of app info
	hWnd = hMainWnd;       // get copy of app window handle
	return;
} 

////////////////////////////////////////////////////////////////////////////
// FUNCTION: TWOpenDSM
//
// ARGS:    none
//
// RETURNS: current state of the Source Manager
//
// NOTES:     1). be sure SM is not already open
//            2). explicitly load the .DLL for the Source Manager
//            3). call Source Manager to:
//                - opens/loads the SM
//                - pass the handle to the app's window to the SM
//                - get the SM assigned appID.id field
//
BOOL TWOpenDSM (VOID)
{
	TW_UINT16     twRC = TWRC_FAILURE;
	OFSTRUCT      OpenFiles;
	#define       WINDIRPATHSIZE 160
	char          WinDir[WINDIRPATHSIZE];
	TW_STR32      DSMName;

	memset(&OpenFiles, 0, sizeof(OFSTRUCT));
	memset(WinDir, 0, sizeof(char[WINDIRPATHSIZE]));
	memset(DSMName, 0, sizeof(TW_STR32));

	// debug only
	if (TWDSMOpen == TRUE)
	{
		LogMessage("DSM already open\r\n");
	}

	// Only open SM if currently closed
	if (TWDSMOpen!=TRUE)
	{
		// Open the SM, Refer explicitly to the library so we can post a nice
		// message to the the user in place of the "Insert TWAIN.DLL in drive A:"
		// posted by Windows if the SM is not found.

		GetWindowsDirectory (WinDir, WINDIRPATHSIZE);
		// Hardcode to seperate dca_main.c & dca_glue.c more completely
		// lstrcpy (DSMName, "TWAIN.DLL");
		
		// NOTE: could use hInst from elsewhere and call name in from resource
		#ifdef WIN32
			LoadString ((HINSTANCE)(GetClassLong (hWnd, GCL_HMODULE)), 
						IDS_DSMNAME,
						DSMName,
						sizeof(TW_STR32));
		#else
			LoadString (GetClassWord (hWnd, GCW_HMODULE), IDS_DSMNAME, DSMName,  sizeof(TW_STR32));
		#endif

		// 2005.07.02 
		lstrcpy (DSMName, "TWAIN_32.DLL");
		//

		if (WinDir[strlen(WinDir)-1] != '\\')
		{
			lstrcat (WinDir, "\\");
		}
		lstrcat (WinDir, DSMName);

		if ((OpenFile(WinDir, &OpenFiles, OF_EXIST) != -1) &&
				(hDSMDLL =     LoadLibrary(DSMName)) != NULL &&
				(hDSMDLL >= (HANDLE)VALID_HANDLE) &&
				(lpDSM_Entry = (DSMENTRYPROC)GetProcAddress((HMODULE)hDSMDLL, MAKEINTRESOURCE (1))) != NULL)
		{
			// This call performs four important functions:
			//  	- opens/loads the SM
			//    	- passes the handle to the app's window to the SM
			//    	- returns the SM assigned appID.id field
			//    	- be sure to test the return code for SUCCESSful open of SM


			twRC = CallDSMEntry(&appID,
								NULL,
								DG_CONTROL,
								DAT_PARENT,
								MSG_OPENDSM,
								(TW_MEMREF)&hWnd);

			switch (twRC)
			{
				case TWRC_SUCCESS:
					// Needed for house keeping.  Do single open and do not
					// close SM which is not already open ....
					TWDSMOpen = TRUE;
					if (MessageLevel() >= ML_FULL)
					{
						ShowRC_CC(hWnd, 0, 0, 0, 
									"Source Manager was Opened successfully", 
									"TWAIN Information");
					}
					break;

				case TWRC_FAILURE:
					LogMessage("OpenDSM failure\r\n");
				default:
					// Trouble opening the SM, inform the user
					TWDSMOpen = FALSE;
					if (MessageLevel() >= ML_ERROR)                
					{
						ShowRC_CC(hWnd, 1, twRC, 0,	//Source Manager
									"",
									"DG_CONTROL/DAT_PARENT/MSG_OPENDSM");
					}
					break;
			}
		}
		else
		{
			if (MessageLevel() >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Error in Open, LoadLibrary, or GetProcAddress.\nTwain DLL may not exist.",
							#ifdef WIN32
								"TWAIN_32.DLL");
							#else
								"TWAIN.DLL");
							#endif
			}
		}
	}  
	// Let the caller know what happened
	return (TWDSMOpen);
} 

////////////////////////////////////////////////////////////////////
// FUNCTION: TWCloseDSM
//
// ARGS:    none
//
// RETURNS: current state of Source Manager
//
// NOTES:    1). be sure SM is already open
//           2). call Source Manager to:
//               - request closure of the Source identified by appID info
//
//
BOOL TWCloseDSM (HANDLE bitmap)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	char buffer[80];

	memset(buffer, 0, sizeof(char[80]));

	if (!TWDSMOpen)
	{
		if (MessageLevel()  >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Close Source Manager\nSource Manager Not Open", 
						"Sequence Error");
		}
	}
	else
	{
		if (TWDSOpen==TRUE)
		{
			if (MessageLevel()  >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"A Source is Currently Open", "Cannot Close Source Manager");
			}
		}
		else
		{
			// Only close something which is already open
			if (TWDSMOpen==TRUE)
			{
				// This call performs one important function:
				// - tells the SM which application, appID.id, is requesting SM to close
				// - be sure to test return code, failure indicates SM did not close !!

			twRC = CallDSMEntry(&appID,
								NULL,
								DG_CONTROL,
								DAT_PARENT,
								MSG_CLOSEDSM,
								&hWnd);

				if (twRC != TWRC_SUCCESS)
				{
					// Trouble closing the SM, inform the user
					if (MessageLevel() >= ML_ERROR)
					{
						ShowRC_CC(hWnd, 1, twRC, 0,
									"",
									"DG_CONTROL/DAT_PARENT/MSG_CLOSEDSM");
					}

					wsprintf(buffer,"CloseDSM failure -- twRC = %d\r\n",twRC);
					LogMessage(buffer);
				}
				else
				{
					TWDSMOpen = FALSE;
					// Explicitly free the SM library
					if (hDSMDLL)
					{        
						FreeLibrary ((HMODULE)hDSMDLL);
						hDSMDLL=NULL;
						// the data source id will no longer be valid after
						// twain is killed.  If the id is left around the
						// data source can not be found or opened
						dsID.Id = 0;	// dglee
					}
					if (MessageLevel() >= ML_FULL)
					{
						ShowRC_CC(hWnd, 0, 0, 0,
									"Source Manager was Closed successfully", 
									"TWAIN Information");
					}
				}
			}
		}
	}
	// Let the caller know what happened
	return (twRC==TWRC_SUCCESS);
} // TWCloseDSM

////////////////////////////////////////////////////////////////////////////
// FUNCTION: TWIsDSMOpen
//
// ARGS:    none
//
// RETURNS: current state of Source Manager (open/closed)
//
// NOTES:   Just a way to reduce the number of global vars and keep the
//          state of SM information local to this module.  Let the caller,
//          app, know current state of the SM.
//
BOOL TWIsDSMOpen (VOID)
{
	return (TWDSMOpen);
}

/////////////////////////////////////////////////////////////////////////
// FUNCTION: TWOpenDS
//
// ARGS:    none
//
// RETURNS: current state of select Source
//
// NOTES:    1). only attempt to open a source if it is currently closed
//           2). call Source Manager to:
//                - open the Source indicated by info in dsID
//                - SM will fill in the unique .Id field
//
BOOL TWOpenDS (VOID)
{
	TW_UINT16 twRC = TWRC_FAILURE;

	if (TWDSMOpen==FALSE)
	{
		LogMessage("DSM not open - cannot open DS\r\n");

		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Open Source\nSource Manager not Open",
						"TWAIN Error");
		}
	}
	else
	{
		//Source Manager is open
		if (TWDSOpen==TRUE)
		{
			LogMessage("Source already open\r\n");

			if (MessageLevel() >= ML_FULL)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Source is already open", "TWAIN Information");
			}
		}
		else
		{
			// This will open the Source.
			twRC = CallDSMEntry(&appID,
								NULL,
							DG_CONTROL,
							DAT_IDENTITY,
							MSG_OPENDS,
							&dsID);

			switch (twRC)
			{
				case TWRC_SUCCESS:
					LogMessage("OpenDS success\r\n");

					// do not change flag unless we successfully open
					TWDSOpen = TRUE;
					if (MessageLevel() >= ML_INFO)
					{
						ShowTW_ID(hWnd, dsID,
									"DG_CONTROL/DAT_IDENTITY/MSG_OPENDS TWRC_SUCCESS");
					}
					break;

				case TWRC_FAILURE:
					LogMessage("OpenDS failure\r\n");

					TW_STATUS twStatus;
					twRC = CallDSMEntry(&appID,
										NULL,
									DG_CONTROL,
									DAT_STATUS,
									MSG_GET,
									&twStatus);

					// Trouble opening the Source
					//Determine Condition Code
					if (MessageLevel()  >= ML_ERROR)                    
					{
						ShowRC_CC(hWnd, 1, twRC, 0, //Source Manager RC
									"",
									"DG_CONTROL/DAT_IDENTITY/MSG_OPENDS");
					}

					// this is a patch required in the event that TWAcquire
					// fails due to bad or non-existant during run of a
					// autotest. The tests are contnued via the WM_PAINT msg
					//InvalidateRect(NULL,NULL,TRUE);
					break;

				default:
					if (MessageLevel()  >= ML_ERROR)
					{
						ShowRC_CC(hWnd, 0, 0, 0,
									"Unknown Return Code",
									"DG_CONTROL/DAT_IDENTITY/MSG_OPENDS");
					}
					break;
			}
		}
	}
	return TWDSOpen;
} 

///////////////////////////////////////////////////////////////////////////
// FUNCTION: TWCloseDS
//
// ARGS:    none
//
// RETURNS: none
//
// NOTES:    1). only attempt to close an open Source
//           2). call Source Manager to:
//                - ask that Source identified by info in dsID close itself
//
BOOL TWCloseDS (VOID)
{
	TW_UINT16 twRC = TWRC_FAILURE;

	if (!TWDSOpen)
	{
		if (MessageLevel()  >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Close Source\nSource Not Open", 
						"Sequence Error");
		}
	}
	else
	{
		if (TWDSEnabled == TRUE)
		{
			if (MessageLevel()  >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Source is Currently Enabled", "Cannot Close Source");
			}
		}
		else
		{
			if (TWDSOpen==TRUE)
			{
				// Close an open Source
				twRC = CallDSMEntry(&appID,
								NULL,
								DG_CONTROL,
								DAT_IDENTITY,
								MSG_CLOSEDS,
								&dsID);

				LogMessage("CloseDS\r\n");

				// show error on close
				if (twRC!= TWRC_SUCCESS) 
				{
					if (MessageLevel() >= ML_ERROR)
					{
						ShowRC_CC(hWnd, 1, twRC, 0,
									"",
									"DG_CONTROL/DAT_IDENTITY/MSG_CLOSEDS");
					}
				} 
				else 
				{
					TWDSOpen = FALSE;
					GreyMenu (FALSE);
					dsID.Id = 0;			
					dsID.ProductName[0] = 0;
					if (MessageLevel() >= ML_FULL)
					{
						ShowRC_CC(hWnd, 0, 0, 0,
									"Source was Closed successfully", 
									"TWAIN Information");
					}
				}

			}
		}
	}
	return(twRC==TWRC_SUCCESS);
} // TWCloseDS

////////////////////////////////////////////////////////////////////////
// FUNCTION: TWEnableDS
//
// ARGS:    none
//
// RETURNS: BOOL for TRUE=open; FALSE=not open/fail
//
// NOTES:    1). only enable an open Source
//           2). call the Source Manager to:
//                - bring up the Source's User Interface
//
BOOL TWEnableDS (TW_BOOL Show)
{
	BOOL Result = FALSE;
	TW_UINT16 twRC = TWRC_FAILURE;

	if (TWDSOpen==FALSE)
	{
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Enable Source\nNo Source Open", 
						"TWAIN Error");
		}
	}
	else
	{	
		// only enable open Source's
		if (TWDSEnabled==TRUE)	//Source is alredy enabled
		{
			if (MessageLevel() >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Cannot Enable Source, already enabled", 
							"TWAIN Error");
			}
		}
		else
		{
			// grey out file menu items
			GreyMenu (TRUE);

			// This will display the Source User Interface. The Source should only display
			// a user interface that is compatible with the group defined
			// by appID.SupportedGroups (in our case DG_IMAGE | DG_CONTROL)
			memset(&twUI, 0, sizeof(TW_USERINTERFACE));
			twUI.hParent = hWnd;
			twUI.ShowUI  = Show;

			twRC = CallDSMEntry(&appID,
						&dsID,
						DG_CONTROL,
						DAT_USERINTERFACE,
						MSG_ENABLEDS,
						(TW_MEMREF)&twUI);

			if (twRC!=TWRC_SUCCESS)
			{
				if (MessageLevel() >= ML_ERROR)
				{
					ShowRC_CC(hWnd, 1, twRC, 1,
								"",
								"DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS");
				}
			}
			else
			{
				Result = TRUE;
				TWDSEnabled = TRUE;
				if (MessageLevel() >= ML_FULL)
				{
					if(twUI.ShowUI == TRUE)
					{
						ShowRC_CC(hWnd, 0, 0, 0,
									"Source Enabled",
									"DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS");
					}
				}
			}
		}
	}
	return (Result);
} 

/*
* Function: TWEnableDSUIOnly
* Author: TWAIN Working Group
* Date: July 1998
* Input: None
* Output:
*		BOOL for TRUE=open; FALSE=not open/fail
* Comments:
*		1). only enable an open Source
*		2). call the Source Manager to:
*			- bring up the Source's User Interface
*/
BOOL TWEnableDSUIOnly()
{
	BOOL Result = FALSE;
	TW_UINT16 twRC = TWRC_FAILURE;

	if (TWDSOpen==FALSE)
	{
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Enable Source\nNo Source Open", 
						"TWAIN Error");
		}
	}
	else
	{	
		/*
		* only enable open Source's
		*/
		if (TWDSEnabled==TRUE)	// Source is alredy enabled
		{
			if (MessageLevel() >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Cannot Enable Source, already enabled", 
							"TWAIN Error");
			}
		}
		else
		{
			/*
			* grey out file menu items
			*/
			GreyMenu (TRUE);

			/*
			* This will display the Source User Interface. The Source should only display
			* a user interface that is compatible with the group defined
			* by appID.SupportedGroups (in our case DG_IMAGE | DG_CONTROL)
			*/
			memset(&twUI, 0, sizeof(TW_USERINTERFACE));
			twUI.hParent = hWnd;
			twUI.ShowUI  = TRUE;

			twRC = CallDSMEntry(&appID,
						&dsID,
						DG_CONTROL,
						DAT_USERINTERFACE,
						MSG_ENABLEDSUIONLY,
						(TW_MEMREF)&twUI);

			if (twRC!=TWRC_SUCCESS)
			{
				if (MessageLevel() >= ML_ERROR)
				{
					ShowRC_CC(hWnd, 1, twRC, 1,
								"",
								"DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY");
				}
			}
			else
			{
				Result = TRUE;
				TWDSEnabled = TRUE;
				if (MessageLevel() >= ML_FULL)
				{
					ShowRC_CC(hWnd, 0, 0, 0,
								"Source Enabled",
								"DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY");
				}
			}
		}
	}
	return (Result);
} 

////////////////////////////////////////////////////////////////////////////
// FUNCTION: TWDisableDS
//
// ARGS:    none
//
// RETURNS: twRC
//
// NOTES:    1). only disable an open Source
//           2). call Source Manager to:
//                - ask Source to hide it's User Interface
//
BOOL TWDisableDS (VOID)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	TW_USERINTERFACE twUI;

	memset(&twUI, 0, sizeof(TW_USERINTERFACE));

	LogMessage("Disable DS\r\n");

	// only disable enabled Source's
	if (TWDSEnabled!=TRUE)
	{
		if (MessageLevel()  >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"Cannot Disable Source\nSource Not Enabled", 
						"Sequence Error");
		}
	}
	else
	{
		twUI.hParent = hWnd;
		twUI.ShowUI = TWON_DONTCARE8;

		twRC = CallDSMEntry(&appID,
					&dsID,
					DG_CONTROL,
					DAT_USERINTERFACE,
					MSG_DISABLEDS,
					(TW_MEMREF)&twUI);

		if (twRC == TWRC_SUCCESS)
		{   
			LogMessage("DS Disabled\r\n");

			TWDSEnabled = FALSE;
			if (MessageLevel() >= ML_FULL)
			{
				ShowRC_CC(hWnd, 0, 0, 0,
							"Source Disabled",
							"DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS");
			}
		}
		else
		{
			if (MessageLevel() >= ML_ERROR)
			{
				ShowRC_CC(hWnd, 1, twRC, 1,
							"",
							"DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS");
			}
		}
	}    	
	return (twRC==TWRC_SUCCESS);
} 

/////////////////////////////////////////////////////////////////////////
// FUNCTION: TWIsDSOpen
//
// ARGS:    none
//
// RETURNS: current state of the Source
//
// NOTES:    Just a way to reduce the number of global vars and keep the
//           state of Source information local to this module.  Let the caller,
//           app, know current state of the Source.
//
BOOL TWIsDSOpen (VOID)
{
	return (TWDSOpen);
} 

///////////////////////////////////////////////////////////////////////////
// FUNCTION: TWIsDSEnabled
//
// ARGS:    none
//
// RETURNS: current state of Source (Enabled?)
//
// NOTES:   Just a way to reduce the number of global vars and keep the
//          state of SM information local to this module.  Let the caller,
//          app, know current state of the SM.
//
BOOL TWIsDSEnabled (VOID)
{
	return (TWDSEnabled);
}

//////////////////////////////////////////////////////////////////////////
// FUNCTION: TWSelectDS
//
// ARGS:    none
//
// RETURNS: twRC TWAIN status return code
//
// NOTES:   1). call the Source Manager to:
//              - have the SM put up a list of the available Sources
//              - get information about the user selected Source from
//                NewDSIdentity, filled by Source
//
int TWSelectDS (VOID)
{
	int rc = TWRC_FAILURE;
	TW_IDENTITY NewDSIdentity;

	memset(&NewDSIdentity, 0, sizeof(TW_IDENTITY));

	if (TWDSOpen)
	{
		LogMessage("TWSelectDS -- source already open\r\n");

		//A Source is already open
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"A Source is already open\nClose Source before Selecting a New Source",
						"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
		}
		rc = TWRC_FAILURE;
	}
	else
	{
		// I will settle for the system default.  Shouldn't I get a highlight
		// on system default without this call?

		rc = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETDEFAULT,
					(TW_MEMREF)&NewDSIdentity);

		// This call performs one important function:
		// - should cause SM to put up dialog box of available Source's
		// - tells the SM which application, appID.id, is requesting, REQUIRED
		// - returns the SM assigned NewDSIdentity.id field, you check if changed
		//  (needed to talk to a particular Data Source)
		// - be sure to test return code, failure indicates SM did not close !!
		//
		rc = CallDSMEntry(&appID,
						NULL,
						DG_CONTROL,
						DAT_IDENTITY,
						MSG_USERSELECT,
						(TW_MEMREF)&NewDSIdentity);

		// Check if the user changed the Source and react as apporpriate.
		// - TWRC_SUCCESS, log in new Source
		// - TWRC_CANCEL,  keep the current Source
		// - default,      check down the codes in a status message, display result
		//
 
		switch (rc)
		{
			case TWRC_SUCCESS:
				dsID = NewDSIdentity; 
				if (MessageLevel() >= ML_INFO)
				{
					ShowTW_ID(hWnd, dsID,
								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
				}
				break;
			case TWRC_CANCEL:
				if (MessageLevel() >= ML_INFO)
				{
					ShowRC_CC(hWnd, 1, rc, 0,
								"",
								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
				}
				break;
			case TWRC_FAILURE:	        
			default:
				if (MessageLevel() >= ML_ERROR)
				{
					ShowRC_CC(hWnd, 1, rc, 0,
								"",
								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
				}
				break;
		}
	}
	return rc;
}  // TWSelectDS

int TWStringDS (const char* szProductName)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	TW_IDENTITY NewDSIdentity;
	int myRC = TWCRC_SUCCESS;
	
	memset(&NewDSIdentity, 0, sizeof(TW_IDENTITY));
	
	if (TWDSOpen)
	{
		LogMessage("TWStringDS -- source already open\r\n");
		
		//A Source is already open
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
				"A Source is already open\nClose Source before Selecting a New Source",
				"TWStringDS");
		}
		twRC = TWRC_FAILURE;
	}
	else
	{
		// I will settle for the system default.  Shouldn't I get a highlight
		// on system default without this call?
		
		twRC = CallDSMEntry(&appID,
			NULL,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_GETDEFAULT,
			(TW_MEMREF)&NewDSIdentity);
		
		twRC = CallDSMEntry(&appID,
			NULL,
			DG_CONTROL,
			DAT_IDENTITY,
			MSG_GETFIRST,
			(TW_MEMREF)&NewDSIdentity);
		switch(twRC)
		{
		case TWRC_SUCCESS:
			// 최소한 하나의 리스트가 있다.
			while(1)
			{
				if(!strncmp(NewDSIdentity.ProductName, szProductName, strlen(szProductName)))		// dglee, 090831, USB 포트에 따른 #number 무시
				{
					// 스트링을 찾았다. 찾는 장치가 등록되어 있다. (연결이 되어있는지는 모름)
					dsID = NewDSIdentity;
					break;
				}
				twRC = CallDSMEntry(&appID,	NULL, DG_CONTROL, DAT_IDENTITY,	MSG_GETNEXT, (TW_MEMREF)&NewDSIdentity);
				if(twRC == TWRC_ENDOFLIST)
				{
					// 찾는 스트링이 없거나 장치 이름이 틀렸다.
					myRC = TWCRC_WRONG_DEVICE;
					// 					goto Return_TWOpenSource;
					break;
				}
			}
			break;
		case TWRC_FAILURE:
			// 사용 가능한 twain 장치가 하나도 없다.
			myRC = TWCRC_NO_DEVICE;
			// 			goto Return_TWOpenSource;
			break;
		default:
			break;
		}
		
		switch (myRC)
		{
		case TWCRC_SUCCESS:
			dsID = NewDSIdentity; 
			break;

		case TWCRC_WRONG_DEVICE:
			break;

		case TWCRC_NO_DEVICE:	
			break;

		default:
			break;
		}
	}
	return (myRC);
}  // TWStringDS



BOOL TWOpenSource(const char* szProductName, BOOL bShow)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	int myRC = TWCRC_SUCCESS;

	TW_IDENTITY NewDSIdentity;

	memset(&NewDSIdentity, 0, sizeof(TW_IDENTITY));

	if (TWDSOpen)
	{
		LogMessage("TWSelectDS -- source already open\r\n");

		//A Source is already open
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"A Source is already open\nClose Source before Selecting a New Source",
						"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
		}
		twRC = TWRC_FAILURE;
	}
	else
	{
		// I will settle for the system default.  Shouldn't I get a highlight
		// on system default without this call?

		twRC = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETDEFAULT,
					(TW_MEMREF)&NewDSIdentity);

		twRC = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETFIRST,
					(TW_MEMREF)&NewDSIdentity);
		switch(twRC)
		{
		case TWRC_SUCCESS:
			// 최소한 하나의 리스트가 있다.
			while(1)
			{
				if(!strcmp(NewDSIdentity.ProductName, szProductName))
				{
					// 스트링을 찾았다. 찾는 장치가 등록되어 있다. (연결이 되어있는지는 모름)
					dsID = NewDSIdentity;
					break;
				}
				twRC = CallDSMEntry(&appID,	NULL, DG_CONTROL, DAT_IDENTITY,	MSG_GETNEXT, (TW_MEMREF)&NewDSIdentity);
				if(twRC == TWRC_ENDOFLIST)
				{
					// 찾는 스트링이 없거나 장치 이름이 틀렸다.
					myRC = TWCRC_WRONG_DEVICE;
					goto Return_TWOpenSource;
					break;
				}
			}
			break;
		case TWRC_FAILURE:
			// 사용 가능한 twain 장치가 하나도 없다.
			myRC = TWCRC_NO_DEVICE;
			goto Return_TWOpenSource;
			break;
		default:
			break;
		}

		TWOpenDS();

		if (TWIsDSOpen())
		{
			if(bShow)
				twRC = TWAcquire(hWnd, 8, 3);
			else
				twRC = TWAcquire(hWnd, 0, 1);
			
			if(twRC == TWRC_SUCCESS)
				myRC = TWCRC_SUCCESS;
			else
			{
				myRC = TWCRC_ACQ_FAILURE;
				goto Return_TWOpenSource;
			}
		}
		else
		{
			myRC = TWCRC_DEVICE_OFF;
			goto Return_TWOpenSource;
		}
		
	}

Return_TWOpenSource:
	return (myRC);
} 



/*
BOOL TWSelectDS_ByString (const char* szProductName, const char* szDSName)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	TW_IDENTITY NewDSIdentity;

	memset(&NewDSIdentity, 0, sizeof(TW_IDENTITY));

	if (TWDSOpen)
	{
		LogMessage("TWSelectDS -- source already open\r\n");

		//A Source is already open
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"A Source is already open\nClose Source before Selecting a New Source",
						"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
		}
		twRC = TWRC_FAILURE;
	}
	else
	{
		// I will settle for the system default.  Shouldn't I get a highlight
		// on system default without this call?

		twRC = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETFIRST,
					(TW_MEMREF)&NewDSIdentity);
		bool bDeviceExist=false;
		switch(twRC)
		{
		case TWRC_SUCCESS:
			// 최소한 하나의 리스트가 있다.
			while(1)
			{
				if(!strcmp(NewDSIdentity.ProductName, szProductName))
				{
					// 스트링을 찾았다. 찾는 장치가 등록되어 있다. (연결이 되어있는지는 모름)
					bDeviceExist = true;
					break;
				}
				twRC = CallDSMEntry(&appID,	NULL, DG_CONTROL, DAT_IDENTITY,	MSG_GETNEXT, (TW_MEMREF)&NewDSIdentity);
				if(twRC == TWRC_ENDOFLIST)
				{
					// 찾는 스트링이 없거나 장치 이름이 틀렸다.
					break;
				}
			}
			break;
		case TWRC_FAILURE:
			// 사용 가능한 twain 장치가 하나도 없다.
			break;
		default:
			break;
		}


		if(bDeviceExist)
		{
			// (기본 값)
			long regRC;
			const char* szSubKey = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Twain";
			regRC = RegSetValue(HKEY_CURRENT_USER, szSubKey, REG_SZ, szDSName, strlen(szDSName));
			
			// 기본 원본, Default Source
			HKEY h_key; 
			int ret = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_SET_VALUE, &h_key);
			if(ret == ERROR_SUCCESS){
				RegSetValueEx(h_key, "기본 원본", 0, REG_SZ, (const unsigned char*)szDSName, strlen(szDSName));
				RegSetValueEx(h_key, "Default Source", 0, REG_SZ, (const unsigned char*)szDSName, strlen(szDSName));
			}
			if(h_key != NULL) RegCloseKey(h_key);
		}


// 		switch (twRC)
// 		{
// 			case TWRC_SUCCESS:
// 				dsID = NewDSIdentity; 
// 				if (MessageLevel() >= ML_INFO)
// 				{
// 					ShowTW_ID(hWnd, dsID,
// 								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
// 				}
// 				break;
// 			case TWRC_CANCEL:
// 				if (MessageLevel() >= ML_INFO)
// 				{
// 					ShowRC_CC(hWnd, 1, twRC, 0,
// 								"",
// 								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
// 				}
// 				break;
// 			case TWRC_FAILURE:	        
// 			default:
// 				if (MessageLevel() >= ML_ERROR)
// 				{
// 					ShowRC_CC(hWnd, 1, twRC, 0,
// 								"",
// 								"DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT");
// 				}
// 				break;
// 		}
	}
	return (twRC);
}  // TWSelectDS
*/
////////////////////////////////////////////////////////////////////////////
// FUNCTION: GreyMenu
//
// ARGS:    flag    desired new state for flags
//
// RETURNS: none
//
// NOTE:    one important "feature" of this routine is that by graying
//          the menu selections it locks out double hits on them by the user
//          while I am dealing with the first hit.
//
//          Including the menu gray code will introduce a requirement in
//          your application to support a similar menu scheme.
//
VOID GreyMenu (int flag)
{
	#ifdef GRAY_MENUS
		// Includes for my the App specific menu labels
		#include "dca_app.h"
		HMENU hmenu = NULL;

		hmenu = GetMenu(hWnd);
	#endif

	if (flag==TRUE)
	{
		#ifdef GRAY_MENUS
			EnableMenuItem (hmenu, TW_APP_ACQUIRE, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem (hmenu, TW_APP_SETUP, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem (hmenu, TW_APP_SELECT_SOURCE, MF_BYCOMMAND | MF_GRAYED);
		#endif
	}
	else
	{
		#ifdef GRAY_MENUS
			EnableMenuItem (hmenu, TW_APP_ACQUIRE, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem (hmenu, TW_APP_SETUP, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem (hmenu, TW_APP_SELECT_SOURCE, MF_BYCOMMAND | MF_ENABLED);
		#endif
	}
	return;
} // GreyMenu

/*
* Fucntion: CallDSMEntry
* Author:	Nancy Letourneau / J.F.L. Peripherals Inc.
* Input:  
*		Function - 
*		pApp - 
*		pSrc - 
*		DG -
*		DAT -
*		MSG -
*		pData -
* Output: 
*		TW_UINT16 - Value of Item field of container. 
* Comments:
*
*/
TW_UINT16 CallDSMEntry(pTW_IDENTITY pApp, pTW_IDENTITY pSrc,
										TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
{ 
	TW_UINT16 twRC = (*lpDSM_Entry)(pApp, pSrc, DG, DAT, MSG, pData);

	if((twRC != TWRC_SUCCESS)&&(DAT!=DAT_EVENT))
	{
		VERIFY((*lpDSM_Entry)(pApp, pSrc, DG_CONTROL, DAT_STATUS, MSG_GET, 
					(TW_MEMREF)&gGlobalStatus) == TWRC_SUCCESS);
		TRACE("CallDSMEntry function: call failed with RC = %d, CC = %d.\n", 
					twRC, gGlobalStatus.ConditionCode);
	}
	return twRC;
}

int TWStringSource(const char* szProductName)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	int myRC = TWCRC_SUCCESS;

	TW_IDENTITY NewDSIdentity;

	memset(&NewDSIdentity, 0, sizeof(TW_IDENTITY));

	if (TWIsDSOpen())
	{
		LogMessage("TWStringSource -- source already open\r\n");

		//A Source is already open
		if (MessageLevel() >= ML_ERROR)
		{
			ShowRC_CC(hWnd, 0, 0, 0,
						"A Source is already open\nClose Source before Selecting a New Source",
						"TWStringSource");
		}
		twRC = TWRC_FAILURE;
	}
	else
	{
		// I will settle for the system default.  Shouldn't I get a highlight
		// on system default without this call?

		twRC = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETDEFAULT,
					(TW_MEMREF)&NewDSIdentity);

		twRC = CallDSMEntry(&appID,
					NULL,
					DG_CONTROL,
					DAT_IDENTITY,
					MSG_GETFIRST,
					(TW_MEMREF)&NewDSIdentity);
		switch(twRC)
		{
		case TWRC_SUCCESS:
			// 최소한 하나의 리스트가 있다.
			while(1)
			{
				if(!strcmp(NewDSIdentity.ProductName, szProductName))
				{
					// 스트링을 찾았다. 찾는 장치가 등록되어 있다. (연결이 되어있는지는 모름)
					dsID = NewDSIdentity;
					break;
				}
				twRC = CallDSMEntry(&appID,	NULL, DG_CONTROL, DAT_IDENTITY,	MSG_GETNEXT, (TW_MEMREF)&NewDSIdentity);
				if(twRC == TWRC_ENDOFLIST)
				{
					// 찾는 스트링이 없거나 장치 이름이 틀렸다.
					myRC = TWCRC_WRONG_DEVICE;
					goto Return_TWStringSource;
					break;
				}
			}
			break;
		case TWRC_FAILURE:
			// 사용 가능한 twain 장치가 하나도 없다.
			myRC = TWCRC_NO_DEVICE;
			goto Return_TWStringSource;
			break;
		default:
			break;
		}
	}

Return_TWStringSource:
	return myRC;
} 
