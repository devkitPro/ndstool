#include "ndstool.h"
#include "default_arm7.h"
#include <time.h>

/*
 * default logo
 */
const unsigned char defaultlogo[] =
{
	0xFF,0xFF,0xAE,0x51,0x14,0xFF,0xFF,0xFF,0x9E,0x51,0x62,0xA2,0x02,0xC3,0xF7,0x30,
	0xCB,0x4C,0xD0,0x03,0x90,0xE0,0x03,0x13,0x00,0x0C,0xA0,0x93,0x13,0x00,0x48,0xAB,
	0x32,0x60,0x62,0xD8,0x73,0xD4,0x24,0x24,0x8A,0x21,0x02,0x3E,0x55,0x14,0x32,0x50,
	0x0D,0x30,0x68,0x45,0xDA,0xDF,0xDE,0x3C,0xEF,0x12,0xB0,0x84,0x58,0x32,0xF8,0xEF,
	0xFF,0xFF,0x1F,0xE4,0xFF,0xFF,0xFF,0xFF,0xCE,0x53,0xFD,0xFC,0x22,0xC3,0x1D,0x60,
	0x68,0x20,0x60,0xC2,0x6C,0x18,0x38,0xC6,0xAD,0x63,0xF0,0x01,0x10,0xCD,0x6F,0x06,
	0xC7,0x52,0x4A,0x18,0x2E,0x34,0xC0,0x80,0x30,0xC3,0x30,0x06,0x30,0xE8,0x32,0x9E,
	0x37,0x00,0x80,0x27,0x8B,0xB0,0x22,0xE0,0x0C,0x75,0x00,0x5C,0x1D,0x55,0x2C,0x74,
	0xC8,0x32,0x67,0xC7,0x68,0x20,0x9C,0xB2,0xFF,0xFF,0x6F,0x80,0x00,0xFF,0xFF,0xFF,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
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
	else
	{
		// clear header
		memset(&header, 0, 0x200);
		header.gamecode[0] = 'A';
		header.gamecode[1] = 'X';
		header.gamecode[2] = 'X';
		header.gamecode[3] = 'E';
		header.reserved2 = 0x04;		// autostart
	}

	// load a logo
	if (logofilename)
	{
		// TODO: check bin or bmp
		FILE *fi = fopen(logofilename, "rb");
		if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", logofilename); exit(1); }
		fread(&header.logo, 1, 156, fi);
		fclose(fi);
	}
	else
	{
		memcpy(header.logo, defaultlogo, sizeof(header.logo));
	}

	// skip header
	unsigned int header_size = header.rom_header_size;
	if (!header_size) { header_size = 0x200; header.rom_header_size = header_size; }
	fseek(fNDS, header_size, SEEK_SET);

	// ARM9 binary
	if (!arm9Entry) arm9Entry = arm9RamAddress;
	if (arm9filename)
	{
		header.arm9_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
		fseek(fNDS, header.arm9_rom_offset, SEEK_SET);
		unsigned int entry_address = header.arm9_entry_address; if (!entry_address) entry_address = arm9Entry;
		unsigned int ram_address = header.arm9_ram_address; if (!ram_address) ram_address = arm9RamAddress;
		unsigned int size = 0;
		if (HasElfExtension(arm9filename))
			CopyFromElf(arm9filename, &entry_address, &ram_address, &size);
		else
			CopyFromBin(arm9filename, &size);
		header.arm9_entry_address = entry_address;
		header.arm9_ram_address = ram_address;
		header.arm9_size = ((size + 3) &~ 3);
	}
	else
	{
		
		header.arm9_entry_address = 0;
		header.arm9_ram_address = 0;
		header.arm9_size = 0;
	}

	// ARM7 binary
	if (!arm7Entry) arm7Entry = arm7RamAddress;
	header.arm7_rom_offset = (ftell(fNDS) + 0x1FF) &~ 0x1FF;	// align to 512 bytes
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);
	if (arm7filename)
	{
		unsigned int entry_address = header.arm7_entry_address; if (!entry_address) entry_address = arm7Entry;
		unsigned int ram_address = header.arm7_ram_address; if (!ram_address) ram_address = arm7RamAddress;
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
		header.arm7_entry_address = arm7Entry;
		header.arm7_ram_address = arm7RamAddress;
		header.arm7_size = ((default_arm7_size + 3) & ~3);
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
		header.fnt_size = _entry_start + ((file_count + directory_count)*1/*length*/ + total_name_size/*names of both dirs and files*/ + directory_count*1/*end of directory*/);
		header.fat_offset = (header.fnt_offset + header.fnt_size + 0x1FF) &~ 0x1FF;		// align to 512 bytes;
		header.fat_size = file_count * 8;		// each entry contains top & bottom offset
		file_top = header.fat_offset + header.fat_size;

		//DebugTree(root);
		unsigned int file_end = WalkTree(root, "/", 0xF000, directory_count);
		fseek(fNDS, file_end, SEEK_SET);

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

	// pad end of file
	int pad = ((ftell(fNDS) + 0x1FF) &~ 0x1FF) - ftell(fNDS);	// align to 512 bytes
	while (pad--) fputc(0, fNDS);
	header.application_end_offset = ftell(fNDS);

	// unique ID
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

	if (gamecode) strncpy(header.gamecode, gamecode, 4);
	if (makercode) strncpy((char *)header.makercode, makercode, 2);
	// header
	header.logo_crc = CalcLogoCRC();
	header.header_crc = CalcHeaderCRC();
	fseek(fNDS, 0, SEEK_SET);
	fwrite(&header, 0x200, 1, fNDS);
	
	fclose(fNDS);
}
