#include <time.h>
#include "ndstool.h"
#include "logo.h"
#include "raster.h"
#include "banner.h"
#include "overlay.h"
#include "loadme.h"
#include "ndstree.h"
#include "elf.h"
#include "sha1.h"

unsigned int arm9_align = 0x1FF;
unsigned int arm7_align = 0x1FF;
unsigned int fnt_align = 0x1FF;		// 0x3 0x1FF
unsigned int fat_align = 0x1FF;		// 0x3 0x1FF
unsigned int banner_align = 0x1FF;
unsigned int file_align = 0x1FF;	// 0x3 0x1FF
unsigned int sector_align = 0x3FF;

unsigned int overlay_files = 0;

unsigned char romcontrol[] = { 0x00,0x60,0x58,0x00,0xF8,0x08,0x18,0x00 };

const unsigned char nintendo_logo[] =
{
	0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0x0A,0x84,0xE4,0x09,0xAD,
	0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x09,0xCE,0x20,
	0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF,
	0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x09,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC,
	0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x03,0x98,0x76,
	0x23,0x1D,0xC7,0x61,0x03,0x04,0xAE,0x56,0xBF,0x38,0x84,0x00,0x40,0xA7,0x0E,0xFD,
	0xFF,0x52,0xFE,0x03,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25,
	0xA9,0x63,0xBE,0x03,0x01,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x03,0x44,
	0x78,0x00,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF,
	0xD6,0x25,0xE4,0x8B,0x38,0x0A,0xAC,0x72,0x21,0xD4,0xF8,0x07,
};

const unsigned char hmac_sha1_key[] =
{
	0x21,0x06,0xC0,0xDE,0xBA,0x98,0xCE,0x3F,0xA6,0x92,0xE3,0x9D,0x46,0xF2,0xED,0x01,
	0x76,0xE3,0xCC,0x08,0x56,0x23,0x63,0xFA,0xCA,0xD4,0xEC,0xDF,0x9A,0x62,0x78,0x34,
	0x8F,0x6D,0x63,0x3C,0xFE,0x22,0xCA,0x92,0x20,0x88,0x97,0x23,0xD2,0xCF,0xAE,0xC2,
	0x32,0x67,0x8D,0xFE,0xCA,0x83,0x64,0x98,0xAC,0xFD,0x3E,0x37,0x87,0x46,0x58,0x24,
};

void Sha1Hmac(u8 output[20], FILE* f, unsigned int pos, unsigned int size)
{
	sha1_ctx cx[1];
	u8 readbuf[4096];
	u8 keypad[0x40];
	for (int i = 0; i < 0x40; i ++) keypad[i] = hmac_sha1_key[i]^0x36;
	sha1_begin(cx);
	sha1_hash(keypad, 0x40, cx);
	unsigned int tmp = ftell(f);
	fseek(f, pos, SEEK_SET);
	while (size)
	{
		unsigned int rdbytes = size > sizeof(readbuf) ? sizeof(readbuf) : size;
		fread(readbuf, 1, rdbytes, f);
		sha1_hash(readbuf, rdbytes, cx);
		size -= rdbytes;
	}
	sha1_end(output, cx);
	for (int i = 0; i < 0x40; i ++) keypad[i] = hmac_sha1_key[i]^0x5c;
	sha1_begin(cx);
	sha1_hash(keypad, 0x40, cx);
	sha1_hash(output, 20, cx);
	sha1_end(output, cx);
	fseek(f, tmp, SEEK_SET);
}

/*
 * HasElfExtension
 */
bool HasElfExtension(char *filename)
{
	char *p = strrchr(filename, '.');
	if (!p) return false;
	return (strcmp(p, ".elf") == 0);
}


bool HasElfHeader(char *filename)
{
	char hdr[4];

	FILE *fi = fopen(filename, "rb");
	if (!fi) return false;
	int bytesread = fread(hdr,1,4,fi);
	fclose(fi);
	if(bytesread<=0) return false;
	if(strncmp(hdr,"\x7f""ELF",4) == 0) return true;
	return false;
}

