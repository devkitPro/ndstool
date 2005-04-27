/*
	Nintendo DS rom tool
	by Rafael Vuijk (aka DarkFader)

	additional code
		natrium42 <natrium@gmail.com>	
		WinterMute <wntrmute@gmail.com>
*/

#include "ndstool.h"

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
int bannertype;
unsigned int defaultArm9entry = 0x02004000;
unsigned int defaultArm7entry = 0x02380000;


#ifdef _NDSTOOL_P_H
	#include "ndstool.p.h"
#endif

/*
 * Help
 */
void Help(char *unknownoption = 0)
{
	if (unknownoption)
	{
		printf("Unknown option: %s\n\n", unknownoption);
	}

	printf("Show header:       -i game.nds\n");
	printf("Fix header CRC     -f game.nds\n");
	printf("List files:        -l game.nds\n");
#ifdef _NDSTOOL_P_H
	printf("Patch header:      -p game.nds        (only for DarkFader)\n");
#endif
	printf("Create             -c game.nds\n");
	printf("Extract            -x game.nds\n");
	printf("Create/Extract options:\n");
	printf("  ARM7 executable  -7 arm7.bin\n");
	printf("  ARM9 executable  -9 arm9.bin\n");
	printf("  ARM7 RAM address -r7 arm7.bin                   (optional)\n");
	printf("  ARM9 RAM address -r9 arm9.bin                   (optional)\n");
	printf("  files            -d directory                   (optional)\n");
	printf("  header           -h header.bin                  (optional)\n");
	printf("  banner           -b icon.bmp \"title;lines;here\" (optional)\n");
	printf("  banner binary    -t banner.bin                  (optional)\n");
	printf("  verbose          -v\n");
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
	if (argc < 2) { Help(); return 0; }

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
					bannerfilename = (argc > a) ? argv[++a] : 0;
					bannertype = BANNER_BINARY;
					break;

				case 'b':
					bannertype = BANNER_IMAGE;
					bannerfilename = (argc > a) ? argv[++a] : 0;
					bannertext = (argc > a) ? argv[++a] : 0;
					break;

				case 'h':	// load header
					headerfilename = (argc > a) ? argv[++a] : 0;
					break;

				case 'v':	// verbose
					verbose = true;
					break;

				case 'r':	// RAM address
					switch (argv[a][2])
					{
						case '7': defaultArm7entry = 0x03800000;
						case '9': defaultArm9entry = 0x02004000;
						default: Help(argv[a]); return 1;
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
		// select default ARM7 binary
		if (!arm7filename)
		{
			char default_arm7[MAX_PATH];
			strcpy(default_arm7, argv[0]);
			char *p = strrchr(default_arm7, '/');
			if (!p) p = strrchr(default_arm7, '\\');
			if (p) p++; else p = default_arm7;
			strcpy(p, "default_arm7.bin");
			FILE *f = fopen(default_arm7, "rb");
			if (!f) { strcpy(p, "default_arm7.bin"); f = fopen(default_arm7, "rb"); }
			if (f) { fclose(f); arm7filename = default_arm7; }
		}
		
		Create();
	}


	return 0;
}
