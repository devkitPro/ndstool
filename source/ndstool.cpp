/*
	Nintendo DS rom tool
	by Rafael Vuijk (aka DarkFader)

	additional code
		natrium42 <natrium@gmail.com>	
		WinterMute <wntrmute@gmail.com>

	v1.00 - shows info, fixes CRC, extracts files
	v1.01 - extracts ARM7/ARM9 code, more header info
	v1.02 - added maker codes, logo CRC, cartridge code
	v1.03 - shows if secure area CRC was ok, added function to create NDS file (ELF files for ARM7/9, no filesystem yet)
	v1.04 - added option to list files without extracting
	v1.05 - fix CRC option only updates header CRC for easier PassMe calculation
	v1.06 - changed parameter style, add/extract icon+title data option
	v1.07 - add/extract header option, filesystem
	v1.08 - added icon converter
	v1.09 - added default arm7 binary

	TODO:
	code cleanup

*/

#include "ndstool.h"





/*
 * AddFile
 */
void AddFile(char *filename)
{
}





/*
void DebugTree(Tree *tree, int level = 0)
{
	tree = tree->next;
	
	for (Tree *t=tree; t; t=t->next)
	{
		for (int i=0; i<level; i++) printf("?/");
		printf("%04X %s\n", t->dir_id, t->name);
		if (t->directory) DebugTree(t->directory, level+1);
	}
}
*/






#ifdef _NDSTOOL_P_H
	#include "ndstool.p.h"
#endif

/*
 * ShowHeader
 */
