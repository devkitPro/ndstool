// Code taken from: BMP LOAD EXAMPLE by Juan Soulie <jsoulie@cplusplus.com>
// Modified by natrium42 <natrium@gmail.com>

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <stdexcept>
using namespace std;



//#ifndef _WINGDI_
// Types - defined as in WinGDI.h

#pragma pack(2)  /* align structure fields on 2-byte boundaries */

typedef struct tagRGBQUAD
{
  unsigned char    rgbBlue;
  unsigned char    rgbGreen;
  unsigned char    rgbRed;
  unsigned char    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER
{
   unsigned_int  biSize;
   signed_int   biWidth;
   signed_int   biHeight;
   unsigned_short   biPlanes;
   unsigned_short   biBitCount ;
   unsigned_int  biCompression;
   unsigned_int  biSizeImage;
   signed_int   biXPelsPerMeter;
   signed_int   biYPelsPerMeter;
   unsigned_int  biClrUsed;
   unsigned_int  biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO;

typedef struct tagBITMAPFILEHEADER
{
  unsigned char    bfType[2];
  unsigned_int   bfSize;
  unsigned_short    bfReserved1;
  unsigned_short    bfReserved2;
  unsigned_int   bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

#pragma pack() /* return to default packing of structure fields*/

//#endif

class CRaster
{
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
