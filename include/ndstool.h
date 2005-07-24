#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <libelf.h>
#include "little.h"
#include "header.h"

#define VER			"1.21"

#define ROMTYPE_HOMEBREW	0
#define ROMTYPE_MULTIBOOT	1
#define ROMTYPE_NDSDUMPED	2	// decrypted secure area
#define ROMTYPE_ENCRSECURE	3
#define ROMTYPE_MASKROM		4	// unknown layout

enum { BANNER_BINARY, BANNER_IMAGE };

struct Tree
{
	unsigned int dir_id;	// directory ID in case of directory entry
	char *name;				// file or directory name
	Tree *directory;		// nonzero indicates directory. first tree node is a dummy
	Tree *next;				// linked list
	
	Tree()
	{
		dir_id = 0;
		name = 0;
		directory = 0;
		next = 0;
	}
};

extern unsigned int free_dir_id;
extern unsigned int directory_count;
extern unsigned int file_count;
extern unsigned int total_name_size;

extern unsigned int free_file_id;
extern unsigned int _entry_start;
extern unsigned int file_top;

extern bool verbose;
extern Header header;
extern FILE *fNDS;
extern char *ndsfilename;
extern char *arm7filename;
extern char *arm9filename;
extern char *filerootdir;
extern char *overlaydir;
extern char *arm7ovltablefilename;
extern char *arm9ovltablefilename;
extern char *bannerfilename;
extern char *bannertext;
extern int bannertype;
extern char *headerfilename;
extern char *uniquefilename;
extern char *logofilename;
extern unsigned int arm9RamAddress;
extern unsigned int arm7RamAddress;
extern unsigned int arm9Entry;
extern unsigned int arm7Entry;
extern char *makercode;
extern char *gamecode;

// ndscreate.cpp
Tree *ReadDirectory(Tree *tree, char *path);
void AddFile(char *rootdir, char *prefix, char *entry_name, unsigned int file_id, unsigned int alignmask);
void AddDirectory(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id, unsigned int alignmask);
void Create();

// ndsextract.cpp
void ExtractFiles();
void ExtractOverlayFiles();
void Extract(char *outfilename, bool indirect_offset, unsigned int offset, bool indirect_size, unsigned size, bool with_footer = false);

// ndstree.cpp
extern unsigned int free_file_id;
extern unsigned int file_end;
