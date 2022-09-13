// -*-C-*-
////////////////////////////////////////////////////////////////////////////////
//
//File:         twacker.h
//RCS:          $Header: $
//Description:  Exerciser application program for Twain
//Author:       TWAIN Working Group
//Created:      Jan 15,92
//Modified:     "
//Language:     C
//Package:      N/A
//Status:       Test tool
//
//(c) Copyright 1992, Hewlett-Packard Company, all rights reserved.
//
/*	Copyright ?1998 TWAIN Working Group: Adobe Systems Incorporated, 
*	Canon Information Systems, Eastman Kodak Company, 
*	Fujitsu Computer Products of America, Genoa Technology, 
*	Hewlett-Packard Company, Intel Corporation, Kofax Image Products, 
*	JFL Peripheral Solutions Inc., Ricoh Corporation, and Xerox Corporation.  
*	All rights reserved.
*/
//

#ifndef _inc_twacker_h
#define _inc_twacker_h

#ifdef _WIN32
#define HUGE
#else
#define HUGE huge
#endif //_WIN32

#include "twain.h"
#include "table.h"
#include "FreeImage.h"

#ifdef _DEBUG
#pragma comment (lib, "FreeImageLib.lib")
#else
#pragma comment (lib, "FreeImageLib.lib")
#endif //_DEBUG


//
//--------------------------------------------------------------------
//					D e f i n i t i o n s
//--------------------------------------------------------------------


#define TWCRC_SUCCESS			 TWRC_SUCCESS          //  0
#define TWCRC_FAILURE			-TWRC_FAILURE          // -1 /* Application may get TW_STATUS for info on failure */
#define TWCRC_CHECKSTATUS		-TWRC_CHECKSTATUS      // -2 /* "tried hard"; get status                  */
#define TWCRC_CANCEL			-TWRC_CANCEL           // -3
#define TWCRC_DSEVENT			-TWRC_DSEVENT          // -4
#define TWCRC_NOTDSEVENT		-TWRC_NOTDSEVENT       // -5
#define TWCRC_XFERDONE			-TWRC_XFERDONE         // -6
#define TWCRC_ENDOFLIST			-TWRC_ENDOFLIST        // -7 /* After MSG_GETNEXT if nothing left         */
#define TWCRC_INFONOTSUPPORTED	-TWRC_INFONOTSUPPORTED // -8
#define TWCRC_DATANOTAVAILABLE	-TWRC_DATANOTAVAILABLE // -9

#define TWCRC_SCAN_CANCELED		-66

/* Condition Codes: Application gets these by doing DG_CONTROL DAT_STATUS MSG_GET.  */

//	#define TWCRC_SUCCESS            TWRC_SUCCESS            /* 0 It worked!                                */
#define TWCRC_BUMMER             TWCC_BUMMER             /* 1 Failure due to unknown causes             */
#define TWCRC_LOWMEMORY          TWCC_LOWMEMORY          /* 2 Not enough memory to perform operation    */
#define TWCRC_NODS               TWCC_NODS               /* 3 No Data Source                            */
#define TWCRC_MAXCONNECTIONS     TWCC_MAXCONNECTIONS     /* 4 DS is connected to max possible applications      */
#define TWCRC_OPERATIONERROR     TWCC_OPERATIONERROR     /* 5 DS or DSM reported error, application shouldn't   */
#define TWCRC_BADCAP             TWCC_BADCAP             /* 6 Unknown capability                        */
#define TWCRC_BADPROTOCOL        TWCC_BADPROTOCOL        /* 9 Unrecognized MSG DG DAT combination       */
#define TWCRC_BADVALUE           TWCC_BADVALUE           /*10 Data parameter out of range              */
#define TWCRC_SEQERROR           TWCC_SEQERROR           /*11 DG DAT MSG out of expected sequence      */
#define TWCRC_BADDEST            TWCC_BADDEST            /*12 Unknown destination Application/Source in DSM_Entry */
#define TWCRC_CAPUNSUPPORTED     TWCC_CAPUNSUPPORTED     /*13 Capability not supported by source            */
#define TWCRC_CAPBADOPERATION    TWCC_CAPBADOPERATION    /*14 Operation not supported by capability         */
#define TWCRC_CAPSEQERROR        TWCC_CAPSEQERROR        /*15 Capability has dependancy on other capability */
/* Added 1.8 */
#define TWCRC_DENIED             TWCC_DENIED             /*16 File System operation is denied (file is protected) */
#define TWCRC_FILEEXISTS         TWCC_FILEEXISTS         /*17 Operation failed because file already exists. */
#define TWCRC_FILENOTFOUND       TWCC_FILENOTFOUND       /*18 File not found */
#define TWCRC_NOTEMPTY           TWCC_NOTEMPTY           /*19 Operation failed because directory is not empty */
#define TWCRC_PAPERJAM           TWCC_PAPERJAM           /*20 The feeder is jammed */
#define TWCRC_PAPERDOUBLEFEED    TWCC_PAPERDOUBLEFEED    /*21 The feeder detected multiple pages */
#define TWCRC_FILEWRITEERROR     TWCC_FILEWRITEERROR     /*22 Error writing the file (meant for things like disk full conditions) */
#define TWCRC_CHECKDEVICEONLINE  TWCC_CHECKDEVICEONLINE  /*23 The device went offline prior to or during this operation */

