#include <windows.h>         // Req. for twain.h type defs and ...
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <winbase.h>

#include "twacker.h"
#include "twndebug.h"

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

#include "triplets.h"

#include "TwainCaLL.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")


#define PHI (3.14159265358979323846)

FI_STRUCT (FREEIMAGEHEADER) {
    FREE_IMAGE_TYPE type;		// data type - bitmap, array of long, double, complex, etc

	unsigned red_mask;			// bit layout of the red components
	unsigned green_mask;		// bit layout of the green components
	unsigned blue_mask;			// bit layout of the blue components

	RGBQUAD bkgnd_color;		// background color used for RGB transparency

	BOOL transparent;			 // why another table? for easy transparency table retrieval!
	int  transparency_count;	 // transparency could be stored in the palette, which is better
	BYTE transparent_table[256]; // overall, but it requires quite some changes and it will render
								 // FreeImage_GetTransparencyTable obsolete in its current form;
	FIICCPROFILE iccProfile;	 // space to hold ICC profile
	//BYTE filler[1];              
};

inline int
CalculateLine(int width, int bitdepth) {
	return ((width * bitdepth) + 7) / 8;
}

inline int
CalculatePitch(int line) {
	return line + 3 & ~3;
}
inline int
CalculateUsedPaletteEntries(int bit_count, int nColors) {
	if (nColors == 0 && bit_count <= 8) 
		nColors = (1 << bit_count);
// 	if ((bit_count >= 1) && (bit_count <= 8))
// 		return 1 << bit_count;

	return nColors;
}

// ==========================================================
// Bitmap Plugin 관련
// ==========================================================

static int s_format_id;

static const BYTE RLE_COMMAND     = 0;
static const BYTE RLE_ENDOFLINE   = 0;
static const BYTE RLE_ENDOFBITMAP = 1;
static const BYTE RLE_DELTA       = 2;

HANDLE ghWnd=NULL;

int gSelectMethod = NOT_SELECTED;

inline unsigned char
HINIBBLE (unsigned char byte) {
	return byte & 0xF0;
}

inline unsigned char
LOWNIBBLE (unsigned char byte) {
	return byte & 0x0F;
}

inline void
SwapShort(unsigned short *sp) {
	unsigned char *cp = (unsigned char *)sp, t = cp[0]; cp[0] = cp[1]; cp[1] = t;
}

inline void
SwapLong(unsigned long *lp) {
	unsigned char *cp = (unsigned char *)lp, t = cp[0]; cp[0] = cp[3]; cp[3] = t;
	t = cp[1]; cp[1] = cp[2]; cp[2] = t;
}

/// INPLACESWAP adopted from codeguru.com 
template <class T> void INPLACESWAP(T& a, T& b) {
	a ^= b; b ^= a; a ^= b;
}

inline void 
myReadProc(void* pDest, BYTE** ppSrc, int Bytes) {
	memcpy(pDest, *ppSrc, Bytes);
	*ppSrc += Bytes;
}

#ifdef FREEIMAGE_BIGENDIAN
static void
SwapInfoHeader(BITMAPINFOHEADER *header) {
	SwapLong(&header->biSize);
	SwapLong((DWORD *)&header->biWidth);
	SwapLong((DWORD *)&header->biHeight);
	SwapShort(&header->biPlanes);
	SwapShort(&header->biBitCount);
	SwapLong(&header->biCompression);
	SwapLong(&header->biSizeImage);
	SwapLong((DWORD *)&header->biXPelsPerMeter);
	SwapLong((DWORD *)&header->biYPelsPerMeter);
	SwapLong(&header->biClrUsed);
	SwapLong(&header->biClrImportant);
}
#endif // FREEIMAGE_BIGENDIAN

//	********** 사용후 메모리 반납해야함 (FreeImage_Unload)
static FIBITMAP *
DibToFIBitmap(HGLOBAL hDib) {
	RelLogMessage("DibToFIBitmap\r\n");

	FIBITMAP* pFIBitmap = NULL;
	BYTE* pDibCurPos = (BYTE*)hDib;

	try {
		// load the info header

		BITMAPINFOHEADER bih;

		myReadProc(&bih, &pDibCurPos, sizeof(BITMAPINFOHEADER));

#ifdef FREEIMAGE_BIGENDIAN
		SwapInfoHeader(&bih);
#endif

		// keep some general information about the bitmap

		int infoHeaderOffset = bih.biSize;
		int used_colors = bih.biClrUsed;
		int width       = bih.biWidth;
		int height      = bih.biHeight;
		int bit_count   = bih.biBitCount;
		int compression = bih.biCompression;
		int pitch       = CalculatePitch(CalculateLine(width, bit_count));


		switch (bit_count) {
			case 1 :
			case 4 :
			case 8 :
			{
				if ((used_colors <= 0) || (used_colors > CalculateUsedPaletteEntries(bit_count, used_colors)))
					used_colors = CalculateUsedPaletteEntries(bit_count, used_colors);
				
				// allocate enough memory to hold the bitmap (header, palette, pixels) and read the palette

				pFIBitmap = FreeImage_Allocate(width, height, bit_count);

				if (pFIBitmap == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(pFIBitmap);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;
				
				// load the palette

				myReadProc(FreeImage_GetPalette(pFIBitmap), &pDibCurPos, used_colors * sizeof(RGBQUAD));
#ifdef FREEIMAGE_BIGENDIAN
				RGBQUAD *pal = FreeImage_GetPalette(pFIBitmap);
				for(int i = 0; i < used_colors; i++) {
					INPLACESWAP(pal[i].rgbRed, pal[i].rgbBlue);
				}
#endif

				// seek to the actual pixel data.
				// this is needed because sometimes the palette is larger than the entries it contains predicts

				if ((UINT)infoHeaderOffset > (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (used_colors * sizeof(RGBQUAD))))
					pDibCurPos = (BYTE*)hDib + infoHeaderOffset;
// 					io->seek_proc(pDibCurPos, infoHeaderOffset, SEEK_SET);
				
				// read the pixel data

				switch (compression) {
					case BI_RGB :
						if (height > 0) {
							myReadProc((void *)FreeImage_GetBits(pFIBitmap), &pDibCurPos, height * pitch);
						} else {
							int tmp = abs(height);

							for (int c = 0; c < tmp; ++c) {
								myReadProc((void *)FreeImage_GetScanLine(pFIBitmap, tmp - c - 1), &pDibCurPos, pitch);
							}
						}
						
						return pFIBitmap;

					case BI_RLE4 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;
						int align = 0;
						BOOL low_nibble = FALSE;

						for (;;) {
							myReadProc(&status_byte, &pDibCurPos, sizeof(BYTE));

							switch (status_byte) {
								case RLE_COMMAND :
									myReadProc(&status_byte, &pDibCurPos, sizeof(BYTE));

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											low_nibble = FALSE;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)pFIBitmap;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x = 0;
											BYTE delta_y = 0;

											myReadProc(&delta_x, &pDibCurPos, sizeof(BYTE));
											myReadProc(&delta_y, &pDibCurPos, sizeof(BYTE));

											// apply them

											bits     += delta_x / 2;
											scanline += delta_y;
											break;
										}

										default :
										{
											myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));

											BYTE *sline = FreeImage_GetScanLine(pFIBitmap, scanline);

											for (int i = 0; i < status_byte; i++) {
												if (low_nibble) {
													*(sline + bits) |= LOWNIBBLE(second_byte);

													if (i != status_byte - 1)
														myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));

													bits++;
												} else {
													*(sline + bits) |= HINIBBLE(second_byte);
												}
												
												low_nibble = !low_nibble;
											}

											// align run length to even number of bytes 

											align = status_byte % 4;
											if((align == 1) || (align == 2)) {
												myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));
											}

											break;
										}
									}

									break;

								default :
								{
									BYTE *sline = FreeImage_GetScanLine(pFIBitmap, scanline);

									myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));

									for (unsigned i = 0; i < status_byte; i++) {
										if (low_nibble) {
											*(sline + bits) |= LOWNIBBLE(second_byte);

											bits++;
										} else {
											*(sline + bits) |= HINIBBLE(second_byte);
										}				
										
										low_nibble = !low_nibble;
									}
								}

								break;
							}
						}

						return (FIBITMAP *)pFIBitmap;
					}

					case BI_RLE8 :
					{
						BYTE status_byte = 0;
						BYTE second_byte = 0;
						int scanline = 0;
						int bits = 0;

						for (;;) {
							myReadProc(&status_byte, &pDibCurPos, sizeof(BYTE));

							switch (status_byte) {
								case RLE_COMMAND :
									myReadProc(&status_byte, &pDibCurPos, sizeof(BYTE));

									switch (status_byte) {
										case RLE_ENDOFLINE :
											bits = 0;
											scanline++;
											break;

										case RLE_ENDOFBITMAP :
											return (FIBITMAP *)pFIBitmap;

										case RLE_DELTA :
										{
											// read the delta values

											BYTE delta_x = 0;
											BYTE delta_y = 0;

											myReadProc(&delta_x, &pDibCurPos, sizeof(BYTE));
											myReadProc(&delta_y, &pDibCurPos, sizeof(BYTE));

											// apply them

											bits     += delta_x;
											scanline += delta_y;

											break;
										}

										default :
											myReadProc((void *)(FreeImage_GetScanLine(pFIBitmap, scanline) + bits), &pDibCurPos, sizeof(BYTE) * status_byte);
											
											// align run length to even number of bytes 

											if ((status_byte & 1) == 1)
												myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));												

											bits += status_byte;													

											break;								
									};

									break;

								default :
									BYTE *sline = FreeImage_GetScanLine(pFIBitmap, scanline);

									myReadProc(&second_byte, &pDibCurPos, sizeof(BYTE));

									for (unsigned i = 0; i < status_byte; i++) {
										*(sline + bits) = second_byte;

										bits++;					
									}

									break;
							};
						}

						break;
					}

					default :
						throw "compression type not supported";
				}

				break;
			}

			case 16 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					DWORD bitfields[3];

					myReadProc(bitfields, &pDibCurPos, 3 * sizeof(DWORD));

					pFIBitmap = FreeImage_Allocate(width, height, bit_count, bitfields[0], bitfields[1], bitfields[2]);
				} else {
					pFIBitmap = FreeImage_Allocate(width, height, bit_count, FI16_555_RED_MASK, FI16_555_GREEN_MASK, FI16_555_BLUE_MASK);
				}

				if (pFIBitmap == NULL)
					throw "DIB allocation failed";						

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(pFIBitmap);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				myReadProc(FreeImage_GetBits(pFIBitmap), &pDibCurPos, height * pitch);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(pFIBitmap); y++) {
					WORD *pixel = (WORD *)FreeImage_GetScanLine(pFIBitmap, y);
					for(int x = 0; x < FreeImage_GetWidth(pFIBitmap); x++) {
						SwapShort(pixel);
						pixel++;
					}
				}
