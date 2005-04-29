// Code taken from: BMP LOAD EXAMPLE by Juan Soulie <jsoulie@cplusplus.com>
// Modified by natrium42 <natrium@gmail.com>

#include "ndstool.h"
#include "raster.h"


// **********
// CRaster::LoadBMPFile (FileName);
//   - loads a BMP file into a CRaster object
//   * supports non-RLE-compressed files of 1, 2, 4, 8 & 24 bits-per-pixel
int CRaster::LoadBMP (char * szFile)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	// Open file.
	ifstream bmpfile (szFile , ios::in | ios::binary);
	if (! bmpfile.is_open()) return 1;		// Error opening file

	// Load bitmap fileheader & infoheader
	bmpfile.read ((char*)&bmfh,sizeof (BITMAPFILEHEADER));
	bmpfile.read ((char*)&bmih,sizeof (BITMAPINFOHEADER));

	// Check filetype signature
	if ((bmfh.bfType[0] != 'B') || (bmfh.bfType[1] != 'M')) return 2;		// File is not BMP

	// Assign some short variables:
	BPP=bmih.biBitCount;
	Width=bmih.biWidth;
	Height=(bmih.biHeight>0) ? (int)bmih.biHeight : -(int)bmih.biHeight; // absoulte value
	BytesPerRow = Width * BPP / 8;
	BytesPerRow += (4-BytesPerRow%4) % 4;	// int alignment

	// If BPP aren't 24, load Palette:
	if (BPP==24) pbmi=(BITMAPINFO*)new char [sizeof(BITMAPINFO)];
	else
	{
		pbmi=(BITMAPINFO*) new char[sizeof(BITMAPINFOHEADER)+(1<<BPP)*sizeof(RGBQUAD)];
		Palette=(RGBQUAD*)((char*)pbmi+sizeof(BITMAPINFOHEADER));
		bmpfile.read ((char*)Palette,sizeof (RGBQUAD) * (1<<BPP));
	}
	pbmi->bmiHeader=bmih;

	// Load Raster
	bmpfile.seekg (bmfh.bfOffBits,ios::beg);

	Raster = new char[BytesPerRow*Height];

	// (if height is positive the bmp is bottom-up, read it reversed)
	if (bmih.biHeight>0)
		for (int n=Height-1;n>=0;n--)
			bmpfile.read (Raster+BytesPerRow*n,BytesPerRow);
	else
		bmpfile.read (Raster,BytesPerRow*Height);

	// so, we always have a up-bottom raster (that is negative height for windows):
	pbmi->bmiHeader.biHeight=-Height;

	bmpfile.close();

	return 0;
}
