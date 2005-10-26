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

	printf("GAME   ");
	for (unsigned int i=0; i<sizeof(header.gamecode); i++) printf("%c", header.gamecode[i]);
	printf("-%d ", header.romversion);
	for (unsigned int i=0; i<sizeof(header.title); i++) printf("%c", header.title[i]);
	printf("\n");

	// load ARM7 binary
	fseek(fNDS, header.arm7_rom_offset, SEEK_SET);
	fread(pc_ram + header.arm7_ram_address - DS_RAM, 1, header.arm7_size, fNDS);

	// load ARM9 binary
	fseek(fNDS, header.arm9_rom_offset, SEEK_SET);
	fread(pc_ram + header.arm9_ram_address - DS_RAM, 1, header.arm9_size, fNDS);


	if (!Find<unsigned long>("BX LR  %08X\n", 0x02000000, 0x02280000, 0xE120001E, 0xFFF000FF, 0))		// BX LR
	if (!Find<unsigned short>("BX LR  %08X\n", 0x02000000, 0x02280000, 0x4770, 0xFFFF, 1))				// BX LR
	printf("BX LR  not found!\n");

	if (!Find<unsigned long>("SWI FF %08X\n", 0x02000000, 0x023FE000, 0xEFFF0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("SWI EA %08X\n", 0x02000000, 0x023FE000, 0xEFEA0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("SWI A4 %08X\n", 0x02000000, 0x023FE000, 0xEFA40000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned long>("SWI AF %08X\n", 0x02000000, 0x023FE000, 0xEFAF0000, 0xFFFF0000, 0))		// SWI
	if (!Find<unsigned short>("SWI FF %08X\n", 0x02000000, 0x023FE000, 0xDFFF, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("SWI EA %08X\n", 0x02000000, 0x023FE000, 0xDFEA, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("SWI A4 %08X\n", 0x02000000, 0x023FE000, 0xDFA4, 0xFFFF, 1))				// SWI
	if (!Find<unsigned short>("SWI AF %08X\n", 0x02000000, 0x023FE000, 0xDFAF, 0xFFFF, 1))				// SWI
	printf("SWI    not found!\n");

	//Find<unsigned long>("%08X\n", 0x037F8000, 0x0380F000, 0xEFEA0000, 0xFFFF0000, 0);		// SWI
	//...

	//Find<unsigned short>("%08X\n", 0x037F8000, 0x0380F000, 0xDFA4, 0xFFFF, 1);				// SWI
	//...


	//*(unsigned short *)(header + 0x15E) = CalcCRC(header, 0x15E);
	//printf("CRC16  %04X\n", *(unsigned short *)(header + 0x15E));

	printf("---\n");

	fclose(fNDS);
}
