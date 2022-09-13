/*
* TWAIN source code:
* Copyright (C) '91-'92 TWAIN Working Group:
* Aldus, Caere, Eastman-Kodak, Logitech,
* Hewlett-Packard Corporations.
* All rights reserved.
*
*	Copyright ?1998 TWAIN Working Group: Adobe Systems Incorporated, 
*	Canon Information Systems, Eastman Kodak Company, 
*	Fujitsu Computer Products of America, Genoa Technology, 
*	Hewlett-Packard Company, Intel Corporation, Kofax Image Products, 
*	JFL Peripheral Solutions Inc., Ricoh Corporation, and Xerox Corporation.  
*	All rights reserved.
*
* Triplets.c
*
* Replaced special.c.  Packs and unpacks the TWAIN triplets.
*/

#include <windows.h>         // Req. for twain.h type defs and ...
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <ctype.h>       

#include "twacker.h"

#include "twndebug.h"

TW_UINT16 AltTWItemSize(TW_INT16 ItemType);
BOOL TestValueRange(HWND hDlg, pTW_RANGE pRange);

#ifdef WIN32
#include "res_32.h"
#else
#include "res_16.h"
#endif

#include "dca_app.h"
#include "dca_glue.h"
#include "dca_acq.h"
#include "captest.h" 
//#include "ctl3d.h"
#include "triplets.h"
#include "special.h"

extern HWND hMainWnd;
extern TABLEENTRY Types[MAX_TYPES];
extern TABLECAP Caps[MAX_CAP];
extern TW_UINT32 uiNumItems;

/*
* Function: SetupFileXfer
* Author: TWAIN Working Group
* Input:
*		filename - 
*		format - 
*		pSetup -
*		msg - The current Message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 SetupFileXfer(char filename[],char format[],pTW_SETUPFILEXFER pSetup,TW_UINT16 msg)
{
	HFILE hfile = 0;  //HFILE_ERROR
	OFSTRUCT of;
	TW_UINT16 status = TWRC_FAILURE;

	memset(&of, 0, sizeof(OFSTRUCT));
	ASSERT(pSetup);

	if (!TWOpenDSM() || !TWIsDSOpen()) 
	{
		ShowRC_CC(NULL,0,0,0,"DSM or DS not open","Application Error");
		return(TWRC_FAILURE);
	}

	if (msg == MSG_SET)
	{
		TW_UINT16 i = 0;

		lstrcpyn(pSetup->FileName,filename,sizeof(pSetup->FileName));
		
		for(i = 0; i < MAX_FORMATTYPE; i++)
		{
			if(lstrcmp(format, FormatType[i].pszItemName) == 0)
			{	
				pSetup->Format = FormatType[i].ItemId;
				break;
			}
		}
	
		pSetup->VRefNum = 0;

		/*
		* create the file and close.  If the file cannot be created here it can be
		* assumed that the file can not be created at the source either
		*/
		if ((hfile = OpenFile(pSetup->FileName, &of, OF_CREATE|OF_SHARE_DENY_NONE)) == HFILE_ERROR)
		{
			ShowRC_CC(NULL,0,0,0,"Unable to create file for file transfer","Application Error");
			status = TWRC_FAILURE;
		}
		else
		{
			_lclose(hfile); 

			status = CallDSMEntry(&appID,
							&dsID,
							DG_CONTROL,
							DAT_SETUPFILEXFER,
							msg,
							pSetup);
		}
	}
	else if(msg == MSG_GET)
	{
		status = CallDSMEntry(&appID,
						&dsID,
						DG_CONTROL,
						DAT_SETUPFILEXFER,
						msg,
						pSetup);
	}
	return(status);
}

/*
* Function: QuerySupportMessage
* Author: Nancy L?ourneau / J.F.L. Peripheral Solutions Inc. / TWAIN Working Group
* Input:
*		pTableCap - Table who contain current capability
*		hDlg - Handle to dit box
*		Id - The current Id
* Output:
*		TW_UINT16 - Return the "return code"
* Comments:
*/
TW_UINT16 QuerySupportMessage(pTABLECAP pTableCap, HWND hDlg, int Id)
{
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pOneValue = NULL;
	TW_STR255 buffer;

	memset(&twCapability, 0, sizeof(TW_CAPABILITY));
	memset(buffer, 0, sizeof(TW_STR255));
	ASSERT(pTableCap);
	ASSERT(hDlg);

	twCapability.Cap = pTableCap->EntryHeader.ItemId;
 	twCapability.ConType = TWON_DONTCARE16;
	
	if (MsgQuerySupport(twCapability.Cap, MSG_QUERYSUPPORT, (TW_CAPABILITY *)&twCapability))
	{
		/*
		* clear the edit control
		*/
		SetWindowText(GetDlgItem(hDlg,Id)," ");

		pOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);

		wsprintf(buffer,"ItemType=%s\r\n",Types[pOneValue->ItemType].pszItemName);
		SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);
		
		strcpy(buffer, "Item=");
		if (pOneValue->Item & TWQC_GET)
		{
			strcat(buffer, "TWQC_GET |");
		}
		if (pOneValue->Item & TWQC_SET)
		{
			strcat(buffer, "TWQC_SET |");
		}
		if (pOneValue->Item & TWQC_GETDEFAULT)
		{
			strcat(buffer, "TWQC_GETDEFAULT |");
		}
		if (pOneValue->Item & TWQC_GETCURRENT)
		{
			strcat(buffer, "TWQC_GETCURRENT |");
		}
		if (pOneValue->Item & TWQC_RESET)
		{
			strcat(buffer, "TWQC_RESET |");
		}
		/*
		* remove last OR "|" at the end
		*/
		buffer[lstrlen(buffer)-1] = '\0';
		lstrcat(buffer,"\r\n");

		SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,1,(DWORD)(LPSTR)buffer);

		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
		return(TWRC_SUCCESS);
	}
	return(TWRC_FAILURE);
}

