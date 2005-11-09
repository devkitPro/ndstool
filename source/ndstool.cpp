/*
	Nintendo DS rom tool
	by Rafael Vuijk (aka DarkFader)
*/

#include <ndstool.h>
#include <ndstool_version.h>
#include <unistd.h>
#include "sha1.h"
#include "ndscreate.h"
#include "ndsextract.h"
#include "passme.h"

/*
 * Variables
 */
bool verbose = false;
Header header;
FILE *fNDS = 0;
char *romlistfilename = 0;
char *filemasks[MAX_FILEMASKS];
int filemask_num = 0;
char *ndsfilename = 0;
char *arm7filename = 0;
char *arm9filename = 0;
char *filerootdir = 0;
char *overlaydir = 0;
char *arm7ovltablefilename = 0;
char *arm9ovltablefilename = 0;
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
 * Title
 */
void Title()
{
	printf("Nintendo DS rom tool "VER" - %s %s by Rafael Vuijk (aka DarkFader)\n",CompileDate,CompileTime);
	if (EncryptSecureArea) printf("WARNING: This is a private version!\n");
}

/*
 * Help
 */
void Help(char *unknownoption = 0)
{
	Title();

	if (unknownoption)
	{
		printf("Unknown option: %s\n\n", unknownoption);
	}

	printf("\n");
	printf("Parameter              Syntax                         Comments\n");
	printf("---------              ------                         --------\n");
	printf("Show information:      -i file.nds\n");
	printf("Show more information: -v -i file.nds [roms_rc.dat]   checksums and warnings\n");
	printf("PassMe:                -p file.nds [passme.vhd] [passme.sav]\n");
	printf("Fix header CRC         -f file.nds\n");
	if (EncryptSecureArea)
	printf("En/decrypt secure area -s file.nds\n");
	//printf("Sign multiboot         -n file.nds");
	//printf("Hash file & compare:   -@ arm7.bin\n");		// used in buildscript
	printf("List files:            -l file.nds\n");
	printf("Create                 -c file.nds\n");
	printf("Extract                -x file.nds [filemask]...\n");
	printf("Create/Extract options:\n");
	printf("  verbose              -v                             optional\n");
	printf("\n");
	printf("  ARM9 executable      -9 file.bin\n");
	printf("  ARM7 executable      -7 file.bin                    optional\n");
	printf("  ARM9 overlay table   -y9 file.bin                   optional\n");
	printf("  ARM7 overlay table   -y7 file.bin                   optional\n");
	printf("  data files           -d directory                   optional\n");
	printf("  overlay files        -y directory                   optional\n");
	printf("  banner bitmap/text   -b file.bmp \"text;text;text\"   optional, 3 lines max.\n");
	printf("  banner binary        -t file.bin                    optional\n");
	printf("\n");
	printf("  header template      -h file.bin                    optional\n");
	printf("  logo bitmap/binary   -o file.bmp/file.bin           optional\n");
	printf("  maker code           -m code                        optional\n");
	printf("  game code            -g code                        optional\n");
	//printf("  unique ID filename  -u file.bin                    optional, for homebrew, auto generated\n");
	printf("  ARM9 RAM address     -r9 address                    optional, 0x prefix for hex\n");
	printf("  ARM7 RAM address     -r7 address                    optional, 0x prefix for hex\n");
	printf("  ARM9 RAM entry       -e9 address                    optional, 0x prefix for hex\n");
	printf("  ARM7 RAM entry       -e7 address                    optional, 0x prefix for hex\n");
}

#define REQUIRED()		((argc > a+1) ? argv[++a] : 0)
#define OPTIONAL()		((argc > a+1) && (argv[a+1][0] != '-') ? argv[++a] : 0)

/*
 * main
 */