// 추가 실패 원인

// CALL_ScanByProductName 함수
#define TWCRC_NO_DEVICE				24
#define TWCRC_WRONG_DEVICE			25
#define TWCRC_DEVICE_OFF			26
#define TWCRC_ACQ_FAILURE			27

// CapNego_PixelType_BitDepth 함수
#define TWCRC_CAP_GET_FAILED		28	// not supported cap negotiation
#define	TWCRC_CAP_SET_FAILED		29
#define TWCRC_CAP_NOT_CHANGED		30
#define TWCRC_CAP_NOT_DEFINED		31
#define TWCRC_CAP_DSM_OPEN_FAILED	32
#define TWCRC_CAP_DS_OPEN_FAILED	33

#define TWCRC_DSM_OPEN_FAILED		34
#define TWCRC_DS_OPEN_FAILED		35

#define TWCRC_NOT_SELECTED			36
#define TWCRC_NOT_SUPPORTED			37
#define TWCRC_GETITEMTYPE_FAILED	38
#define TWCRC_NOTINTHELIST			39
#define TWCRC_NOT_FRAME				40
#define TWCRC_NOT_ARRAY				41
#define TWCRC_NOT_STR				42
#define TWCRC_NOT_SUPPORTED_FORMAT	43

// 스캔 상태
#define TWCRC_HOPPER_EMPTY			44
#define TWCRC_SCAN_DONE				45


#define NOT_SELECTED	101
#define STRING_SOURCE	102
#define SELECT_SOURCE	103




//
//--------------------------------------------------------------------
//					V a r i a b l e s
//--------------------------------------------------------------------

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT16   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_AU16, FAR * pTW_CON_AU16;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_INT8	  ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EI8, FAR * pTW_CON_EI8;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_INT16   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EI16, FAR * pTW_CON_EI16;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_INT32   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EI32, FAR * pTW_CON_EI32;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_UINT8	  ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EU8, FAR * pTW_CON_EU8;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_UINT16   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EU16, FAR * pTW_CON_EU16;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_UINT32   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EU32, FAR * pTW_CON_EU32;


typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_FIX32   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EF32, FAR * pTW_CON_EF32;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_BOOL    ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EBOOL, FAR * pTW_CON_EBOOL;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_FRAME   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_EFRAME, FAR * pTW_CON_EFRAME;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_STR32   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_ES32, FAR * pTW_CON_ES32;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_STR64   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_ES64, FAR * pTW_CON_ES64;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_STR128   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_ES128, FAR * pTW_CON_ES128;

typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;			/* How many items in ItemList								*/
   TW_UINT32  CurrentIndex;		/* Current value is in ItemList[CurrentIndex]				*/
   TW_UINT32  DefaultIndex;		/* Powerup value is in ItemList[DefaultIndex]				*/
   TW_STR255   ItemList[128];	/* Array of ItemType values starts here, 256 Byte reserved	*/
} TW_CON_ES255, FAR * pTW_CON_ES255;

typedef TW_RANGE	TW_CON_RANGE, FAR * pTW_CON_RANGE;
typedef TW_ONEVALUE	TW_CON_ONEVAL, FAR * pTW_CON_ONEVAL;