/*
* Function: GetCapabilitySpecial
* Author: TWAIN Working Group
* Input:
*		pTableCap - Table who contain current capability
*		gettype - The current Message
*		hDlg - Handle to dit box
*		Id - The current Id
* Output:
*		TW_UINT16 - Return the "return code"
* Comments:
*		Write in the edit box the value in when it's MSG_GET, 
*		MSG_GETCURRENT, MSG_GETDEFAULT
*/
TW_UINT16 GetCapabilitySpecial(pTABLECAP pTableCap,TW_UINT16 gettype,HWND hDlg,int Id)
{
	TW_CAPABILITY twCapability;
	pTW_RANGE pRange = NULL;
	pTW_ONEVALUE pOneValue = NULL;
	pTW_ENUMERATION pEnumeration = NULL;
	pTW_ARRAY pArray = NULL;
	TW_STR255 buffer;
	TW_UINT16 index = 0;
	TW_UINT16 x = 0;

	memset(&twCapability, 0, sizeof(TW_CAPABILITY));
	memset(buffer, 0, sizeof(TW_STR255));
	ASSERT(pTableCap);
	ASSERT(hDlg);

	twCapability.Cap = pTableCap->EntryHeader.ItemId;
	
	if (MsgGetCapability(twCapability.Cap, gettype, (TW_CAPABILITY *)&twCapability))
	{
		/*
		* clear the edit control
		*/
		SetWindowText(GetDlgItem(hDlg,Id)," ");

		switch(twCapability.ConType)
		{
			case TWON_ONEVALUE:
				pOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);

				wsprintf(buffer,"ItemType=%s\r\n",Types[pOneValue->ItemType].pszItemName);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);
				lstrcpy(buffer,"");
				FillOutputString(pTableCap, buffer, "Value=",
											pOneValue->ItemType,(void *)&pOneValue->Item);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,1,(DWORD)(LPSTR)buffer);

				GlobalUnlock(twCapability.hContainer);
				break;

			case TWON_ENUMERATION:
				pEnumeration = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);

				wsprintf(buffer,"ItemType=%s\r\n",Types[pEnumeration->ItemType].pszItemName);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);
				wsprintf(buffer,"NumItems=%ld\r\n",(TW_UINT32)pEnumeration->NumItems);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,1,(DWORD)(LPSTR)buffer);
				wsprintf(buffer,"CurrentIndex=%ld\r\n",(TW_UINT32)pEnumeration->CurrentIndex);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,2,(DWORD)(LPSTR)buffer);
				wsprintf(buffer,"DefaultIndex=%ld\r\n",(TW_UINT32)pEnumeration->DefaultIndex);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,3,(DWORD)(LPSTR)buffer);
				lstrcpyn(buffer,"ItemList=",sizeof(buffer));
				for (index=0;index<pEnumeration->NumItems;index++)
				{
					/*
					* Condition if the ItemList does'nt want to be change in a string
					*/
					FillOutputString(pTableCap, buffer, "",pEnumeration->ItemType,
											(void *)&pEnumeration->ItemList[index*AltTWItemSize(pEnumeration->ItemType)]);

					lstrcat(buffer,",");	
				}
				/*
				* remove last comma at the end
				*/
				buffer[lstrlen(buffer)-1] = '\0';
				lstrcat(buffer,"\r\n");

				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);

				GlobalUnlock(twCapability.hContainer);
				break;

			case TWON_ARRAY:
				pArray = (pTW_ARRAY)GlobalLock(twCapability.hContainer);

				wsprintf(buffer,"ItemType=%s\r\n",Types[pArray->ItemType].pszItemName);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);

				for (x=0; x<pArray->NumItems; x++)
				{
					pTW_UINT16 val = NULL;
					TW_UINT16 i = 0;
					TW_BOOL bFind = FALSE;

					val =(pTW_UINT16)&(pArray->ItemList[x*AltTWItemSize(pArray->ItemType)]);

					for(i = 0; i < MAX_CAP; i++)
					{ 
						if (Caps[i].EntryHeader.ItemId == *val)
						{
							lstrcpy(buffer, "ItemList=");
							lstrcat(buffer, Caps[i].EntryHeader.pszItemName);
							lstrcat(buffer,"\r\n");
							SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,x,(DWORD)(LPSTR)buffer);
							bFind = TRUE;	
							break; 
						}
					}
					if (!bFind)
					{
						lstrcpy(buffer, "ItemList=");
						if(*val < CAP_CUSTOMBASE)
						{
							wsprintf(buffer, "ItemList=0x%x", *val);
						}
						else
						{
							wsprintf(buffer, "ItemList=CAP_CUSTOMBASE + 0x%x", *val - CAP_CUSTOMBASE);
						}
						lstrcat(buffer,"\r\n");
						SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,x,(DWORD)(LPSTR)buffer);
					}
				}

				GlobalUnlock(twCapability.hContainer);
				break;

			case TWON_RANGE:
				pRange = (pTW_RANGE)GlobalLock(twCapability.hContainer);

				wsprintf(buffer,"ItemType=%s\r\n",Types[pRange->ItemType].pszItemName);
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);

				lstrcpy(buffer,"");
				FillOutputString(pTableCap, buffer, "Min=",pRange->ItemType,(void *)&pRange->MinValue);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,1,(DWORD)(LPSTR)buffer);

				lstrcpy(buffer,"");
				FillOutputString(pTableCap,buffer,"Max=",pRange->ItemType,(void *)&pRange->MaxValue);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,2,(DWORD)(LPSTR)buffer);

				lstrcpy(buffer,"");
				FillOutputString(pTableCap,buffer,"StepSize=",pRange->ItemType,(void *)&pRange->StepSize);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,3,(DWORD)(LPSTR)buffer);

				lstrcpy(buffer,"");
				FillOutputString(pTableCap,buffer,"Default=",pRange->ItemType,(void *)&pRange->DefaultValue);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,4,(DWORD)(LPSTR)buffer);

				lstrcpy(buffer,"");
				FillOutputString(pTableCap,buffer, "Current=",pRange->ItemType,(void *)&pRange->CurrentValue);
				lstrcat(buffer,"\r\n");
				SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,5,(DWORD)(LPSTR)buffer);

				SendDlgItemMessage(hDlg,Id,EM_LINESCROLL,0,(LPARAM)(-1));

				GlobalUnlock(twCapability.hContainer);
				break;

			default:
				break;
		}
		GlobalFree(twCapability.hContainer);
		return(TWRC_SUCCESS);
	}
	return(TWRC_FAILURE);
}