#endif

				return pFIBitmap;
			}

			case 24 :
			case 32 :
			{
				if (bih.biCompression == BI_BITFIELDS) {
					DWORD bitfields[3];

					myReadProc(bitfields, &pDibCurPos, 3 * sizeof(DWORD));

					pFIBitmap = FreeImage_Allocate(width, height, bit_count, bitfields[0], bitfields[1], bitfields[2]);
				} else {
					if( bit_count == 32 ) {
						pFIBitmap = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
					} else {
						pFIBitmap = FreeImage_Allocate(width, height, bit_count, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
					}
				}

				if (pFIBitmap == NULL)
					throw "DIB allocation failed";

				BITMAPINFOHEADER *pInfoHeader = FreeImage_GetInfoHeader(pFIBitmap);
				pInfoHeader->biXPelsPerMeter = bih.biXPelsPerMeter;
				pInfoHeader->biYPelsPerMeter = bih.biYPelsPerMeter;

				// Skip over the optional palette 
				// A 24 or 32 bit DIB may contain a palette for faster color reduction

				if (pInfoHeader->biClrUsed > 0) {
					pDibCurPos += pInfoHeader->biClrUsed * sizeof(RGBQUAD);
// 				    io->seek_proc(pDibCurPos, pInfoHeader->biClrUsed * sizeof(RGBQUAD), SEEK_CUR);
				} else if ((bih.biCompression != BI_BITFIELDS) && (infoHeaderOffset > sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))) {
					pDibCurPos += infoHeaderOffset;
// 					io->seek_proc(pDibCurPos, infoHeaderOffset, SEEK_CUR);
				}

				// read in the bitmap bits

				myReadProc(FreeImage_GetBits(pFIBitmap), &pDibCurPos, height * pitch);
#ifdef FREEIMAGE_BIGENDIAN
				for(int y = 0; y < FreeImage_GetHeight(pFIBitmap); y++) {
					BYTE *pixel = FreeImage_GetScanLine(pFIBitmap, y);
					for(int x = 0; x < FreeImage_GetWidth(pFIBitmap); x++) {
						INPLACESWAP(pixel[0], pixel[2]);
						pixel += (bit_count>>3);
					}
				}
#endif

				// check if the bitmap contains transparency, if so enable it in the header

				FreeImage_SetTransparent(pFIBitmap, (FreeImage_GetColorType(pFIBitmap) == FIC_RGBALPHA));

				return pFIBitmap;
			}
		}
	} catch(const char *message) {
		if(pFIBitmap)
			FreeImage_Unload(pFIBitmap);

		FreeImage_OutputMessageProc(s_format_id, message);
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////

extern HWND hMainWnd;

LRESULT FAR (PASCAL * PrevProc)(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
BOOL InitInstanceAppId(HANDLE hWnd);
LRESULT FAR (PASCAL * CallBackProc)(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
LRESULT FAR (PASCAL * CallBackEjectProc)(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
// LRESULT FAR (PASCAL * CallBackStopProc)(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
void SaveBitmap(HWND hWnd, HGLOBAL _hDIB, const char* szFileTitle);
VOID CleanUpApp (VOID);

BOOL gbUseFixedName = FALSE;
char gImgFixedName[MAX_PATH] = {0,};
char gImgFileName[MAX_PATH] = {0,};
char gImgDir[MAX_PATH];
char gImgPrefix[MAX_PATH];
int gImgCount=0;
int gOptFlag=0;

int gCropLeft=0;
int gCropTop=0;
int gCropRight=0;
int gCropBottom=0;

const char* szFileExts[25] = {
	"bmp",			// FIF_BMP		
	"ico",			// FIF_ICO		
	"jpg",			// FIF_JPEG	
	"jng",			// FIF_JNG		
	"koala",		// FIF_KOALA	
	"lbm_iff",		// FIF_LBM	,FIF_IFF
	"mng",			// FIF_MNG		
	"pbm",			// FIF_PBM		
	"pbmraw",		// FIF_PBMRAW	
	"pcd",			// FIF_PCD		
	"pcx",			// FIF_PCX		
	"pgm",			// FIF_PGM		
	"pgmraw",		// FIF_PGMRAW	
	"png",			// FIF_PNG		
	"ppm",			// FIF_PPM		
	"ppmraw",		// FIF_PPMRAW	
	"ras",			// FIF_RAS		
	"targa",		// FIF_TARGA	
	"tif",			// FIF_TIFF	
	"wbmp",			// FIF_WBMP	
	"psd",			// FIF_PSD		
	"cut",			// FIF_CUT		
	"xbm",			// FIF_XBM		
	"xpm",			// FIF_XPM		
	"dds"			// FIF_DDS		
};

FREE_IMAGE_FORMAT gFileFormat=FIF_BMP;

HGLOBAL _ghDIB = NULL; // hDib to the current DIB

void swap(int* a, int* b)
{
	int tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}



LRESULT FAR PASCAL DLLWndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	RelLogMessage("DLLWndProc\r\n");
	char szDebug[256]={0,};

	MSG msg;
	long pos;
	POINTS pts;
	
	msg.hwnd = hWnd;
	msg.message = iMessage;
	msg.wParam = wParam;
	msg.lParam = lParam;
	msg.time = GetMessageTime();
	pos = GetMessagePos();
	pts = MAKEPOINTS(pos);
	msg.pt.x = pts.x;
	msg.pt.y = pts.y;
	
// 	char s[100];
	
	if ((!TWIsDSOpen()) || (!ProcessTWMessage ((LPMSG)&msg, hWnd)))
	{

		if(msg.message == PM_XFERDONE ) 
		{
			CleanUpApp ();

			if((void*)wParam != NULL)
			{
				char szDebug[256];
				DWORD dwTime = timeGetTime();
				sprintf(szDebug, "time - %d", dwTime/1000);
				OutputDebugString(szDebug);
				if( (_ghDIB = (LPBITMAPINFO) GlobalLock((HGLOBAL)msg.wParam))!=NULL )
				{
					wParam = (WPARAM)_ghDIB;

					if( (gImgDir != NULL) && (gImgPrefix != NULL) && (2 < strlen(gImgDir)))
					{
						BOOL bSuccess = FALSE;
						FIBITMAP* pFIBitmap;

						switch(gFileFormat)
						{
						// 지원할 파일 포멧 나열
						case FIF_BMP: 
						case FIF_TIFF:
						case FIF_JPEG:
							pFIBitmap = DibToFIBitmap(_ghDIB);
							if(pFIBitmap)
							{
								if(gbUseFixedName == TRUE)
									strcpy(gImgFileName, gImgFixedName);
								else
									sprintf(gImgFileName, "%s\\%s%06d.%s", gImgDir, gImgPrefix, gImgCount, szFileExts[gFileFormat]);

								FIBITMAP* pFICrop=NULL;
								int cropActivate = GetPrivateProfileInt("Crop", "Activate", 0, "./TwainCaLL.ini");
								sprintf(szDebug, "TwainCaLL_DLL>GetPrivateProfileInt: %d", cropActivate);
								OutputDebugString(szDebug);
								if(cropActivate)
								{
									gCropLeft = GetPrivateProfileInt("Crop", "left", 0, "./TwainCaLL.ini");
									gCropTop = GetPrivateProfileInt("Crop", "top", 0, "./TwainCaLL.ini");
									gCropRight = GetPrivateProfileInt("Crop", "right", 10000, "./TwainCaLL.ini");
									gCropBottom = GetPrivateProfileInt("Crop", "bottom", 10000, "./TwainCaLL.ini");

									int orgWidth, orgHeight;
									orgWidth = FreeImage_GetWidth(pFIBitmap);
									orgHeight = FreeImage_GetHeight(pFIBitmap);

									// 상,하,좌,우 음수를 양수로
									if( gCropLeft < 0 )
										gCropLeft = -gCropLeft;
									if( gCropTop < 0 )
										gCropTop = -gCropTop;
									if( gCropRight < 0 )
										gCropRight = -gCropRight;
									if( gCropBottom < 0 )
										gCropBottom = -gCropBottom;

									// 상/하, 좌/우 좌표 바뀐거 수정
									if( gCropRight < gCropLeft )
										swap(&gCropLeft, &gCropRight);
									if( gCropBottom < gCropTop )
										swap(&gCropTop, &gCropBottom);

									// 상,하,좌,우 한계값 초과하면 한계값으로 조정
									if( gCropLeft < 0 )
										gCropLeft = 0;
									if( gCropTop < 0 )
										gCropTop = 0;
									if( orgWidth < gCropRight)
										gCropRight = orgWidth;
									if( orgHeight < gCropBottom )
										gCropBottom = orgHeight;

									int cropWidth, cropHeight;
									cropWidth = gCropRight-gCropLeft+1;
									cropHeight = gCropBottom-gCropTop+1;

									if(cropWidth == 1 || cropHeight == 1)		// no crop
									{
										bSuccess = FreeImage_Save(gFileFormat, pFIBitmap, gImgFileName, gOptFlag);
									}
									else
									{
										sprintf(szDebug, "TwainCaLL_DLL>(%d, %d), (%d, %d)", gCropLeft, gCropTop, gCropRight, gCropBottom);
										OutputDebugString(szDebug);
										pFICrop = FreeImage_Allocate(cropWidth, cropHeight, FreeImage_GetBPP(pFIBitmap));
										pFICrop = FreeImage_Copy(pFIBitmap, gCropLeft, gCropTop, gCropRight, gCropBottom);
										BITMAPINFOHEADER *pCropInfo, *pOrgInfo;
										pOrgInfo = FreeImage_GetInfoHeader(pFIBitmap);
										pCropInfo = FreeImage_GetInfoHeader(pFICrop);
										pCropInfo->biXPelsPerMeter = pOrgInfo->biXPelsPerMeter;
										pCropInfo->biYPelsPerMeter = pOrgInfo->biYPelsPerMeter;
										bSuccess = FreeImage_Save(gFileFormat, pFICrop, gImgFileName, gOptFlag);
										FreeImage_Unload(pFICrop);
									}
								}
								else
								{
									bSuccess = FreeImage_Save(gFileFormat, pFIBitmap, gImgFileName, gOptFlag);
								}
								FreeImage_Unload(pFIBitmap);
							}
							break;
						default:
							break;
						}

						gImgCount++;
					}
					
					GlobalUnlock((HGLOBAL)msg.wParam);
				}


// 				//////////////////////////////////////////////////////////////////////////
// 				FILE* fp;
// 				fp = fopen(gImgFileName, "r");
// 				if(fp != NULL)
// 				{
// 					OutputDebugString("[TWAIN_CALL] DLLWndProc: 파일 생성 완료");
// 					fclose(fp);
// 				}
// 				else
// 					OutputDebugString("[TWAIN_CALL] DLLWndProc: 파일이 아직 생성되지 않음");
// 				//////////////////////////////////////////////////////////////////////////

// 				OutputDebugString("[TWAIN_CALL] DLLWndProc: CallBackProc 호출 전");
				(*CallBackProc)(hWnd, iMessage, wParam, lParam);
// 				OutputDebugString("[TWAIN_CALL] DLLWndProc: CallBackProc 호출 후");
			}
			
			return 0;
			
		} 
		else if(msg.message == PM_CANCEL)
		{
			(*CallBackProc)(hWnd,iMessage, wParam, TWCRC_SCAN_CANCELED);
			return 0;
		}
		else
		{
			return (*PrevProc)(hWnd,iMessage, wParam,lParam);
		}
	}
	return 0;
}

int __stdcall CALL_InitInstance(HANDLE hWnd, long ProcXFerDone, long ProcEject)
{
	RelLogMessage("CALL_InitInstance\r\n");

	OFSTRUCT of;

	memset(&of, 0, sizeof(OFSTRUCT));

	// erase the twacker.log file
	OpenFile("c:\\twacker.log",&of,OF_DELETE);
	OpenFile("c:\\twsrc.log",&of,OF_DELETE);


// 	_ghDIB = (BYTE*) GlobalAlloc (GPTR, 3000*3000*3 );	// dglee

	if (!InitInstanceAppId(hWnd))
	{
		return TWCRC_FAILURE;
	}

	ghWnd = hWnd;


// 	MessageBox(NULL,"twcall1", "t", MB_OK);

// 	PrevProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM,LPARAM))SetWindowLong((HWND)hWnd,GWL_WNDPROC,(LONG)DLLWndProc);  //2016.03.09 상위 dll 혹은 c#에서 호출 
	CallBackProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM ,LPARAM ))ProcXFerDone;
	CallBackEjectProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM ,LPARAM ))ProcEject;