// /* TWON_ENUMERATION. Container for a collection of values. */
// typedef struct {
//    TW_UINT16  ItemType;
//    TW_UINT32  NumItems;     /* How many items in ItemList                 */
//    TW_UINT32  CurrentIndex; /* Current value is in ItemList[CurrentIndex] */
//    TW_UINT32  DefaultIndex; /* Powerup value is in ItemList[DefaultIndex] */
//    TW_UINT8   ItemList[1];  /* Array of ItemType values starts here       */
// } TW_ENUMERATION, FAR * pTW_ENUMERATION;
// 
// /* TWON_ONEVALUE. Container for one value. */
// typedef struct {
//    TW_UINT16  ItemType;
//    TW_UINT32  Item;
// } TW_ONEVALUE, FAR * pTW_ONEVALUE;
// 
// /* TWON_RANGE. Container for a range of values. */
// typedef struct {
//    TW_UINT16  ItemType;
//    TW_UINT32  MinValue;     /* Starting value in the range.           */
//    TW_UINT32  MaxValue;     /* Final value in the range.              */
//    TW_UINT32  StepSize;     /* Increment from MinValue to MaxValue.   */
//    TW_UINT32  DefaultValue; /* Power-up value.                        */
//    TW_UINT32  CurrentValue; /* The value that is currently in effect. */
// } TW_RANGE, FAR * pTW_RANGE;

typedef TW_ONEVALUE TW_CON_OU16, FAR * pTW_CON_OU16, TW_CON_OF32, FAR * pTW_CON_OF32;
typedef TW_RANGE TW_CON_RF32, FAR * pTW_CON_RF32;

typedef struct {
	TW_UINT16 contype;			// TWON_ENUMERATION or TWON_ONEVALUE
	TW_UINT16 support;			// Support = TRUE, Not Support = FALSE
	TW_UINT16 set;				// Set = TRUE, Reset = FALSE
	TW_CON_EU16	eu16;			// TW_ENUMERATION	, ItemType = TWTY_UINT16
	TW_CON_OU16	ou16;			// TW_ONEVALUE		, ItemType = TWTY_UINT16
} TW_EO_U16, FAR * pTW_EO_U16;

typedef struct {
	TW_UINT16 contype;			// TWON_ENUMERATION or TWON_ONEVALUE or TWON_RANGE
	TW_UINT16 support;			// Support = TRUE, Not Support = FALSE
	TW_UINT16 set;				// Set = TRUE, Reset = FALSE
	TW_CON_EF32 ef32;			// TW_ENUMERATION	, ItemType = TWTY_FIX32
	TW_CON_OF32	of32;			// TW_ONEVALUE		, ItemType = TWTY_FIX32
	TW_CON_RF32 rf32;			// TW_RANGE			, ItemType = TWTY_FIX32
} TW_EORF32, FAR * pTW_EORF32;

typedef struct {
	TW_EO_U16 bitdepth;
	TW_EORF32 brightness;
	TW_EORF32 contrast;
	TW_EORF32 gamma;
	TW_EORF32 highlight;
	TW_EO_U16 lightsource;
	TW_EO_U16 pixelfalvor;
	TW_EO_U16 pixeltype;
	TW_EORF32 rotation;
	TW_EORF32 shadow;
	TW_EO_U16 supportedsizes;
	TW_EORF32 threshold;
	TW_EO_U16 units;
	TW_EORF32 resolution;		// x, y resolution 항상 값 같게 세팅
} TW_SETTING, FAR * pTW_SETTING;

//
//--------------------------------------------------------------------
//					F u n c t i o n s
//--------------------------------------------------------------------
//

TW_INT16 InitTwainCombo(HWND hDlg, TW_INT16 Id, pTABLEENTRY pTable, int nSizeTable);
BOOL CloseApplication(HWND hWnd);

int GetType(TW_UINT16 cap, pTW_UINT16 pConType, pTW_UINT16 pItemType);

