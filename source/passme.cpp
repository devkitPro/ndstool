#include "ndstool.h"
#include "crc16.h"

#define DS_RAM			0x02000000
#define RAM_SIZE		(4*1024*1024)
unsigned char *pc_ram;


template <typename T> unsigned long Find(char *fmt, unsigned long begin, unsigned long end, T data, T mask, int add)
{
	for (unsigned long i=begin; i<end; i+=sizeof(T))
	{
		T w = *(T *)(pc_ram + i - DS_RAM);
		w &= mask;
		if (w == data)
		{
			if (add)
				header.arm7_entry_address = i + add;		// ARM7 entry
			else
				header.arm9_entry_address = i + add;		// ARM9 entry
			printf(fmt, i + add);
			return i;
		}
	}
	//printf(fmt, 0);
	return 0;
}

/*
 * PassMe
 */
void PassMe(char *ndsfilename)
{
	fNDS = fopen(ndsfilename, "r+b");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	fread(&header, 512, 1, fNDS);

	pc_ram = new unsigned char[RAM_SIZE];

	printf("-- PassMe for game: ");
	for (unsigned int i=0; i<sizeof(header.gamecode); i++) printf("%c", header.gamecode[i]);
	printf("_%d - ", header.romversion);
	for (unsigned int i=0; i<sizeof(header.title); i++) if (header.title[i]) printf("%c", header.title[i]);
	printf("\n");

	// load ARM7 binary
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);
	fread(pc_ram + header.arm7_ram_address - DS_RAM, 1, header.arm7_size, fNDS);

	// load ARM9 binary
	fseek(fNDS, header.arm9_rom_offset, SEEK_SET);
	fread(pc_ram + header.arm9_ram_address - DS_RAM, 1, header.arm9_size, fNDS);

	unsigned char old_header[512];
	memcpy(old_header, &header, 512);

	bool bError = false;

	if (!Find<unsigned long>("-- BX LR @ 0x%08X\n", 0x02000000, 0x02280000, 0xE120001E, 0xFFF000FF, 0))		// BX LR
	if (!Find<unsigned short>("-- BX LR @ 0x%08X\n", 0x02000000, 0x02280000, 0x4770, 0xFFFF, 1))				// BX LR
	{ printf("BX LR instruction not found!\n"); bError = true; }

	if (!Find<unsigned long>("-- SWI 0xFF @ 0x%08X\n", 0x02000000, 0x023FE000, 0xEFFF0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("-- SWI 0xEA @ 0x%08X\n", 0x02000000, 0x023FE000, 0xEFEA0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("-- SWI 0xA4 @ 0x%08X\n", 0x02000000, 0x023FE000, 0xEFA40000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("-- SWI 0xAF @ 0x%08X\n", 0x02000000, 0x023FE000, 0xEFAF0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned short>("-- SWI 0xFF @ 0x%08X\n", 0x02000000, 0x023FE000, 0xDFFF, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("-- SWI 0xEA @ 0x%08X\n", 0x02000000, 0x023FE000, 0xDFEA, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("-- SWI 0xA4 @ 0x%08X\n", 0x02000000, 0x023FE000, 0xDFA4, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("-- SWI 0xAF @ 0x%08X\n", 0x02000000, 0x023FE000, 0xDFAF, 0xFFFF, 1))				// SWI
	{ printf("SWI instruction not found!\n"); bError = true; }

	//Find<unsigned long>("%08X\n", 0x037F8000, 0x0380F000, 0xEFEA0000, 0xFFFF0000, 0);		// SWI
	//...

	//Find<unsigned short>("%08X\n", 0x037F8000, 0x0380F000, 0xDFA4, 0xFFFF, 1);				// SWI
	//...

	if (bError)
	{
		printf("Sorry.\n");
	}
	else
	{
		header.reserved2 |= 0x04;	// set autostart bit
	
		header.header_crc = CalcCRC((unsigned char *)&header, 0x15E);
	
		puts(
			#include "passme_vhd1.h"
		);

		for (int i=0; i<512; i++)
		{
			if (((unsigned char *)&header)[i] != old_header[i])
			{
				printf("\t\t\twhen 16#%03X# => patched_data <= X\"%02X\";\n", i, ((unsigned char *)&header)[i]);
			}
		}

		puts(
			#include "passme_vhd2.h"
		);	
	}

	fclose(fNDS);
}
