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

#define VER			"1.10"

static const unsigned int defaultArm7entry = 0x03800000;
static const unsigned int defaultArm9entry = 0x02000000;

struct Tree
{
	unsigned int dir_id;	// directory IDs are allocated first
	char *name;
	Tree *directory;		// nonzero indicates directory. first entry in directory is dummy.
	Tree *next;
	
	Tree()
	{
		dir_id = 0;
		name = 0;
		directory = 0;
		next = 0;
	}
};

enum { BINARY, IMAGE };

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
extern char *icontitlefilename;
extern char *icontitlename;
extern char *icontitletext;
extern int icontype;
extern char *headerfilename;

// ndscreate
unsigned int WalkTree(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id);
void ReadDirectory(Tree *tree, char *path);
void Create();

// ndsextract.cpp
void ExtractFile(char *filename, unsigned int file_id);
void ExtractDirectory(char *prefix, unsigned int dir_id);
void ExtractFiles();
void Extract(char *outfilename, bool indirect_offset, unsigned int offset, bool indirect_size, unsigned size);

// banner.cpp
int InsertTitleString(char *String, FILE *file);
void IconFromBMP();