// 	CallBackStopProc = (LRESULT FAR (PASCAL *)(HWND,UINT,WPARAM ,LPARAM ))ProcStop;

	FreeImage_Initialise();

	return TWCRC_SUCCESS;
}

int __stdcall CALL_SelectSource()
{
	RelLogMessage("CALL_SelectSource\r\n");
	int rc;

	if (TWOpenDSM () == TRUE)
	{
		rc =TWSelectDS ();
		TWCloseDSM (NULL);
	}

	if(rc == TWRC_SUCCESS)
		gSelectMethod = SELECT_SOURCE;

	return (int)(-rc);
}

int __stdcall HPG3110_StartSetup(void)
{
	RelLogMessage("CALL_StartSetup\r\n");
// 	int nRet = CALL_StringSource("FUJITSU fi-4120Cdj");		// for test
	int nRet = CALL_StringSource("HP Scanjet G3110");
	if(nRet == TWCRC_SUCCESS)
	{
		if (TWOpenDSM () == TRUE)
		{
			if(TWOpenDS() == TRUE)
				return TWCRC_SUCCESS;
			else
				return TWCRC_DS_OPEN_FAILED;
		}
		else
			return TWCRC_DSM_OPEN_FAILED;
	}
	else
		return nRet;

	return TWCRC_SUCCESS;
}

int __stdcall CALL_StartSetup(void)
{
	RelLogMessage("CALL_StartSetup\r\n");
	if (TWOpenDSM () == TRUE)
	{
		if(TWOpenDS() == TRUE)
			return TWCRC_SUCCESS;
		else
			return TWCRC_DS_OPEN_FAILED;
	}
	else
		return TWCRC_DSM_OPEN_FAILED;
}

int __stdcall CALL_StringSource(const char* szProductName)
{
	RelLogMessage("CALL_StringSource\r\n");
	int myRC;

	if (TWOpenDSM () == TRUE)
	{
		myRC = TWStringDS (szProductName);
		TWCloseDSM (NULL);
	}

	if(myRC == TWCRC_SUCCESS)
		gSelectMethod = STRING_SOURCE;

	return myRC;
}

int __stdcall CALL_ChangeCount(int count)
{
	RelLogMessage("CALL_ChangeCount\r\n");
	gImgCount = count;
	return TWCRC_SUCCESS;
}

int __stdcall CALL_TWAppAquire(int ShowUI, char* pImgDir, char* pImgPrefix, 
							   FREE_IMAGE_FORMAT fiFormat, int optFlag)
{
	RelLogMessage("CALL_TWAppAquire\r\n");
	TW_INT16 Flag;
	HWND hWnd = hMainWnd;
	int show;

// 	char buff[256];
// 	sprintf(buff, "format: %d, flag: 0x%x", fiFormat, optFlag); 
// 	MessageBox(hWnd, buff, "VC++", MB_OK);

	strcpy(gImgDir, pImgDir);
	strcpy(gImgPrefix, pImgPrefix);
	gFileFormat = fiFormat;
	gOptFlag = optFlag;

	if( !ShowUI ) // without BlinkUI
	{
		Flag = 1; 
		show = 0;
	}
	else
	{
		Flag = 3;
		show = 8;
	}

	//Setup dsID for default Source                             
	if (!TWIsDSOpen())
	{
		return TWCRC_DS_OPEN_FAILED;  
	}

	if(TWAcquire(hWnd,show,Flag) == TWCRC_SUCCESS)
	{
		LogMessage("Acquire OK\r\n");
		return TWCRC_SUCCESS;
	}
	else  
	{
		LogMessage("Acquire returned false\r\n");
		return TWCRC_FAILURE;
	}

}

int __stdcall CALL_TWAppAquireFixedName(int ShowUI, char* szFileName, int optFlag)
{
	RelLogMessage("CALL_TWAppAquireFixedName\r\n");
	TW_INT16 Flag;
	HWND hWnd = hMainWnd;
	int show;

	strcpy(gImgFixedName, szFileName);
	gbUseFixedName = TRUE;
	gOptFlag = optFlag;

	char* szExt = szFileName + strlen(szFileName) - 3;


	if(!strcmp(szFileExts[FIF_BMP], strlwr(szExt)))
		gFileFormat = FIF_BMP;
	else if(!strcmp(szFileExts[FIF_TIFF], strlwr(szExt)))
		gFileFormat = FIF_TIFF;
	else if(!strcmp(szFileExts[FIF_JPEG], strlwr(szExt)))
		gFileFormat = FIF_JPEG;
	else
		return TWCRC_NOT_SUPPORTED_FORMAT;

	if( !ShowUI ) // without BlinkUI
	{
		Flag = 1; 
		show = 0;
	}
	else
	{
		Flag = 3;
		show = 8;
	}

	//Setup dsID for default Source                             
	if (!TWIsDSOpen())
	{
		return TWCRC_DS_OPEN_FAILED;  
	}

	if(TWAcquire(hWnd,show,Flag) == TWCRC_SUCCESS)
	{
		LogMessage("Acquire OK\r\n");
		return TWCRC_SUCCESS;
	}
	else  
	{
		LogMessage("Acquire returned false\r\n");
		return TWCRC_FAILURE;
	}

}

int __stdcall Call_ProcessTWMessage(LPMSG lpMsg, HWND hWnd)
{
// 	MessageBox(NULL,"Call_ProcessTWMessage", "a",MB_OK);
	RelLogMessage("Call_ProcessTWMessage\r\n");
	 return ProcessTWMessage(lpMsg, hWnd);
}

int __stdcall CALL_TWAppQuit()
{
	FreeImage_DeInitialise();

	RelLogMessage("CALL_TWAppQuit\r\n");
	if(CloseApplication(hMainWnd))
		CleanKillApp();

	SetWindowLong(hMainWnd,GWL_WNDPROC,(long)PrevProc);
	PrevProc=0;

	return TWCRC_SUCCESS;
}



/////////////////////////////////////////////////////////////////////////////
// FUNCTION: CleanUpApp
//
// ARGS:    none
//
// RETURNS: none
//
// NOTES:   1). delete any bit maps laying around
//          2). delete any palettes laying around
//
VOID CleanUpApp (VOID)
{   
	RelLogMessage("CleanUpApp\r\n");
	/*
	*	Free any previous DIB image
	*/
	if(_ghDIB)
	{
 		GlobalFree(_ghDIB);
		_ghDIB = NULL;
	}

	return;
} 