//////////////////////////////////////////////////////////////////////////
// ARRAY
//
int SetArrayI8(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayI16(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayI32(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayU8(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayU16(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayU32(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기
int SetArrayBool(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum);						// 여기




//////////////////////////////////////////////////////////////////////////
// ENUMERATION
//
// TW_INT8
int GetEnumI8(TW_UINT16 cap, pTW_CON_EI8 pConEnum);
int SetEnumI8(TW_UINT16 cap, pTW_CON_EI8 pConSrc, TW_INT8 val);
// TW_INT16
int GetEnumI16(TW_UINT16 cap, pTW_CON_EI16 pConEnum);
int SetEnumI16(TW_UINT16 cap, pTW_CON_EI16 pConSrc, TW_INT16 val);	
// TW_INT32
int GetEnumI32(TW_UINT16 cap, pTW_CON_EI32 pConEnum);				
int SetEnumI32(TW_UINT16 cap, pTW_CON_EI32 pConSrc, TW_INT32 val);
// TW_UINT8
int GetEnumU8(TW_UINT16 cap, pTW_CON_EU8 pConEnum);
int SetEnumU8(TW_UINT16 cap, pTW_CON_EU8 pConSrc, TW_UINT8 val);		
// TW_UINT16
int GetEnumU16(TW_UINT16 cap, pTW_CON_EU16 pConEnum);
int SetEnumU16(TW_UINT16 cap, pTW_CON_EU16 pConSrc, TW_UINT16 val);
// TW_UINT32
int GetEnumU32(TW_UINT16 cap, pTW_CON_EU32 pConEnum);						
int SetEnumU32(TW_UINT16 cap, pTW_CON_EU32 pConSrc, TW_UINT32 val);		
// TW_FIX32
int GetEnumF32(TW_UINT16 cap, pTW_CON_EF32 pConEnum);
int SetEnumF32(TW_UINT16 cap, pTW_CON_EF32 pConSrc, TW_FIX32 val);
// TW_BOOL
int GetEnumBool(TW_UINT16 cap, pTW_CON_EBOOL pConEnum);					
int SetEnumBool(TW_UINT16 cap, pTW_CON_EBOOL pConSrc, TW_BOOL val);		

//////////////////////////////////////////////////////////////////////////
//	Range
//
// For All Item Types
int GetRange(TW_UINT16 cap, pTW_CON_RANGE pConRng);						
int SetRange(TW_UINT16 cap, pTW_CON_RANGE pConSrc, pTW_UINT32 pVal);		// 주의 : 마지막 인자 value 주소값	

//////////////////////////////////////////////////////////////////////////
//	OneValue
//
// TW_INT8
int SetOneValI8(TW_UINT16 cap, TW_INT8 val);
// TW_INT16
int SetOneValI16(TW_UINT16 cap, TW_INT16 val);
// TW_INT32
int SetOneValI32(TW_UINT16 cap, TW_INT32 val);				
// TW_UINT8
int SetOneValU8(TW_UINT16 cap, TW_UINT8 val);
// TW_UINT16
int SetOneValU16(TW_UINT16 cap, TW_UINT16 val);
// TW_UINT32
int SetOneValU32(TW_UINT16 cap, TW_UINT32 val);
// TW_FIX32
int SetOneValF32(TW_UINT16 cap, TW_FIX32 val);
// TW_BOOL
int SetOneValBool(TW_UINT16 cap, TW_BOOL val);


//////////////////////////////////////////////////////////////////////////
// Frame
//
int SetArrayFrame(TW_UINT16 cap, pTW_FRAME pFrm, int itemNum);

int GetEnumFrame(TW_UINT16 cap, pTW_CON_EFRAME pConDst);
int SetEnumFrame(TW_UINT16 cap, pTW_CON_EFRAME pConSrc, pTW_FRAME pFrm);

int SetOneValFrame(TW_UINT16 cap, pTW_FRAME pFrm);


//////////////////////////////////////////////////////////////////////////
// STRXXX
//
int SetArrayS32(TW_UINT16 cap, char* szSrc, int itemNum);
int SetArrayS64(TW_UINT16 cap, char* szSrc, int itemNum);
int SetArrayS128(TW_UINT16 cap, char* szSrc, int itemNum);
int SetArrayS255(TW_UINT16 cap, char* szSrc, int itemNum);


int GetEnumS32(TW_UINT16 cap, pTW_CON_ES32 pConDst);
int GetEnumS64(TW_UINT16 cap, pTW_CON_ES64 pConDst);
int GetEnumS128(TW_UINT16 cap, pTW_CON_ES128 pConDst);
int GetEnumS255(TW_UINT16 cap, pTW_CON_ES255 pConDst);
				

int SetEnumS32(TW_UINT16 cap, pTW_CON_ES32 pConSrc, pTW_STR32 szSrc);
int SetEnumS64(TW_UINT16 cap, pTW_CON_ES64 pConSrc, pTW_STR64 szSrc);
int SetEnumS2128(TW_UINT16 cap, pTW_CON_ES128 pConSrc, pTW_STR128 szSrc);
int SetEnumS255(TW_UINT16 cap, pTW_CON_ES255 pConSrc, pTW_STR255 szSrc);

int SetOneValS32(TW_UINT16 cap, pTW_STR32 szSrc);
int SetOneValS64(TW_UINT16 cap, pTW_STR64 szSrc);
int SetOneValS128(TW_UINT16 cap, pTW_STR128 szSrc);
int SetOneValS255(TW_UINT16 cap, pTW_STR255 szSrc);





#endif //_inc_twacker_h

