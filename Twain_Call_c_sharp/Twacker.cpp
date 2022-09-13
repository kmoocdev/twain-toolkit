// File:         twacker.c
// RCS:          $Header: $
// Description:  Exerciser application program for Twain
// Author:       TWAIN Working Group
// Created:      Jan 15,92
// Modified:     "
// Language:     C
// Package:      N/A
// Status:       Test tool
//
// (c) Copyright 1992, Hewlett-Packard Company, all rights reserved.
//
/*
*	Copyright ?1998 TWAIN Working Group: Adobe Systems Incorporated, 
*	Canon Information Systems, Eastman Kodak Company, 
*	Fujitsu Computer Products of America, Genoa Technology, 
*	Hewlett-Packard Company, Intel Corporation, Kofax Image Products, 
*	JFL Peripheral Solutions Inc., Ricoh Corporation, and Xerox Corporation.  
*	All rights reserved.
*/
//----------------------------------------------------------------------
//                            I n c l u d e s
//----------------------------------------------------------------------

//  This is a TWAIN sample application.  This simple application
//  will display .BMP files into a dynamically resized window.  The handle
//  to the .BMP file is passed to this application via TWAIN protocol
//  from the underlying sample data source.
//
//  The application was written assuming the use of MS Windows version 3.0.
//  It further assumes operation in Standard or Enhanced mode, no attempt
//  has been made to fit it into Real mode.

#include <windows.h>         // Req. for twain.h type defs and ...
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "twacker.h"

#include "twndebug.h"
// #include "UsbBsTw.h"

#ifdef WIN32
#include "res_32.h"
#else
#include "res_16.h"
#endif

#include "dca_glue.h"        // for function prototypes of glue code
#include "dca_type.h"        // contains function protos for this module
#include "dca_acq.h"         // contains buffered glue code function prototypes
#include "dca_app.h"
#include "captest.h"

//  SDH - 01/30/95: NULL under C with this compiler is void* not 0 which causes a
//  compiler warning.  Rather than change every line, we kludge this here.
#ifdef NULL
#undef NULL                      
#endif
#define NULL 0

//----------------------------------------------------------------------
//                            V a r i a b l e s
//----------------------------------------------------------------------

// Global variables
HWND hMainWnd = NULL;							// main app window handle
static HBITMAP hbm = NULL;				// holder handle to bit map passed from Source
static HPALETTE hDibPal = NULL;		// handle to palette
char MainTitle[40];
BOOL CapSupportedCaps = FALSE;
//BOOL done = FALSE;
int nPosition = 0;					/* Keep the scrollbar position */
BOOL	bSelection = TRUE;		/* To know the scrollbar in use TRUE for VSCROLL */
POINT	Point = {0,0};				/* Coordinate for the upper-left corner */
POINT PtV = {0,0}, PtH = {0,0};
RECT Rect, rc;
SCROLLINFO sbInfoV, sbInfoH;
BOOL bHideV = FALSE;
BOOL bHideH = FALSE;

//----------------------------------------------------------------------
//                            F u n c t i o n s
//----------------------------------------------------------------------

BOOL InitInstanceAppId(HANDLE hWnd)
{
	TW_IDENTITY AppIdentity;

	// Set up the information about your application which you want to pass to
	// the SM in this call.
	//
	// Move all these text strings off to a resource fill for a real application.
	// They are here for easier readablilty for the student.

	//TWAIN initialization
	AppIdentity.Id = 0; 				// init to 0, but Source Manager will assign real value
	AppIdentity.Version.MajorNum = 1;
	AppIdentity.Version.MinorNum = 703;
	AppIdentity.Version.Language = TWLG_USA;
	AppIdentity.Version.Country  = TWCY_USA;
	#ifdef WIN32
		lstrcpy (AppIdentity.Version.Info,  "TWAIN_32 Twacker 1.7.0.3  01/18/1999");
		lstrcpy (AppIdentity.ProductName,   "TWACKER_32");
	#else
		lstrcpy (AppIdentity.Version.Info,  "TWAIN Twacker 1.7.0.3  01/18/1999");
		lstrcpy (AppIdentity.ProductName,   "TWACKER_16");
	#endif

	AppIdentity.ProtocolMajor = 1;//TWON_PROTOCOLMAJOR;
	AppIdentity.ProtocolMinor = 7;//TWON_PROTOCOLMINOR;
	AppIdentity.SupportedGroups =  DG_IMAGE | DG_CONTROL;
	lstrcpy (AppIdentity.Manufacturer,  "TWAIN Working Group");
	lstrcpy (AppIdentity.ProductFamily, "TWAIN Toolkit");

	// pass app particulars to glue code
	TWInitialize (&AppIdentity, (HWND)hWnd);

	hMainWnd = (HWND)hWnd;

	return (TRUE);
}

