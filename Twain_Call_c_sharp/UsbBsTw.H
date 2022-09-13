/*********************************************************
All Rights Reserved, Copyright(C)2002 Hitachi, Ltd.
Name		: USB版 Blinkscan ＴＷＡＩＮドライバ
			　USB版 Blinkscanカスタム機能共通インクルード
Function	: 定数、構造体定義
Date		: 2002.02.12
Author		: Y.Futamura(Hitachi SAS)
Revision History:
R0			  2002.02.12    Futamura（Blinkscan R3.23より変更）
R1			  2002.02.15    Futamura（Farmware機能追加）
R2			  2002.04.23    Futamura（条件コード整理）
**********************************************************/

/************************************************************/
/* Structure Definitions									*/
/************************************************************/
/* DAT_ADTOPLEVEL */
typedef struct {															// P100QC005
	TW_UINT16		TopLevel;				/* A/Dﾄｯﾌﾟﾚﾍﾞﾙ				*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_ADTOPLEVEL, FAR * pTW_ADTOPLEVEL;										// P100QC005

/* DAT_AUTOSENS */
typedef struct {															// P100QC010
	TW_UINT16		Status;					/* 自動感度調整有／無		*/	// P100QC010
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC010
} TW_AUTOSENS, FAR * pTW_AUTOSENS;											// P100QC010

/* DAT_DEVRESET */	// QR-S DUMMY
typedef struct {															// P100QC060
	TW_UINT16		ResetMode;				/* リセットモード			*/	// P100QC060
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC060
} TW_DEVRESET, FAR * pTW_DEVRESET;											// P100QC060

/* DAT_DRIVERVERSION */
typedef struct {															// P100QC005
	TW_UINT8		driver[12];				/* ドライバVer(VVRRTTNN\0)	*/	// P100QC005
	TW_UINT32		fpga;					/* FPGA Ver					*/	// P100QC010
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_DRIVERVERSION, FAR * pTW_DRIVERVERSION;								// P100QC005

/* DAT_DOWNLOAD */
typedef struct {															// P100UBS001
	TW_UINT32		Type;					/* データタイプ				*/	// P100UBS001
	TW_UINT32		Data[65536];			/* ダウンロードデータ		*/	// P100UBS001
	TW_UINT32		Revision;				/* データレビジョン			*/	// P100UBS001
	TW_UINT16		Reserved;				/* 予備						*/	// P100UBS001
} TW_DOWNLOAD, FAR * pTW_DOWNLOAD;											// P100UBS001

/* DAT_ERRORLED */
typedef struct {															// P100QC005
	TW_UINT16		Status;					/* レディランプ状態			*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_ERRORLED, FAR * pTW_ERRORLED;											// P100QC005

/* DAT_HTERRINF */
typedef struct {															// P100QC013
	TW_UINT8		Code[6];				/* 障害コード				*/	// P100QC013
	TW_UINT16		Unit;					/* 装置番号					*/	// P100QC013
	TW_UINT8		hms[6];					/* 時・分・秒				*/	// P100QC013
	TW_UINT16		ms;						/* ミリ秒					*/	// P100QC013
	TW_UINT8		mm;						/* 月						*/	// P100QC013
	TW_UINT8		dd;						/* 日						*/	// P100QC013
	TW_UINT32		Status;					/* 内部状態					*/	// P100QC013
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC013
} TW_HTERRREC, FAR * pTW_HTERRREC;											// P100QC013

/* DAT_HTERRINF */
typedef struct {															// P100QC013
	TW_UINT16		Count;					/* ｴﾗｰﾚｺｰﾄﾞのエントリ数		*/	// P100QC013
	TW_HTERRREC		HtErrRec[16];			/* エラーレコード			*/	// P100QC013
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC013
} TW_HTERRTBL, FAR * pTW_HTERRTBL;											// P100QC013

/* DAT_LASTERROR */
typedef struct {																			// PC000013
	TW_INT8			LastError[6];							/* 拡張エラーコード			*/	// P100C000
	TW_UINT16		Reserved;								/* 予備						*/	// PC000013
} TW_LASTERROR, FAR * pTW_LASTERROR;														// PC000013

/* DAT_LIGHTCHECK */
typedef struct {															// P100QC010
	TW_UINT16		Status;					/* 光量チェック有／無		*/	// P100QC010
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC010
} TW_LIGHTCHECK, FAR * pTW_LIGHTCHECK;										// P100QC010

/* DAT_MARKSPECT */
typedef struct {															// P100QC013
	TW_UINT16		Level[6];				/* 白マーク内光量測定結果	*/	// P100QC013
	TW_UINT16		BLevel[6];				/* 黒マーク内光量測定結果	*/	// P100QC027
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC013
} TW_MARKSPECT, FAR * pTW_MARKSPECT;										// P100QC013

/* DAT_PCIINFO */
typedef struct {															// P100QC005
	TW_UINT32		BusNO;					/* バスNO					*/	// P100QC005
	TW_UINT32		DeviceNumber;			/* デバイスNO				*/	// P100QC005
	TW_UINT32		baseAdd0;				/* データベースアドレス		*/	// P100QC005
	TW_UINT16		baseAdd1;				/* レジスタベースアドレス	*/	// P100QC005
	TW_UINT16		Command;				/* コマンドレジスタ			*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_PCIINFO, FAR * pTW_PCIINFO;											// P100QC005

/* DAT_POWER */
typedef struct {															// P100QC005
	TW_UINT16		Status;					/* カメラパワー状態			*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_POWER, FAR * pTW_POWER;												// P100QC005

/* DAT_READYLED */
typedef struct {															// P100QC005
	TW_UINT16		Status;					/* レディランプ状態			*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_READYLED, FAR * pTW_READYLED;											// P100QC005

/* DAT_SCANCOUNT */
typedef struct {															// P100UBS001
	TW_UINT32		ScanCount;				/* 読み込み回数				*/	// P100UBS001
	TW_UINT16		Reserved;				/* 予備						*/	// P100UBS001
} TW_SCANCOUNT, FAR * pTW_SCANCOUNT;										// P100UBS001

/* DAT_SW */
typedef struct {															// P100QC005
	TW_UINT16		SwStatus;				/* スイッチ状態				*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_SW, FAR * pTW_SW;														// P100QC005

/* DAT_USBINFO */
typedef struct  {															// P100UBS001
    TW_UINT8		bLength;												// P100UBS001
    TW_UINT8		bDescriptor;											// P100UBS001
    TW_UINT16		wTotalLength;											// P100UBS001
    TW_UINT8		bNumInterface;											// P100UBS001
    TW_UINT8		bConfigurationValue;									// P100UBS001
    TW_UINT8		iConfiguration;											// P100UBS001
    TW_UINT8		bmAttributes;											// P100UBS001
    TW_UINT8		MaxPower;												// P100UBS001
} TW_USB_CONFIGURATION, FAR * pTW_USB_CONFIGURATION;						// P100UBS001

/* DAT_USBINFO */
typedef struct  {															// P100UBS001
    TW_UINT8		bLength;												// P100UBS001
    TW_UINT8		bDescriptor;											// P100UBS001
    TW_UINT8		bInterfaceNumber;										// P100UBS001
    TW_UINT8		bAlternateSetting;										// P100UBS001
    TW_UINT8		bNumEndpoints;											// P100UBS001
    TW_UINT8		bInterfaceClass;										// P100UBS001
    TW_UINT8		bInterfaceSubClass;										// P100UBS001
    TW_UINT8		bInterfaceProtocol;										// P100UBS001
    TW_UINT8		iInterface;												// P100UBS001
} TW_USB_INTERFACE, FAR * pTW_USB_INTERFACE;								// P100UBS001

/* DAT_USBINFO */
typedef struct  {															// P100UBS001
    TW_UINT8		bLength;												// P100UBS001
    TW_UINT8		bDescriptor;											// P100UBS001
    TW_UINT8		bEndpointAddress;										// P100UBS001
    TW_UINT8		bmAttributes;											// P100UBS001
    TW_UINT16		wMaxPacketSize;											// P100UBS001
    TW_UINT8		bInterval;												// P100UBS001
} TW_USB_ENDPOINT, FAR * pTW_USB_ENDPOINT;									// P100UBS001

/* DAT_USBINFO */
typedef struct	{															// P100UBS001
	TW_USB_CONFIGURATION	cd;				/* USB構成ﾃﾞｨｽｸﾘﾌﾟﾀ			*/	// P100UBS001
	TW_USB_INTERFACE		id;				/* USBｲﾝﾀｰﾌｪｰｽﾃﾞｨｽｸﾘﾌﾟﾀ		*/	// P100UBS001
	TW_USB_ENDPOINT			ed[2];			/* USBｴﾝﾄﾞﾎﾟｲﾝﾄﾃﾞｨｽｸﾘﾌﾟﾀ	*/	// P100UBS001
	TW_UINT16				Reserved;		/* 予備						*/	// P100UBS001
} TW_USBINFO, FAR * pTW_USBINFO;											// P100UBS001

/* DAT_WHITEBALANCE */
typedef struct {															// P100QC005
	TW_UINT8		WR;						/* ﾎﾜｲﾄﾊﾞﾗﾝｽ R の値 0～255	*/	// P100QC010
	TW_UINT8		WGr;					/* ﾎﾜｲﾄﾊﾞﾗﾝｽ Grの値 0～255	*/	// P100QC010
	TW_UINT8		WGb;					/* ﾎﾜｲﾄﾊﾞﾗﾝｽ Gbの値 0～255	*/	// P100QC010
	TW_UINT8		WB;						/* ﾎﾜｲﾄﾊﾞﾗﾝｽ B の値 0～255	*/	// P100QC010
	TW_UINT8		BR;						/* ﾌﾞﾗｯｸﾊﾞﾗﾝｽR の値 0～255	*/	// P100QC010
	TW_UINT8		BGr;					/* ﾌﾞﾗｯｸﾊﾞﾗﾝｽGrの値 0～255	*/	// P100QC010
	TW_UINT8		BGb;					/* ﾌﾞﾗｯｸﾊﾞﾗﾝｽGbの値 0～255	*/	// P100QC010
	TW_UINT8		BB;						/* ﾌﾞﾗｯｸﾊﾞﾗﾝｽB の値 0～255	*/	// P100QC010
	TW_UINT16		CameraGain;				/* ﾎﾜｲﾄﾊﾞﾗﾝｽ取得時のｶﾒﾗｹﾞｲﾝ	*/	// P100QC005
	TW_UINT16		Reserved;				/* 予備						*/	// P100QC005
} TW_WHITEBALANCE, FAR * pTW_WHITEBALANCE;									// P100QC005

/************************************************************/
/* Generic Constants										*/
/************************************************************/
// Auto Sensitive ON/OFF for TW_AUTOSENS
#define	TWAS_OFF			0x00			//	自動感度調整 設定 "OFF"		// P100QC010
#define	TWAS_ON				0x01			//	自動感動調整 設定 "ON"		// P100QC010

// Ready LED ON/OFF for TW_READYLED
#define	TWRL_OFF			0x00			//	READY LED 設定 "OFF"		// P100QC005
#define	TWRL_ON				0x01			//	READY LED 設定 "ON"			// P100QC005

// Error LED ON/OFF for TW_ERRORLED
#define	TWEL_OFF			0x00			//	ERROR LED 設定 "OFF"		// P100QC005
#define	TWEL_ON				0x01			//	ERROR LED 設定 "ON"			// P100QC005

// Light Check ON/OFF for TW_LIGHTCHECK
#define	TWLC_OFF			0x00			//	光量チェック 設定 "OFF"		// P100QC010
#define	TWLC_ON				0x01			//	光量チェック 設定 "ON"		// P100QC010

// Camera Power ON/OFF for TW_POWER
#define	TWCP_OFF			0x00			//	Camera POWER 設定 "OFF"		// P100QC005
#define	TWCP_ON				0x01			//	Camera POWER 設定 "ON"		// P100QC005

// Start Switch Status for TW_SW
#define TWSW_OFF			0x00			//	Start Switch "OFF"			// P100QC005
#define TWSW_ON				0x01			//	Start Switch "ON"			// P100QC005

// A/D Top Level for TW_ADTOPLEVEL
#define TWTL_LOW			0x00			//	低レベル					// P100QC002
#define TWTL_HIGH			0x01			//	高レベル					// P100QC002

// Camera Gain for TW_WHITEBALANCE
#define TWCG_SMALL			0x01			//	カメラゲイン 小				// P100QC010
#define TWCG_NORMAL			0x02			//	カメラゲイン 標準			// P100QC010
#define TWCG_BIG			0x03			//	カメラゲイン 大				// P100QC010

// Device Reset(QR-S DUMMY) 
#define TWDR_HARD			0				// HardWareリセット				// P100QC060
#define TWDR_SOFT			1				// SoftWareリセット				// P100QC060

// Download 
#define TWDL_SH				1				// ＳＨマイクロプログラム		// P100UBS001
#define TWDL_FPGA			2				// ＦＰＧＡ論理データ			// P100UBS001

/************************************************************/
/* Capability Constants										*/
/************************************************************/
// ICAP_COLORGRAY
#define TWCG_OFF			0x00			// 行わない						// P100QC002
#define TWCG_FAST			0x01			// 高速変換（G成分抽出）		// P100QC002
#define TWCG_FINE			0x02			// 高精細変換					// P100QC002

// ICAP_FILECOMP
#define TWFC_OFF			0x00			// 無し							// P100QC002
#define TWFC_SMOOTH1		0x01			// 平滑化（弱）					// P100QC002
#define TWFC_SMOOTH2		0x02			// 平滑化（強）					// P100QC002
#define TWFC_EDGE1			0x03			// エッジ強調（弱）				// P100QC002
#define TWFC_EDGE2			0x04			// エッジ強調（強）				// P100QC002
#define TWFC_CONTRAST		0x05			// コントラスト補正				// P100QC002

// ICAP_GAMMACORRECT
#define TWGC_GAMMA1			0x01			// γ１							// P100QC002
#define TWGC_GAMMA2			0x02			// γ２							// P100QC002

// ICAP_GENERATEPIXEL
//#define TWGP_HIGHSPEED	0x00			// 高速							// P100QC002
#define TWGP_STANDARD		0x01			// 標準							// P100QC002
//#define TWGP_HIGHDETAIL	0x02			// 高精細						// P100QC002
#define TWGP_THROUGH		0x03			// スルー						// P100QC002

// ICAP_SCANMODE
#define TWSM_ONEFRAME		0x01			// １フレームモード（１Ｐ）		// P100QC002
#define TWSM_HIGHSPEED		0x09			// 高速モード（９Ｐ）			// P100QC002
#define TWSM_STANDARD		0x12			// 標準モード（１８Ｐ）			// P100QC002
#define TWSM_HIGHQUALITY	0x24			// 高画質モード（３６Ｐ）		// P100QC002

// ICAP_NOSCAN
#define TWNS_NEW			0x00			// 新規画像読み込み				// P100QC038
#define TWNS_EXIST			0x01			// 既存画像読み込み				// P100QC038

// ICAP_PREVENTREFLECTION
#define TWPR_STANDARD		0x00			// 標準							// P100QC058
#define TWPR_PREVENT		0x01			// 反射抑制						// P100QC058
/************************************************************/
/* Data Argument Types										*/
/************************************************************/
#define DAT_SW				DAT_CUSTOMBASE + 1				// スイッチ状態	// P100QC005
#define DAT_DEVRESET		DAT_CUSTOMBASE + 2				// ＳＣＳＩリセット(QR-S DUMMY)	// P100QC060
#define DAT_SHADING			DAT_CUSTOMBASE + 3				// ｼｪｰﾃﾞｨﾝｸﾞﾃﾞｰﾀ// P100QC005
#define DAT_LASTERROR		DAT_CUSTOMBASE + 6				/* 拡張エラーコード取得		*/	// PC000013
//#define DAT_PEDESTALMARK	DAT_CUSTOMBASE + 8				// マーク検出	// P100C003
#define DAT_WHITEBALANCE	DAT_CUSTOMBASE + 9				// ﾎﾜｲﾄﾊﾞﾗﾝｽﾃﾞｰﾀ// P100C004
#define	DAT_PCIINFO			DAT_CUSTOMBASE + 10				// PCI情報		// P100QC005
//#define	DAT_LENSCORRECT	DAT_CUSTOMBASE + 11				// ﾚﾝｽﾞ歪み補正	// P100QC005
//#define	DAT_BRIGHT		DAT_CUSTOMBASE + 12				// 明るさ		// P100QC005
#define	DAT_ADTOPLEVEL		DAT_CUSTOMBASE + 13				// A/Dﾄｯﾌﾟﾚﾍﾞﾙ	// P100QC005
#define	DAT_DRIVERVERSION	DAT_CUSTOMBASE + 14				// ﾄﾞﾗｲﾊﾞVer	// P100QC005
#define	DAT_ERRORLED		DAT_CUSTOMBASE + 15				// ｴﾗｰLED状態	// P100QC005
#define	DAT_HTERRINF		DAT_CUSTOMBASE + 16				// 障害情報		// P100QC005
#define	DAT_POWER			DAT_CUSTOMBASE + 17				// ｶﾒﾗﾊﾟﾜｰ状態	// P100QC005
#define	DAT_READYLED		DAT_CUSTOMBASE + 18				// ﾚﾃﾞｨLED状態	// P100QC005
#define	DAT_SDRAM			DAT_CUSTOMBASE + 19				// SDRAMﾁｪｯｸ	// P100QC005
#define DAT_ADJRESOLUTION	DAT_CUSTOMBASE + 20				// 解像度調整	// P100QC010
#define	DAT_AUTOSENS		DAT_CUSTOMBASE + 21				// 自動感度調整	// P100QC010
#define DAT_LIGHTCHECK		DAT_CUSTOMBASE + 22				// 光量判定		// P100QC010
#define	DAT_MARKSPECT		DAT_CUSTOMBASE + 23				// 光量測定結果	// P100QC013
#define DAT_RSHADING		DAT_CUSTOMBASE + 24				// 反射抑制用ｼｪｰﾃﾞｨﾝｸﾞ	// P100QC058
#define	DAT_USBINFO			DAT_CUSTOMBASE + 25				// USB情報		// P100UBS001
#define	DAT_SCANCOUNT		DAT_CUSTOMBASE + 26				// 読取り回数(保守用)	// P100UBS001
#define	DAT_DOWNLOAD		DAT_CUSTOMBASE + 27				// ダウンロード(保守用)	// P100UBS001

/************************************************************/
/* Messages                                                 */
/************************************************************/
#define	MSG_CHECK			MSG_CUSTOMBASE + 1				// SDRAMﾁｪｯｸ	// P100QC005

/************************************************************/
/* Capabilities 											*/
/************************************************************/
#define ICAP_FLICKER		CAP_CUSTOMBASE + 1		// ちらつき補正の有無(QR-S DUMMY)		// P100QC060
#define ICAP_SHADING		CAP_CUSTOMBASE + 2		// ｼｪｰﾃﾞｨﾝｸﾞ補正の有無	// P100QC002
#define ICAP_GEOMETRIC		CAP_CUSTOMBASE + 3		// 幾何学補正の有無(QR-S DUMMY)			// P100QC060
#define ICAP_NOSCAN			CAP_CUSTOMBASE + 4		// 新規／既存画像読取り設定	// P100QC038
#define ICAP_AFLTBIN		CAP_CUSTOMBASE + 5		// ２値化濃度設定α
#define ICAP_BFLTBIN		CAP_CUSTOMBASE + 6		// ２値化濃度設定β
#define ICAP_PKDWNLMT		CAP_CUSTOMBASE + 7		// ピークダウンリミッタ(QR-S DUMMY)		// P100QC060
#define ICAP_ALLTHROUGH		CAP_CUSTOMBASE + 8		// 全スルーモード(QR-S DUMMY)			// P100QC060
#define ICAP_DPIRESOLUTION	CAP_CUSTOMBASE + 17		// 解像度（DPI）(QR-S DUMMY)			// P100QC060
#define ICAP_WHITEFLICKER	CAP_CUSTOMBASE + 18		// 白ちらつき率(QR-S DUMMY)				// P100QC060
#define ICAP_BLACKFLICKER	CAP_CUSTOMBASE + 19		// 黒ちらつき率(QR-S DUMMY)				// P100QC060
#define ICAP_PHOTODIODEMAX	CAP_CUSTOMBASE + 20		// フォトダイオード最大値(QR-S DUMMY)	// P100QC060
#define ICAP_SCANGAIN		CAP_CUSTOMBASE + 21		// スキャナ読み取りゲイン値(QR-S DUMMY)	// P100QC060
//#define ICAP_BRRED		CAP_CUSTOMBASE + 22		// カメラの明るさ(赤)	// P100C005
//#define ICAP_BRGREEN		CAP_CUSTOMBASE + 23		// カメラの明るさ(緑)	// P100C005
//#define ICAP_BRBLUE		CAP_CUSTOMBASE + 24		// カメラの明るさ(青)	// P100C005
//#define ICAP_BRGRAY		CAP_CUSTOMBASE + 25		// カメラの明るさ(ｸﾞﾚｰ)	// P100C005
#define	ICAP_FILTERCOMP		CAP_CUSTOMBASE + 26		// 二次元フィルター		// P100C006
//#define ICAP_LENS			CAP_CUSTOMBASE + 27		// レンズ歪補正			// P100C007
//#define ICAP_WBRED		CAP_CUSTOMBASE + 28		// ﾎﾜｲﾄﾊﾞﾗﾝｽﾃﾞｰﾀ(赤)	// P100C004
//#define ICAP_WBGREEN		CAP_CUSTOMBASE + 29		// ﾎﾜｲﾄﾊﾞﾗﾝｽﾃﾞｰﾀ(緑)	// P100C004
//#define ICAP_WBBLUE		CAP_CUSTOMBASE + 30		// ﾎﾜｲﾄﾊﾞﾗﾝｽﾃﾞｰﾀ(青)	// P100C004
//#define ICAP_WBGRAY		CAP_CUSTOMBASE + 31		// ﾎﾜｲﾄﾊﾞﾗﾝｽﾃﾞｰﾀ(ｸﾞﾚｰ)	// P100C004
//#define ICAP_BRIGHT		CAP_CUSTOMBASE + 32		// カメラの明るさ		// P100C005
//#define ICAP_ASBINLEVEL	CAP_CUSTOMBASE + 33		// ２値化レベル自動設定	// P100QC002
//#define ICAP_ASGAIN		CAP_CUSTOMBASE + 34		// カメラゲイン自動設定	// P100QC002
//#define ICAP_ASSPEED		CAP_CUSTOMBASE + 35		// ｼｬｯﾀｰｽﾋﾟｰﾄﾞ自動設定	// P100QC002
//#define ICAP_CAMERAGAIN	CAP_CUSTOMBASE + 36		// カメラゲイン			// P100QC002
#define ICAP_COLORGRAY		CAP_CUSTOMBASE + 37		// カラーグレー変換方式	// P100QC002
#define ICAP_GAMMACORRECT	CAP_CUSTOMBASE + 38		// カメラガンマ補正		// P100QC002
#define	ICAP_GENERATEPIXEL	CAP_CUSTOMBASE + 39		// 画素生成				// P100QC002
//#define ICAP_PCICLAMPLV	CAP_CUSTOMBASE + 40		// A/Dクランプレベル	// P100QC002
//#define ICAP_PCITOPLV		CAP_CUSTOMBASE + 41		// A/Dトップレベル		// P100QC002
#define	ICAP_SCANMODE		CAP_CUSTOMBASE + 42		// 画像取り込みモード	// P100QC002
#define ICAP_SHUTTERSPEED	CAP_CUSTOMBASE + 43		// シャッタースピード	// P100QC002
//#define ICAP_XPIEZO		CAP_CUSTOMBASE + 44		// 水平ピエゾ位置電圧値	// P100QC002
//#define ICAP_YPIEZO		CAP_CUSTOMBASE + 45		// 垂直ピエゾ位置電圧値	// P100QC002
#define ICAP_PRESCANTIME	CAP_CUSTOMBASE + 46		// 最大先読み時間		// P100QC008
#define ICAP_FPGAGAMMA		CAP_CUSTOMBASE + 47		// FPGAガンマ補正		// P100QC009
#define ICAP_RESOCORRECT	CAP_CUSTOMBASE + 48		// 解像度補正			// P100QC009
#define CAP_MOVIE			CAP_CUSTOMBASE + 49		// 動画像表示機能		// P100QC018
#define ICAP_RSHUTTERSPEED	CAP_CUSTOMBASE + 50		// 反射抑制用ｼｬｯﾀｽﾋﾟｰﾄﾞ	// P100QC058
#define ICAP_PREVENTREFLECTION	CAP_CUSTOMBASE + 51	// 反射抑制モード		// P100QC058

/************************************************************/
/* Condition Code 											*/
/************************************************************/
#define		TWCC_QRSC_NO_ERROR			0x0000		// 正常終了				// P100C002

// (System)
#define		TWCC_QRSC_SYS_ALLOC_ERR		0x8001		// メモリ異常（確保失敗）/ P100C002
#define		TWCC_QRSC_SYS_THREAD_ERR	0x8002		// スレッド生成失敗		// P100QC001

// (Image Read)
#define	TWCC_QRSC_ERROR_LIGHT_LOW			0x9301	// 光量過小							// P100QC013
#define	TWCC_QRSC_ERROR_LIGHT_HIGH			0x9303	// 光量過大							// P100QC013

// (STI Interface)
#define	TWCC_STI_CHANGENOEFFECT				0xA001	// デバイス設定変更無効				// P100UBS001
#define	TWCC_STIERR_OLD_VERSION				0xA002	// ＡＰのバージョンが古い			// P100UBS001
#define	TWCC_STIERR_BETA_VERSION			0xA003	// βバージョンのＡＰ				// P100UBS001
#define	TWCC_STIERR_BADDRIVER				0xA004	// ドライバの組み合わせ不正			// P100UBS001
#define	TWCC_STIERR_DEVICENOTREG			0xA005	// レジストリにデバイス指定無し		// P100UBS001
#define	TWCC_STIERR_OBJECTNOTFOUND			0xA006	// オブジェクトが無い				// P100UBS001
#define	TWCC_STIERR_INVALID_PARAM			0xA007	// パラメタ異常						// P100UBS001
#define	TWCC_STIERR_NOINTERFACE				0xA008	// インターフェース不正				// P100UBS001
#define	TWCC_STIERR_GENERIC					0xA009	// 一般的なエラー					// P100UBS001
#define	TWCC_STIERR_OUTOFMEMORY				0xA00A	// メモリ不足						// P100UBS001
#define	TWCC_STIERR_UNSUPPORTED				0xA00B	// ファンクション不正				// P100UBS001
#define	TWCC_STIERR_NOT_INITIALIZED			0xA00C	// 初期化未実施						// P100UBS001
#define	TWCC_STIERR_ALREADY_INITIALIZED		0xA00D	// 初期化済み						// P100UBS001
#define	TWCC_STIERR_DEVICE_LOCKED			0xA00E	// デバイスロック未実施				// P100UBS001
#define	TWCC_STIERR_READONLY				0xA00F	// デバイスと通信出来ない			// P100UBS001
#define	TWCC_STIERR_NEEDS_LOCK				0xA010	// デバイスロックが必要				// P100UBS001
#define	TWCC_STIERR_SHARING_VIOLATION		0xA011	// 別ＡＰにてデバイスオープン済み	// P100UBS001
#define	TWCC_STIERR_HANDLEEXISTS			0xA012	// ハンドルは既にある				// P100UBS001
#define	TWCC_STIERR_INVALID_DEVICE_NAME		0xA013	// デバイス名を認識出来ない			// P100UBS001
#define	TWCC_STIERR_INVALID_HW_TYPE			0xA014	// ハードウェアタイプが不正			// P100UBS001
#define	TWCC_STIERR_NOEVENTS				0xA015	// 利用可能なイベントが無い			// P100UBS001

// Minidriver
#define	TWCC_BS_FAIL						0xA101	// 一般エラー						// P100UBS001
#define	TWCC_BS_ERR_OUT_OF_MEMORY			0xA102	// メモリ不足						// P100UBS001
#define	TWCC_BS_ERR_INVALID_PARAM			0xA103	// パラメタ不正						// P100UBS001
#define	TWCC_BS_ERR_DEVICE_IN_USE			0xA111	// デバイス使用中					// P100UBS001
#define	TWCC_BS_ERR_DEVICE_NOT_LOCKED		0xA112	// デバイスが未ロック				// P100UBS001
#define	TWCC_BS_ERR_NEED_TO_OPEN_IN_DATA_MODE	0xA113	// データオープン出来ないモード	// P100UBS001
#define	TWCC_BS_ERR_BRIGHTNESS_VERIFICATION_NOT_OK	0xA121	// 光量異常					// P100UBS001
#define	TWCC_BS_ERR_SCAN_CANCELLED			0xA122	// 読取りキャンセル					// P100UBS001
#define	TWCC_BS_AUTO_SP_IS_OFF				0xA123	// 自動感度調整ＯＦＦ				// P100UBS001
#define	TWCC_BS_ERR_SP_LOGINF				0xA124	// シャッタスピードログ異常			// P100UBS001
#define	TWCC_BS_ERR_WB_LOGINF				0xA125	// ホワイトバランスログ異常			// P100UBS001
#define	TWCC_BS_ERR_WAIT_FAILED				0xA126	// 待ち状態異常						// P100UBS001
#define	TWCC_BS_ERR_NO_RAW_DATA				0xA127	// 画像ﾃﾞｰﾀ無し						// P100UBS001
#define	TWCC_BS_ERR_AUTOSP_NOT_OK			0xA128	// 自動感度調整無効					// P100UBS001
#define	TWCC_BS_ERR_KEY_NOTFOUND			0xA131	// ホワイトバランスログ異常			// P100UBS001
#define	TWCC_BS_ERR_IMG_PROCLIB_LOADFAIL	0xA141	// 待ち状態異常						// P100UBS001
#define	TWCC_STIERR_UNKNOWN					0xA600	// ＳＴＩ未定義エラー				// P100UBS001

// (Karnel Interface)
#define	TWCC_QRSC_ERROR_PARAM				0xB107	// パラメタ異常						// P100QC013
#define	TWCC_QRSC_ERROR_INVALID_HANDLE		0xB112	// ドライバハンドル不正				// P100QC013
#define	TWCC_QRSC_ERROR_CAMERA_STOP			0xB117	// カメラ停止異常					// P100QC013
#define	TWCC_QRSC_ERROR_IO_CANCEL			0xB118	// サービスキャンセル				// P100QC013
#define	TWCC_QRSC_ERROR_ENXIO				0xB119	// ドライバが存在しない				// B050006
#define	TWCC_QRSC_ERROR_ACCESS_DENIED		0xB120	// デバイスアクセス異常				// P100QC035
#define	TWCC_QRSC_ERROR_NO_SYSTEM_RESOURCES	0xB121	// ｼｽﾃﾑﾘｿｰｽ不足						// P100UBS001
#define	TWCC_QRSC_ERROR_SEM_TIMEOUT			0xB122	// I/Oﾀｲﾑｱｳﾄｴﾗｰ						// P100UBS001
#define	TWCC_QRSC_ERROR_INVALID_FUNCTION	0xB123	// 関数不正							// P100UBS001
#define	TWCC_QRSC_ERROR_CRC					0xB124	// ﾃﾞｰﾀ異常							// P100UBS001
#define	TWCC_QRSC_ERROR_DR_USING			0xB300	// ドライバ使用中					// P100QC013

// (Image Proc)
#define	TWCC_QRSC_ERROR_PARAMETER			0xD100	// 入力パラメタ矛盾					// P100QC013
#define	TWCC_QRSC_ERROR_BUFFER_OVERFLOW		0xD101	// バッファオーバーフロー			// P100QC013
#define	TWCC_QRSC_ERROR_SHAD_DARK			0xD300	// ｼｪｰﾃﾞｨﾝｸﾞ波形生成時光量過小		// P100UBS001
#define	TWCC_QRSC_ERROR_SHAD_BRIGHT			0xD301	// ｼｪｰﾃﾞｨﾝｸﾞ波形生成時光量過大		// P100UBS001
#define	TWCC_QRSC_ERROR_IMG_SHAD_EDGELESS	0xD400	// ｼｪｰﾃﾞｨﾝｸﾞ時基準シート無し		// P100QC037
#define	TWCC_QRSC_ERROR_GEOMETRIC_CALC		0xF200	// 補正係数出力時矛盾				// P100QC013

// (Maintenance) FARMWARE
#define	TWCC_QRSC_ERROR_BASE_PAPER			0xF100	// 基準シート無し/基準シート異常	// P100QC013
#define	TWCC_QRSC_ERROR_WB_SPEED_LOW		0xF101	// シャッタースピード異常 低速		// P100QC013
#define	TWCC_QRSC_ERROR_WB_SPEED_HIGH		0xF102	// シャッタースピード異常 高速		// P100QC013
#define	TWCC_QRSC_ERROR_WB_GAIN_RETRY		0xF111	// ゲイン調整リトライ失敗			// P100QC013
#define	TWCC_QRSC_ERROR_WB_GAIN_WHITE_LOW	0xF112	// 白ゲイン調整異常 低				// P100QC013
#define	TWCC_QRSC_ERROR_WB_GAIN_WHITE_HIGH	0xF113	// 白ゲイン調整異常 高				// P100QC013
#define	TWCC_QRSC_ERROR_WB_GAIN_BLACK_LOW	0xF114	// 黒ゲイン調整異常 低				// P100QC013
#define	TWCC_QRSC_ERROR_WB_GAIN_BLACK_HIGH	0xF115	// 黒ゲイン調整異常 高				// P100QC013
#define	TWCC_QRSC_ERROR_WB_TIMEOUT			0xF121	// 撮影回数タイムアウトオーバー		// P100QC013
#define	TWCC_QRSC_ERROR_WB_INITIALIZE		0xF131	// 初期化処理エラー					// P100QC013
#define	TWCC_QRSC_ERROR_WB_SET_WHITE		0xF132	// 白基本値設定処理エラー			// P100QC013
#define	TWCC_QRSC_ERROR_WB_SET_BLACK		0xF133	// 黒基本値設定処理エラー			// P100QC013
#define	TWCC_QRSC_ERROR_WB_SET_OBJECT		0xF134	// 目標値設定処理エラー				// P100QC013

// FARMWARE（WBから追加分）
#define	TWCC_FRM_UNDEFINED					0xF700		// 未定義						// P100UBS001
#define	TWCC_FRM_GENERAL_ERROR				0xF702		// 一般エラー					// P100UBS001
#define	TWCC_FRM_INVALID_TRANSACTION_ID		0xF704		// 処理ID不正					// P100UBS001
#define	TWCC_FRM_OPERATION_NOT_SUPPORTED	0xF705		// 未定義の制御コード			// P100UBS001
#define	TWCC_FRM_PARAMETER_NOT_SUPPORTED	0xF706		// 未定義のパラメタ				// P100UBS001
#define	TWCC_FRM_INVALID_CODE_FORMAT		0xF716		// コンテナコードの定義が不正	// P100UBS001
#define	TWCC_FRM_INVALID_PARAMETER			0xF71D		// パラメタ不正					// P100UBS001
#define	TWCC_FRM_TRANSACTION_CANCELLED		0xF71F		// 処理の取り消し				// P100UBS001
#define	TWCC_FRM_UNDEFINED_A				0xF800		// 未定義						// P100UBS001
#define	TWCC_FRM_INVALID_COMMAND_PROCEDURE	0xF801		// コマンド発行順序が不正		// P100UBS001
#define	TWCC_FRM_DEVICE_INF_ERR				0xF81E		// ﾃﾞﾊﾞｲｽ情報にｴﾗｰ有り			// P100UBS001
#define	TWCC_FRM_SCAN_COUNT_ERR				0xF81F		// ｽｷｬﾝｶｳﾝﾄ設定ｴﾗｰ				// P100UBS001
#define TWCC_FRM_FPGA_REG_ERR               0xF830		// FPGAﾚｼﾞｽﾀﾃｽﾄでｴﾗｰ            // P100UBS001
#define TWCC_FRM_USB_REG_ERR                0xF831		// USBﾚｼﾞｽﾀﾃｽﾄでｴﾗｰ             // P100UBS001
#define TWCC_FRM_SDRAM_ERR                  0xF832		// SDRAMﾃｽﾄでｴﾗｰ                // P100UBS001
#define TWCC_FRM_FPGA_SRAM_SRL_ERR          0xF833		// FPGA SRAM ﾃｽﾄでｴﾗｰ(通信用)   // P100UBS001
#define TWCC_FRM_FPGA_SRAM_IMG_ERR          0xF834		// FPGA SRAM ﾃｽﾄでｴﾗｰ(画像用)   // P100UBS001
#define TWCC_FRM_GET_LOG_INF_ERR            0xF840		// SHﾛｷﾞﾝｸﾞﾃﾞｰﾀ取得ｴﾗｰ          // P100UBS001
#define TWCC_FRM_FPGA_DOWNDLOAD_ERR         0xF850		// FPGAﾃﾞｰﾀﾀﾞｳﾝﾛｰﾄﾞｴﾗｰ          // P100UBS001
#define TWCC_FRM_SH_DOWNDLOAD_ERR           0xF860		// SHﾏｲｸﾛﾀﾞｳﾝﾛｰﾄﾞｴﾗｰ            // P100UBS001
#define TWCC_FRM_CABLE_CUTOFF_ERR           0xF870		// ｹｰﾌﾞﾙ断検知                  // P100UBS001

// USB版未使用条件コード
#define	TWCC_QRSC_ERROR_SRAM_RDWT			0x9200	// ＳＲＡＭエラー					// P100UBS001
#define	TWCC_QRSC_ERROR_SDRAM_RDWT			0x9201	// ＳＤＲＡＭエラー					// P100UBS001
#define	TWCC_QRSC_ERROR_FPGAREG_RDWT		0x9202	// ＦＰＧＡレジスタエラー			// P100UBS001
#define	TWCC_QRSC_ERROR_FPGARAM_RDWT		0x9204	// ＦＰＧＡ内蔵ＲＡＭエラー			// P100UBS001
#define	TWCC_QRSC_ERROR_TIMING				0xB108	// シーケンス異常					// P100UBS001
#define	TWCC_QRSC_ERROR_EAGAIN				0xB111	// ドライバ２重起動					// P100UBS001
#define	TWCC_QRSC_ERROR_PROGRAM_ERROR		0xB113	// ドライバ内部異常					// P100UBS001
#define	TWCC_QRSC_ERROR_INVADD				0xB114	// 仮想アドレス取得失敗				// P100UBS001
#define	TWCC_QRSC_ERROR_IO_INCOMPLETE		0xB115	// ＤＭＡ操作が未完了				// P100UBS001
#define	TWCC_QRSC_ERROR_TIMEOUT_DMA			0xB116	// ＤＭＡタイムアウト				// P100UBS001
#define	TWCC_QRSC_ERROR_CABLECUTOFF			0xB400	// ケーブル断検知					// P100UBS001
#define	TWCC_QRSC_ERROR_INSUFFICIENT_QUOTA	0xB500	// リソース不足						// P100UBS001
#define	TWCC_QRSC_ERROR_PCI_UNKNOWN			0xB600	// ＰＣＩドライバ未定義エラー		// P100UBS001
#define	TWCC_QRSC_ERROR_IMGATTRIBUTE		0xD200	// 前回画像モード情報と相違			// P100UBS001
#define	TWCC_QRSC_ERROR_IMGPROC_LOAD		0xD500	// 画像補正ライブラリが存在しない	// P100UBS001
#define	TWCC_QRSC_ERROR_IMGPROC_NOFUNC		0xD501	// 画像補正ライブラリＡＰＩ未定義	// P100UBS001
#define	TWCC_QRSC_ERROR_IMGPROC_UNKNOWN		0xD600	// 画像補正ライブラリ未定義エラー	// P100UBS001
#define	TWCC_QRSC_ERROR_ADJRESOLUTION		0xF202	// 解像度調整失敗					// P100UBS001
#define	TWCC_QRSC_ERROR_SHFILE_ACCESSFAIL	0xF300	// ｼｪｰﾃﾞｨﾝｸﾞﾃﾞｰﾀ格納ﾌｧｲﾙｱｸｾｽ失敗	// P100UBS001
#define	TWCC_QRSC_ERROR_WBPROC_LOAD			0xF500	// ﾎﾜｲﾄﾊﾞﾗﾝｽﾗｲﾌﾞﾗﾘが存在しない		// P100UBS001
#define	TWCC_QRSC_ERROR_WBPROC_NOFUNC		0xF501	// ﾎﾜｲﾄﾊﾞﾗﾝｽﾗｲﾌﾞﾗﾘAPI未定義			// P100UBS001
#define	TWCC_QRSC_ERROR_WBPROC_UNKNOWN		0xF600	// ﾎﾜｲﾄﾊﾞﾗﾝｽﾗｲﾌﾞﾗﾘ未定義エラー		// P100UBS001
