/*
	Nintendo DS rom tool
	by Rafael Vuijk (aka DarkFader)
*/

#include <unistd.h>
#include "ndstool.h"
#include "ndstool_version.h"
#include "sha1.h"
#include "ndscreate.h"
#include "ndsextract.h"
#include "passme.h"
#include "hook.h"
#include "encryption.h"

/*
 * Variables
 */
int verbose = 0;
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
//bool compatibility = false;
char *headerfilename = 0;
//char *uniquefilename = 0;
char *logofilename = 0;
char *title = 0;
char *makercode = 0;
char *gamecode = 0;
char *vhdfilename = 0;
char *sramfilename = 0;
int latency1 = 0x1FFF;	//0x8F8;
int latency2 = 0x3F;	//0x18;
int romversion = 0;
char endecrypt_option = 0;

int bannertype;
unsigned int arm9RamAddress = 0;
unsigned int arm7RamAddress = 0;
unsigned int arm9Entry = 0;
unsigned int arm7Entry = 0;

const char CompileDate[] = __DATE__;
const char CompileTime[] = __TIME__;

/*
 * Title
 */
void Title()
{
	printf("Nintendo DS rom tool "VER" - %s %s by Rafael Vuijk (aka DarkFader)\n",CompileDate,CompileTime);
}

/*
 * Help data
 */
struct HelpLine
{
	char *line;
	char option_char;
	char *context;
};

HelpLine helplines[] =
{
	{"Parameter              Syntax                         Comments\n"},
	{"---------              ------                         --------\n"},
	{"Show this help:        -?[option]                     global or specific help\n"},
	{"Show information:      -i [file.nds]\n", 'i', "Header information. Use in conjuction with -v to show even more information such as CRC32 of the file and even look it up in the specified dat-file for ROM release information.\n"},
	{"Information options:                                  optional\n"},
	{"  Show more info       -v [roms_rc.dat]               checksums, warnings, release info\n"},
	{"PassMe:                -p [file.nds] [passme.vhd] [passme.sav]\n", 'p', "\n"},
	{"Hook ARM7 executable   -k [file.nds]                  see manual\n", 'k', "ask me for example\n"},
	{"Fix header CRC         -f [file.nds]\n", 'f', "Some commands do this automatically.\n"},
	//"Test                   -T [file.nds]\n"},
	{"En/decrypt secure area -s[e|E|d] [file.nds]           also adds testing data\n", 's', "En/decrypt the secure area and put/remove card encryption tables and test patterns. Optionally add: d for decryption, e for Nintendo offsets, E for most other cards.\n"},
	//"Hash file & compare:   -@ [arm7.bin]\n"},		// used in buildscript
	{"List files:            -l [file.nds]\n", 'l', "Gives a list of files in de NDS file. Use -v option to show more information such as offset and length.\n"},
	{"Create                 -c [file.nds]\n"},
	{"Extract                -x [file.nds]\n"},
	{"Create/Extract options:                               optional\n"},
	{"  Show more info       -v[v...]                       filenames etc.\n", 'v', "Use multiple v's for more information.\n"},
	//"\n"},
	{"  ARM9 executable      -9 file.bin\n"},
	{"  ARM7 executable      -7 file.bin\n"},
	{"  ARM9 overlay table   -y9 file.bin\n"},
	{"  ARM7 overlay table   -y7 file.bin\n"},
	{"  Data files           -d directory\n"},
	{"  Overlay files        -y directory\n"},
	{"  Banner bitmap/text   -b file.bmp \"text;text;text\"   3 lines max.\n", 'b', "The three lines are showed in the firmware. Seperate with a semicolon.\n"},
	{"  Banner binary        -t file.bin\n"},
	//"\n"},
	{"  Header template      -h file.bin\n", 'h', "You can copy the header from another ROM and use it as a basis.\n"},
	{"  Latency              -n [L1] [L2]                   default=maximum\n", 'n'},
	{"  Logo bitmap/binary   -o file.bmp/file.bin\n"},
	//{"  Maker code           -m code                        (deprecated, use -g)\n"},
	{"  Game info            -g gamecode [makercode] [short-title] [rom-version]\n", 'g', "Game code is 4 characters. Maker code is 2 characters. Title can be up to 12 characters.\n"},
	//"  unique ID filename  -u file.bin                    for homebrew, auto generated\n"},
	{"  ARM9 RAM address     -r9 address\n"},
	{"  ARM7 RAM address     -r7 address\n"},
	{"  ARM9 RAM entry       -e9 address\n"},
	{"  ARM7 RAM entry       -e7 address\n"},
	{"  Wildcard filemask(s) -w [filemask]...               * and ? are special\n"},
	{"Common options:\n"},
	{"  NDS filename         [file.nds]\n"},
	{"\n"},
	{"You can perform multiple actions. They are performed in specified order.\n"},
	{"You only need to specify the NDS filename once. This and other options can appear anywhere.\n"},
	{"Addresses can be prefixed with '0x' to use hexadecimal format.\n"},
};