/*
* Function: SetCapabilitySpecial
* Author: TWAIN Working Group	
* Input:		
*		pTableCap - Table with current capability
*		settype - Current Message
*		hDlg - Handle to dialog box
*		Id - The curent Id
*		pConType - Table who contain the current container
* Output:
*		TW_UINT16 - Return the "return code"
* Comments:
*		Read in the edit box the value in when it's MSG_SET, 
*		MSG_RESET and filled the corresponding structure.
*/
TW_UINT16 SetCapabilitySpecial(pTABLECAP pTableCap,TW_UINT16 settype,HWND hDlg,
															int Id, pTABLEENTRY pConType)
{
	TW_CAPABILITY twCapability;
	pTW_RANGE pRange = NULL;
	pTW_ONEVALUE pOneValue = NULL;
	pTW_ENUMERATION pEnumeration = NULL;
	pTW_ARRAY pArray = NULL;
	char string[64];
	char *token = NULL;
	LONG count = 0;
	TW_UINT32 i = 0, j = 0;
	BOOL bFilledIn = FALSE;
	BOOL bInvalidValue = FALSE;
	TW_FRAME Frame;

	memset(&Frame, 0, sizeof(TW_FRAME));
	memset(&twCapability, 0, sizeof(TW_CAPABILITY));
	memset(string, 0, 64);
	ASSERT(pTableCap);
	ASSERT(hDlg);
	ASSERT(pConType);

	twCapability.ConType = pConType->ItemId;
	twCapability.Cap = pTableCap->EntryHeader.ItemId;

	if(settype == MSG_SET)
	{
		/*
		* allocate and fill the container
		*/
		switch(twCapability.ConType)
		{
			case TWON_ONEVALUE:
				/*
				* Condition to allocate the good size of container if
				* it's TW_FRAME type
				*/
				GetDlgItemText(hDlg, IDITEMTYPE, string, sizeof(string));
				for (i = 0; i < MAX_TYPES; i++)
				{
					if(strcmp(string, Types[i].pszItemName) == 0)
					{
						if(Types[i].ItemId == TWTY_FRAME)
						{
							twCapability.hContainer = GlobalAlloc(GHND,(sizeof(TW_ONEVALUE)+sizeof(TW_FRAME)));
							pOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
						}
						else
						{
							twCapability.hContainer = GlobalAlloc(GHND,sizeof(TW_ONEVALUE));
							pOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
						}
						pOneValue->ItemType = Types[i].ItemId;
						break;
					}
				}

				if(pTableCap->pItemTable != NULL)
				{
					GetDlgItemText(hDlg, IDVALUECB, string, sizeof(string));
					for(i = 0; i < pTableCap->ItemTableSize; i++)
					{
						if(lstrcmp(string, pTableCap->pItemTable[i].pszItemName) == 0)
						{
							pOneValue->Item = pTableCap->pItemTable[i].ItemId;
							bFilledIn = TRUE;
							break;
						}
					}
					GlobalUnlock(twCapability.hContainer);
				}
				else
				{
					/*
					* Case  for the ICAP_FRAME capability
					*/
					if(pTableCap->EntryHeader.ItemId != ICAP_FRAMES)
					{
						memset(string, 0, 64);
						count = GetWindowText(GetDlgItem(hDlg, IDVALUE), string, 64);
						count = SendDlgItemMessage(hDlg, IDVALUE, EM_GETLINE, 0, (DWORD)(LPSTR)string);
						if(strcmp(string, "")!= 0)
						{
							bFilledIn = TRUE;
							pOneValue->Item = ConvertToNumber(pTableCap , string, pOneValue->ItemType);
						}
					}
					else
					{
						memset(string, 0, 64);
						count = GetWindowText(GetDlgItem(hDlg, IDLEFT), string, 64);
						if(strcmp(string, "")!= 0)
						{
							bFilledIn = TRUE;
							Frame.Left = FloatToFIX32((float)atof(string));

							memset(string, 0, 64);
							count = GetWindowText(GetDlgItem(hDlg, IDTOP), string, 64);
							Frame.Top = FloatToFIX32((float)atof(string));

							memset(string, 0, 64);
							count = GetWindowText(GetDlgItem(hDlg, IDRIGHT), string, 64);
							Frame.Right = FloatToFIX32((float)atof(string));

							memset(string, 0, 64);
							count = GetWindowText(GetDlgItem(hDlg, IDBOTTOM), string, 64);
							Frame.Bottom = FloatToFIX32((float)atof(string));

							memcpy(&pOneValue->Item, &Frame, sizeof(TW_FRAME));

							if(pTableCap->ExpectedType == TWTY_FRAME)
							{
								if((Frame.Left.Whole >= Frame.Right.Whole) && 
									 (Frame.Top.Whole >= Frame.Bottom.Whole))
								{
									ShowRC_CC(hDlg, 0, 0, 0,
												"The Left value should be smaller that the Right value and\r\n"
												"the Top value should be smaller that the Bottom value\r\n", 
												"Error");
									bInvalidValue = TRUE;
								}
								else if(Frame.Left.Whole >= Frame.Right.Whole)
								{
									ShowRC_CC(hDlg, 0, 0, 0,
												"The Left value should be smaller that the Right value\r\n", "Error");
									bInvalidValue = TRUE;
								}
								else if(Frame.Top.Whole >= Frame.Bottom.Whole)
								{
									ShowRC_CC(hDlg, 0, 0, 0,
												"The Top value should be smaller that the Bottom value\r\n", "Error");
									bInvalidValue = TRUE;
								}
							}
						}
					}
					GlobalUnlock(twCapability.hContainer);
				}			
				break;

			case TWON_RANGE:
				{
					twCapability.hContainer = GlobalAlloc(GHND,sizeof(TW_RANGE));
					pRange = (pTW_RANGE)GlobalLock(twCapability.hContainer);

					GetDlgItemText(hDlg, IDITEMTYPE, string, sizeof(string));
					/*
					* set string to a viable value, EM_GETLINE will not fill a null string
					*/
					for (i = 0; i < MAX_TYPES; i++)
					{
						if(strcmp(string, Types[i].pszItemName) == 0)
						{
							pRange->ItemType = Types[i].ItemId;
							break;
						}
					}

					lstrcpy(string, "");
					GetWindowText(GetDlgItem(hDlg, IDMINVALUE) , (LPTSTR)string ,sizeof(string));
					if(strcmp(string, "")!= 0)
					{
						bFilledIn = TRUE;
						pRange->MinValue = ConvertToNumber(pTableCap,string,pRange->ItemType);

						lstrcpy(string, "");
						GetWindowText(GetDlgItem(hDlg, IDMAXVALUE), (LPTSTR)string ,sizeof(string));
						bFilledIn = FALSE;
										
						if(strcmp(string, "")!= 0)
						{
							bFilledIn = TRUE;
							pRange->MaxValue = ConvertToNumber(pTableCap,string,pRange->ItemType);

							lstrcpy(string, "");
							GetWindowText(GetDlgItem(hDlg, IDSTEPSIZE), (LPTSTR)string ,sizeof(string));
							bFilledIn = FALSE;

							if(strcmp(string, "")!= 0)
							{
								bFilledIn = TRUE;
								pRange->StepSize = ConvertToNumber(pTableCap,string,pRange->ItemType);

								lstrcpy(string, "");
								GetWindowText(GetDlgItem(hDlg, IDDEFAULTVALUE), (LPTSTR)string ,sizeof(string));
								bFilledIn = FALSE;

								if(strcmp(string, "")!= 0)
								{
									bFilledIn = TRUE;
									pRange->DefaultValue = ConvertToNumber(pTableCap,string,pRange->ItemType);

									lstrcpy(string, "");
									GetWindowText(GetDlgItem(hDlg, IDCURRENTVALUE), (LPTSTR)string ,sizeof(string));
									bFilledIn = FALSE;

									if(strcmp(string, "")!= 0)
									{
										bFilledIn = TRUE;
										pRange->CurrentValue = ConvertToNumber(pTableCap,string,pRange->ItemType);
									}
								}
							}
						}
					}
					bInvalidValue = TestValueRange(hDlg, pRange);
					GlobalUnlock(twCapability.hContainer);
				}
				break;


			case TWON_ARRAY:
				GetDlgItemText(hDlg, IDITEMTYPE, string, sizeof(string));
				/*
				* set string to a viable value, EM_GETLINE will not fill a null string
				*/
				for (i = 0; i < MAX_TYPES; i++)
				{
					if(strcmp(string, Types[i].pszItemName) == 0)
					{
						twCapability.hContainer = GlobalAlloc(GHND,(sizeof(TW_ARRAY)+
																			AltTWItemSize(Types[i].ItemId) * (uiNumItems)));
						pArray = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
						pArray->ItemType = Types[i].ItemId;
						break;
					}
				}

				memset(string, 0, 64);
				count = GetWindowText(GetDlgItem(hDlg, IDNUMITEMS), string, 64);
				count = SendDlgItemMessage(hDlg,IDNUMITEMS,EM_GETLINE,0,(DWORD)(LPSTR)string);
				if(strcmp(string, "")!= 0)
				{
					bFilledIn = TRUE;
					pArray->NumItems = ConvertToNumber(pTableCap,string,pArray->ItemType);
				}

				for(i = 0; i < pArray->NumItems; i++)
				{
					SendDlgItemMessage(hDlg,IDITEMLISTARRAY,LB_GETTEXT,i,(DWORD)(LPSTR)string);
					if(pTableCap->pItemTable == NULL)
					{
						TW_UINT32 Result = ConvertToNumber(pTableCap,string,pArray->ItemType);
						memcpy(&(pArray->ItemList[i*AltTWItemSize(pArray->ItemType)]),
									&Result, AltTWItemSize(pArray->ItemType));
					}
					else
					{
						for(j = 0; j < pTableCap->ItemTableSize; j++)
						{
							if(lstrcmp(string, pTableCap->pItemTable[j].pszItemName) == 0)
							{
								memcpy(&(pArray->ItemList[i*AltTWItemSize(pArray->ItemType)]),
											&(pTableCap->pItemTable[j].ItemId), AltTWItemSize(pArray->ItemType));
								break;
							}
						}
					}
				}
				GlobalUnlock(twCapability.hContainer);
				break;

			case TWON_ENUMERATION:
			{
				GetDlgItemText(hDlg, IDITEMTYPE, string, sizeof(string));
				for (i = 0; i < MAX_TYPES; i++)
				{
					if(strcmp(string, Types[i].pszItemName) == 0)
					{
						twCapability.hContainer = GlobalAlloc(GHND, (sizeof(TW_ENUMERATION)+
																			AltTWItemSize(Types[i].ItemId) * (uiNumItems)));
						pEnumeration = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
						pEnumeration->ItemType = Types[i].ItemId;
						break;
					}
				}

				memset(string, 0, 64);
				count = GetWindowText(GetDlgItem(hDlg, IDNUMITEMS), string, 64);
				count = SendDlgItemMessage(hDlg,IDNUMITEMS,EM_GETLINE,0,(DWORD)(LPSTR)string);
				if(strcmp(string, "")!= 0)
				{
					bFilledIn = TRUE;
					if(pEnumeration->ItemType == TWTY_FRAME)
					{
						pEnumeration->NumItems = ConvertToNumber(pTableCap,string,TWTY_UINT16);
					}
					else
					{
						pEnumeration->NumItems = ConvertToNumber(pTableCap,string,pEnumeration->ItemType);
					}
					memset(string, 0, 64);
					count = GetWindowText(GetDlgItem(hDlg, IDCURRENTINDEX), string, 64);
					count = SendDlgItemMessage(hDlg,IDCURRENTINDEX,EM_GETLINE,0,(DWORD)(LPSTR)string);
					bFilledIn = FALSE;

				if(strcmp(string, "")!= 0)
					{
						bFilledIn = TRUE;
						if(pEnumeration->ItemType == TWTY_FRAME)
						{
							pEnumeration->CurrentIndex = ConvertToNumber(pTableCap,string,TWTY_UINT16);
						}
						else
						{
							pEnumeration->CurrentIndex = ConvertToNumber(pTableCap,string,pEnumeration->ItemType);
						}
						memset(string, 0, 64);
						count = GetWindowText(GetDlgItem(hDlg, IDDEFAULTINDEX), string, 64);
						count = SendDlgItemMessage(hDlg,IDDEFAULTINDEX,EM_GETLINE,0,(DWORD)(LPSTR)string);
						bFilledIn = FALSE;
		
						if(strcmp(string, " ")!= 0)
						{
							bFilledIn = TRUE;
							if(pEnumeration->ItemType == TWTY_FRAME)
							{
								pEnumeration->DefaultIndex = ConvertToNumber(pTableCap,string,TWTY_UINT16);
							}
							else
							{
								pEnumeration->DefaultIndex = ConvertToNumber(pTableCap,string,pEnumeration->ItemType);
							}
						}
						if((pEnumeration->DefaultIndex >= pEnumeration->NumItems) &&
							 (pEnumeration->CurrentIndex >= pEnumeration->NumItems))
						{
							ShowRC_CC(hDlg, 0, 0, 0,
							"The DefautIndex and the CurrentIndex should be smaller that NumItems\r\n", "Error");
							bInvalidValue = TRUE;
						}
						else if((pEnumeration->DefaultIndex >= pEnumeration->NumItems))
						{
							ShowRC_CC(hDlg, 0, 0, 0,
							"The DefautIndex should be smaller that NumItems\r\n", "Error");
							bInvalidValue = TRUE;
						}
						else if((pEnumeration->CurrentIndex >= pEnumeration->NumItems))
						{
							ShowRC_CC(hDlg, 0, 0, 0,
							"The CurrentIndex should be smaller that NumItems\r\n", "Error");
							bInvalidValue = TRUE;
						}


					}
				}

				for(i = 0; i < pEnumeration->NumItems; i++)
				{
					SendDlgItemMessage(hDlg,IDITEMLISTENUM,LB_GETTEXT,i,(DWORD)(LPSTR)string);
					if(pTableCap->pItemTable == NULL)
					{
						if(pTableCap->EntryHeader.ItemId != ICAP_FRAMES)
						{
							TW_UINT32 Result = ConvertToNumber(pTableCap,string,pEnumeration->ItemType);
							memcpy(&(pEnumeration->ItemList[i*AltTWItemSize(pEnumeration->ItemType)]),
										&Result, AltTWItemSize(pEnumeration->ItemType));
						}
						else
						{
							char *token = NULL;
							TW_STR64 text;

							memset(text, 0, sizeof(TW_STR64));
							
							token = strtok(string,",");

							while(token != NULL)
							{
								if(token[0] == 'L')
								{
									RemoveEquals(token, lstrlen(token));
									Frame.Left = FloatToFIX32((float)atof(token));
								}
								else if(token[0] == 'T')
								{
									RemoveEquals(token, lstrlen(token));
									Frame.Top = FloatToFIX32((float)atof(token));
								}
								else if(token[0] == 'R')
								{
									RemoveEquals(token, lstrlen(token));
									Frame.Right = FloatToFIX32((float)atof(token));
								}
								else if(token[0] == 'B')
								{
									RemoveEquals(token, lstrlen(token));
									Frame.Bottom = FloatToFIX32((float)atof(token));
								}
								token = strtok(NULL,",");
							}

							memcpy(&(pEnumeration->ItemList[i*AltTWItemSize(pEnumeration->ItemType)]),
										&(Frame), AltTWItemSize(pEnumeration->ItemType));
						}
					}
					else
					{
						for(j = 0; j < pTableCap->ItemTableSize; j++)
						{
							if(lstrcmp(string, pTableCap->pItemTable[j].pszItemName) == 0)
							{
								memcpy(&(pEnumeration->ItemList[i*AltTWItemSize(pEnumeration->ItemType)]),
											&(pTableCap->pItemTable[j].ItemId), AltTWItemSize(pEnumeration->ItemType));
								break;
							}
						}
					}
				}
				GlobalUnlock(twCapability.hContainer);
			}
				break;

			default:
				break;
		}

		/*
		* Message when one of the Edit Box is not filled in
		*/
		if(!bFilledIn)
		{
			ShowRC_CC(hDlg, 0, 0, 0,
						"The items for the structure must be filled in.", "Error");
		}
		else if(!bInvalidValue)
		{
			if (twCapability.hContainer != NULL)
			{
				if (MsgSetCapability(twCapability.Cap,settype,(TW_CAPABILITY *)&twCapability))
				{
					GlobalFree(twCapability.hContainer);
					return(TWRC_SUCCESS);
				}
			}
		}
		GlobalFree(twCapability.hContainer);
	}
	else if(settype == MSG_RESET)
	{
		TW_STR128 buffer;

		memset(buffer, 0, sizeof(TW_STR128));
		SetWindowText(GetDlgItem(hDlg,Id)," ");

		if (MsgSetCapability(twCapability.Cap,settype,(TW_CAPABILITY *)&twCapability))
		{
			pOneValue = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);

			wsprintf(buffer,"ItemType=%s\r\n",Types[pOneValue->ItemType].pszItemName);
			SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,0,(DWORD)(LPSTR)buffer);
			lstrcpy(buffer,"");
			FillOutputString(pTableCap, buffer, "Value=",
										pOneValue->ItemType,(void *)&pOneValue->Item);
			lstrcat(buffer,"\r\n");
			SendDlgItemMessage(hDlg,Id,EM_REPLACESEL,1,(DWORD)(LPSTR)buffer);

			GlobalUnlock(twCapability.hContainer);
			return(TWRC_SUCCESS);
		}
	}
	return(TWRC_FAILURE);
}