int GetEnumI8(TW_UINT16 cap, pTW_CON_EI8 pConEnum)
{
	RelLogMessage("GetEnumI8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_INT8)pvalEnum->ItemList)[i];
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}

int GetEnumI16(TW_UINT16 cap, pTW_CON_EI16 pConEnum)
{
	RelLogMessage("GetEnumI16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_INT16)pvalEnum->ItemList)[i];
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}


int GetEnumI32(TW_UINT16 cap, pTW_CON_EI32 pConEnum)						// 여기 - 함수이름, 인자
{
	RelLogMessage("GetEnumI32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_INT32)pvalEnum->ItemList)[i];		// 여기
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}

int GetEnumU8(TW_UINT16 cap, pTW_CON_EU8 pConEnum)
{
	RelLogMessage("GetEnumU8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_UINT8)pvalEnum->ItemList)[i];
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}

int GetEnumU16(TW_UINT16 cap, pTW_CON_EU16 pConEnum)						// 여기 - 함수이름, 인자
{
	RelLogMessage("GetEnumU16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_UINT16)pvalEnum->ItemList)[i];		// 여기
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}


int GetEnumU32(TW_UINT16 cap, pTW_CON_EU32 pConEnum)						// 여기 - 함수이름, 인자
{
	RelLogMessage("GetEnumU32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_UINT32)pvalEnum->ItemList)[i];		// 여기
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}


int GetEnumF32(TW_UINT16 cap, pTW_CON_EF32 pConEnum)
{
	RelLogMessage("GetEnumF32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_FIX32)pvalEnum->ItemList)[i];
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}


int GetEnumBool(TW_UINT16 cap, pTW_CON_EBOOL pConEnum)						// 여기 - 함수이름, 인자
{
	RelLogMessage("GetEnumBool\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pvalEnum;
	TW_UINT16 i;
	int rc;

	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalEnum = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConEnum->ItemType		= pvalEnum->ItemType;
		pConEnum->NumItems		= pvalEnum->NumItems;
		pConEnum->DefaultIndex	= pvalEnum->DefaultIndex;
		pConEnum->CurrentIndex	= pvalEnum->CurrentIndex;
		for (i = 0; i < pvalEnum->NumItems; i++)
		{
			(pConEnum->ItemList)[i] = ((pTW_BOOL)pvalEnum->ItemList)[i];		// 여기
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}

	return rc;
}


int GetRange(TW_UINT16 cap, pTW_CON_RANGE pConRng)						
{
	RelLogMessage("GetRange\r\n");
	TW_CAPABILITY twCapability;
	pTW_RANGE pvalRange;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pvalRange = (pTW_RANGE)GlobalLock(twCapability.hContainer);
		*(pTW_RANGE)pConRng = *pvalRange;
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}


int SetRange(TW_UINT16 cap, pTW_CON_RANGE pConSrc, pTW_UINT32 pVal)			
{
	RelLogMessage("SetRange\r\n");
	TW_CAPABILITY twCapability;
	pTW_RANGE pConDst = NULL;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_RANGE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_RANGE));
	
	pConDst = (pTW_RANGE)GlobalLock(twCapability.hContainer);
	*pConDst = *(pTW_RANGE)pConSrc;
	pConDst->CurrentValue = *pVal;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetEnumI8(TW_UINT16 cap, pTW_CON_EI8 pConSrc, TW_INT8 val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumI8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_INT8 pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_INT8) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT8;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_INT8)pConSrc->ItemList;									// 여기
	pItemDst = (pTW_INT8)pConDst->ItemList;									// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumI16(TW_UINT16 cap, pTW_CON_EI16 pConSrc, TW_INT16 val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumI16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_INT16 pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_INT16) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT16;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_INT16)pConSrc->ItemList;									// 여기
	pItemDst = (pTW_INT16)pConDst->ItemList;									// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumI32(TW_UINT16 cap, pTW_CON_EI32 pConSrc, TW_INT32 val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumI32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_INT32 pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_INT32) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT32;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_INT32)pConSrc->ItemList;									// 여기
	pItemDst = (pTW_INT32)pConDst->ItemList;									// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetEnumU8(TW_UINT16 cap, pTW_CON_EU8 pConSrc, TW_UINT8 val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumU8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_UINT8 pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_UINT8) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT8;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_UINT8)pConSrc->ItemList;									// 여기
	pItemDst = (pTW_UINT8)pConDst->ItemList;									// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumU16(TW_UINT16 cap, pTW_CON_EU16 pConSrc, TW_UINT16 val)
{
	RelLogMessage("SetEnumU16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	pTW_UINT16 pItemSrc, pItemDst;
	BOOL bIsSet = FALSE;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_UINT16) * (pConSrc->NumItems)));
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT16;
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_UINT16)pConSrc->ItemList;
	pItemDst = (pTW_UINT16)pConDst->ItemList;
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetEnumU32(TW_UINT16 cap, pTW_CON_EU32 pConSrc, TW_UINT32 val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumU32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_UINT32 pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_UINT32) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT32;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_UINT32)pConSrc->ItemList;									// 여기
	pItemDst = (pTW_UINT32)pConDst->ItemList;									// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetEnumF32(TW_UINT16 cap, pTW_CON_EF32 pConSrc, TW_FIX32 val)
{
	RelLogMessage("SetEnumF32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_FIX32 pItemSrc, pItemDst;
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_FIX32) * (pConSrc->NumItems)));
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FIX32;
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_FIX32)pConSrc->ItemList;
	pItemDst = (pTW_FIX32)pConDst->ItemList;
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(FIX32ToFloat(pItemSrc[i]) == FIX32ToFloat(val))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetEnumBool(TW_UINT16 cap, pTW_CON_EBOOL pConSrc, TW_BOOL val)			// 여기 - 함수이름, 인자 2개
{
	RelLogMessage("SetEnumBool\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	pTW_BOOL pItemSrc, pItemDst;												// 여기
	BOOL bIsSet = FALSE;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_BOOL) * (pConSrc->NumItems)));		// 여기
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_BOOL;										// 여기
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_BOOL)pConSrc->ItemList;										// 여기
	pItemDst = (pTW_BOOL)pConDst->ItemList;										// 여기
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(pItemSrc[i] == val)
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetOneValI8(TW_UINT16 cap, TW_INT8 val)
{
	RelLogMessage("SetOneValI8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_INT8 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT8;
	pItemDst = (pTW_INT8)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}


int SetOneValI16(TW_UINT16 cap, TW_INT16 val)
{
	RelLogMessage("SetOneValI16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_INT16 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT16;
	pItemDst = (pTW_INT16)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValI32(TW_UINT16 cap, TW_INT32 val)								// 여기
{
	RelLogMessage("SetOneValI32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_INT32 pItemDst;														// 여기
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT32;									// 여기
	pItemDst = (pTW_INT32)&pConDst->Item;									// 여기
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}


int SetOneValU8(TW_UINT16 cap, TW_UINT8 val)
{
	RelLogMessage("SetOneValU8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_UINT8 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT8;
	pItemDst = (pTW_UINT8)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}


int SetOneValU16(TW_UINT16 cap, TW_UINT16 val)
{
	RelLogMessage("SetOneValU16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_UINT16 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT16;
	pItemDst = (pTW_UINT16)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}


int SetOneValU32(TW_UINT16 cap, TW_UINT32 val)
{
	RelLogMessage("SetOneValU32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_UINT32 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT32;
	pItemDst = (pTW_UINT32)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValF32(TW_UINT16 cap, TW_FIX32 val)
{
	RelLogMessage("SetOneValF32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_FIX32 pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FIX32;
	pItemDst = (pTW_FIX32)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValBool(TW_UINT16 cap, TW_BOOL val)
{
	RelLogMessage("SetOneValBool\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_BOOL pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_BOOL;
	pItemDst = (pTW_BOOL)&pConDst->Item;
	*pItemDst = val;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}



//-------------------------------------------------
//	Example of DG_CONTROL / DAT_CAPABILITY / MSG_GET
//-------------------------------------------------
TW_INT16 ExampleOf_MSG_GET(pTW_CAPABILITY ptwCapability)
{
	RelLogMessage("ExampleOf_MSG_GET\r\n");
	TW_INT16 rc;

	TW_UINT32 NumItems;
	TW_UINT32 CurrentIndex;
	TW_UINT32 DefaultIndex;

	//Send the Triplet
	rc = CallDSMEntry(	&appID,
						&dsID,
						DG_CONTROL,
						DAT_CAPABILITY,
						MSG_GET,
						(TW_MEMREF)ptwCapability);
	//Check return code
	if (rc == TWRC_SUCCESS)
	{
		//Switch on Container Type
		switch (ptwCapability->ConType)
		{
		//-----ENUMERATION
		case TWON_ENUMERATION:
			{
				pTW_ENUMERATION pvalEnum;
				TW_UINT16 valueU16;
				TW_BOOL valueBool;
				TW_UINT16 i;
				pvalEnum = (pTW_ENUMERATION)GlobalLock(ptwCapability->hContainer);
				NumItems = pvalEnum->NumItems;
				CurrentIndex = pvalEnum->CurrentIndex;
				DefaultIndex = pvalEnum->DefaultIndex;
				for (i = 0; i < pvalEnum->NumItems; i++)
				{
					if (pvalEnum->ItemType == TWTY_UINT16)
					{
						valueU16 = ((TW_UINT16)(pvalEnum->ItemList[i*2]));
						//Store Item Value
					}
					else if (pvalEnum->ItemType == TWTY_BOOL)
					{
						valueBool = ((TW_BOOL*)&pvalEnum->ItemList)[i];
						//Store Item Value
					}
				}
				GlobalUnlock(ptwCapability->hContainer);
			}
			break;

		//-----ONEVALUE
		case TWON_ONEVALUE:
			{
				pTW_ONEVALUE pvalOneValue;
				TW_BOOL valueBool;
				pvalOneValue = (pTW_ONEVALUE)GlobalLock(ptwCapability->hContainer);
				if (pvalOneValue->ItemType == TWTY_BOOL)
				{
					valueBool = (TW_BOOL)pvalOneValue->Item;
					//Store Item Value
				}
				GlobalUnlock(ptwCapability->hContainer);
			}
			break;

		//-----RANGE
		case TWON_RANGE:
			{
				pTW_RANGE pvalRange;
				pTW_FIX32 pTWFix32;
				float valueF32;
				pvalRange = (pTW_RANGE)GlobalLock(ptwCapability->hContainer);
				if ((TW_UINT16)pvalRange->ItemType == TWTY_FIX32)
				{
					pTWFix32 = (pTW_FIX32)&(pvalRange->MinValue);
					valueF32 = FIX32ToFloat(*pTWFix32);
					//Store Item Value
					pTWFix32 = (pTW_FIX32)&(pvalRange->MaxValue);
					valueF32 = FIX32ToFloat(*pTWFix32);
					//Store Item Value
					pTWFix32 = (pTW_FIX32)&(pvalRange->StepSize);
					valueF32 = FIX32ToFloat(*pTWFix32);
					//Store Item Value
				}
				GlobalUnlock(ptwCapability->hContainer);
			}
			break;

		//-----ARRAY
		case TWON_ARRAY:
			{
				pTW_ARRAY pvalArray;
				TW_UINT16 valueU16;
				TW_UINT16 i;
				
				pvalArray = (pTW_ARRAY)GlobalLock(ptwCapability->hContainer);
				for (i = 0; i < pvalArray->NumItems; i++)
				{
					if (pvalArray->ItemType == TWTY_UINT16)
					{
						valueU16 = ((TW_UINT16)(pvalArray->ItemList[i*2]));
						//Store Item Value
					}
				}
				GlobalUnlock(ptwCapability->hContainer);
			}
			break;
		} //End Switch Statement
	} 
	else 
	{
		//Capability MSG_GET Failed check Condition Code
	}

	return rc;
}


//-------------------------------------------------
//	Example of DG_CONTROL / DAT_CAPABILITY / MSG_SET
//-------------------------------------------------
TW_INT16 ExampleOf_MSG_SET(pTW_CAPABILITY ptwCapability)
{
	RelLogMessage("ExampleOf_MSG_SET\r\n");
	TW_INT16 rc;
	TW_UINT32 NumberOfItems;

	switch (ptwCapability->ConType)
	{
	//-----ENUMERATION
	case TWON_ENUMERATION:
		{
			pTW_ENUMERATION pvalEnum;
			//The number of Items in the ItemList
			NumberOfItems = 2;
			//Allocate memory for the container and additional ItemList
			// entries
			ptwCapability->hContainer = GlobalAlloc(GHND,
				(sizeof(TW_ENUMERATION) + sizeof(TW_UINT16) * (NumberOfItems)));
			pvalEnum = (pTW_ENUMERATION)GlobalLock(ptwCapability->hContainer);
			pvalEnum->NumItems = 2; //Number of Items in ItemList
			pvalEnum->ItemType = TWTY_UINT16;
			((pTW_UINT16)pvalEnum->ItemList)[0] = 1;
			((pTW_UINT16)pvalEnum->ItemList)[1] = 2;
			GlobalUnlock(ptwCapability->hContainer);
		}
		break;

	//-----ONEVALUE
	case TWON_ONEVALUE:
		{
			pTW_ONEVALUE pvalOneValue;
			pTW_UINT16 pItem;
			ptwCapability->hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
			pvalOneValue = (pTW_ONEVALUE)GlobalLock(ptwCapability->hContainer);
			pvalOneValue->ItemType = TWTY_UINT16;
			pItem = (pTW_UINT16)&pvalOneValue->Item;
			*pItem = 1;
			GlobalUnlock(ptwCapability->hContainer);
		}
		break;

	//-----RANGE
	case TWON_RANGE:
		{
			pTW_RANGE pvalRange;
			TW_FIX32 TWFix32;
			float valueF32;
			ptwCapability->hContainer = GlobalAlloc(GHND, sizeof(TW_RANGE));
			pvalRange = (pTW_RANGE)GlobalLock(ptwCapability->hContainer);
			pvalRange->ItemType = TWTY_FIX32;
			valueF32 = 100;
			TWFix32 = FloatToFIX32 (valueF32);
			pvalRange->MinValue = *((pTW_INT32) &TWFix32);
			valueF32 = 200;
			TWFix32 = FloatToFIX32 (valueF32);
			pvalRange->MaxValue = *((pTW_INT32) &TWFix32);
			GlobalUnlock(ptwCapability->hContainer);
		}
		break;

	//-----ARRAY
	case TWON_ARRAY:
		{
			pTW_ARRAY pvalArray;
			//The number of Items in the ItemList
			NumberOfItems = 2;
			//Allocate memory for the container and additional ItemList entries
			ptwCapability->hContainer = GlobalAlloc(GHND,
				(sizeof(TW_ARRAY) + sizeof(TW_UINT16) * (NumberOfItems)));
			pvalArray = (pTW_ARRAY)GlobalLock(ptwCapability->hContainer);
			pvalArray->ItemType = TWTY_UINT16;
			pvalArray->NumItems = 2;
			((pTW_UINT16)pvalArray->ItemList)[0] = 1;
			((pTW_UINT16)pvalArray->ItemList)[1] = 2;
			GlobalUnlock(ptwCapability->hContainer);
		}
		break;
	}

	//-----MSG_SET
	rc = CallDSMEntry(	&appID,
						&dsID,
						DG_CONTROL,
						DAT_CAPABILITY,
						MSG_SET,
						(TW_MEMREF)ptwCapability);
	GlobalFree(ptwCapability->hContainer);

	switch (rc)
	{
	case TWRC_SUCCESS:
		//Capability's Current or Available value was set as specified
		break;

	case TWRC_CHECKSTATUS:
		//The Source matched the specified value(s) as closely as possible
		//Do a MSG_GET to determine the settings made
		break;

	case TWRC_FAILURE:
		//Check the Condition Code for more information
		break;
	}

	return rc;
}

int GetType(TW_UINT16 cap, pTW_UINT16 pConType, pTW_UINT16 pItemType)
{
	RelLogMessage("GetType\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pOneValue;
	TW_INT16 rc;
	int myRC = TWCRC_SUCCESS;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
						&dsID,
						DG_CONTROL,
						DAT_CAPABILITY,
						MSG_GET,
						(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pOneValue	= (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
		*pConType	= twCapability.ConType;
		*pItemType	= pOneValue->ItemType;
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
		myRC = TWCRC_SUCCESS;
	}
	else
	{
		myRC = TWCRC_NOT_SUPPORTED;
	}
	
	return myRC;
}


int SetArrayFrame(TW_UINT16 cap, pTW_FRAME pFrm, int itemNum)
{
	RelLogMessage("SetArrayFrame\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_FRAME pItemSrc, pItemDst;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_FRAME) * itemNum));
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FRAME;
	pConDst->NumItems		= itemNum;	
	
	pItemSrc = pFrm;
	pItemDst = (pTW_FRAME)pConDst->ItemList;
	
	for (i = 0; i < itemNum; i++)
		pItemDst[i] = pItemSrc[i];
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}


int GetEnumFrame(TW_UINT16 cap, pTW_CON_EFRAME pConDst)
{
	RelLogMessage("GetEnumFrame\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConSrc;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pConSrc = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConDst->ItemType		= pConSrc->ItemType;
		pConDst->NumItems		= pConSrc->NumItems;
		pConDst->DefaultIndex	= pConSrc->DefaultIndex;
		pConDst->CurrentIndex	= pConSrc->CurrentIndex;
		for (i = 0; i < pConSrc->NumItems; i++)
		{
			(pConDst->ItemList)[i] = ((pTW_FRAME)pConSrc->ItemList)[i];
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}


int SetEnumFrame(TW_UINT16 cap, pTW_CON_EFRAME pConSrc, pTW_FRAME pFrm)
{
	RelLogMessage("SetEnumFrame\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_FRAME pItemSrc, pItemDst;
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_FRAME) * (pConSrc->NumItems)));
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FRAME;
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_FRAME)pConSrc->ItemList;
	pItemDst = (pTW_FRAME)pConDst->ItemList;
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(!memcmp(&pItemSrc[i], pFrm, sizeof(TW_FRAME)))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}


int SetOneValFrame(TW_UINT16 cap, pTW_FRAME pFrm)
{
	RelLogMessage("SetOneValFrame\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_FRAME pItemDst;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE) + sizeof(TW_FRAME));
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FRAME;
	pItemDst = (pTW_FRAME)&pConDst->Item;
	*pItemDst = *pFrm;
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

// TWON_ARRAY의 경우에만 itemNum 이 1이아닌 수고 나머지는 모두 1, (다른 숫자 넣어도 상관없음)
int __stdcall CALL_SetProperty_Frame(TW_UINT16 cap, pTW_FRAME pFrm, int itemNum)
{
	RelLogMessage("CALL_SetProperty_Frame\r\n");
	TW_UINT16 conType, itemType;
	int rc;

	static TW_CON_EFRAME conEFrame;

	rc = GetType(cap, &conType, &itemType);

	if(rc == TWCRC_SUCCESS)
	{
		if(itemType == TWTY_FRAME)
		{
			switch(conType)
			{
			case TWON_ARRAY:
				rc = SetArrayFrame(cap, pFrm, itemNum);
				break;
				
			case TWON_ENUMERATION:
				GetEnumFrame(cap, &conEFrame);
				rc = SetEnumFrame(cap, &conEFrame, pFrm);
				break;
				
			case TWON_ONEVALUE:
				rc = SetOneValFrame(cap, pFrm);
				break;
				
			case TWON_RANGE:
				return TWCRC_NOT_SUPPORTED;
			}
		}
		else
			return TWCRC_NOT_FRAME;
	}

	return (-rc);
}



int SetArrayS32(TW_UINT16 cap, char* szSrc, int itemNum)
{
	RelLogMessage("SetArrayS32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR32 pItemSrc, pItemDst;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_STR32) * itemNum));
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR32;
	pConDst->NumItems		= itemNum;	
	
	pItemSrc = (pTW_STR32)szSrc;
	pItemDst = (pTW_STR32)pConDst->ItemList;
	
	for (i = 0; i < itemNum; i++)
		strcpy(&pItemDst[i], &pItemSrc[i]);
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayS64(TW_UINT16 cap, char* szSrc, int itemNum)
{
	RelLogMessage("SetArrayS64\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR64 pItemSrc, pItemDst;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_STR64) * itemNum));
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR64;
	pConDst->NumItems		= itemNum;	
	
	pItemSrc = (pTW_STR64)szSrc;
	pItemDst = (pTW_STR64)pConDst->ItemList;
	
	for (i = 0; i < itemNum; i++)
		strcpy(&pItemDst[i], &pItemSrc[i]);
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayS128(TW_UINT16 cap, char* szSrc, int itemNum)
{
	RelLogMessage("SetArrayS128\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR128 pItemSrc, pItemDst;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_STR128) * itemNum));
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR128;
	pConDst->NumItems		= itemNum;	
	
	pItemSrc = (pTW_STR64)szSrc;
	pItemDst = (pTW_STR64)pConDst->ItemList;
	
	for (i = 0; i < itemNum; i++)
		strcpy(&pItemDst[i], &pItemSrc[i]);
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}


int SetArrayS255(TW_UINT16 cap, char* szSrc, int itemNum)
{
	RelLogMessage("SetArrayS255\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR255 pItemSrc, pItemDst;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_STR255) * itemNum));
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR255;
	pConDst->NumItems		= itemNum;	
	
	pItemSrc = (pTW_STR255)szSrc;
	pItemDst = (pTW_STR255)pConDst->ItemList;
	
	for (i = 0; i < itemNum; i++)
		strcpy(&pItemDst[i], &pItemSrc[i]);
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int GetEnumS32(TW_UINT16 cap, pTW_CON_ES32 pConDst)				
{
	RelLogMessage("GetEnumS32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConSrc;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pConSrc = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConDst->ItemType		= pConSrc->ItemType;
		pConDst->NumItems		= pConSrc->NumItems;
		pConDst->DefaultIndex	= pConSrc->DefaultIndex;
		pConDst->CurrentIndex	= pConSrc->CurrentIndex;
		for (i = 0; i < pConSrc->NumItems; i++)
		{
			strcpy(&((pTW_STR32)pConDst->ItemList)[i], &((pTW_STR32)pConSrc->ItemList)[i]);
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}

int GetEnumS64(TW_UINT16 cap, pTW_CON_ES64 pConDst)				
{
	RelLogMessage("GetEnumS64\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConSrc;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pConSrc = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConDst->ItemType		= pConSrc->ItemType;
		pConDst->NumItems		= pConSrc->NumItems;
		pConDst->DefaultIndex	= pConSrc->DefaultIndex;
		pConDst->CurrentIndex	= pConSrc->CurrentIndex;
		for (i = 0; i < pConSrc->NumItems; i++)
		{
			strcpy(&((pTW_STR64)pConDst->ItemList)[i], &((pTW_STR64)pConSrc->ItemList)[i]);
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}

int GetEnumS128(TW_UINT16 cap, pTW_CON_ES128 pConDst)				
{
	RelLogMessage("GetEnumS128\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConSrc;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pConSrc = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConDst->ItemType		= pConSrc->ItemType;
		pConDst->NumItems		= pConSrc->NumItems;
		pConDst->DefaultIndex	= pConSrc->DefaultIndex;
		pConDst->CurrentIndex	= pConSrc->CurrentIndex;
		for (i = 0; i < pConSrc->NumItems; i++)
		{
			strcpy(&((pTW_STR128)pConDst->ItemList)[i], &((pTW_STR128)pConSrc->ItemList)[i]);
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}

int GetEnumS255(TW_UINT16 cap, pTW_CON_ES255 pConDst)				
{
	RelLogMessage("GetEnumS255\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConSrc;
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap = cap;
	twCapability.ConType = TWON_DONTCARE16;
	twCapability.hContainer = NULL;
	
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
								&dsID,
								DG_CONTROL,
								DAT_CAPABILITY,
								MSG_GET,
								(TW_MEMREF)&twCapability);
	if(rc == TWRC_SUCCESS)
	{
		pConSrc = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
		
		pConDst->ItemType		= pConSrc->ItemType;
		pConDst->NumItems		= pConSrc->NumItems;
		pConDst->DefaultIndex	= pConSrc->DefaultIndex;
		pConDst->CurrentIndex	= pConSrc->CurrentIndex;
		for (i = 0; i < pConSrc->NumItems; i++)
		{
			strcpy(&((pTW_STR255)pConDst->ItemList)[i], &((pTW_STR255)pConSrc->ItemList)[i]);
		}
		GlobalUnlock(twCapability.hContainer);
		GlobalFree(twCapability.hContainer);
	}
	
	return rc;
}


int SetEnumS32(TW_UINT16 cap, pTW_CON_ES32 pConSrc, pTW_STR32 szSrc)						//
{
	RelLogMessage("SetEnumS32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR32 pItemSrc, pItemDst;															//
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_STR32) * (pConSrc->NumItems)));					//
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR32;													//
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_STR32)pConSrc->ItemList;												//
	pItemDst = (pTW_STR32)pConDst->ItemList;												//
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(!strcmp(&pItemSrc[i], szSrc))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumS64(TW_UINT16 cap, pTW_CON_ES64 pConSrc, pTW_STR64 szSrc)						//
{
	RelLogMessage("SetEnumS64\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR64 pItemSrc, pItemDst;															//
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_STR64) * (pConSrc->NumItems)));					//
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR64;													//
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_STR64)pConSrc->ItemList;												//
	pItemDst = (pTW_STR64)pConDst->ItemList;												//
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(!strcmp(&pItemSrc[i], szSrc))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumS128(TW_UINT16 cap, pTW_CON_ES128 pConSrc, pTW_STR128 szSrc)						//
{
	RelLogMessage("SetEnumS128\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR128 pItemSrc, pItemDst;															//
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_STR128) * (pConSrc->NumItems)));					//
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR128;													//
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_STR128)pConSrc->ItemList;												//
	pItemDst = (pTW_STR128)pConDst->ItemList;												//
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(!strcmp(&pItemSrc[i], szSrc))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}

int SetEnumS255(TW_UINT16 cap, pTW_CON_ES255 pConSrc, pTW_STR255 szSrc)						//
{
	RelLogMessage("SetEnumS255\r\n");
	TW_CAPABILITY twCapability;
	pTW_ENUMERATION pConDst = NULL;
	TW_UINT16 i;
	int rc;
	pTW_STR255 pItemSrc, pItemDst;															//
	BOOL bIsSet = FALSE;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ENUMERATION;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ENUMERATION) + sizeof(TW_STR255) * (pConSrc->NumItems)));					//
	
	pConDst = (pTW_ENUMERATION)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR255;													//
	pConDst->NumItems		= pConSrc->NumItems;	
	pConDst->CurrentIndex	= pConSrc->CurrentIndex;
	pConDst->DefaultIndex	= pConSrc->DefaultIndex;
	
	pItemSrc = (pTW_STR255)pConSrc->ItemList;												//
	pItemDst = (pTW_STR255)pConDst->ItemList;												//
	
	for (i = 0; i < pConSrc->NumItems; i++)
	{
		pItemDst[i] = pItemSrc[i];
		if(!strcmp(&pItemSrc[i], szSrc))
		{
			pConDst->CurrentIndex = i;
			bIsSet = TRUE;
		}
	}
	GlobalUnlock(twCapability.hContainer);
	
	if(bIsSet == TRUE)
	{
		//Send the Triplet
		rc = CallDSMEntry(	&appID,
			&dsID,
			DG_CONTROL,
			DAT_CAPABILITY,
			MSG_SET,
			(TW_MEMREF)&twCapability);
		GlobalFree(twCapability.hContainer);
		return rc;
	}
	else
		return TWRC_FAILURE;
}



int SetOneValS32(TW_UINT16 cap, pTW_STR32 szSrc)												//
{
	RelLogMessage("SetOneValS32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_STR32 pItemDst;																		//
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE) + sizeof(TW_STR32));	//
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR32;													//
	pItemDst = (pTW_STR32)&pConDst->Item;													//	
	strcpy(pItemDst, szSrc);	
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValS64(TW_UINT16 cap, pTW_STR64 szSrc)												//
{
	RelLogMessage("SetOneValS64\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_STR32 pItemDst;																		//
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE) + sizeof(TW_STR64));	//
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR64;													//
	pItemDst = (pTW_STR64)&pConDst->Item;													//	
	strcpy(pItemDst, szSrc);	
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValS128(TW_UINT16 cap, pTW_STR128 szSrc)												//
{
	RelLogMessage("SetOneValS128\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_STR128 pItemDst;																		//
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE) + sizeof(TW_STR128));	//
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR128;													//
	pItemDst = (pTW_STR128)&pConDst->Item;													//	
	strcpy(pItemDst, szSrc);	
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}

int SetOneValS255(TW_UINT16 cap, pTW_STR255 szSrc)												//
{
	RelLogMessage("SetOneValS255\r\n");
	TW_CAPABILITY twCapability;
	pTW_ONEVALUE pConDst = NULL;
	pTW_STR255 pItemDst;																		//
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ONEVALUE;
	twCapability.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE) + sizeof(TW_STR255));	//
	
	pConDst = (pTW_ONEVALUE)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_STR255;													//
	pItemDst = (pTW_STR255)&pConDst->Item;													//	
	strcpy(pItemDst, szSrc);	
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);

	return rc;
}


int __stdcall CALL_SetProperty_String(TW_UINT16 cap, char* szSrc, int itemNum)
{
	RelLogMessage("CALL_SetProperty_String\r\n");
	TW_UINT16 conType, itemType;
	int rc;

	static TW_CON_ES32	conES32;
	static TW_CON_ES64	conES64;
	static TW_CON_ES128	conES128;
	static TW_CON_ES255	conES255;

	rc = GetType(cap, &conType, &itemType);
	
	if(rc == TWCRC_SUCCESS)
	{
		switch(conType)
		{
		case TWON_ARRAY:
			switch(itemType)
			{
			case TWTY_STR32:
				rc = SetArrayS32(cap, szSrc, itemNum);
				break;

			case TWTY_STR64:
				rc = SetArrayS64(cap, szSrc, itemNum);
				break;

			case TWTY_STR128:
				rc = SetArrayS128(cap, szSrc, itemNum);
				break;

			case TWTY_STR255:
				rc = SetArrayS255(cap, szSrc, itemNum);
				break;

			default:
				return TWCRC_NOT_STR;
			}
			break;
			
		case TWON_ENUMERATION:
			switch(itemType)
			{
			case TWTY_STR32:
				GetEnumS32(cap, &conES32);
				rc = SetEnumS32(cap, &conES32, szSrc);
				break;

			case TWTY_STR64:
				GetEnumS64(cap, &conES64);
				rc = SetEnumS64(cap, &conES64, szSrc);
				break;

			case TWTY_STR128:
				GetEnumS128(cap, &conES128);
				rc = SetEnumS128(cap, &conES128, szSrc);
				break;

			case TWTY_STR255:
				GetEnumS255(cap, &conES255);
				rc = SetEnumS255(cap, &conES255, szSrc);
				break;

			default:
				return TWCRC_NOT_STR;
			}
			break;
			
		case TWON_ONEVALUE:
			switch(itemType)
			{
			case TWTY_STR32:
				rc = SetOneValS32(cap, szSrc);
				break;

			case TWTY_STR64:
				rc = SetOneValS64(cap, szSrc);
				break;

			case TWTY_STR128:
				rc = SetOneValS128(cap, szSrc);
				break;

			case TWTY_STR255:
				rc = SetOneValS255(cap, szSrc);
				break;

			default:
				return TWCRC_NOT_STR;
			}
			break;
			
		case TWON_RANGE:
			return TWCRC_NOT_SUPPORTED;
		}
	}

	return (-rc);
}


int SetArrayI8(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayI8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_INT8 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_INT8) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT8;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_INT8)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_INT8)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayI16(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayI16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_INT16 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_INT16) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT16;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_INT16)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_INT16)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayI32(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayI32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_INT32 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_INT32) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_INT32;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_INT32)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_INT32)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}


int SetArrayU8(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayU8\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_UINT8 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_UINT8) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT8;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_UINT8)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_UINT8)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayU16(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayU16\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_UINT16 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_UINT16) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT16;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_UINT16)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_UINT16)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayU32(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayU32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_UINT32 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_UINT32) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_UINT32;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_UINT32)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_UINT32)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}


