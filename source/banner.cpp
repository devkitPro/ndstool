#include "ndstool.h"
#include "raster.h"

using namespace std;

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
 * InsertTitleString
 */
int InsertTitleString(char *String, FILE *file)
{
	if (String == NULL) return 0;
	int Count=0;
	
	char *token = String;
	while (*token)
	{
		char curr = *token;
		if (curr == ';') curr = 0x0A;	// new line with ';' character
		fputc(curr, file);
		fputc(0x00, file);
		Count += 2;
		token++;
	}

	return Count;
}

/*
 * IconFromBMP
 */
void IconFromBMP()
{
	CRaster bmp;
	int rval = bmp.LoadBMP(icontitlefilename);

	if(rval == 1) {
		printf("Error: Couldn't open icon file\n");
		exit(1);
	}
	if(rval == 2) {
		printf("Error: File is not BMP\n");
		exit(1);
	}

	printf("Size: %i x %i\n", bmp.Width, bmp.Height);
	printf("Bits per pixel: %i\n", bmp.BPP);
	printf("Colors used: %i\n", bmp.pbmi->bmiHeader.biClrUsed);

	if(bmp.Width != 32 || bmp.Height != 32) {
		printf("Error: Image should be 32 x 32\n");
		exit(1);
	}
	if(bmp.BPP != 8) {
		printf("Error: Image should use 8-bit indexed colors\n");
		exit(1);
	}
	header.icon_title_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
	fseek(fNDS, header.icon_title_offset, SEEK_SET);


	// initial 32 bytes (0x00)
	for(int i = 0; i < 32; i++) {
		fputc(0x00, fNDS);
	}

	// tile data (4 bit / tile, 4x4 total tiles)
	// 32 bytes per tile (in 4 bit mode)
	for(int row = 0; row < 4; row++) {
		for(int col = 0; col < 4; col++) {
			for(int y = 0; y < 8; y++) {
				for(int x = 0; x < 4; x++) {	
					unsigned char b0 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + col * 8)];
					unsigned char b1 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + 1 + col * 8)];
					// two nibbles form a byte:
					fputc(((b1 & 0xF) << 4) | (b0 & 0xF), fNDS);
			}	
			}
		}
	}

	// palette
	for(int i = 0; i < 16; i++) {
		unsigned short entry = RGBQuadToRGB16(bmp.Palette[i]);
		fputc(entry & 0xFF, fNDS);
		fputc((entry >> 8) & 0xFF, fNDS);
	}

	// title text (6 languages)
	// first language (English) is filled with the remaining args
	// everything else is filled with 0x00
	int dataCount = 0;

	dataCount += InsertTitleString(icontitletext, fNDS);
	
	// fill the rest of the string with 0x00
	for(int i = 0; i < 256 - dataCount; i++) {
		fputc(0x00, fNDS);
	}
	// fill the other 5 languages with 0x00
	for(int i = 0; i < 5 * 256; i++) {
		fputc(0x00, fNDS);
	}
}