/*
* Fucntion: PendingXfers -- 
* Author: TWAIN Working Group
* Input:
*		pPendingXfers - 
*		msg - The current Message Id
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 PendingXfers(pTW_PENDINGXFERS pPendingXfers,TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pPendingXfers);

	pPendingXfers->Count = 0;
	pPendingXfers->EOJ = 0;

	status = CallDSMEntry(&appID,
					&dsID,
					DG_CONTROL,
					DAT_PENDINGXFERS,
					msg,
					pPendingXfers);

	return(status);         
}

/*
* Function: SetupMemXfer -- 
* Author: TWAIN Working Group
* Input:
*		pSetup - 
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 SetupMemXfer(pTW_SETUPMEMXFER pSetup)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pSetup);

	pSetup->MinBufSize = 0;
	pSetup->MaxBufSize = 0;
	pSetup->Preferred = 0;

	status = CallDSMEntry(&appID,
					&dsID,
					DG_CONTROL,
					DAT_SETUPMEMXFER,
					MSG_GET,
					pSetup);

	return(status);
}

/*
* Function: Status -- 
* Author: TWAIN Working Group
* Input:
*		pStatus -
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 Status(pTW_STATUS pStatus)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pStatus);

	pStatus->ConditionCode = 0;
	pStatus->Reserved = 0;

	status = CallDSMEntry(&appID,
					&dsID,
					DG_CONTROL,
					DAT_STATUS,
					MSG_GET,
					pStatus);

	return(status);
}

/*
* Function: UserInterface -- 
* Author: TWAIN Working Group
* Input:
*		pUI -
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 UserInterface(pTW_USERINTERFACE pUI,TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pUI);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_CONTROL,
					DAT_USERINTERFACE,
					msg,
					pUI);

	return(status);
}

/*
* Function: ImageFileXfer --  
* Author: TWAIN Working Group
* Input:
*		Bitmap -
* Output:
*		TW_UINT16
* Comments:
*		Starts the file transfer mechanism.  If the transfer completes
* without error, success is returned, otherwise the RC status code is returned. 
*/
TW_UINT16 ImageFileXfer(HANDLE Bitmap)
{
	TW_UINT16 status = TWRC_FAILURE;
	HFILE hfile = 0;
	OFSTRUCT of;
	char filename[MAX_TWPATH];
	BITMAPFILEHEADER header;
	TW_UINT16 num = 0x8000;
	TW_UINT32 count = 0;
	unsigned char TW_HUGE *ptr = NULL;

	memset(&of, 0, sizeof(OFSTRUCT));
	memset(&header, 0, sizeof(BITMAPFILEHEADER));
	memset(filename, 0, sizeof(char[MAX_TWPATH]));
	ASSERT(Bitmap);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_IMAGEFILEXFER,
					MSG_GET,
					NULL);

	if (status == TWRC_XFERDONE)
	{
		/*
		* read the bitmap header and verify the transfer is a valid bmp
		* and create a handle to that bitmap
		*/
		if ((hfile = OpenFile(filename, &of, OF_READ)) != -1)
		{
			_lread(hfile, (LPSTR)&header, sizeof(BITMAPFILEHEADER));
			if (Bitmap = GlobalAlloc(GHND, header.bfSize))
			{
				ptr = (unsigned char*)GlobalLock(Bitmap);
				for (count=header.bfSize-sizeof(BITMAPFILEHEADER)
							; count; count-=num, ptr+=num)
				{
					if (count < num) 
					{
						num = (TW_UINT16) count;
					}
					_lread(hfile, ptr, num);
				}   
				GlobalUnlock(Bitmap);
				GlobalFree(Bitmap);
			}   
			_lclose(hfile);
		}   
		return(TWRC_SUCCESS);
	}
	return(status);
}