/*
 * CopyFromBin
 */
int CopyFromBin(char *binFilename, unsigned int *size = 0, unsigned int *size_without_footer = 0)
{
	FILE *fi = fopen(binFilename, "rb");
	if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", binFilename); exit(1); }
	unsigned int _size = 0;
	while (1)
	{
		unsigned char buffer[1024];
		int bytesread = fread(buffer, 1, sizeof(buffer), fi);
		if (bytesread <= 0) break;
		fwrite(buffer, 1, bytesread, fNDS);
		_size += bytesread;
	}
	if (size) *size = _size;

	// check footer
	if (size_without_footer)
	{
		fseek(fi, _size - 3*4, SEEK_SET);
		unsigned_int nitrocode;
		fread(&nitrocode, sizeof(nitrocode), 1, fi);
		if (nitrocode == 0xDEC00621)
			*size_without_footer = _size - 3*4;
		else
			*size_without_footer = _size;
	}

	fclose(fi);
	return 0;
}

/*
 * AddFile
 */
void AddFile(const char *rootdir, const char *prefix, const char *entry_name, unsigned int file_id)
{
	// make filename
	char strbuf[MAXPATHLEN];
	strcpy(strbuf, rootdir);
	strcat(strbuf, prefix);
	strcat(strbuf, entry_name);

	//unsigned int file_end = ftell(fNDS);

	file_top = (file_top + file_align) &~ file_align;
	fseek(fNDS, file_top, SEEK_SET);

	FILE *fi = fopen(strbuf, "rb");
	if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", strbuf); exit(1); }
	fseek(fi, 0, SEEK_END);
	unsigned int size = ftell(fi);
	unsigned int file_bottom = file_top + size;
	fseek(fi, 0, SEEK_SET);

	// print
	if (verbose)
	{
		printf("%5u 0x%08X 0x%08X %9u %s%s\n", file_id, file_top, file_bottom, size, prefix, entry_name);
	}

	// write data
	unsigned int sizeof_copybuf = 256*1024;
	unsigned char *copybuf = new unsigned char [sizeof_copybuf];
	while (size > 0)
	{
		unsigned int size2 = (size >= sizeof_copybuf) ? sizeof_copybuf : size;
		fread(copybuf, 1, size2, fi);
		fwrite(copybuf, 1, size2, fNDS);
		size -= size2;
	}
	delete [] copybuf;
	fclose(fi);
	if ((unsigned int)ftell(fNDS) > file_end) file_end = ftell(fNDS);

	// write fat
	fseek(fNDS, header.fat_offset + 8*file_id, SEEK_SET);
	unsigned_int top = file_top;
	fwrite(&top, 1, sizeof(top), fNDS);
	unsigned_int bottom = file_bottom;
	fwrite(&bottom, 1, sizeof(bottom), fNDS);

	file_top = file_bottom;
}

/*
 * AddDirectory
 * Walks the tree and adds files to NDS
 */