int SetArrayBool(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayBool\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_BOOL pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_BOOL) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_BOOL;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_BOOL)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = (TW_BOOL)pVal[i];											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}

int SetArrayF32(TW_UINT16 cap, pTW_UINT32 pVal, int ItemNum)						// 여기
{
	RelLogMessage("SetArrayF32\r\n");
	TW_CAPABILITY twCapability;
	pTW_ARRAY pConDst = NULL;
	pTW_FIX32 pItemDst;												// 여기
	TW_UINT16 i;
	int rc;
	
	//Setup TW_CAPABILITY Structure
	twCapability.Cap		= cap;
	twCapability.ConType	= TWON_ARRAY;
	twCapability.hContainer = GlobalAlloc(GHND,
		(sizeof(TW_ARRAY) + sizeof(TW_FIX32) * ItemNum));						// 여기
	
	pConDst = (pTW_ARRAY)GlobalLock(twCapability.hContainer);
	pConDst->ItemType		= TWTY_FIX32;										// 여기
	pConDst->NumItems		= ItemNum;	
	
	pItemDst = (pTW_FIX32)pConDst->ItemList;										// 여기
	for (i = 0; i < ItemNum; i++)
		pItemDst[i] = FloatToFIX32((float)pVal[i]);											// 여기
	GlobalUnlock(twCapability.hContainer);
	
	//Send the Triplet
	rc = CallDSMEntry(	&appID,
		&dsID,
		DG_CONTROL,
		DAT_CAPABILITY,
		MSG_SET,
		(TW_MEMREF)&twCapability);
	GlobalFree(twCapability.hContainer);
	return rc;
}