/*
* Function: ImageNativeXfer -- 
* Author: TWAIN Working Group
* Input:
*		Bitmap - 
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 ImageNativeXfer(TW_MEMREF Bitmap)
{
	TW_UINT16 status = TWRC_FAILURE;

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_IMAGENATIVEXFER,
					MSG_GET,
					Bitmap);

	return(status);
}

/*
* Function: ImageMemXfer
* Author: TWAIN Working Group
* Input:
*		Bitmap -
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 ImageMemXfer(TW_MEMREF Bitmap)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(FALSE);
// incomplete
//	status = (*lpDSM_Entry)(&appID,&dsID,DG_IMAGE,DAT_IMAGEMEMXFER,MSG_GET,NULL);

	return(status);
}

/*
* Function: ImageLayout -- 
* Author: TWAIN Working Group
* Input:
*		layout - 
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 ImageLayout(pTW_IMAGELAYOUT layout,TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(layout);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_IMAGELAYOUT,
					msg,
					layout);

	return(status);
}

/*
* Function: ImageInfo -- 
* Author: TWAIN Working Group
* Input:
*		info -
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 ImageInfo(pTW_IMAGEINFO info)
{
	TW_UINT16 status = TWRC_FAILURE;
	
	ASSERT(info);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_IMAGEINFO,
					MSG_GET,
					info);

	return(status);
}

/*
* Function: ExtImageInfo
* Author: TWAIN Working Group
* Input:
*		pExtImageInfo - 
* Output:
*		TW_UINT16
* Comment:
*/
TW_UINT16 ExtImageInfo(pTW_EXTIMAGEINFO pExtImageInfo)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pExtImageInfo);

	pExtImageInfo->NumInfos = 0;

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_EXTIMAGEINFO,
					MSG_GET,
					pExtImageInfo);

	return(status);
}

