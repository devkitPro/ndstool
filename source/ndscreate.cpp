#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <libelf.h>


#include "ndstool.h"
#include "raster.h"

extern "C" {

#include "default_arm7.h"

}

/*
 * Variables
 */
bool verbose = false;
Header header;
FILE *fNDS = 0;
char *ndsfilename = 0;
char *arm7filename = 0;
char *arm9filename = 0;
char *filerootdir = 0;
char *icontitlefilename = 0;
char *icontitletext = 0;
char *headerfilename = 0;
int icontype;

const unsigned short crc16tab[] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

const unsigned char logo[] =
{
	0xDB,0x00,0x51,0xAE,0x96,0x65,0x5D,0xDE,0xC2,0x7B,0x7D,0xF5,0x7B,
	0x1B,0xF6,0x52,0xEE,0xDB,0x74,0x67,0x3F,0x7E,0x80,0xDE,0x5C,0xAD,
	0x41,0xE6,0x6C,0xF6,0x31,0xDF,0xEF,0xB9,0xB5,0xB5,0x07,0xD8,0xCE,
	0x13,0xA7,0x38,0x17,0xCC,0x7D,0x1C,0x31,0x40,0x7A,0x0B,0x20,0x6B,
	0x31,0xB4,0xF6,0x3E,0x6B,0xA9,0x75,0x3F,0xEC,0x8D,0x58,0x03,0x60,
	0x7B,0xB2,0x8C,0x5C,0x35,0x65,0x9E,0xA7,0x68,0x5C,0xD8,0x03,0xFC,
	0x67,0x89,0xDC,0xE2,0x38,0x9E,0xFC,0xFB,0x51,0xA9,0x40,0xC7,0x7B,
	0xFF,0xBF,0x58,0xF1,0x02,0x00,0xAD,0x01,0xFC,0x90,0x6A,0xCF,0x0E,
	0x68,0x04,0x3F,0x7A,0x9F,0x29,0x7F,0xDA,0x56,0x9C,0x41,0xFC,0xFE,
	0xB1,0xC7,0x1D,0x06,0x5D,0xCB,0x00,0x44,0xC1,0xFC,0xBB,0x87,0xFF,
	0x6F,0x34,0x77,0xEE,0xC5,0x6B,0x9A,0x3F,0x83,0x9C,0x78,0x0F,0xC3,
	0x50,0x29,0xDA,0x1B,0x74,0xC7,0xF5,0x53,0x8D,0xDE,0x2B,0x07,0xF8,	
};

/*
 * HasElfExtension
 */
bool HasElfExtension(char *filename)
{
	char *p = strrchr(filename, '.');
	if (!p) return false;
	return (strcmp(p, ".elf") == 0);
}
/*
 * CopyFromFile
 */
unsigned int CopyFromFile(FILE *fo, char *filename)
{
	FILE *fi = fopen(filename, "rb");
	if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", filename); exit(1); }
	
	unsigned char copybuf[1024];
	unsigned int totalread = 0;
	while (1)
	{
		unsigned int read = fread(copybuf, 1, sizeof(copybuf), fi);
		totalread += read;
		fwrite(copybuf, 1, read, fo);
		if (!read) break;
	}

	fclose(fi);
	return totalread;
}

/*
 * CopyFromBin
 */
int CopyFromBin(char *binFilename, unsigned int *size = 0)
{
	FILE *fi = fopen(binFilename, "rb");
	if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", binFilename); exit(1); }
	if (size) *size = 0;
	while (1)
	{
		unsigned char buffer[1024];
		int bytesread = fread(buffer, 1, sizeof(buffer), fi);
		if (bytesread <= 0) break;
		fwrite(buffer, 1, bytesread, fNDS);
		if (size) *size += bytesread;
	}
	fclose(fi);
	return 0;
}

/*
 * CopyFromElf
 */