/*
 * Help
 */
void Help(char *specificoption = 0)
{
	Title();
	printf("\n");

	if (specificoption)
	{
		HelpLine *hl = 0;
		for (unsigned int i=0; i<(sizeof(helplines) / sizeof(helplines[0])); i++)
		{
			if (helplines[i].option_char == *specificoption) { hl = helplines + i; break; }
		}
		if (hl)
		{
			puts(hl->line);
			puts(hl->context);
		}
		else
		{
			printf("Unknown option: %s\n\n", specificoption);
		}
	}
	else
	{
		for (unsigned int i=0; i<(sizeof(helplines) / sizeof(helplines[0])); i++)
		{
			printf(helplines[i].line);
		}
	}
}


#define REQUIRED(var)	var = ((argc > a+1) ? argv[++a] : 0)								// must precede OPTIONAL
#define OPTIONAL(var)	{ /*fprintf(stderr, "'%s'\n", argv[a]);*/ char *t = ((argc > a+1) && (argv[a+1][0] != '-') ? argv[++a] : 0); if (!var) var = t; else if (t) fprintf(stderr, "%s is already specified!\n", #var); }		// final paramter requirement checks are done when performing actions
#define OPTIONAL_INT(var)	{ char *t = ((argc > a+1) && (argv[a+1][0] != '-') ? argv[++a] : 0); if (t) var = strtoul(t,0,0); }		// like OPTIONAL, but for (positive) integers
#define MAX_ACTIONS		32
#define ADDACTION(a)	{ if (num_actions < MAX_ACTIONS) actions[num_actions++] = a; }

enum {
	ACTION_SHOWINFO,
	ACTION_FIXHEADERCRC,
	ACTION_ENCRYPTSECUREAREA,
	ACTION_PASSME,
	ACTION_LISTFILES,
	ACTION_EXTRACT,
	ACTION_CREATE,
	ACTION_HASHFILE,
	ACTION_HOOK,
};

/*
 * main
 */