/*
* Fucntion: Palette8
* Author: TWAIN Working Group
* Input:
*		pal -
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 Palette8(pTW_PALETTE8 pal,TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(pal);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_PALETTE8,
					msg,
					pal);

	return(status);
}

/*
* Function: GrayResponse --
* Author: TWAIN Working Group
* Input:
*		resp -
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 GrayResponse(pTW_GRAYRESPONSE resp, TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(resp);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_GRAYRESPONSE,
					msg,
					resp);

	return(status);
}

/*
* Function: RGBResponse -- 
* Author: TWAIN Working Group
* Input:
*		resp -
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 RGBResponse(pTW_GRAYRESPONSE resp, TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(resp);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_RGBRESPONSE,
					msg,
					resp);

	return(status);
}

/*
* Function: CIEColor --
* Author: TWAIN Working Group
* Input:
*		cie -
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 CIEColor(pTW_CIECOLOR cie)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(cie);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_CIECOLOR,
					MSG_GET,
					cie);

	return(status);
}

/*
* Function: JPEGCompression
* Author: TWAIN Working Group
* Input:
*		jpeg -
*		msg - The current message ID
* Output:
*		TW_UINT16
* Comments:
*/
TW_UINT16 JPEGCompression(pTW_JPEGCOMPRESSION jpeg, TW_UINT16 msg)
{
	TW_UINT16 status = TWRC_FAILURE;

	ASSERT(jpeg);

	status = CallDSMEntry(&appID,
					&dsID,
					DG_IMAGE,
					DAT_JPEGCOMPRESSION,
					msg,
					jpeg);

	return(status);
}

