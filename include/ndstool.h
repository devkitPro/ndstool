#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <libelf.h>

#pragma pack(1)

#define VER			"1.09"

#ifndef __MINGW32__
#define MAX_PATH	2048

#else
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412

#define __BYTE_ORDER	__LITTLE_ENDIAN

#endif

typedef	unsigned char			u8;
typedef	unsigned short int		u16;
typedef	unsigned int			u32;


struct unsigned_int
{
	unsigned int i;
	#if __BYTE_ORDER == __BIG_ENDIAN
	#error "big endian!!!"
		operator unsigned int () { return i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
		unsigned int & operator = (unsigned int i) { return this->i = i<<24 | i<<8&0xFF0000 | i>>8&0xFF00 | i>>24; }
	#else
		operator unsigned int () { return i; }
		unsigned int & operator = (unsigned int i) { return this->i = i; }
	#endif
	unsigned_int() {}
	unsigned_int(unsigned int i) { *this = i; }
};


struct unsigned_short
{
	unsigned short i;
	#if __BYTE_ORDER == __BIG_ENDIAN
		operator unsigned short () { return i>>8 | i<<8; }
		unsigned short & operator = (unsigned short i) { return this->i = i>>8 | i<<8; }
	#else
		operator unsigned short () { return i; }
		unsigned short & operator = (unsigned short i) { return this->i = i; }
	#endif
	unsigned_short() {}
	unsigned_short(unsigned short i) { *this = i; }
};

struct Header
{
	char title[0xC];
	char gamecode[0x4];
	unsigned char makercode[2];
	unsigned char unitcode;
	unsigned char devicetype;		// type of device in the game card
	unsigned char devicecap;		// capacity
	unsigned char reserved1[0x9];
	unsigned char romversion;
	unsigned char reserved2;
	unsigned_int arm9_rom_offset;
	unsigned_int arm9_entry_address;
	unsigned_int arm9_ram_address;
	unsigned_int arm9_size;
	unsigned_int arm7_rom_offset;
	unsigned_int arm7_entry_address;
	unsigned_int arm7_ram_address;
	unsigned_int arm7_size;
	unsigned_int fnt_offset;
	unsigned_int fnt_size;
	unsigned_int fat_offset;
	unsigned_int fat_size;
	unsigned_int arm9_overlay_offset;
	unsigned_int arm9_overlay_size;
	unsigned_int arm7_overlay_offset;
	unsigned_int arm7_overlay_size;
	unsigned char rom_control_info1[8];
	unsigned_int icon_title_offset;
	unsigned_short secure_area_crc;
	unsigned_short rom_control_info2;
	unsigned_int offset_0x70;
	unsigned_int offset_0x74;
	unsigned_int offset_0x78;
	unsigned_int offset_0x7C;
	unsigned_int application_end_offset;			// rom size
	unsigned_int rom_header_size;
	unsigned_int offset_0x88;
	unsigned_int offset_0x8C;
	unsigned_int offset_0x90;
	unsigned_int offset_0x94;
	unsigned_int offset_0x98;
	unsigned_int offset_0x9C;
	unsigned_int offset_0xA0;
	unsigned_int offset_0xA4;
	unsigned_int offset_0xA8;
	unsigned_int offset_0xAC;
	unsigned_int offset_0xB0;
	unsigned_int offset_0xB4;
	unsigned_int offset_0xB8;
	unsigned_int offset_0xBC;
	unsigned char logo[156];
	unsigned_short logo_crc;
	unsigned_short header_crc;
	unsigned_int offset_0x160;
	unsigned_int offset_0x164;
	unsigned_int offset_0x168;
	unsigned_int offset_0x16C;
	unsigned char zero[0x90];
};

struct Tree
{
	unsigned int dir_id;	// directory IDs are allocated first
	char *name;
	Tree *directory;		// nonzero indicates directory. first entry in directory is dummy.
	Tree *next;
	
	Tree()
	{
		dir_id = 0;
		name = 0;
		directory = 0;
		next = 0;
	}
};

struct Country
{
	char countrycode;
	char *name;
};

struct Maker
{
	char *makercode;
	char *name;
};

#pragma pack()

enum { BINARY, IMAGE };

extern unsigned int free_dir_id;
extern unsigned int directory_count;
extern unsigned int file_count;
extern unsigned int total_name_size;

extern unsigned int free_file_id;
extern unsigned int _entry_start;
extern unsigned int file_top;

extern bool verbose;
extern Header header;
extern FILE *fNDS;
extern char *ndsfilename;
extern char *arm7filename;
extern char *arm9filename;
extern char *filerootdir;
extern char *icontitlefilename;
extern char *icontitlename;
extern char *icontitletext;
extern int icontype;
extern char *headerfilename;

extern const unsigned char logo[];

extern Country countries[];
extern int NumCountries;

extern Maker makers[];
extern int NumMakers;


void WalkTree(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id);
void ReadDirectory(Tree *tree, char *path);


unsigned short CalcHeaderCRC();
unsigned short CalcLogoCRC();
unsigned short CalcSecureAreaCRC();
void FixHeaderCRC(char *ndsfilename);
void Create();

void ExtractFile(char *filename, unsigned int file_id);
void ExtractDirectory(char *prefix, unsigned int dir_id);
void ExtractFiles();
void Extract(char *outfilename, bool indirect_offset, unsigned int offset, bool indirect_size, unsigned size);