//	입력 값은 4바이트 배열로 고정. 내부적으로  아이템 타입을 알아내서 변환시킨다.
int __stdcall CALL_SetProperty_Array(TW_UINT16 cap, pTW_UINT32 pVal, int itemNum)
{
	RelLogMessage("CALL_SetProperty_Array\r\n");
	int rc;
	TW_UINT16 conType;
	TW_UINT16 itemType;

	rc = GetType(cap, &conType, &itemType);

	if(rc == TWCRC_SUCCESS)
	{
		if(conType == TWON_ARRAY)
		{
			switch(itemType)
			{
			case TWTY_INT8    :
				rc = SetArrayI8(cap, pVal, itemNum);
				break;
			case TWTY_INT16   :
				rc = SetArrayI16(cap, pVal, itemNum);
				break;
			case TWTY_INT32   :
				rc = SetArrayI32(cap, pVal, itemNum);
				break;

			case TWTY_UINT8   :
				rc = SetArrayU8(cap, pVal, itemNum);
				break;
			case TWTY_UINT16  :
				rc = SetArrayU16(cap, pVal, itemNum);
				break;
			case TWTY_UINT32  :
				rc = SetArrayU32(cap, pVal, itemNum);
				break;

			case TWTY_BOOL    :
				break;

			case TWTY_FIX32   :
				break;

			default:
				return TWCRC_NOT_SUPPORTED;
			}
		}
		else
			return TWCRC_NOT_ARRAY;
	}

	return (-rc);
}