int CopyFromElf(char *elfFilename, unsigned int *entry, unsigned int *ram_address, unsigned int *size)
{
	int fd = open(elfFilename, O_RDONLY);
	if (fd < 0) { fprintf(stderr, "Cannot open file '%s'.\n", elfFilename); exit(1); }
	if (elf_version(EV_CURRENT) == EV_NONE) { fprintf(stderr, "libelf out of date!\n"); exit(1); }
	Elf *elf;
	if ((elf = elf_begin(fd, ELF_C_READ, 0)) == 0) { fprintf(stderr, "Cannot open ELF file!\n"); exit(1); }
	Elf32_Ehdr *ehdr;
	if ((ehdr = elf32_getehdr(elf)) == 0) { fprintf(stderr, "Cannot read ELF header!\n"); exit(1); }
	if (ehdr->e_machine != EM_ARM) { fprintf(stderr, "Not an ARM ELF file!\n"); exit(1); }
	
	*entry = ehdr->e_entry;
	*size = 0;
	*ram_address = 0;
//	printf("entry = 0x%X\n", ehdr->e_entry);

    Elf_Scn *scn = elf_getscn(elf, 0);
	Elf32_Shdr *shdr = elf32_getshdr(scn);
    while (shdr)
    {
		if (shdr->sh_flags & SHF_ALLOC)
		{
/*			char *name;
			if (!(name = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name))) name = "???";
	    	printf("%s\n", name);*/

			if (!*ram_address) *ram_address = shdr->sh_addr;		// use first address (assume it's .text)

// don't mind the garbage here

//			printf("sh_addr=0x%X sh_offset=0x%X sh_size=0x%X sh_link=%u sh_entsize=%u sh_addralign=%u\n", shdr->sh_addr, shdr->sh_offset, shdr->sh_size, shdr->sh_link, shdr->sh_entsize, shdr->sh_addralign);
			//    Elf32_Word		sh_name;
			//    Elf32_Word		sh_type;
			//    Elf32_Word		sh_flags;
			//    Elf32_Addr		sh_addr;
			//    Elf32_Off		sh_offset;
			//    Elf32_Word		sh_size;
			//    Elf32_Word		sh_link;
			//    Elf32_Word		sh_info;
			//    Elf32_Word		sh_addralign;
			//    Elf32_Word		sh_entsize;

			Elf_Data *data;
			if ((data = elf_getdata(scn, NULL)))
			{
		    	/*for (int i=0; i<data->d_size; i++)
		    	{
					printf("%02X ", ((unsigned char *)data->d_buf)[i]);
		    	}
		    	printf("\n");*/
		    	fwrite(data->d_buf, 1, data->d_size, fNDS);
				*size += data->d_size;
			}
		}

		scn = elf_nextscn(elf, scn);
		shdr = elf32_getshdr(scn);
    }
 
	elf_end(elf);

	return 0;
}



/*
 * CalcHeaderCRC
 */
unsigned short CalcHeaderCRC()
{
	unsigned short crc = 0xFFFF;
	for (int i=0; i<0x15E; i++)
	{
		unsigned char data = *((unsigned char *)&header + i);
		crc = (crc >> 8) ^ crc16tab[(crc ^ data) & 0xFF];
	}
	return crc;
}

/*
 * CalcLogoCRC
 */
unsigned short CalcLogoCRC()
{
	unsigned short crc = 0xFFFF;
	for (int i=0xC0; i<0xC0+156; i++)
	{
		unsigned char data = *((unsigned char *)&header + i);
		crc = (crc >> 8) ^ crc16tab[(crc ^ data) & 0xFF];
	}
	return crc;
}

/*
 * CalcSecureAreaCRC
 */
unsigned short CalcSecureAreaCRC()
{
	fseek(fNDS, 0x4000, SEEK_SET);
	unsigned short crc = 0xFFFF;
	for (int i=0x0; i<0x4000; i++)
	{
		crc = (crc >> 8) ^ crc16tab[(crc ^ fgetc(fNDS)) & 0xFF];
	}
	return crc;
}

/*
 * FixHeaderCRC
 */
void FixHeaderCRC(char *ndsfilename)
{
	fNDS = fopen(ndsfilename, "r+b");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	fread(&header, 512, 1, fNDS);
	header.header_crc = CalcHeaderCRC();
	fseek(fNDS, 0, SEEK_SET);
	fwrite(&header, 512, 1, fNDS);
	fclose(fNDS);
}


typedef	unsigned char			u8;
typedef	unsigned short int		u16;
typedef	unsigned int			u32;

#define RGB16(r,g,b)			((r)+(g<<5)+(b<<10))

using namespace std;

u16 RGBQuadToRGB16(RGBQUAD quad) {
	u16 r = quad.rgbRed;
	u16 g = quad.rgbGreen;
	u16 b = quad.rgbBlue;

	r >>= 3; g >>= 3; b >>= 3;

	return RGB16(r, g, b);
}

