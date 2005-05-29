/*
	Nintendo DS rom tool
	by Rafael Vuijk (aka DarkFader)
*/

#include "ndstool.h"
#include <unistd.h>

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
char *bannerfilename = 0;
char *bannertext = 0;
char *headerfilename = 0;
char *uniquefilename = 0;
char *logofilename = 0;
char *makercode = 0;
char *gamecode = 0;

int bannertype;
unsigned int arm9RamAddress = 0;
unsigned int arm7RamAddress = 0;
unsigned int arm9Entry = 0;
unsigned int arm7Entry = 0;

/*
 * Help
 */
void Help(char *unknownoption = 0)
{
	if (unknownoption)
	{
		printf("Unknown option: %s\n\n", unknownoption);
	}

	printf("Show header:         -i game.nds\n");
	printf("Fix header CRC       -f game.nds\n");
	printf("List files:          -l game.nds\n");
	printf("Create               -c game.nds\n");
	printf("Extract              -x game.nds\n");
	printf("Create/Extract options:\n");
	printf("  ARM7 executable    -7 arm7.bin\n");
	printf("  ARM9 executable    -9 arm9.bin\n");
	printf("  ARM7 RAM address   -r7 address                    (optional, 0x for hex)\n");
	printf("  ARM9 RAM address   -r9 address                    (optional, 0x for hex)\n");
	printf("  ARM7 RAM entry     -e7 address                    (optional, 0x for hex)\n");
	printf("  ARM9 RAM entry     -e9 address                    (optional, 0x for hex)\n");
	printf("  files              -d directory                   (optional)\n");
	printf("  header template    -h header.bin                  (optional)\n");
	printf("  banner             -b icon.bmp \"title;lines;here\" (optional)\n");
	printf("  banner binary      -t banner.bnr                  (optional)\n");
	printf("  logo binary        -o logo.bin                    (optional)\n");
	printf("  maker code         -m code                        (optional)\n");
	printf("  game code          -g code                        (optional)\n");
	printf("  unique ID filename -u game.uid                    (optional, auto generated)\n");
	printf("  verbose            -v\n");
}

/*
 * main
 */
int main(int argc, char *argv[])
{
	#ifdef _NDSTOOL_P_H
		if (sizeof(Header) != 0x200) { fprintf(stderr, "Header size %d != %d\n", sizeof(Header), 0x200); exit(1); }
	#endif

	printf("Nintendo DS rom tool "VER" - %s by Rafael Vuijk (aka DarkFader)\n",__DATE__);
	if (argc < 2) { Help(); return 0; }

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
					bannerfilename = (argc > a) ? argv[++a] : 0;
					bannertype = BANNER_BINARY;
					break;

				case 'b':
					bannertype = BANNER_IMAGE;
					bannerfilename = (argc > a) ? argv[++a] : 0;
					bannertext = (argc > a) ? argv[++a] : 0;
					break;

				case 'o':
					logofilename = (argc > a) ? argv[++a] : 0;
					break;

				case 'h':	// load header
					headerfilename = (argc > a) ? argv[++a] : 0;
					break;

				case 'u':	// unique ID file
					uniquefilename = (argc > a) ? argv[++a] : 0;
					break;

				case 'v':	// verbose
					verbose = true;
					break;

				case 'r':	// RAM address
					switch (argv[a][2])
					{
						case '7': arm7RamAddress = (argc > a) ? strtoul(argv[++a], 0, 0) : 0; break;
						case '9': arm9RamAddress = (argc > a) ? strtoul(argv[++a], 0, 0) : 0; break;
						default: Help(argv[a]); return 1;
					}
					break;

				case 'e':	// entry point
					switch (argv[a][2])
					{
						case '7': arm7Entry = (argc > a) ? strtoul(argv[++a], 0, 0) : 0; break;
						case '9': arm9Entry = (argc > a) ? strtoul(argv[++a], 0, 0) : 0; break;
						default: Help(argv[a]); return 1;
					}
					break;

				case 'm':	// maker code
					makercode = (argc > a) ? argv[++a] : 0;
					if (strlen(makercode) != 2) {
						printf("maker code must be 2 characters!\n");
						exit(1);
					}
					break;
				
				case 'g':	// game code
					gamecode = (argc > a) ? argv[++a] : 0;
					if (strlen(gamecode) != 4) {
						printf("game code must be 4 characters!\n");
						exit(1);
					}
					break;
					


				default:
				{
					Help(argv[a]);
					return 1;
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
		if (bannerfilename) Extract(bannerfilename, true, 0x68, false, 0x840);
		if (headerfilename) Extract(headerfilename, false, 0x0, false, 0x200);
	}
	else if (create)
	{
		if (!arm9RamAddress && arm9Entry) arm9RamAddress = arm9Entry;
		if (!arm9Entry && arm9RamAddress) arm9Entry = arm9RamAddress;
		if (!arm9RamAddress) arm9RamAddress = arm9Entry = 0x02000000;
		
		if (!arm7RamAddress && arm7Entry) arm7RamAddress = arm7Entry;
		if (!arm7Entry && arm7RamAddress) arm7Entry = arm7RamAddress;
		if (!arm7RamAddress) arm7RamAddress = arm7Entry = 0x03800000;

		Create();
	}

	return 0;
}