void ShowHeader(char *ndsfilename)
{
	fNDS = fopen(ndsfilename, "rb");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	fread(&header, 512, 1, fNDS);

	printf("0x00  %-25s  ", "Game title");

	for (unsigned int i=0; i<sizeof(header.title); i++)
		printf("%c", header.title[i]); printf("\n");

	printf("0x0C  %-25s  ", "Game code");
	for (unsigned int i=0; i<sizeof(header.gamecode); i++)
		printf("%c", header.gamecode[i]);
	for (int i=0; i<NumCountries; i++)
	{
		if (countries[i].countrycode == header.gamecode[3])
		{
			printf(" (NTR-");
			for (unsigned int j=0; j<sizeof(header.gamecode); j++) printf("%c", header.gamecode[j]);
			printf("-%s)", countries[i].name);
			break;
		}
	}
	printf("\n");
	
	printf("0x10  %-25s  ", "Maker code"); for (unsigned int i=0; i<sizeof(header.makercode); i++) printf("%c", header.makercode[i]);
	for (int j=0; j<NumMakers; j++)
	{
		if ((makers[j].makercode[0] == header.makercode[0]) && (makers[j].makercode[1] == header.makercode[1]))
		{
			printf(" (%s)", makers[j].name);
			break;
		}
	}
	printf("\n");

	printf("0x12  %-25s  0x%02X\n", "Unit code", header.unitcode);
	printf("0x13  %-25s  0x%02X\n", "Device type", header.devicetype);
	printf("0x14  %-25s  0x%02X (%d Mbit)\n", "Device capacity", header.devicecap, 1<<header.devicecap);
	printf("0x15  %-25s  ", "reserved 1"); for (unsigned int i=0; i<sizeof(header.reserved1); i++) printf("%02X", header.reserved1[i]); printf("\n");
	printf("0x1E  %-25s  0x%02X\n", "ROM version", header.romversion);
	printf("0x1F  %-25s  0x%02X\n", "reserved 2", header.reserved2);
	printf("0x20  %-25s  0x%X\n", "ARM9 ROM offset", (int)header.arm9_rom_offset);
	printf("0x24  %-25s  0x%X\n", "ARM9 entry address", (int)header.arm9_entry_address);
	printf("0x28  %-25s  0x%X\n", "ARM9 RAM address", (int)header.arm9_ram_address);
	printf("0x2C  %-25s  0x%X\n", "ARM9 code size", (int)header.arm9_size);
	printf("0x30  %-25s  0x%X\n", "ARM7 ROM offset", (int)header.arm7_rom_offset);
	printf("0x34  %-25s  0x%X\n", "ARM7 entry address", (int)header.arm7_entry_address);
	printf("0x38  %-25s  0x%X\n", "ARM7 RAM address", (int)header.arm7_ram_address);
	printf("0x3C  %-25s  0x%X\n", "ARM7 code size", (int)header.arm7_size);
	printf("0x40  %-25s  0x%X\n", "File name table offset", (int)header.fnt_offset);
	printf("0x44  %-25s  0x%X\n", "File name table size", (int)header.fnt_size);
	printf("0x48  %-25s  0x%X\n", "FAT offset", (int)header.fat_offset);
	printf("0x4C  %-25s  0x%X\n", "FAT size", (int)header.fat_size);
	printf("0x50  %-25s  0x%X\n", "ARM9 overlay offset", (int)header.arm9_overlay_offset);
	printf("0x54  %-25s  0x%X\n", "ARM9 overlay size", (int)header.arm9_overlay_size);
	printf("0x58  %-25s  0x%X\n", "ARM7 overlay offset", (int)header.arm7_overlay_offset);
	printf("0x5C  %-25s  0x%X\n", "ARM7 overlay size", (int)header.arm7_overlay_size);
	printf("0x60  %-25s  ", "ROM control info 1"); for (unsigned int i=0; i<sizeof(header.rom_control_info1); i++) printf("%02X", header.rom_control_info1[i]); printf("\n");
	printf("0x68  %-25s  0x%X\n", "Icon/title offset", (int)header.icon_title_offset);
	unsigned short secure_area_crc = CalcSecureAreaCRC();
	printf("0x6C  %-25s  0x%04X (%s)\n", "Secure area CRC", (int)header.secure_area_crc, (secure_area_crc == header.secure_area_crc) ? "OK" : "INVALID");
	printf("0x6E  %-25s  0x%04X\n", "ROM control info 2", (int)header.rom_control_info2);
	printf("0x70  %-25s  0x%X\n", "?", (int)header.offset_0x70);
	printf("0x74  %-25s  0x%X\n", "?", (int)header.offset_0x74);
	printf("0x78  %-25s  0x%08X\n", "?", (int)header.offset_0x78);
	printf("0x7C  %-25s  0x%08X\n", "?", (int)header.offset_0x7C);
	printf("0x80  %-25s  0x%08X\n", "Application end offset", (int)header.application_end_offset);
	printf("0x84  %-25s  0x%08X\n", "ROM header size", (int)header.rom_header_size);
	printf("0x88  %-25s  0x%08X\n", "?", (int)header.offset_0x88);
	printf("0x8C  %-25s  0x%08X\n", "?", (int)header.offset_0x8C);
	printf("0x90  %-25s  0x%08X\n", "?", (int)header.offset_0x90);
	printf("0x94  %-25s  0x%08X\n", "?", (int)header.offset_0x94);
	printf("0x98  %-25s  0x%08X\n", "?", (int)header.offset_0x98);
	printf("0x9C  %-25s  0x%08X\n", "?", (int)header.offset_0x9C);
	printf("0xA0  %-25s  0x%08X\n", "?", (int)header.offset_0xA0);
	printf("0xA4  %-25s  0x%08X\n", "?", (int)header.offset_0xA4);
	printf("0xA8  %-25s  0x%08X\n", "?", (int)header.offset_0xA8);
	printf("0xAC  %-25s  0x%08X\n", "?", (int)header.offset_0xAC);
	printf("0xB0  %-25s  0x%08X\n", "?", (int)header.offset_0xB0);
	printf("0xB4  %-25s  0x%08X\n", "?", (int)header.offset_0xB4);
	printf("0xB8  %-25s  0x%08X\n", "?", (int)header.offset_0xB8);
	printf("0xBC  %-25s  0x%08X\n", "?", (int)header.offset_0xBC);
	unsigned short logo_crc = CalcLogoCRC();
	printf("0x15C %-25s  0x%04X (%s)\n", "Logo CRC", (int)header.logo_crc, (logo_crc == header.logo_crc) ? "OK" : "INVALID");
	unsigned short header_crc = CalcHeaderCRC();
	printf("0x15E %-25s  0x%04X (%s)\n", "Header CRC", (int)header.header_crc, (header_crc == header.header_crc) ? "OK" : "INVALID");

	// give final CRC warning
	if (
		(logo_crc != header.logo_crc) ||
		(secure_area_crc != header.secure_area_crc) ||
		(header_crc != header.header_crc)
	)
	{
		printf("WARNING: The NDS file might be currupted!\n");
	}

	fclose(fNDS);
}

/*
 * Help
 */