int main(int argc, char *argv[])
{
	#ifdef _NDSTOOL_P_H
		if (sizeof(Header) != 0x200) { fprintf(stderr, "Header size %d != %d\n", sizeof(Header), 0x200); exit(1); }
	#endif

	if (argc < 2) { Help(); return 0; }

	// what to do
	bool extract = false;
	bool create = false;
	
	int status = 0;

	// parse parameters
	for (int a=1; a<argc; a++)
	{
		if (argv[a][0] == '-')
		{
			switch (argv[a][1])
			{
				case 'i':	// show information
				{
					ndsfilename = REQUIRED();
					romlistfilename = OPTIONAL();
					ShowInfo(ndsfilename);
					break;
				}

				case 'f':	// fix header CRC
				{
					ndsfilename = REQUIRED();
					FixHeaderCRC(ndsfilename);
					break;
				}

				case 's':	// en-/decrypt secure area
				{
					if (EncryptSecureArea)
					{
						ndsfilename = REQUIRED();
						EnDecryptSecureArea(ndsfilename);
					}
					break;
				}

				case 'p':	// PassMe
				{
					ndsfilename = REQUIRED();
					char *vhdfilename = OPTIONAL();
					char *sramfilename = OPTIONAL();
					status = PassMe(ndsfilename, vhdfilename, sramfilename);
					break;
				}

				case 'l':	// list files
				{
					ndsfilename = REQUIRED();
					ExtractFiles(ndsfilename);
					break;
				}

				///

				case 'x':	// extract
				{
					extract = true;
					ndsfilename = REQUIRED();
					while (1)
					{
						if (!(filemasks[filemask_num] = OPTIONAL())) break;
						if (++filemask_num >= MAX_FILEMASKS) return 1;
					}
					if (!ndsfilename) return 1;
					break;
				}

				case 'c':	// create
				{
					create = true;
					ndsfilename = REQUIRED();
					if (!ndsfilename) return 1;
					break;
				}

				// create/extract options
				case 'd': filerootdir = REQUIRED(); break;
				case '7': arm7filename = REQUIRED(); break;
				case '9': arm9filename = REQUIRED(); break;

				// hash file
				case '@':
				{
					unsigned char sha1[SHA1_DIGEST_SIZE];
					char *filename = REQUIRED();
					int r = HashAndCompareWithList(filename, sha1);
					status = -1;
					if (r > 0)
					{
						for (int i=0; i<SHA1_DIGEST_SIZE; i++) printf("%02X", sha1[i]);
						printf("\n");
						status = 0;
					}
					break;
				}

				case 't':
					bannerfilename = REQUIRED();
					bannertype = BANNER_BINARY;
					break;

				case 'b':
					bannertype = BANNER_IMAGE;
					bannerfilename = REQUIRED();
					bannertext = REQUIRED();
					break;

				case 'o':
					logofilename = REQUIRED();
					break;

				case 'h':	// load header
					headerfilename = REQUIRED();
					break;

				case 'u':	// unique ID file
					uniquefilename = REQUIRED();
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
					makercode = REQUIRED();
					if (strlen(makercode) != 2)
					{
						fprintf(stderr, "Maker code must be 2 characters!\n");
						exit(1);
					}
					break;

				case 'g':	// game code
					gamecode = REQUIRED();
					if (strlen(gamecode) != 4) {
						fprintf(stderr, "Game code must be 4 characters!\n");
						exit(1);
					}
					for (int i=0; i<4; i++) if ((gamecode[a] >= 'a') && (gamecode[a] <= 'z'))
					{
						fprintf(stderr, "Warning: Gamecode contains lowercase characters.\n");
						break;
					}
					if (gamecode[a] == 'A')
					{
						fprintf(stderr, "Warning: Gamecode starts with 'A', which might be used for another commercial product.\n");
						break;
					}
					break;

				case 'y':	// overlay table file / directory
					switch (argv[a][2])
					{
						case '7': arm7ovltablefilename = REQUIRED(); break;
						case '9': arm9ovltablefilename = REQUIRED(); break;
						case 0: overlaydir = REQUIRED(); break;
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

	Title();

	if (extract && create)
	{
		fprintf(stderr, "Cannot both extract and create!\n");
		return 1;
	}
	else if (extract)
	{
		if (arm9filename) Extract(arm9filename, true, 0x20, true, 0x2C, true);
		if (arm7filename) Extract(arm7filename, true, 0x30, true, 0x3C);
		if (bannerfilename) Extract(bannerfilename, true, 0x68, false, 0x840);
		if (headerfilename) Extract(headerfilename, false, 0x0, false, 0x200);
		if (arm9ovltablefilename) Extract(arm9ovltablefilename, true, 0x50, true, 0x54);
		if (arm7ovltablefilename) Extract(arm7ovltablefilename, true, 0x58, true, 0x5C);
		if (overlaydir) ExtractOverlayFiles();
		if (filerootdir) ExtractFiles(ndsfilename);
	}
	else if (create)
	{
		Create();
	}

	return (status < 0) ? 1 : 0;
}