int InsertTitleString(char *String, FILE* file)
{
	if ( String == NULL) return 0;
	int Count=0;
	
	char *token = String;
	
	while (*token != 0) {
		char curr = *token;
		if(curr == ';')	// new line with ';' character
			curr = 0x0A;

		fputc(curr, file);
		fputc(0x00, file);
		Count += 2;
		token++;
	}

	return Count;
}

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
					u8 b0 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + col * 8)];
					u8 b1 = bmp.Raster[row * 64 * 4 + y * 8 * 4 + (x * 2 + 1 + col * 8)];
					// two nibbles form a byte:
					fputc(((b1 & 0xF) << 4) | (b0 & 0xF), fNDS);
			}	
			}
		}
	}

	// palette
	for(int i = 0; i < 16; i++) {
		u16 entry = RGBQuadToRGB16(bmp.Palette[i]);
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

/*
 * AddFile
 */
void AddFile(char *filename)
{
}

/*
 * Create
 */
void Create()
{
	fNDS = fopen(ndsfilename, "wb");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	
	// header
	if (headerfilename)
	{
		FILE *fi = fopen(headerfilename, "rb");
		if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", headerfilename); exit(1); }
		fread(&header, 1, 0x200, fi);
		fclose(fi);
	}

	// skip header
	unsigned int header_size = header.rom_header_size; if (!header_size) header_size = 0x200;
	fseek(fNDS, header_size, SEEK_SET);

	// ARM9 binary
	if (arm9filename)
	{
		header.arm9_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
		//header.arm9_rom_offset = header.arm9_rom_offset;		// ******************
		fseek(fNDS, header.arm9_rom_offset, SEEK_SET);
		unsigned int entry_address = header.arm9_entry_address; if (!entry_address) entry_address = 0x02004000;
		unsigned int ram_address = header.arm9_ram_address; if (!ram_address) ram_address = 0x02004000;
		unsigned int size = 0;
		if (HasElfExtension(arm9filename))
			CopyFromElf(arm9filename, &entry_address, &ram_address, &size);
		else
			CopyFromBin(arm9filename, &size);
		header.arm9_entry_address = entry_address;
		header.arm9_ram_address = ram_address;
		header.arm9_size = size;
	}
	else
	{
		header.arm9_entry_address = 0;
		header.arm9_ram_address = 0;
		header.arm9_size = 0;
	}

	header.arm7_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
	//header.arm7_rom_offset = header.arm7_rom_offset;		// ******************
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);

	unsigned int entry_address = header.arm7_entry_address; if (!entry_address) entry_address = 0x03800000;
	unsigned int ram_address = header.arm7_ram_address; if (!ram_address) ram_address = 0x03800000;
	unsigned int size = 0;

	// ARM7 binary
	if (arm7filename)
	{
		if (HasElfExtension(arm7filename))
			CopyFromElf(arm7filename, &entry_address, &ram_address, &size);
		else
			CopyFromBin(arm7filename, &size);
	}
	else
	{
		fwrite(default_arm7,(u32)default_arm7_size,1,fNDS);
		size = default_arm7_size;
	}

	header.arm7_entry_address = entry_address;
	header.arm7_ram_address = ram_address;
	header.arm7_size = size;
	
	// icon/title
	if (icontitlefilename)
	{
		if ( icontype == IMAGE ) {
			IconFromBMP();
		} else {
			header.icon_title_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
			fseek(fNDS, header.icon_title_offset, SEEK_SET);
			CopyFromBin(icontitlefilename, 0);
		}
		

	}
	else
	{
		header.icon_title_offset = 0;
	}
	
	// filesystem
	if (filerootdir)
	{
		//struct dirent **eps;
		//int n = scandir("./", &eps, one, alphasort);
		//printf("%d\n", n);
		
		
		//ExtractDirectory(filerootdir, "/", 0xF000);
		//ReadDirectory(filerootdir, "/", 0/*first is not parent_id*/, 1);
		Tree *root = new Tree();
		ReadDirectory(root, filerootdir);

		_entry_start = 8*directory_count;
		header.fnt_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;		// align to 512 bytes
		header.fnt_size = _entry_start + ((file_count+directory_count)*1/*length*/ + total_name_size/*names of both dirs and files*/ + directory_count*1/*end of directory*/);
		header.fat_offset = (header.fnt_offset + header.fnt_size + 0x1FF) &~ 0x1FF;		// align to 512 bytes;
		header.fat_size = file_count*8;
		file_top = header.fat_offset + header.fat_size;

		//DebugTree(root);
		WalkTree(root, "/", 0xF000, directory_count);

//		printf("fnt_offset %X\n", (int)header.fnt_offset);
//		printf("fnt_size %X\n", (int)header.fnt_size);
//		printf("fat_offset %X\n", (int)header.fat_offset);
//		printf("fat_size %X\n", (int)header.fat_size);

		if (verbose)
		{
			printf("%d directories.\n", directory_count);
			printf("%d files.\n", file_count);
			//printf("%d total_name_size\n", total_name_size);
		}
	}
	else
	{
		header.fnt_offset = 0;
		header.fnt_size = 0;
		header.fat_offset = 0;
		header.fat_size = 0;
	}

	// header
	header.secure_area_crc = CalcSecureAreaCRC();
	header.logo_crc = CalcLogoCRC();
	header.header_crc = CalcHeaderCRC();
	fseek(fNDS, 0, SEEK_SET);
	fwrite(&header, 512, 1, fNDS);
	
	fclose(fNDS);
}