void AddDirectory(TreeNode *node, const char *prefix, unsigned int this_dir_id, unsigned int _parent_id)
{
	// skip dummy node
	node = node->next;

	if (verbose) printf("%s\n", prefix);

	// write directory info
	fseek(fNDS, header.fnt_offset + 8*(this_dir_id & 0xFFF), SEEK_SET);
	unsigned_int entry_start = _entry_start;	// reference location of entry name
	fwrite(&entry_start, 1, sizeof(entry_start), fNDS);
	unsigned int _top_file_id = free_file_id;
	unsigned_short top_file_id = _top_file_id;	// file ID of top entry
	fwrite(&top_file_id, 1, sizeof(top_file_id), fNDS);
	unsigned_short parent_id = _parent_id;	// ID of parent directory or directory count (root)
	fwrite(&parent_id, 1, sizeof(parent_id), fNDS);

	//printf("dir %X file_id %u +\n", this_dir_id, (int)top_file_id);

	// directory entrynames
	{
		// start of directory entrynames
		fseek(fNDS, header.fnt_offset + _entry_start, SEEK_SET);

		// write filenames
		for (TreeNode *t=node; t; t=t->next)
		{
			if (!t->directory)
			{
				int namelen = strlen(t->name);
				fputc(t->directory ? namelen | 128 : namelen, fNDS); _entry_start += 1;
				fwrite(t->name, 1, namelen, fNDS); _entry_start += namelen;

				//printf("[ %s -> %u ]\n", t->name, free_file_id);

				free_file_id++;
			}
		}

		// write directorynames
		for (TreeNode *t=node; t; t=t->next)
		{
			if (t->directory)
			{
				//printf("*entry %s\n", t->name);

				int namelen = strlen(t->name);
				fputc(t->directory ? namelen | 128 : namelen, fNDS); _entry_start += 1;
				fwrite(t->name, 1, namelen, fNDS); _entry_start += namelen;

				//printf("[ %s -> %X ]\n", t->name, t->dir_id);

				unsigned_short _dir_id_tmp = t->dir_id;
				fwrite(&_dir_id_tmp, 1, sizeof(_dir_id_tmp), fNDS);
				_entry_start += sizeof(_dir_id_tmp);
			}
		}

		fputc(0, fNDS); _entry_start += 1;	// end of directory entrynames
	}

	// add files
	unsigned int local_file_id = _top_file_id;
	for (TreeNode *t=node; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (!t->directory)
		{
			AddFile(filerootdir, prefix, t->name, local_file_id++);
		}
	}

	// add subdirectories
	for (TreeNode *t=node; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (t->directory)
		{
			char strbuf[MAXPATHLEN];
			strcpy(strbuf, prefix);
			strcat(strbuf, t->name);
			strcat(strbuf, "/");
			AddDirectory(t->directory, strbuf, t->dir_id, this_dir_id);
		}
	}
}

/*
 * Create
 */