void Help()
{
	printf("Show header:       -i game.nds\n");
	printf("Fix header CRC     -f game.nds\n");
	printf("List files:        -l game.nds\n");
#ifdef _NDSTOOL_P_H
	printf("Patch header:      -p game.nds        (only for DarkFader)\n");
#endif
	printf("Create             -c game.nds\n");
	printf("Extract            -x game.nds\n");
	printf("Create/Extract options:\n");
	printf("  ARM7 executable  -7 arm7.bin                  (optional)\n");
	printf("  ARM9 executable  -9 arm9.bin\n");
	printf("  files            -d directory                 (optional)\n");
	printf("  header           -h header.bin                (optional)\n");
	printf("  icon/title       -b icontitle.bmp title text  (optional)\n");
	printf("  icon/title       -t icontitle.bin             (optional)\n");
	printf("               Separate lines with ';' for the title text.\n");
	printf("  verbose          -v\n");
	exit(0);
}

/*
 * main
 */
int main(int argc, char *argv[])
{
	#ifdef _NDSTOOL_P_H
		if (sizeof(Header) != 0x200) { printf("%d != %d\n", sizeof(Header), 0x200); exit(1); }
	#endif

	printf("Nintendo DS rom tool "VER" by Rafael Vuijk (aka DarkFader)\n\n");
	if (argc < 2) Help();

	// initialize default header
	memset(&header, 0, 0x200);
	header.gamecode[0] = 'A';
	header.gamecode[1] = 'X';
	header.gamecode[2] = 'X';
	header.gamecode[3] = 'E';
	header.reserved2 = 0x04;		// autostart
	for (unsigned int i=0; i<sizeof(header.logo); i++) header.logo[i] = ~logo[i];

	// what to do
	bool extract = false;
	bool create = false;

	// parse parameters
	for (int a=1; a<argc; a++)
	{
		if (argv[a][0] == '-')
		{
			switch (argv[a][1])
			{
				case 'i':	// show information
				{
					ndsfilename = (argc > a) ? argv[++a] : 0;
					ShowHeader(ndsfilename);
					return 0;
				}

				case 'f':	// fix header CRC
				{
					ndsfilename = (argc > a) ? argv[++a] : 0;
					FixHeaderCRC(ndsfilename);
					return 0;
				}

				case 'l':	// list files
				{
					ndsfilename = (argc > a) ? argv[++a] : 0;
					ExtractFiles();
					return 0;
				}

#ifdef _NDSTOOL_P_H
				case 'p':	// patch
				{
					ndsfilename = (argc > a) ? argv[++a] : 0;
					char *outfilename = (argc > a) ? argv[++a] : 0;
					Patch(ndsfilename, outfilename);
					return 0;
				}
#endif

				///

				case 'x':	// extract
				{
					extract = true;
					ndsfilename = (argc > a) ? argv[++a] : 0;
					if (!ndsfilename) return 1;
					break;
				}

				case 'c':	// create
				{
					create = true;
					ndsfilename = (argc > a) ? argv[++a] : 0;
					if (!ndsfilename) return 1;
					break;
				}
				
				// create/extract options
				case 'd': filerootdir = (argc > a) ? argv[++a] : 0; break;
				case '7': arm7filename = (argc > a) ? argv[++a] : 0; break;
				case '9': arm9filename = (argc > a) ? argv[++a] : 0; break;
				case 't':
					icontitlefilename = (argc > a) ? argv[++a] : 0;
					icontype = BINARY;
					break;
				case 'b':
					icontype = IMAGE;
					icontitlefilename = (argc > a) ? argv[++a] : 0;
					icontitlename = (argc > a) ? argv[++a] : 0;
					icontitletext = (argc > a) ? argv[++a] : 0;
					break;
				case 'h': headerfilename = (argc > a) ? argv[++a] : 0; break;
				case 'v': verbose = true; break;

				default:
				{
					printf("Unknown option: %s\n", argv[a]);
					Help();
					break;
				}
			}
		}
		else
		{
			Help();
			return 0;
		}

		
	}

	if (extract && create)
	{
		printf("Cannot both extract and create!\n");
		return 1;
	}
	else if (extract)
	{
		if (arm7filename) Extract(arm7filename, true, 0x30, true, 0x3C);
		if (arm9filename) Extract(arm9filename, true, 0x20, true, 0x2C);
		if (filerootdir) ExtractFiles();
		if (icontitlefilename) Extract(icontitlefilename, true, 0x68, false, 0x840);
		if (headerfilename) Extract(headerfilename, false, 0x0, false, 0x200);
	}
	else if (create)
	{
		Create();
	}


	return 0;
}