/*
* Function: CloseApplication
* Author: Nancy L?ourneau / J.F.L. Peripheral Solutions Inc. / TWAIN Working Group
* Date: May 25/1998
* Input: 
*		hWnd - handle to main app window
* Output:
*		TRUE  if successful closing
* Comment:
*		Close the application but make sure that the Source and the 
*		Source Manager already close.
*/
BOOL CloseApplication(HWND hWnd)
{
	int Message = 0;

	ASSERT(hWnd);

	/* 
	* if the source is enable 
	*/
	if (TWIsDSEnabled())
	{
// 		Message = MessageBox (NULL, "소스를 비활성화 하고, \n\r소스와 스소 매니저를 닫으시겠습니까?",
// 								"Close message", MB_OKCANCEL);
		Message = IDOK;				// dglee 메시지 박스 제거
		if (Message == IDOK)
		{
			if (TWDisableDS())
				if (TWCloseDS())
					TWCloseDSM(NULL);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	/*
	* if the source is open
	*/
	if (TWIsDSOpen())
	{
// 		Message = MessageBox (NULL, "소스와 소스 매니저를 닫으시겠습니까?",
// 								"Close message", MB_OKCANCEL);
		Message = IDOK;				// dglee 메시지 박스 제거
		if (Message == IDOK)
		{
			if (TWCloseDS())
				TWCloseDSM(NULL);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	/*
	* if the source manager is open
	*/
	if (TWIsDSMOpen())
	{
// 		Message = MessageBox (NULL, "소스 매니저를 닫으시겠습니까?",
// 								"Close message", MB_OKCANCEL);
		Message = IDOK;				// dglee 메시지 박스 제거
		if (Message == IDOK)
		{
			TWCloseDSM(NULL);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	/*
	* if nothing is open
	*/
	if (!TWIsDSEnabled() & !TWIsDSOpen() & !TWIsDSMOpen())
	{
		return TRUE;
	}

	return FALSE;
}

/*
* Function: InitTwainCombo
* Author:	TWAIN Working Group
* Input: 
*			hDlg - Handle for the Dialog 
*			Id - Id for Combo box reference.
*			pTable - Contain the new list of items for each combo box.
*			nSizeTable - Maximum of items in each list.
* Output:
*			Return 1 when all is set.
* Comments:
*			Sets the combo box values for the special menu dialog.
*/
TW_INT16 InitTwainCombo(HWND hDlg, TW_INT16 Id, 
												pTABLEENTRY pTable, int nSizeTable)
{
	TW_INT16 result = 0;
	int i = 0;

	ASSERT(hDlg);
	ASSERT(pTable);

	/*
	* Reset old content, prepare to replace with new list
	*/
	SendDlgItemMessage(hDlg, Id, CB_RESETCONTENT, 0, 0);
	for (i = 0; i < nSizeTable; i++)
	{
		int nIndex = SendDlgItemMessage(hDlg, Id, CB_ADDSTRING , 0, 
											(LPARAM)(pTABLEENTRY)pTable[i].pszItemName);

		if(nIndex >= 0)
		{
			/*
			*	Store reference to the Table Entry associated with this string
			*/
			SendDlgItemMessage(hDlg, Id, CB_SETITEMDATA, (WPARAM)nIndex, 
												(LPARAM)&pTable[i]);
		}
		#ifdef _DEBUG
		else
		{
			TRACE("Serious problem adding item %s, to Combo box.\n", (pTABLEENTRY)pTable[i].pszItemName);
		}
		#endif //_DEBUG
	}
	SendDlgItemMessage(hDlg, Id, CB_SETCURSEL, 0, 0L);
	result = 1;

	return  result;
}   


/////////////////////////////////////////////////////////////////////////////
// FUNCTION: CleanKillApp
//
// ARGS:    none
//
// RETURNS: none
//
// NOTES:   1). delete any bit maps laying around
//          2). post window quit message
//
VOID CleanKillApp (VOID)
{
	CleanUpApp();
	return;
} 

/////////////////////////////////////////////////////////////////////////////
// FUNCTION: DibNumColors
//
// ARGS:    pv  pointer to bitmap data
//
// RETURNS: number of colors, 0, 2, 16, 256, in the DIB
//
// NOTES:
//
WORD DibNumColors (VOID FAR *pv)
{
	int Bits = 0;
	LPBITMAPINFOHEADER lpbi = NULL;
	LPBITMAPCOREHEADER lpbc = NULL;

	ASSERT(pv);

	lpbi = ((LPBITMAPINFOHEADER)pv);
	lpbc = ((LPBITMAPCOREHEADER)pv);

	//    With the BITMAPINFO format headers, the size of the palette
	//    is in biClrUsed, whereas in the BITMAPCORE - style headers, it
	//    is dependent on the bits per pixel ( = 2 raised to the power of
	//    bits/pixel).
	if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
	{
		if (lpbi->biClrUsed != 0)
		{
			return (WORD)lpbi->biClrUsed;
		}

		Bits = lpbi->biBitCount;
	}
	else
	{ 
		Bits = lpbc->bcBitCount;
	}

	switch (Bits)
	{
		case 1:
			return 2;

		case 4:
			return 16;

		case 8:
			return 256;

		default:
			// A 24 bitcount DIB has no color table
			return 0;
	}
}