void Create()
{
	fNDS = fopen(ndsfilename, "wb+");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }

	bool bSecureSyscalls = false;
	char *headerfilename = (headerfilename_or_size && (strtoul(headerfilename_or_size,0,0) == 0)) ? headerfilename_or_size : 0;
	u32 headersize = headerfilename_or_size ? strtoul(headerfilename_or_size,0,0) : 0x4000;

	// initial header data
	if (headerfilename)
	{
		// header template
		FILE *fi = fopen(headerfilename, "rb");
		if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", headerfilename); exit(1); }
		fread(&header, 1, 0x200, fi);
		fclose(fi);

		if ((header.arm9_ram_address + 0x800 == header.arm9_entry_address) || (header.rom_header_size > 0x200))
		{
			bSecureSyscalls = true;
		}
	}
	else	// set header default values
	{
		// clear header
		memset(&header, 0, sizeof(header));
		memcpy(header.gamecode, "####", 4);

		if (arm9RamAddress + 0x800 == arm9Entry)
		{
			bSecureSyscalls = true;
		}
		else if (headersize == 0x200)
		{
			header.reserved2 = 0x04;		// autostart
			*(unsigned_int *)(((unsigned char *)&header) + 0x0) = 0xEA00002E;		// for PassMe's that start @ 0x08000000
		} else if (!title)
		{
			memcpy(header.title, "HOMEBREW", 8);
		}
		header.rom_control_info1 = 1<<22 | latency2<<16 | 1<<14 | 1<<13 | latency1;	// ROM control info 1
		header.rom_control_info2 = 1<<29 | latency2<<16 | latency1;	// ROM control info 2
		header.rom_control_info3 = 0x051E;	// ROM control info 3
	}
	if (headersize) header.rom_header_size = headersize;
	if (header.rom_header_size == 0) header.rom_header_size = bSecureSyscalls ? 0x4000 : 0x200;

	// load a logo
	if (logofilename)
	{
		char *p = strrchr(logofilename, '.');
		if (!strcmp(p, ".bmp"))
		{
			CRaster raster;
			if (raster.LoadBMP(logofilename) < 0) exit(1);
			unsigned char white = (raster.palette[0].rgbGreen >= 128) ? 0 : 1;
			if (LogoConvert(raster.raster, header.logo, white) < 0) exit(1);
		}
		else
		{
			FILE *fi = fopen(logofilename, "rb");
			if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", logofilename); exit(1); }
			fread(&header.logo, 1, 156, fi);
			fclose(fi);
		}
	}
	else if (header.rom_header_size > 0x200)	// use Nintendo logo
	{
		memcpy(((unsigned char *)&header) + 0xC0, nintendo_logo, sizeof(nintendo_logo));
	}
	else	// add small NDS loader
	{
		if (loadme_size != 156) { fprintf(stderr, "loadme size error\n"); exit(1); }
		memcpy(header.logo, loadme, loadme_size);		// self-contained NDS loader for *Me GBA cartridge boot
		memcpy(&header.offset_0xA0, "SRAM_V110", 9);		// allow GBA cartridge SRAM backup
		memcpy(&header.offset_0xAC, "PASS01\x96", 7);		// automatically start with FlashMe, make it look more like a GBA rom
	}

	// override default title/game/maker codes
	if (title) strncpy(header.title, title, 12);
	if (gamecode) strncpy(header.gamecode, gamecode, 4);
	if (makercode) strncpy((char *)header.makercode, makercode, 2);

	// --------------------------

	fseek(fNDS, header.rom_header_size, SEEK_SET);

	// ARM9 binary
	bool is_arm9_elf;
	if (arm9filename)
	{
		header.arm9_rom_offset = (ftell(fNDS) + arm9_align) &~ arm9_align;
		fseek(fNDS, header.arm9_rom_offset, SEEK_SET);

		unsigned int entry_address = arm9Entry ? arm9Entry : (unsigned int)header.arm9_entry_address;		// template
		unsigned int ram_address = arm9RamAddress ? arm9RamAddress : (unsigned int)header.arm9_ram_address;		// template
		if (!ram_address && entry_address) ram_address = entry_address;
		if (!entry_address && ram_address) entry_address = ram_address;
		if (!ram_address) { ram_address = entry_address = 0x02000000; }

		// add dummy area for secure syscalls
		header.arm9_size = 0;
		if (bSecureSyscalls)
		{
			unsigned_int x;
			FILE *fARM9 = fopen(arm9filename, "rb");
			if (fARM9)
			{
				fread(&x, sizeof(x), 1, fARM9);
				fclose(fARM9);
				if (x != 0xE7FFDEFF)	// not already exist?
				{
					x = 0xE7FFDEFF;
					for (int i=0; i<0x800/4; i++) fwrite(&x, sizeof(x), 1, fNDS);
					header.arm9_size = 0x800;
				}
			}
		}

		unsigned int size = 0;

		is_arm9_elf = HasElfExtension(arm9filename) || HasElfHeader(arm9filename);
		if (is_arm9_elf)
			CopyFromElf(arm9filename, &entry_address, &ram_address, &size, false);
		else
			CopyFromBin(arm9filename, 0, &size);
		header.arm9_entry_address = entry_address;
		header.arm9_ram_address = ram_address;
		header.arm9_size = header.arm9_size + ((size + 3) &~ 3);
	}
	else
	{
		fprintf(stderr, "ARM9 binary file required.\n");
		exit(1);
	}

	// ARM9 overlay table
	if (arm9ovltablefilename)
	{
		unsigned_int x1 = 0xDEC00621; fwrite(&x1, sizeof(x1), 1, fNDS);		// 0x2106c0de magic
		unsigned_int x2 = 0x00000AD8; fwrite(&x2, sizeof(x2), 1, fNDS);		// ???
		unsigned_int x3 = 0x00000000; fwrite(&x3, sizeof(x3), 1, fNDS);		// ???

		header.arm9_overlay_offset = ftell(fNDS);		// do not align
		fseek(fNDS, header.arm9_overlay_offset, SEEK_SET);
		unsigned int size = 0;
		CopyFromBin(arm9ovltablefilename, &size);
		header.arm9_overlay_size = size;
		overlay_files += size / sizeof(OverlayEntry);
		if (!size) header.arm9_overlay_offset = 0;
	}

	// COULD BE HERE: ARM9 overlay files, no padding before or between. end is padded with 0xFF's and then followed by ARM7 binary
	// fseek(fNDS, 1388772, SEEK_CUR);		// test for ASME

	// ARM7 binary
	header.arm7_rom_offset = (ftell(fNDS) + arm7_align) &~ arm7_align;
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);

	char *devkitProPATH;
	devkitProPATH = getenv("DEVKITPRO");

	#ifdef __WIN32__
	// convert to standard windows path
	if ( devkitProPATH && devkitProPATH[0] == '/' ) {
		devkitProPATH[0] = devkitProPATH[1];
		devkitProPATH[1] = ':';
	}
	#endif

	if ( !arm7filename) {
		char arm7PathName[MAXPATHLEN];

		if (!devkitProPATH) {
			fprintf(stderr,"No arm7 specified and DEVKITPRO missing from environment!\n");
			exit(1);
		}

		strcpy(arm7PathName,devkitProPATH);
		strcat(arm7PathName,"/libnds/default.elf");
		arm7filename = arm7PathName;
	}

	bool is_arm7_elf = HasElfExtension(arm7filename) || HasElfHeader(arm7filename);
	// if (arm7filename)
	{
		unsigned int entry_address = arm7Entry ? arm7Entry : (unsigned int)header.arm7_entry_address;		// template
		unsigned int ram_address = arm7RamAddress ? arm7RamAddress : (unsigned int)header.arm7_ram_address;		// template
		if (!ram_address && entry_address) ram_address = entry_address;
		if (!entry_address && ram_address) entry_address = ram_address;
		if (!ram_address) { ram_address = entry_address = 0x037f8000; }

		unsigned int size = 0;

		if (is_arm7_elf)
			CopyFromElf(arm7filename, &entry_address, &ram_address, &size, false);
		else
			CopyFromBin(arm7filename, &size);

		header.arm7_entry_address = entry_address;
		header.arm7_ram_address = ram_address;
		header.arm7_size = ((size + 3) &~ 3);
	}

	// ARM7 overlay table
	if (arm7ovltablefilename)
	{
		header.arm7_overlay_offset = ftell(fNDS);		// do not align
		fseek(fNDS, header.arm7_overlay_offset, SEEK_SET);
		unsigned int size = 0;
		CopyFromBin(arm7ovltablefilename, &size);
		header.arm7_overlay_size = size;
		overlay_files += size / sizeof(OverlayEntry);
		if (!size) header.arm7_overlay_offset = 0;
	}

	// COULD BE HERE: probably ARM7 overlay files, just like for ARM9
	//

	if (overlay_files && !overlaydir)
	{
		fprintf(stderr, "Overlay directory required!.\n");
		exit(1);
	}

	// filesystem
	//if (filerootdir || overlaydir)
	{
		// read directory structure
		free_file_id = overlay_files;
		free_dir_id++;
		directory_count++;
		TreeNode *filetree;
		if (filerootdir)
			filetree = ReadDirectory(new TreeNode(), filerootdir);
		else
			filetree = new TreeNode();		// dummy root node 0xF000

		// calculate offsets required for FNT and FAT
		_entry_start = 8*directory_count;		// names come after directory structs
		header.fnt_offset = (ftell(fNDS) + fnt_align) &~ fnt_align;
		header.fnt_size =
			_entry_start +		// directory structs
			total_name_size +	// total number of name characters for dirs and files
			directory_count*4 +	// directory: name length (1), dir id (2), end-character (1)
			file_count*1 +		// files: name length (1)
			- 3;				// root directory only has an end-character
		file_count += overlay_files;		// didn't take overlay files into FNT size, but have to be calculated into FAT size
		header.fat_offset = (header.fnt_offset + header.fnt_size + fat_align) &~ fat_align;
		header.fat_size = file_count * 8;		// each entry contains top & bottom offset

		// banner after FNT/FAT
		if (bannerfilename)
		{
			header.banner_offset = (header.fat_offset + header.fat_size + banner_align) &~ banner_align;
			file_top = header.banner_offset + 0x840;
			fseek(fNDS, header.banner_offset, SEEK_SET);
			if (bannertype == BANNER_IMAGE)
			{
				char * Ext = strrchr(bannerfilename, '.');
				if (Ext && strcasecmp(Ext, ".bmp") == 0)
					IconFromBMP();
				else if (Ext && strcasecmp(Ext, ".grf") == 0)
					IconFromGRF();
				else
				{
					fprintf(stderr,
						"Banner File Error: Unknown extension '%s'!\n", Ext);
					exit(1);
				}
			}
			else
			{
				CopyFromBin(bannerfilename, 0);
			}
		}
		else
		{
			file_top = header.fat_offset + header.fat_size;
			header.banner_offset = 0;
		}

		file_end = file_top;	// no file data as yet

		// add (hidden) overlay files
		for (unsigned int i=0; i<overlay_files; i++)
		{
			char s[32]; sprintf(s, OVERLAY_FMT, i/*free_file_id*/);
			AddFile(overlaydir, "/", s, i/*free_file_id*/);
			//free_file_id++;		// incremented up to overlay_files
		}

		// add all other (visible) files
		AddDirectory(filetree, "/", 0xF000, directory_count);
		fseek(fNDS, file_end, SEEK_SET);

		if (verbose)
		{
			printf("%u directories.\n", directory_count);
			printf("%u normal files.\n", file_count - overlay_files);
			printf("%u overlay files.\n", overlay_files);
		}
	}

	// --------------------------

	// align file size
	unsigned int newfilesize = file_end;	//ftell(fNDS);
	newfilesize = (newfilesize + 3) & ~3;	// align to 4 bytes
	header.application_end_offset = newfilesize;
	if (newfilesize != file_end ) {
		fseek(fNDS, newfilesize-1, SEEK_SET);
		fputc(0, fNDS);
	}

	// DSi ARM9 binary
	if (header.rom_header_size > 0x200 && is_arm9_elf)
	{
		header.dsi9_rom_offset = (ftell(fNDS) + sector_align) &~ sector_align;
		fseek(fNDS, header.dsi9_rom_offset, SEEK_SET);

		unsigned int ram_address = 0;
		unsigned int size = 0;
		CopyFromElf(arm9filename, NULL, &ram_address, &size, true);
		if (!size)
		{
			ram_address = 0x2400000;
			size = 0x200;
			fwrite("----DSi9----", 1, 12, fNDS);
			fseek(fNDS, header.dsi9_rom_offset+size-1, SEEK_SET);
			fputc(0, fNDS);
		}
		header.dsi9_ram_address = ram_address;
		header.dsi9_size = ((size + 3) &~ 3);
	}

	// DSi ARM7 binary
	if (header.rom_header_size > 0x200 && is_arm7_elf)
	{
		header.dsi7_rom_offset = (ftell(fNDS) + arm7_align) &~ arm7_align;
		fseek(fNDS, header.dsi7_rom_offset, SEEK_SET);

		unsigned int ram_address = 0;
		unsigned int size = 0;
		CopyFromElf(arm7filename, NULL, &ram_address, &size, true);
		if (!size)
		{
			ram_address = 0x2E80000;
			size = 0x200;
			fwrite("----DSi7----", 1, 12, fNDS);
			fseek(fNDS, header.dsi7_rom_offset+size-1, SEEK_SET);
			fputc(0, fNDS);
		}
		header.dsi7_ram_address = ram_address;
		header.dsi7_size = ((size + 3) &~ 3);
	}

	if (header.dsi9_size || header.dsi7_size)
	{
		// This is a DSi application!
		header.unitcode = 2;
		header.dsi_flags = 0x3;
		header.rom_control_info1 = 0x00586000;
		header.rom_control_info2 = 0x001808F8;
		header.rom_control_info3 = 0x051E;
		header.offset_0x88 = 0x0004D0B8;
		header.offset_0x8C = 0x00000544;
		header.offset_0x90 = 0x00160016;

		static const u8 global_mbk[5][4] =
		{
			{0x81, 0x85, 0x89, 0x8D},
			{0x80, 0x84, 0x88, 0x8C},
			{0x90, 0x94, 0x98, 0x9C},
			{0x80, 0x84, 0x88, 0x8C},
			{0x90, 0x94, 0x98, 0x9C},
		};

		memcpy(header.global_mbk_setting, global_mbk, sizeof(header.global_mbk_setting));
		header.arm9_mbk_setting[0] = 0x00000000;
		header.arm9_mbk_setting[1] = 0x07C03740;
		header.arm9_mbk_setting[2] = 0x07403700;
		header.arm7_mbk_setting[0] = 0x00403000;
		header.arm7_mbk_setting[1] = 0x07C03740;
		header.arm7_mbk_setting[2] = 0x07403700;
		header.mbk9_wramcnt_setting = (0x03<<24) | 0x00000F;

		header.region_flags = 0xFFFFFFFF;
		header.access_control = 0x00000138;
		header.scfg_ext_mask = scfgExtMask;
		header.appflags = 1;
		header.banner_size = 2112;
		header.offset_0x20C = 0x00010000;
		header.offset_0x218 = 0x0004D084;
		header.offset_0x21C = 0x0000052C;
		header.tid_low  = header.gamecode[3] | (header.gamecode[2]<<8) | (header.gamecode[1]<<16) | (header.gamecode[0]<<24);
		header.tid_high = titleidHigh;
		memset(header.age_ratings, 0x80, sizeof(header.age_ratings));

		Sha1Hmac(header.hmac_arm9, fNDS, header.arm9_rom_offset, header.arm9_size);
		Sha1Hmac(header.hmac_arm7, fNDS, header.arm7_rom_offset, header.arm7_size);
		Sha1Hmac(header.hmac_icon_title, fNDS, header.banner_offset, header.banner_size);
		Sha1Hmac(header.hmac_arm9i, fNDS, header.dsi9_rom_offset, header.dsi9_size);
		Sha1Hmac(header.hmac_arm7i, fNDS, header.dsi7_rom_offset, header.dsi7_size);
		memset(header.rsa_signature, 0xFF, 0x80);

		newfilesize = (ftell(fNDS) + file_align) & ~file_align;
		header.total_rom_size = newfilesize;

		if (newfilesize != ftell(fNDS) ) {
			fseek(fNDS, newfilesize-1, SEEK_SET);
			fputc(0, fNDS);
		}
	}

	// calculate device capacity
	newfilesize |= newfilesize >> 16; newfilesize |= newfilesize >> 8;
	newfilesize |= newfilesize >> 4; newfilesize |= newfilesize >> 2;
	newfilesize |= newfilesize >> 1; newfilesize++;
	if (newfilesize <= 128*1024) newfilesize = 128*1024;
	int devcap = -18;
	unsigned int x = newfilesize;
	while (x != 0) { x >>= 1; devcap++; }
	header.devicecap = (devcap < 0) ? 0 : devcap;

	// fix up header CRCs and write header
	header.logo_crc = CalcLogoCRC(header);
	header.header_crc = CalcHeaderCRC(header);

	if (header.unitcode & 2)
	{
		// Dummy signature for no$gba
		header.rsa_signature[0x00] = 0;
		header.rsa_signature[0x01] = 1;
		header.rsa_signature[0x6B] = 0;
		sha1(&header.rsa_signature[0x6C], (const unsigned char*)&header, 0xE00);
	}

	fseek(fNDS, 0, SEEK_SET);
	fwrite(&header, (header.unitcode&2) ? 0x1000 : 0x200, 1, fNDS);

	fclose(fNDS);
}