/*
* Function: FillOutputString
* Author: TWAIN Working Group
* Input:
*		TableCap - Current capability	 
*		value - At the end contain the output string
*		string - Field string
*		ItemType - The current Data type
*		Item - 
* Output:
*		TRUE is successful
*	Comments:
*		Fill the good string in value for each type.
*/
BOOL FillOutputString(pTABLECAP pTableCap, char value[],char string[], 
											TW_UINT16 ItemType, void* Item)
{
	float fl = 0;
	pTW_FIX32 pfix32 = NULL;
	TW_UINT16 i = 0;
	pTW_STR255 pbuffer = NULL;

	ASSERT(pTableCap);
	ASSERT(Item);

	switch(ItemType)		
	{	 
		case TWTY_UINT32:
			TRACE("Following code is broken for this type.\n");
		case TWTY_UINT16:
		case TWTY_INT16:
		case TWTY_INT32:
		case TWTY_BOOL:
			/* 
			* Contition to know if the capability has a secials types.
			*/
			if(pTableCap->pItemTable != NULL)
			{
				for(i = 0; i < pTableCap->ItemTableSize; i++)
				{
					if(*(TW_UINT16*)Item == pTableCap->pItemTable[i].ItemId)
					{
						lstrcat(value, string);
						lstrcat(value, pTableCap->pItemTable[i].pszItemName);
						break;
					}
				}
			}
			else
			{
				TW_STR255 Buf;

				memset(Buf, 0, sizeof(TW_STR255));

				/*
				* Convert to string
				* Maintain the sign
				*/
				if(ItemType == TWTY_INT16)
				{
					/*
					*	16bit signed value cast from unsigned 32bit item
					*/
					wsprintf(Buf,"%s%ld",string,*(TW_INT16*)Item);
				}
				else if (ItemType == TWTY_INT32)
				{
					/*
					*	32bit signed value cast from unsigned 32bit item
					*/
					wsprintf(Buf,"%s%ld",string,*(TW_INT32*)Item);
				}
				else if(ItemType == TWTY_UINT16)
				{
					wsprintf(Buf,"%s%ld",string,*(TW_UINT16*)Item);
				}
				else
				{
					/*
					*	No sign, then no difference
					*/
					wsprintf(Buf,"%s%ld",string,*(TW_UINT32*)Item);
				}

				lstrcat(value, Buf);				
			}
			break;

		case TWTY_FIX32:
			{
				TW_STR255 Buf;

				memset(Buf, 0, sizeof(TW_STR255));

				/*
				* FIX32 change
				*/
				pfix32 = (pTW_FIX32)Item;
				fl = FIX32ToFloat(*pfix32);
				sprintf(Buf,"%s%.3f",string,fl);
				lstrcat(value, Buf);
			}
			break;

 
		case TWTY_STR32:
		case TWTY_STR64:
		case TWTY_STR128:
		case TWTY_STR255:
			pbuffer = (pTW_STR255)Item;
			lstrcat(value, string);
			lstrcat(value, pbuffer);
			break;

		case TWTY_FRAME:
			{
				pTW_FRAME pFrame = NULL;

				char Left[10], Top[10], Right[10], Bottom[10];
				float fLeft = 0, fTop = 0, fRight = 0, fBottom = 0;

				memset(Left, 0, sizeof(char[10]));
				memset(Top, 0, sizeof(char[10]));
				memset(Right, 0, sizeof(char[10]));
				memset(Bottom, 0, sizeof(char[10]));
				
				pFrame = (pTW_FRAME)Item;

				lstrcpy(Left, "Left=");
				lstrcpy(Top, "Top=");
				lstrcpy(Right, "Right=");
				lstrcpy(Bottom, "Bottom=");

				fLeft = FIX32ToFloat(pFrame->Left);
				fTop = FIX32ToFloat(pFrame->Top);
				fRight = FIX32ToFloat(pFrame->Right);
				fBottom = FIX32ToFloat(pFrame->Bottom);

				sprintf(value,"%s%.3f\r\n%s%.3f\r\n%s%.3f\r\n%s%.3f",
								Left,fLeft,Top,fTop,Right,fRight,Bottom,fBottom);
			}
			break;

		default:
			break;
	}
	return(TRUE);
}

/*
* Fucntion: ConvertToNumber
* Author: TWAIN Working Group
* Input:  
*		pTableCap -	Current capability
*		tok - The value read in the edit box.
*		type - The data type request
* Output: 
*		TW_UINT32 - Value of Item field of container.
* Comments:
*		Either converts the string token to a number representing the #define
*		or to the actual number.
*/
TW_UINT32 ConvertToNumber(pTABLECAP pTableCap, char *tok, TW_UINT16 type)
{
	TW_UINT32 value = 0;
	TW_FIX32 fix32;
	TW_UINT16 i = 0;

	memset(&fix32, 0, sizeof(TW_FIX32));
	ASSERT(pTableCap);
	ASSERT(tok);
	/* 
	* Look if the entry is a string or number
	*/
	if (isalpha(tok[0]))
	{
		if(pTableCap->pItemTable != NULL)
		{
			for (i = 0; i < pTableCap->ItemTableSize; i++)
			{
				if(lstrcmp(tok, pTableCap->pItemTable[i].pszItemName) == 0)
				{
					value = pTableCap->pItemTable[i].ItemId;
					break;
				}
			}
		}
		else
		{
			switch(type)
			{
				case TWTY_STR32:
				case TWTY_STR64:
				case TWTY_STR128:
				case TWTY_STR255:
						value = *((pTW_UINT32)(&tok));
					break;

				default:
					break;
			}
		}
	}
	else
	{
		switch(type)
		{
			case TWTY_INT16:
			case TWTY_INT32:
			case TWTY_UINT16:
			case TWTY_UINT32:
				value = (TW_UINT32)atoi(tok);
				break;

			case TWTY_FIX32:
				fix32 = FloatToFIX32((float)atof(tok));
				value = *((pTW_UINT32)(&fix32));
				break;     
			
			default:
				break;
		}
	}
	return(value);
}

