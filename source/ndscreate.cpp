#include <time.h>
#include <ndstool.h>
#include <ndstool_version.h>
#include "default_arm7.h"
#include "logo.h"
#include "raster.h"
#include "banner.h"
#include "overlay.h"
#include "loadme.h"
#include "ndstree.h"

unsigned int overlay_files = 0;
const char CompileDate[] = __DATE__;
const char CompileTime[] = __TIME__;


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
 * AddFile
 */
void AddFile(char *rootdir, char *prefix, char *entry_name, unsigned int file_id, unsigned int alignmask)
{
	// make filename
	char strbuf[MAXPATHLEN];
	strcpy(strbuf, rootdir);
	strcat(strbuf, prefix);
	strcat(strbuf, entry_name);

	//unsigned int file_end = ftell(fNDS);

	file_top = (file_top + alignmask) &~ alignmask;		// align to alignmask+1 bytes
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
void AddDirectory(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id, unsigned int alignmask)
{
	// skip dummy node
	tree = tree->next;

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
		for (Tree *t=tree; t; t=t->next)
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
		for (Tree *t=tree; t; t=t->next)
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
	unsigned int file_id = _top_file_id;
	for (Tree *t=tree; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (!t->directory)
		{
			AddFile(filerootdir, prefix, t->name, file_id++, alignmask);
		}
	}

	// add subdirectories
	for (Tree *t=tree; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (t->directory)
		{
			char strbuf[MAXPATHLEN];
			strcpy(strbuf, prefix);
			strcat(strbuf, t->name);
			strcat(strbuf, "/");
			AddDirectory(t->directory, strbuf, t->dir_id, this_dir_id, alignmask);
		}
	}
}

/*
 * Create
 */
void Create()
{
	fNDS = fopen(ndsfilename, "wb");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }

	// initial header data
	if (headerfilename)
	{
		// header template
		FILE *fi = fopen(headerfilename, "rb");
		if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", headerfilename); exit(1); }
		fread(&header, 1, 0x200, fi);
		fclose(fi);
	}
	else
	{
		// clear header
		memset(&header, 0, 0x200);
		memcpy(header.gamecode, "####", 4);
		header.reserved2 = 0x04;		// autostart
		unsigned char romcontrol[] = { 0x00,0x60,0x58,0x00,0xF8,0x08,0x18,0x00 };
		memcpy(((unsigned char *)&header) + 0x60, romcontrol, sizeof(romcontrol));
		*(unsigned_int *)&header = 0xEA00002E;		// for PassMe's that start @ 0x08000000
	}

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
	else if (!headerfilename)	// homebrew?
	{
		if (loadme_size != 156) { fprintf(stderr, "loadme size error\n"); exit(1); }
		memcpy(header.logo, loadme, loadme_size);		// self-contained NDS loader for *Me GBA cartridge boot
		memcpy(&header.offset_0xA0, "SRAM_V110", 9);		// allow GBA cartridge SRAM backup
		memcpy(&header.offset_0xAC, "PASS01\x96", 7);		// automatically start with FlashMe, make it look more like a GBA rom
	}

	// unique ID... just for homebrew, not very used... obsolete?
	if (uniquefilename)
	{
		unsigned_int id[2];
		FILE *f = fopen(uniquefilename, "rb");
		if (f)
		{
			fread(&id[0], sizeof(id[0]), 1, f);
			fread(&id[1], sizeof(id[1]), 1, f);
		}
		else
		{
			f = fopen(uniquefilename, "wb");
			if (!f) { fprintf(stderr, "Cannot open file '%s'.\n", uniquefilename); exit(1); }
			for (char *p=ndsfilename; *p; p++) srand(*p ^ VER[1] ^ VER[3] ^ (rand()<<30) ^ (rand()<<15) ^ rand() ^ time(0) ^ header.arm9_size);
			id[0] = (rand()<<15) ^ rand() ^ header.arm7_size;
			for (char *p=ndsfilename; *p; p++) srand(*p ^ VER[0] ^ VER[2] ^ (rand()<<30) ^ (rand()<<15) ^ rand() ^ time(0) ^ header.arm7_size);
			fwrite(&id[0], sizeof(id[0]), 1, f);
			id[1] = (rand()<<30) ^ (rand()<<15) ^ rand() ^ header.arm9_size;
			fwrite(&id[1], sizeof(id[1]), 1, f);
		}
		fclose(f);
		header.offset_0x78 = id[0];
		header.offset_0x7C = id[1];
	}

	// game/maker codes
	if (gamecode) strncpy(header.gamecode, gamecode, 4);
	if (makercode) strncpy((char *)header.makercode, makercode, 2);

	// header size
	unsigned int header_size = header.rom_header_size;
	if (!header_size) { header_size = 0x200; header.rom_header_size = header_size; }
	fseek(fNDS, header_size, SEEK_SET);

	// --------------------------

	// ARM9 binary
	if (arm9filename)
	{
		header.arm9_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
		fseek(fNDS, header.arm9_rom_offset, SEEK_SET);

		unsigned int entry_address = arm9Entry ? arm9Entry : (unsigned int)header.arm9_entry_address;		// template
		unsigned int ram_address = arm9RamAddress ? arm9RamAddress : (unsigned int)header.arm9_ram_address;		// template
		if (!ram_address && entry_address) ram_address = entry_address;
		if (!entry_address && ram_address) entry_address = ram_address;
		if (!ram_address) { ram_address = entry_address = 0x02000000; }

		unsigned int size = 0;
		if (HasElfExtension(arm9filename))
			CopyFromElf(arm9filename, &entry_address, &ram_address, &size);
		else
			CopyFromBin(arm9filename, 0, &size);
		header.arm9_entry_address = entry_address;
		header.arm9_ram_address = ram_address;
		header.arm9_size = ((size + 3) &~ 3);
	}
	else
	{
		fprintf(stderr, "ARM9 binary file required.\n");
		exit(1);
	}

	// ARM9 overlay table
	if (arm9ovltablefilename)
	{
		unsigned_int x1 = 0xDEC00621; fwrite(&x1, sizeof(x1), 1, fNDS);		// ???
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
	header.arm7_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);
	if (arm7filename)
	{
		unsigned int entry_address = arm7Entry ? arm7Entry : (unsigned int)header.arm7_entry_address;		// template
		unsigned int ram_address = arm7RamAddress ? arm7RamAddress : (unsigned int)header.arm7_ram_address;		// template
		if (!ram_address && entry_address) ram_address = entry_address;
		if (!entry_address && ram_address) entry_address = ram_address;
		if (!ram_address) { ram_address = entry_address = 0x03800000; }

		unsigned int size = 0;
		if (HasElfExtension(arm7filename))
			CopyFromElf(arm7filename, &entry_address, &ram_address, &size);
		else
			CopyFromBin(arm7filename, &size);

		header.arm7_entry_address = entry_address;
		header.arm7_ram_address = ram_address;
		header.arm7_size = ((size + 3) &~ 3);
	}
	else	// default ARM7 binary
	{
		fwrite(default_arm7, 1, default_arm7_size, fNDS);
		header.arm7_entry_address = 0x03800000;
		header.arm7_ram_address = 0x03800000;
		header.arm7_size = ((default_arm7_size + 3) & ~3);
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

	// banner
	if (bannerfilename)
	{
		header.banner_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
		fseek(fNDS, header.banner_offset, SEEK_SET);
		if (bannertype == BANNER_IMAGE)
		{
			IconFromBMP();
		}
		else
		{
			CopyFromBin(bannerfilename, 0);
		}
	}
	else
	{
		header.banner_offset = 0;
	}

	// filesystem
	if (filerootdir || overlaydir)
	{
		// read directory structure
		Tree *filetree = new Tree();		// dummy root node 0xF000
		free_dir_id++;
		directory_count++;
		if (filerootdir) ReadDirectory(filetree, filerootdir);	// fill empty directory

		// calculate offsets required for FNT and FAT
		_entry_start = 8*directory_count;		// names come after directory structs
		header.fnt_offset = ftell(fNDS);	// not aligned			//(ftell(fNDS) + 0x1FF) &~ 0x1FF;		// align to 512 bytes
		header.fnt_size =
			_entry_start +		// directory structs
			total_name_size +	// total number of name characters for dirs and files
			directory_count*4 +	// directory: name length (1), dir id (2), end-character (1)
			file_count*1 +		// files: name length (1)
			- 3;				// root directory only has an end-character
		file_count += overlay_files;		// didn't take overlay files into FNT size, but have to be calculated into FAT size
		//header.fat_offset = (header.fnt_offset + header.fnt_size + 0x1FF) &~ 0x1FF;		// align to 512 bytes;
		header.fat_offset = (header.fnt_offset + header.fnt_size + 0x3) &~ 0x3;		// align to 4 bytes. should be 0xFF's
		//header.fat_offset = header.fnt_offset + header.fnt_size;		// not aligned
		header.fat_size = file_count * 8;		// each entry contains top & bottom offset
		file_top = header.fat_offset + header.fat_size;

		// add overlay files
		for (unsigned int i=0; i<overlay_files; i++)
		{
			char s[32]; sprintf(s, OVERLAY_FMT, free_file_id);
			AddFile(overlaydir, "/", s, free_file_id, 0x3);
			free_file_id++;		// incremented up to overlay_files
		}

		AddDirectory(filetree, "/", 0xF000, directory_count, 0x3);
		fseek(fNDS, file_end, SEEK_SET);

		if (verbose)
		{
			printf("%u directories.\n", directory_count);
			printf("%u normal files.\n", file_count - overlay_files);
			printf("%u overlay files.\n", overlay_files);
		}
	}

	// --------------------------

	// application end offset
	int pad = ((ftell(fNDS) + 0x3) &~ 0x3) - ftell(fNDS);	// align to 4 bytes
	while (pad--) fputc(0, fNDS);
	header.application_end_offset = ftell(fNDS);

	// fix up header CRCs and write header
	header.logo_crc = CalcLogoCRC(header);
	header.header_crc = CalcHeaderCRC(header);
	fseek(fNDS, 0, SEEK_SET);
	fwrite(&header, 0x200, 1, fNDS);

	fclose(fNDS);
}
