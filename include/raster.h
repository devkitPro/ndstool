// Code taken from: BMP LOAD EXAMPLE by Juan Soulie <jsoulie@cplusplus.com>
// Modified by natrium42 <natrium@gmail.com>

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <stdexcept>
using namespace std ;



#ifndef _WINDEF_    
// Types - defined as in WinDef.h 

typedef int LONG ;
typedef unsigned int  DWORD ; 
typedef unsigned short WORD ;

#endif



#ifndef _WINGDI_
// Types - defined as in WinGDI.h

#pragma pack(2)  /* align structure fields on 2-byte boundaries */

typedef struct tagRGBQUAD {
  unsigned char    rgbBlue; 
  unsigned char    rgbGreen; 
  unsigned char    rgbRed; 
  unsigned char    rgbReserved; 
} RGBQUAD; 

typedef struct tagBITMAPINFOHEADER{ // bmih 
   DWORD  biSize; 
   LONG   biWidth; 
   LONG   biHeight; 
   WORD   biPlanes; 
   WORD   biBitCount ;
   DWORD  biCompression; 
   DWORD  biSizeImage; 
   LONG   biXPelsPerMeter; 
   LONG   biYPelsPerMeter; 
   DWORD  biClrUsed; 
   DWORD  biClrImportant; 
} BITMAPINFOHEADER; 

typedef struct tagBITMAPINFO { 
  BITMAPINFOHEADER bmiHeader; 
  RGBQUAD          bmiColors[1]; 
} BITMAPINFO, *PBITMAPINFO; 

typedef struct tagBITMAPFILEHEADER { 
  WORD    bfType; 
  DWORD   bfSize; 
  WORD    bfReserved1; 
  WORD    bfReserved2; 
  DWORD   bfOffBits; 
} BITMAPFILEHEADER, *PBITMAPFILEHEADER; 

#pragma pack() /* return to default packing of structure fields*/

#endif

class CRaster {
	public:
		int Width,Height;		// Dimensions
		int BPP;				// Bits Per Pixel.
		char * Raster;			// Bits of the Image.
		RGBQUAD * Palette;		// RGB Palette for the image.
		int BytesPerRow;		// Row Width (in bytes).
		BITMAPINFO * pbmi;		// BITMAPINFO structure

		// Member functions (defined later):
		int LoadBMP (char * szFile);
};