/*
* Function: TestValueRange
* Author: Nancy L?ourneau / J.F.L. Peripheral Solutions Inc. / TWAIN Working Group
* Date: September 18/1998
* Input:
*		hDlg - Handle of Dialog Box
*		pRange - Pointer to a Range structure
* Output:
*		TRUE when one value is invalid.
* Comments:
*		Test all the value that the user write in the EditBox when it's TWON_RANGE
* container to see if these are valid or not.
*/
BOOL TestValueRange(HWND hDlg, pTW_RANGE pRange)
{
	BOOL bInvalidValue = FALSE;

	switch(pRange->ItemType)
	{
		case TWTY_FIX32:
			{
				pTW_FIX32 pFix32 = NULL;
				float fMinValue = 0;
				float fMaxValue = 0; 
				float fCurrentValue = 0;
				float fDefaultValue = 0;

				/*
				* Convert The MaxValue in float
				*/
				pFix32 = (pTW_FIX32)&pRange->MaxValue;
				fMaxValue = FIX32ToFloat(*pFix32);

				/*
				* Convert The MinValue in float
				*/
				pFix32 = (pTW_FIX32)&pRange->MinValue;
				fMinValue = FIX32ToFloat(*pFix32);

				/*
				* Convert The DefaultValue in float
				*/
				pFix32 = (pTW_FIX32)&pRange->DefaultValue;
				fDefaultValue = FIX32ToFloat(*pFix32);

				/*
				* Convert The CurrentValue in float
				*/
				pFix32 = (pTW_FIX32)&pRange->CurrentValue;
				fCurrentValue = FIX32ToFloat(*pFix32);

				if(fMaxValue < fMinValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"MaxValue should be bigger that MinValue\r\n", "Error");
					return TRUE;
				}
				else
				{
					if(fDefaultValue < fMinValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The DefaultValue should be bigger or equal to MinValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
					else if(fDefaultValue > fMaxValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The DefaultValue should be smaller or equal to MaxValue\r\n", "Error");
						bInvalidValue = TRUE;
					}

					if(fCurrentValue < fMinValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The CurrentValue should be bigger or equal to MinValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
					else if(fCurrentValue > fMaxValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The CurrentValue should be smaller or equal to MaxValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
				}
			}
			break;

		case TWTY_INT32:
		case TWTY_INT16:
		case TWTY_INT8:
			{
				pTW_FIX32 pFix32 = NULL;
				int iMinValue = 0;
				int iMaxValue = 0; 
				int iCurrentValue = 0;
				int iDefaultValue = 0;

				if (pRange->ItemType == TWTY_INT32)
				{
					/*
					*	32bit signed value cast from unsigned 32bit item
					*/
					iMaxValue = *(TW_INT32*)&pRange->MaxValue;
					iMinValue = *(TW_INT32*)&pRange->MinValue;
					iCurrentValue = *(TW_INT32*)&pRange->CurrentValue;
					iDefaultValue = *(TW_INT32*)&pRange->DefaultValue;
				}		 
				else if(pRange->ItemType == TWTY_INT16)
				{
					/*
					*	16bit signed value cast from unsigned 32bit item
					*/
					iMaxValue = *(TW_INT16*)&pRange->MaxValue;
					iMinValue = *(TW_INT16*)&pRange->MinValue;
					iCurrentValue = *(TW_INT16*)&pRange->CurrentValue;
					iDefaultValue = *(TW_INT16*)&pRange->DefaultValue;
				}
				else if (pRange->ItemType == TWTY_INT8)
				{
					/*
					*	32bit signed value cast from unsigned 32bit item
					*/
					iMaxValue = *(TW_INT8*)&pRange->MaxValue;
					iMinValue = *(TW_INT8*)&pRange->MinValue;
					iCurrentValue = *(TW_INT8*)&pRange->CurrentValue;
					iDefaultValue = *(TW_INT8*)&pRange->DefaultValue;
				}

				if(iMaxValue < iMinValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"MaxValue should be bigger that MinValue\r\n", "Error");
					return TRUE;
				}
				else
				{
					if(iDefaultValue < iMinValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The DefaultValue should be bigger or equal to MinValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
					else if(iDefaultValue > iMaxValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The DefaultValue should be smaller or equal to MaxValue\r\n", "Error");
						bInvalidValue = TRUE;
					}

					if(iCurrentValue < iMinValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The CurrentValue should be bigger or equal to MinValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
					else if(iCurrentValue > iMaxValue)
					{
						ShowRC_CC(hDlg, 0, 0, 0,
									"The CurrentValue should be smaller or equal to MaxValue\r\n", "Error");
						bInvalidValue = TRUE;
					}
				}
			}
			break;

		default:
			if(pRange->MaxValue < pRange->MinValue)
			{
				ShowRC_CC(hDlg, 0, 0, 0,
							"MaxValue should be bigger that MinValue\r\n", "Error");
				bInvalidValue = TRUE;
			}
			else
			{
				if(pRange->DefaultValue < pRange->MinValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"The DefaultValue should be bigger or equal to MinValue\r\n", "Error");
					bInvalidValue = TRUE;
				}
				else if(pRange->DefaultValue > pRange->MaxValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"The DefaultValue should be smaller or equal to MaxValue\r\n", "Error");
					bInvalidValue = TRUE;
				}

				if(pRange->CurrentValue < pRange->MinValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"The CurrentValue should be bigger or equal to MinValue\r\n", "Error");
					bInvalidValue = TRUE;
				}
				else if(pRange->CurrentValue > pRange->MaxValue)
				{
					ShowRC_CC(hDlg, 0, 0, 0,
								"The CurrentValue should be smaller or equal to MaxValue\r\n", "Error");
					bInvalidValue = TRUE;
				}
			}
			break;
	}
	return bInvalidValue;
}