int __stdcall CALL_SetProperty(TW_UINT16 cap, int val)
{
	RelLogMessage("CALL_SetProperty\r\n");
	int rc;
	TW_UINT16 conType;
	TW_UINT16 itemType;

	static TW_CON_EI8	conEnumI8;
	static TW_CON_EI16	conEnumI16;
	static TW_CON_EI32	conEnumI32;

	static TW_CON_EU8	conEnumU8;
	static TW_CON_EU16	conEnumU16;
	static TW_CON_EU32	conEnumU32;

	static TW_CON_EF32	conEnumF32;
	static TW_CON_EBOOL	conEnumBool;


	static TW_CON_RANGE		conRng;

	static TW_UINT32		u32;
	static TW_FIX32			f32;
	
	TW_INT8		li8		= val;
	TW_INT16	li16	= val;
	TW_UINT8	lu8		= val;
	TW_UINT16	lu16	= val;
	TW_BOOL		lbool	= val;
	u32 = 0;
				
	// 예외 처리 - Get에서 받는 자료형과 Set으로 넘겨야 하는 자료형이 다르다.
	switch(cap)
	{
	case ICAP_ZOOMFACTOR:
		rc = SetOneValI16(cap, (TW_INT16)val);
		goto Return_SetProperty;

	case CAP_PRINTERMODE:
	case CAP_JOBCONTROL:
	case ICAP_BARCODESEARCHMODE:
	case ICAP_BITORDER:
	case ICAP_BITORDERCODES:
	case ICAP_ICCPROFILE:
	case ICAP_LIGHTPATH:
	case ICAP_PATCHCODESEARCHMODE:
	case ICAP_PIXELFLAVOR:
	case ICAP_PIXELFLAVORCODES:
	case ICAP_TIMEFILL:
		rc = SetOneValU16(cap, (TW_UINT16)val);
		goto Return_SetProperty;

	case ICAP_ROTATION:
		rc = SetOneValF32(cap, FloatToFIX32((float)val));
		goto Return_SetProperty;

	case CAP_MAXBATCHBUFFERS:
	case ICAP_BARCODEMAXRETRIES:
	case ICAP_BARCODEMAXSEARCHPRIORITIES:
	case ICAP_BARCODETIMEOUT:
	case ICAP_PATCHCODEMAXRETRIES:
	case ICAP_PATCHCODEMAXSEARCHPRIORITIES:
	case ICAP_PATCHCODETIMEOUT:
		rc = SetOneValU32(cap, (TW_UINT32)val);
		goto Return_SetProperty;
	}


	rc = GetType(cap, &conType, &itemType);


	if(rc == TWCRC_SUCCESS)
	{
		switch(conType)
		{
		case TWON_ARRAY:
			// 값 하나만 받는 함수에서 현재값을 여러개 가지는 어레이를 지원할 수 없다. 
			return TWCRC_NOT_SUPPORTED;

		case TWON_ENUMERATION:
			switch(itemType)
			{
			case TWTY_INT8  :
				memset(&conEnumI8, 0, sizeof(TW_CON_EI8));
				GetEnumI8(cap, &conEnumI8);
				rc = SetEnumI8(cap, &conEnumI8, (TW_INT8)val);				// 미확인 
				goto Return_SetProperty;

			case TWTY_INT16 :	
				memset(&conEnumI16, 0, sizeof(TW_CON_EI16));
				GetEnumI16(cap, &conEnumI16);
				rc = SetEnumI16(cap, &conEnumI16, (TW_INT16)val);				// 미확인 
				goto Return_SetProperty;

			case TWTY_INT32 :
				memset(&conEnumI32, 0, sizeof(TW_CON_EI32));
				GetEnumI32(cap, &conEnumI32);
				rc = SetEnumI32(cap, &conEnumI32, (TW_INT32)val);				// 미확인 
				goto Return_SetProperty;

			case TWTY_UINT8 :	
				memset(&conEnumU8, 0, sizeof(TW_CON_EU8));
				GetEnumU8(cap, &conEnumU8);
				rc = SetEnumU8(cap, &conEnumU8, (TW_UINT8)val);				// 미확인 
				goto Return_SetProperty;

			case TWTY_UINT16:
				memset(&conEnumU16, 0, sizeof(TW_CON_EU16));
				GetEnumU16(cap, &conEnumU16);
				rc = SetEnumU16(cap, &conEnumU16, (TW_UINT16)val);				// 확인 
				goto Return_SetProperty;

			case TWTY_UINT32:	
				memset(&conEnumU32, 0, sizeof(TW_CON_EU32));
				GetEnumU32(cap, &conEnumU32);
				rc = SetEnumU32(cap, &conEnumU32, (TW_UINT32)val);				// 미확인 
				goto Return_SetProperty;

			case TWTY_FIX32 :	
				memset(&conEnumF32, 0, sizeof(TW_CON_EF32));
				GetEnumF32(cap, &conEnumF32);
				rc = SetEnumF32(cap, &conEnumF32, FloatToFIX32((float)val));	// 미확인 - S1500
				goto Return_SetProperty;

			case TWTY_BOOL  :	
				memset(&conEnumBool, 0, sizeof(TW_CON_EBOOL));
				GetEnumBool(cap, &conEnumBool);
				rc = SetEnumBool(cap, &conEnumBool, (TW_BOOL)val);				// 확인 
				goto Return_SetProperty;

			// 미지원
			case TWTY_FRAME :
			case TWTY_STR32 :	
			case TWTY_STR64 :
			case TWTY_STR128:
			case TWTY_STR255:	
			default:
				return TWCRC_NOT_SUPPORTED;
			} // switch(itemType)
			break;

		case TWON_ONEVALUE:
			switch(itemType)
			{
			case TWTY_INT8  :	
				rc = SetOneValI8(cap, (TW_INT8)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_INT16 :	
				rc = SetOneValI16(cap, (TW_INT16)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_INT32 :	
				rc = SetOneValI32(cap, (TW_INT32)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_UINT8 :	
				rc = SetOneValU8(cap, (TW_UINT8)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_UINT16:
				rc = SetOneValU16(cap, (TW_UINT16)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_UINT32:	
				rc = SetOneValU32(cap, (TW_UINT32)val);			// 미확인
				goto Return_SetProperty;

			case TWTY_FIX32 :	
				rc = SetOneValF32(cap, FloatToFIX32((float)val));	// 확인
				goto Return_SetProperty;

			case TWTY_BOOL  :	
				rc = SetOneValBool(cap, (TW_BOOL)val);			// 미확인
				goto Return_SetProperty;

			// 미지원
			case TWTY_FRAME :	
			case TWTY_STR32 :	
			case TWTY_STR64 :
			case TWTY_STR128:	
			case TWTY_STR255:	
			default:
				return TWCRC_NOT_SUPPORTED;
			} // switch(itemType)
			break;

		case TWON_RANGE:
			switch(itemType)
			{
			case TWTY_INT8  : 
				memcpy(&u32, &li8, sizeof(TW_INT8));
				goto GetSetRange;
				
			case TWTY_INT16 : 
				memcpy(&u32, &li16, sizeof(TW_INT16));
				goto GetSetRange;
				
			case TWTY_UINT8 : 
				memcpy(&u32, &lu8, sizeof(TW_UINT8));
				goto GetSetRange;
				
			case TWTY_UINT16: 
				memcpy(&u32, &lu16, sizeof(TW_UINT16));
				goto GetSetRange;
				
			case TWTY_BOOL  : 
				memcpy(&u32, &lbool, sizeof(TW_BOOL));
				goto GetSetRange;
				
			case TWTY_INT32 :	
			case TWTY_UINT32:	
				u32 = val;
				goto GetSetRange;

			case TWTY_FIX32 :	
				memset(&conRng, 0, sizeof(TW_CON_RANGE));
				GetRange(cap, &conRng);
				f32 = FloatToFIX32((float)val);
				rc = SetRange(cap, &conRng, (pTW_UINT32)&f32);		// 확인
				goto Return_SetProperty;

			// 미지원
			case TWTY_FRAME :
			case TWTY_STR32 :
			case TWTY_STR64 :
			case TWTY_STR128:
			case TWTY_STR255:
			default:
				return TWCRC_NOT_SUPPORTED;
			} // switch(itemType)

GetSetRange:
			memset(&conRng, 0, sizeof(TW_CON_RANGE));
			GetRange(cap, &conRng);
			rc = SetRange(cap, &conRng, &u32);		// 확인
			goto Return_SetProperty;

		default:
			return TWCRC_NOT_SUPPORTED;
		} // switch(conType)
	} // if(GetType == success)
	

Return_SetProperty:
	int myRC = -rc;
	return myRC;
}

int __stdcall CALL_CheckCondition(void)
{
	RelLogMessage("CALL_CheckCondition\r\n");
	return (int)gGlobalStatus.ConditionCode;
}

int __stdcall CALL_GetType(TW_UINT16 cap, pTW_UINT16 pConType, pTW_UINT16 pItemType)
{
	RelLogMessage("CALL_GetType\r\n");
	return GetType(cap, pConType, pItemType);
}



typedef struct {
	TW_STR128 NoUIParmFile;		// Parameter file name
} TW_NOUIPARMFILE, FAR* pTW_NOUIPARMFILE;

typedef struct {
	TW_BOOL DivScanFlg;		
} TW_DIVSCANCTL, FAR* pTW_DIVSCANCTL;

typedef struct {
	TW_UINT16 EjectInstruct;
} TW_EJECTCTL, FAR* pTW_EJECTCTL;

typedef struct {
	TW_BOOL NumberingFlg;
	TW_STR128 NumberingChar;
} TW_EJECTNUMBERING, FAR* pTW_EJECTNUMBERING;

#define DAT_NOUIPARMFILE	0x8001
#define DAT_DIVSCANCTL		0x8002
#define DAT_EJECTCTL		0x8003
#define DAT_EJECTNUMBERING	0x8004

//************************************************************************
// name   :SetCap_SetParm
// func   :UIなし時?取パラメタ設定
//
// return :BOOL ?理結果
//
//************************************************************************
BOOL __stdcall CALL_SetCap_SetParm()
{
	BOOL Ret=TRUE; //?り値
	TW_UINT16 twRC = TWRC_FAILURE; //DSM_Entry?り値
	char ErrMsg[512]; //エラ?メッセ?ジ
	TW_NOUIPARMFILE twNoUIParmFile; //UIなし時?取パラメタ情報
	
// 	if((m_RXTwainFlg)&&(m_FLG_SetParm))
	if(1)
	{
// 		lstrcpy(twNoUIParmFile.NoUIParmFile,m_SetParmName);
		lstrcpy(twNoUIParmFile.NoUIParmFile,"HT4139_setting");
		
		//------------------------------
		//UI無し時?取パラメタファイル指定
		//------------------------------
		twRC = CallDSMEntry(&appID,
			&dsID,
			DG_CONTROL, 
			DAT_NOUIPARMFILE, 
			MSG_RESET,					// MSG_RESET - 설정파일 사용안함, MSG_SET - 설정 파일 사용
			(TW_MEMREF)&twNoUIParmFile);
		switch (twRC)
		{
		case TWRC_SUCCESS:
			Ret=TRUE;
			break;
		case TWRC_FAILURE:         
		default:
			Ret=FALSE;
			TW_UINT16 ConditionCode=999; //コンディションコ?ド
// 			ConditionCode=GetConditionCode();
			
			wsprintf(ErrMsg,"ソ?ス?マネ?ジャのオ?プン(DG_CONTROL/DAT_NOUIPARMFILE/MSG_SET)に失敗しました。(ConditionCode=%d)",ConditionCode);
			MessageBox(NULL,ErrMsg,"Error",MB_OK|MB_ICONSTOP);
			
			break;
		}
	}
	return Ret;
}
//************************************************************************
// name   :SetCap_DivScan
// func   :分割?取設定
//
// return :BOOL ?理結果
//
//************************************************************************
BOOL __stdcall CALL_SetCap_DivScan()
{
	BOOL Ret=TRUE; //?り値
	TW_DIVSCANCTL twDivScanCtl; //分割?取情報
	TW_UINT16 twRC = TWRC_FAILURE; //DSM_Entry?り値
	char ErrMsg[512]; //エラ?メッセ?ジ
	TW_UINT16 ConditionCode=999; //コンディションコ?ド

	
// 	if((m_RXTwainFlg)&&(m_FLG_DivScan))
	if(1)
	{
		twDivScanCtl.DivScanFlg=TRUE;
		
		//------------------------------
		//分割??設定
		//------------------------------
		twRC = CallDSMEntry(&appID,
			&dsID,
			DG_CONTROL, 
			DAT_DIVSCANCTL, 
			MSG_SET,
			(TW_MEMREF)&twDivScanCtl);
		switch (twRC)
		{
		case TWRC_SUCCESS:
			Ret=TRUE;
			break;
		case TWRC_FAILURE:         
		default:
			Ret=FALSE;
			
// 			ConditionCode=GetConditionCode();
			wsprintf(ErrMsg,"分割?取(DG_CONTROL/DAT_DIVSCANCTL/MSG_SET)に失敗しました。(ConditionCode=%d)",ConditionCode);
			MessageBox(NULL,ErrMsg,"Error",MB_OK|MB_ICONSTOP);
			
			break;
		}
	}
	return Ret;
	
}
//************************************************************************
// name   :EjectCtl_MSG_SET
// func   :排出制御設定
//
// return :BOOL ?理結果
//
//************************************************************************

BOOL __stdcall CALL_EjectCtl_MSG_SET(
								  BOOL EjectInstruct) //排出指定
{
	BOOL Ret=TRUE; //?り値
	TW_UINT16 twRC = TWRC_FAILURE; //DSM_Entry?り値
	char ErrMsg[512]; //エラ?メッセ?ジ
	TW_UINT16 ConditionCode=999; //コンディションコ?ド
	TW_EJECTCTL twEjectCtl; //排出制御情報
	

	twEjectCtl.EjectInstruct=EjectInstruct;
	
	//------------------------------
	//排出制御設定
	//------------------------------
	twRC = CallDSMEntry(&appID,
		&dsID, 
		DG_CONTROL,
		DAT_EJECTCTL, 
		MSG_SET,
		(TW_MEMREF)&twEjectCtl);
	switch (twRC)
	{
	case TWRC_SUCCESS:
		break;
	case TWRC_FAILURE:
	default:
		Ret=FALSE;
// 		ConditionCode=GetConditionCode();
		
		wsprintf(ErrMsg,"レイアウト情報の取得(DG_CONTROL/DAT_EJECTCTL/MSG_SET)に失敗しました。(ConditionCode=%lu)",ConditionCode);
		MessageBox(NULL,ErrMsg,"Error",MB_OK|MB_ICONSTOP);
		
		break;
	}
	return Ret;
}
//************************************************************************
// name   :EjectNumbering_MSG_GET
// func   :排出時ナンバリング桁?取得
//
// return :BOOL ?理結果
//
//************************************************************************

BOOL __stdcall CALL_EjectNumbering_MSG_GET(
										TW_UINT16 * pNumberingCnt) //ナンバリング桁?(出力)
{
	BOOL Ret=TRUE; //?り値
	TW_UINT16   twRC = TWRC_FAILURE; //DSM_Entry?り値
	char ErrMsg[512]; //エラ?メッセ?ジ
	TW_UINT16 ConditionCode=999; //コンディションコ?ド
	
	//------------------------------
	//排出時ナンバリング設定
	//------------------------------
	twRC = CallDSMEntry(&appID,
		&dsID, 
		DG_CONTROL,
		DAT_EJECTNUMBERING, 
		MSG_GET,
		(TW_MEMREF)pNumberingCnt);
	switch (twRC)
	{
	case TWRC_SUCCESS:
		break;
	case TWRC_FAILURE:
	default:
		Ret=FALSE;
// 		ConditionCode=GetConditionCode();
		
		wsprintf(ErrMsg,"レイアウト情報の取得(DG_CONTROL/DAT_EJECTCTL/MSG_SET)に失敗しました。(ConditionCode=%lu)",ConditionCode);
		MessageBox(NULL,ErrMsg,"Error",MB_OK|MB_ICONSTOP);
		
		break;
	}
	return Ret;
}
//************************************************************************
// name   :EjectNumbering_MSG_SET
// func   :排出時ナンバリング設定
//
// return :BOOL ?理結果
//
//************************************************************************
BOOL __stdcall CALL_EjectNumbering_MSG_SET(
										TW_BOOL NumberingFlg, //ナンバリング印字を行うか
										TW_STR128 NumberingChar) //ナンバリング印字文字
{
	BOOL Ret=TRUE; //?り値
	TW_UINT16           twRC = TWRC_FAILURE; //DSM_Entry?り値
	char ErrMsg[512]; //エラ?メッセ?ジ
	TW_UINT16 ConditionCode=999; //コンディションコ?ド
	TW_EJECTNUMBERING twEjectNumbering; //排出時ナンバリング情報
	
	twEjectNumbering.NumberingFlg=NumberingFlg; //ナンバリング印字を行うか
	memcpy(twEjectNumbering.NumberingChar,NumberingChar,sizeof(TW_STR128)); //ナンバリング印字文字
	
	//------------------------------
	//排出時ナンバリング設定
	//------------------------------
	twRC = CallDSMEntry(&appID,
		&dsID, 
		DG_CONTROL,
		DAT_EJECTNUMBERING, 
		MSG_SET,
		(TW_MEMREF)&twEjectNumbering);
	switch (twRC)
	{
	case TWRC_SUCCESS:
		break;
	case TWRC_FAILURE:
	default:
		Ret=FALSE;
// 		ConditionCode=GetConditionCode();
		
		wsprintf(ErrMsg,"レイアウト情報の取得(DG_CONTROL/DAT_EJECTCTL/MSG_SET)に失敗しました。(ConditionCode=%lu)",ConditionCode);
		MessageBox(NULL,ErrMsg,"Error",MB_OK|MB_ICONSTOP);
		
		break;
	}
	
	return Ret;
}

BOOL gbStop = FALSE;
TW_UINT16 gtwRC2 = TWRC_FAILURE;

int __stdcall CALL_StopFeeding()
{
	gbStop = TRUE;
	return gtwRC2;
}

int __stdcall CALL_SetWH(int nWidth, int nHeight)
{
	TW_UINT16       twRC = TWRC_FAILURE;
	TW_IMAGELAYOUT	twImageLayout;


	memset(&twImageLayout, 0, sizeof(TW_IMAGELAYOUT));
	// don't care
	twImageLayout.DocumentNumber = -1;
	twImageLayout.FrameNumber = -1;
	twImageLayout.PageNumber = -1;

	// set width, height
	twImageLayout.Frame.Right = FloatToFIX32((float)nWidth);
	twImageLayout.Frame.Bottom = FloatToFIX32((float)nHeight);

	// Check ImageInfo information
	twRC = CallDSMEntry(&appID,
					&dsID, 
					DG_IMAGE,
					DAT_IMAGELAYOUT, 
					MSG_SET, 
					(TW_MEMREF)&twImageLayout);

	return -twRC;
}

//////////////////////////////////////////////////////////////////////////
////// 2016.03.09 c#에서 사용하기 위해 새로 만든 함수 
void __stdcall CALL_Set_PrevProc(long prevproc)
{
	
	PrevProc = 
		(*((LRESULT FAR (PASCAL * )(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam))prevproc));
	
}

long __stdcall CALL_Get_WinProc()
{
	return (long)DLLWndProc;
	
}

int __stdcall CALL_Set_Ejection()
{
// 	static int rtn = 99999;
// 
// 	if(rtn)
// 	{
		return Set_Ejection();
// 	}
	

}

//////////////////////////////////////////////////////////////////////////


