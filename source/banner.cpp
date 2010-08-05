#include "ndstool.h"
#include "raster.h"
#include "banner.h"
#include "crc.h"

const char *bannerLanguages[] = { "Japanese", "English", "French", "German", "Italian", "Spanish" };

#define RGB16(r,g,b)			((r) | (g<<5) | (b<<10))

/*
 * RGBQuadToRGB16
 */
inline unsigned short RGBQuadToRGB16(RGBQUAD quad)
{
	unsigned short r = quad.rgbRed;
	unsigned short g = quad.rgbGreen;
	unsigned short b = quad.rgbBlue;
	return RGB16(r>>3, g>>3, b>>3);
}

/*
 * CalcBannerCRC
 */
unsigned short CalcBannerCRC(Banner &banner)
{
	return CalcCrc16((unsigned char *)&banner + 32, 0x840 - 32);
}

/*
 * IconFromBMP
 */
void IconFromBMP()
{
	CRaster bmp;
	int rval = bmp.LoadBMP(bannerfilename);
	if (rval < 0) exit(1);

	if (bmp.width != 32 || bmp.height != 32) {
		fprintf(stderr, "Image should be 32 x 32.\n");
		exit(1);
	}

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
				for (int x=0; x<8; x+=2)
				{
					unsigned char b0 = bmp[row*8 + y][col*8 + x + 0];
					unsigned char b1 = bmp[row*8 + y][col*8 + x + 1];
					banner.tile_data[row][col][y][x/2] = (b1 << 4) | b0;
				}
			}
		}
	}

	// palette
	for (int i = 0; i < 16; i++)
	{
		banner.palette[i] = RGBQuadToRGB16(bmp.palette[i]);
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
	banner.crc = CalcBannerCRC(banner);

	fwrite(&banner, 1, sizeof(banner), fNDS);
}

/*
 * IconFromGRF
 * 
 * Assumes Input File to be a 32x32 pixel 4bpp tiled image
 * grit command line:
 *      grit icon.png -g -gt -gB4 -gT <color> -m! -p -pe 16 -fh! -ftr
 */

typedef struct
{
	unsigned char GfxAttr;
	unsigned char MapAttr;
	unsigned char MMapAttr;
	unsigned char PalAttr;
	
	unsigned char TileWidth;
	unsigned char TileHeight;
	unsigned char MetaTileWidth;
	unsigned char MetaTileHeight;
	
	unsigned int  GfxWidth;
	unsigned int  GfxHeight;
}
GRF_HEADER;

void IconFromGRF()
{
	FILE     * GrfFile;
	unsigned * GrfData;
	unsigned * GrfPtr;
	unsigned   GrfSize;
	
	GRF_HEADER * GrfHeader;
	unsigned   * GfxData;
	unsigned   * PalData;
	
	Banner banner;
	
	// Open File and Read to Memory
	GrfFile = fopen(bannerfilename, "rb");
	if (!GrfFile)
	{
		perror("Cannot open Banner File");
		exit(1);
	}
	
	fseek(GrfFile, 0, SEEK_END);
	GrfSize = ftell(GrfFile);
	fseek(GrfFile, 0, SEEK_SET);
	
	GrfData = (unsigned *)malloc(GrfSize);
	if (!GrfData)
	{
		fclose(GrfFile);
		fprintf(stderr, "Cannot read Banner File: Out of Memory\n");
		exit(1);
	}
	
	fread(GrfData, 1, GrfSize, GrfFile);
	if (ferror(GrfFile))
	{
		perror("Cannot read Banner File");
		fclose(GrfFile);
		exit(1);
	}
	
	fclose(GrfFile);
	
	// Parse RIFF File Structure : Check File Format
	GrfHeader = NULL;
	GfxData   = NULL;
	PalData   = NULL;
	
	if (
		(memcmp(&GrfData[0], "RIFF", 4) != 0) ||
		(GrfData[1] != GrfSize-8) ||
		(memcmp(&GrfData[2], "GRF ", 4) != 0)
		)
	{
		fprintf(stderr, "Banner File Error: File is no GRF File!\n");
		goto error;
	}
	
	GrfPtr = GrfData + 3;
	
	// Parse RIFF File Structure : Read Chunks
	while ((unsigned)GrfPtr - (unsigned)GrfData < GrfSize)
	{
		if (memcmp(&GrfPtr[0], "HDR ", 4) == 0)
		{
			GrfHeader =  (GRF_HEADER *)&GrfPtr[2];
		}
		else if (memcmp(&GrfPtr[0], "GFX ", 4) == 0)
		{
			GfxData = &GrfPtr[2];
		}
		else if (memcmp(&GrfPtr[0], "PAL ", 4) == 0)
		{
			PalData = &GrfPtr[2];
		}
		
		GrfPtr += (GrfPtr[1]+8)/4;
	}
	
	// Check Chunks
	if (!GrfHeader || !GfxData || !PalData)
	{
		fprintf(stderr, "Banner File Error: GRF File is incomplete!\n");
		goto error;
	} 
	
	// Check Header
	// Note: Error checking is probably incomplete
	if (GrfHeader->GfxWidth != 32 && GrfHeader->GfxHeight != 32)
	{
		fprintf(stderr, "Banner File Error: Image must be 32x32pixels!\n");
		goto error;
	}
	if (GrfHeader->GfxAttr != 4)
	{
		fprintf(stderr, "Banner File Error: Image must have 16 colors!\n");
		goto error;
	}
	if (GrfHeader->TileWidth != 8 && GrfHeader->TileHeight != 8)
	{
		fprintf(stderr, 
			"Banner File Error: Image must consist of 8x8 pixel tiles!\n");
		goto error;
	}
	
	// Check Compression
	if (
		((GfxData[0] & 0xF0) != 0x00) ||
		((PalData[0] & 0xF0) != 0x00)
		)
	{
		fprintf(stderr, 
			"Banner File Error: Image must be uncompressed!\n");
		goto error;
	}
	
	// Finally build Banner (Same as IconFromBMP)
	memset(&banner, 0, sizeof(banner));
	banner.version = 1;
	
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
	
	// put Gfx Data
	memcpy(banner.tile_data, &GfxData[1], 32*16);
	
	// put Pal Data
	memcpy(banner.palette, &PalData[1], 16*2);
	
	// calculate CRC
	banner.crc = CalcBannerCRC(banner);
	
	// write to file
	fwrite(&banner, 1, sizeof(banner), fNDS);
	
	// free Memory
	free(GrfData);
	return;
	
	
error:
	free(GrfData);
	exit(1);
}