int main(int argc, char *argv[])
{
	#ifdef _NDSTOOL_P_H
		if (sizeof(Header) != 0x200) { fprintf(stderr, "Header size %d != %d\n", sizeof(Header), 0x200); return 1; }
	#endif

	if (argc < 2) { Help(); return 0; }
	
	int num_actions = 0;
	int actions[MAX_ACTIONS];

	/*
	 * parse parameters to actions
	 */

	for (int a=1; a<argc; a++)
	{
		if (argv[a][0] == '-')
		{
			switch (argv[a][1])
			{
				case 'i':	// show information
				{
					ADDACTION(ACTION_SHOWINFO);
					OPTIONAL(ndsfilename);
					break;
				}

				case 'f':	// fix header CRC
				{
					ADDACTION(ACTION_FIXHEADERCRC);
					OPTIONAL(ndsfilename);
					break;
				}

				case 's':	// en-/decrypt secure area
				{
					ADDACTION(ACTION_ENCRYPTSECUREAREA);
					endecrypt_option = argv[a][2];
					OPTIONAL(ndsfilename);
					break;
				}

				case 'p':	// PassMe
				{
					ADDACTION(ACTION_PASSME);
					OPTIONAL(ndsfilename);
					OPTIONAL(vhdfilename);
					OPTIONAL(sramfilename);
					break;
				}

				case 'l':	// list files
				{
					ADDACTION(ACTION_LISTFILES);
					OPTIONAL(ndsfilename);
					break;
				}

				case 'x':	// extract
				{
					ADDACTION(ACTION_EXTRACT);
					OPTIONAL(ndsfilename);
					break;
				}
				
				case 'w':	// wildcard filemasks
				{
					while (1)
					{
						char *filemask = 0;
						OPTIONAL(filemask);
						if (!(filemasks[filemask_num] = filemask)) break;
						if (++filemask_num >= MAX_FILEMASKS) return 1;
					}
					break;
				}

				case 'c':	// create
				{
					ADDACTION(ACTION_CREATE);
					OPTIONAL(ndsfilename);
					break;
				}

				// file root directory
				case 'd': REQUIRED(filerootdir); break;

				// ARM7 filename
				case '7': REQUIRED(arm7filename); break;

				// ARM9 filename
				case '9': REQUIRED(arm9filename); break;

				// hash file
				case '@':
				{
					ADDACTION(ACTION_HASHFILE);
					OPTIONAL(arm7filename);
					break;
				}

				// hook ARM7 executable
				case 'k':
				{
					ADDACTION(ACTION_HOOK);
					OPTIONAL(ndsfilename);
					break;
				}

				case 't':
					REQUIRED(bannerfilename);
					bannertype = BANNER_BINARY;
					break;

				case 'b':
					bannertype = BANNER_IMAGE;
					REQUIRED(bannerfilename);
					REQUIRED(bannertext);
					break;

				case 'o':
					REQUIRED(logofilename);
					break;

				case 'h':	// load header
					REQUIRED(headerfilename);
					break;

				/*case 'u':	// unique ID file
					REQUIRED(uniquefilename);
					break;*/

				case 'v':	// verbose
					for (char *p=argv[a]; *p; p++) if (*p == 'v') verbose++;
					OPTIONAL(romlistfilename);
					break;

				case 'n':	// latency
					//compatibility = true;
					OPTIONAL_INT(latency1);
					OPTIONAL_INT(latency2);
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
					REQUIRED(makercode);
					break;

				case 'g':	// game code
					REQUIRED(gamecode);
					OPTIONAL(makercode);
					OPTIONAL(title);
					OPTIONAL_INT(romversion);
					break;

				case 'y':	// overlay table file / directory
					switch (argv[a][2])
					{
						case '7': REQUIRED(arm7ovltablefilename); break;
						case '9': REQUIRED(arm9ovltablefilename); break;
						case 0: REQUIRED(overlaydir); break;
						default: Help(argv[a]); return 1;
					}
					break;
				
				case '?':	// global or specific help
				{
					Help(argv[a][2] ? argv[a]+2 : 0);	// 0=global help
					return 0;	// do not perform any other actions
				}

				default:
				{
					Help(argv[a]);
					return 1;
				}
			}
		}
		else
		{
			//Help();
			if (ndsfilename)
			{
				fprintf(stderr, "NDS filename is already given!\n");
				return 1;
			}
			ndsfilename = argv[a];
			break;
		}
	}

	Title();

	/*
	 * sanity checks
	 */

	if (gamecode)
	{
		if (strlen(gamecode) != 4)
		{
			fprintf(stderr, "Game code must be 4 characters!\n");
			return 1;
		}
		for (int i=0; i<4; i++) if ((gamecode[i] >= 'a') && (gamecode[i] <= 'z'))
		{
			fprintf(stderr, "Warning: Gamecode contains lowercase characters.\n");
			break;
		}
		if (gamecode[0] == 'A')
		{
			fprintf(stderr, "Warning: Gamecode starts with 'A', which might be used for another commercial product.\n");
		}
	}
	if (makercode && (strlen(makercode) != 2))
	{
		fprintf(stderr, "Maker code must be 2 characters!\n");
		return 1;
	}
	if (title && (strlen(title) > 12))
	{
		fprintf(stderr, "Title can be no more than 12 characters!\n");
		return 1;
	}

	/*
	 * perform actions
	 */

	int status = 0;
	for (int i=0; i<num_actions; i++)
	{
//printf("action %d\n", actions[i]);
		switch (actions[i])
		{
			case ACTION_SHOWINFO:
				ShowInfo(ndsfilename);
				break;

			case ACTION_FIXHEADERCRC:
				FixHeaderCRC(ndsfilename);
				break;

			case ACTION_EXTRACT:
				if (arm9filename) Extract(arm9filename, true, 0x20, true, 0x2C, true);
				if (arm7filename) Extract(arm7filename, true, 0x30, true, 0x3C);
				if (bannerfilename) Extract(bannerfilename, true, 0x68, false, 0x840);
				if (headerfilename) Extract(headerfilename, false, 0x0, false, 0x200);
				if (logofilename) Extract(logofilename, false, 0xC0, false, 156);	// *** bin only
				if (arm9ovltablefilename) Extract(arm9ovltablefilename, true, 0x50, true, 0x54);
				if (arm7ovltablefilename) Extract(arm7ovltablefilename, true, 0x58, true, 0x5C);
				if (overlaydir) ExtractOverlayFiles();
				if (filerootdir) ExtractFiles(ndsfilename);
				break;

			case ACTION_CREATE:
				//if (+ 0x800) compatibility = true;
				/*status =*/ Create();
				break;

			case ACTION_PASSME:
				status = PassMe(ndsfilename, vhdfilename, sramfilename);
				break;

			case ACTION_LISTFILES:
				filerootdir = 0;
				/*status =*/ ExtractFiles(ndsfilename);
				break;

			case ACTION_HASHFILE:
			{
				char *filename = arm7filename;
				if (!filename) filename = ndsfilename;
				if (!filename) return 1;
				unsigned char sha1[SHA1_DIGEST_SIZE];
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
			
			case ACTION_HOOK:
			{
				Hook(ndsfilename, arm7filename);
				break;
			}

			case ACTION_ENCRYPTSECUREAREA:
			{
				/*status =*/ EnDecryptSecureArea(ndsfilename, endecrypt_option);
				break;
			}
		}
	}

	return (status < 0) ? 1 : 0;
}
