#include "ndstool.h"
#include "raster.h"

using namespace std;

#pragma pack(1)

struct Banner
{
	unsigned_short version;
	unsigned_short crc;
	unsigned char reserved[28];
	unsigned char tile_data[4][4][8][4];
	unsigned_short palette[16];
	unsigned_short title[6][128];
};

#pragma pack()


#define RGB16(r,g,b)			((r) | (g<<5) | (b<<10))

/*
 * RGBQuadToRGB16
 */
unsigned short RGBQuadToRGB16(RGBQUAD quad)
{
	unsigned short r = quad.rgbRed;
	unsigned short g = quad.rgbGreen;
	unsigned short b = quad.rgbBlue;

	r >>= 3; g >>= 3; b >>= 3;

	return RGB16(r, g, b);
}

/*
 * IconFromBMP
 */
void IconFromBMP()
{
	CRaster bmp;
	int rval = bmp.LoadBMP(bannerfilename);
	if (rval == 1) { printf("Error: Couldn't open icon file\n"); exit(1); }
	if (rval == 2) { printf("Error: File is not BMP\n"); exit(1); }

	if (verbose)
	{
		printf("Size: %i x %i\n", (int)bmp.Width, (int)bmp.Height);
		printf("Bits per pixel: %i\n", (int)bmp.BPP);
		printf("Colors used: %i\n", (int)bmp.pbmi->bmiHeader.biClrUsed);
	}

	if (bmp.Width != 32 || bmp.Height != 32) { fprintf(stderr, "Error: Image should be 32 x 32\n"); exit(1); }
	if (bmp.BPP != 8) { fprintf(stderr, "Error: Image should use 8-bit indexed colors\n"); exit(1); }

	Banner banner;
	memset(&banner, 0, sizeof(banner));
	banner.version = 1;

	// tile data (4 bit / tile, 4x4 total tiles)
	// 32 bytes per tile (in 4 bit mode)
	for (int row=0; row<4; row++)
	{
		for (int col=0; col<4; col++)
		{
			for (int y=0; y<8; y++)
			{
				for (int x=0; x<4; x++)
				{
					unsigned char b0 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + col * 8)];
					unsigned char b1 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + 1 + col * 8)];
					banner.tile_data[row][col][y][x] = ((b1 & 0xF) << 4) | (b0 & 0xF);
				}
			}
		}
	}

	// palette
	for (int i = 0; i < 16; i++)
	{
		banner.palette[i] = RGBQuadToRGB16(bmp.Palette[i]);
	}

	// put title
	for (int i=0; bannertext[i]; i++)
	{
		char c = bannertext[i];
		if (c == ';') c = 0x0A;
		for (int l=0; l<6; l++)
		{
			banner.title[l][i] = c;
		}
	}
	
	// calculate CRC
	banner.crc = CalcCRC((unsigned char *)&banner + 32, 0x840 - 32);

	fwrite(&banner, 1, sizeof(banner), fNDS);
}
