#include "ndstool.h"

/*
 * Variables
 */
unsigned int _entry_start;			// current position in name entry table
unsigned int file_top;				// current position to write new file to
unsigned int free_dir_id = 0xF000;	// incremented in ReadDirectory
unsigned int directory_count = 0;	// incremented in ReadDirectory
unsigned int file_count = 0;		// incremented in ReadDirectory
unsigned int total_name_size = 0;	// incremented in ReadDirectory
unsigned int file_end = 0;			// end of all file data. updated in AddFile
unsigned int free_file_id = 0;		// incremented in AddDirectory

/*
 * ReadDirectory
 * Read directory tree into memory structure
 * returns last directory entry
 */
Tree *ReadDirectory(Tree *tree, char *path)
{
	//printf("%s\n", path);

	DIR *dir = opendir(path);
	if (!dir) { fprintf(stderr, "Cannot open directory '%s'.\n", path); exit(1); }

	struct dirent *de;
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".")) continue;
		if (!strcmp(de->d_name, "..")) continue;

		char strbuf[MAXPATHLEN];
		strcpy(strbuf, path);
		strcat(strbuf, "/");
		strcat(strbuf, de->d_name);
		//printf("%s\n", strbuf);

		struct stat st;
		if (stat(strbuf, &st)) { fprintf(stderr, "Cannot get stat of '%s'.\n", strbuf); exit(1); }

		//if (S_ISDIR(st.st_mode) && !subdirs) continue;		// skip subdirectories

		tree->next = new Tree();
		tree = tree->next;
		tree->name = strdup(de->d_name);
		total_name_size += strlen(de->d_name);

		if (S_ISDIR(st.st_mode))
		{
			tree->dir_id = free_dir_id++;
			tree->directory = new Tree();
			directory_count++;
			ReadDirectory(tree->directory, strbuf);
		}
		else if (S_ISREG(st.st_mode))
		{
			file_count++;
		}
		else
		{
			fprintf(stderr, "'%s' is not a file or directory!\n", strbuf);
			exit(1);
		}
	}
	closedir(dir);
	
	return tree;
}

/*
 * AddFile
 */
void AddFile(char *rootdir, char *prefix, char *entry_name, unsigned int file_id, unsigned int alignmask)
{
	// make filename
	char strbuf[MAXPATHLEN];
	strcpy(strbuf, rootdir);
	strcat(strbuf, prefix);
	strcat(strbuf, entry_name);

	//unsigned int file_end = ftell(fNDS);

	file_top = (file_top + alignmask) &~ alignmask;		// align to alignmask+1 bytes
	fseek(fNDS, file_top, SEEK_SET);


	FILE *fi = fopen(strbuf, "rb");
	if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", strbuf); exit(1); }
	fseek(fi, 0, SEEK_END);
	unsigned int size = ftell(fi);
	unsigned int file_bottom = file_top + size;
	fseek(fi, 0, SEEK_SET);

	// print
	if (verbose)
	{
		printf("%5u 0x%08X 0x%08X %9u %s%s\n", file_id, file_top, file_bottom, size, prefix, entry_name);
	}

	// write data
	unsigned int sizeof_copybuf = 256*1024;
	unsigned char *copybuf = new unsigned char [sizeof_copybuf];
	while (size > 0)
	{
		unsigned int size2 = (size >= sizeof_copybuf) ? sizeof_copybuf : size;
		fread(copybuf, 1, size2, fi);
		fwrite(copybuf, 1, size2, fNDS);
		size -= size2;
	}
	delete [] copybuf;
	fclose(fi);
	if ((unsigned int)ftell(fNDS) > file_end) file_end = ftell(fNDS);

	// write fat
	fseek(fNDS, header.fat_offset + 8*file_id, SEEK_SET);
	unsigned_int top = file_top;
	fwrite(&top, 1, sizeof(top), fNDS);
	unsigned_int bottom = file_bottom;
	fwrite(&bottom, 1, sizeof(bottom), fNDS);
	
	file_top = file_bottom;
}

/*
 * AddDirectory
 * Walks the tree and adds files to NDS
 */
void AddDirectory(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id, unsigned int alignmask)
{
	// skip dummy node
	tree = tree->next;

	if (verbose) printf("%s\n", prefix);

	// write directory info
	fseek(fNDS, header.fnt_offset + 8*(this_dir_id & 0xFFF), SEEK_SET);
	unsigned_int entry_start = _entry_start;	// reference location of entry name
	fwrite(&entry_start, 1, sizeof(entry_start), fNDS);
	unsigned int _top_file_id = free_file_id;
	unsigned_short top_file_id = _top_file_id;	// file ID of top entry 
	fwrite(&top_file_id, 1, sizeof(top_file_id), fNDS);
	unsigned_short parent_id = _parent_id;	// ID of parent directory or directory count (root)
	fwrite(&parent_id, 1, sizeof(parent_id), fNDS);

	//printf("dir %X file_id %u +\n", this_dir_id, (int)top_file_id);

	// directory entrynames
	{
		// start of directory entrynames
		fseek(fNDS, header.fnt_offset + _entry_start, SEEK_SET);
	
		// write filenames
		for (Tree *t=tree; t; t=t->next)
		{
			if (!t->directory)
			{
				int namelen = strlen(t->name);
				fputc(t->directory ? namelen | 128 : namelen, fNDS); _entry_start += 1;
				fwrite(t->name, 1, namelen, fNDS); _entry_start += namelen;
	
				//printf("[ %s -> %u ]\n", t->name, free_file_id);
	
				free_file_id++;
			}
		}
	
		// write directorynames
		for (Tree *t=tree; t; t=t->next)
		{
			if (t->directory)
			{
				//printf("*entry %s\n", t->name);
	
				int namelen = strlen(t->name);
				fputc(t->directory ? namelen | 128 : namelen, fNDS); _entry_start += 1;
				fwrite(t->name, 1, namelen, fNDS); _entry_start += namelen;
	
				//printf("[ %s -> %X ]\n", t->name, t->dir_id);
	
				unsigned_short _dir_id_tmp = t->dir_id;
				fwrite(&_dir_id_tmp, 1, sizeof(_dir_id_tmp), fNDS);
				_entry_start += sizeof(_dir_id_tmp);
			}
		}
		
		fputc(0, fNDS); _entry_start += 1;	// end of directory entrynames
	}

	// add files
	unsigned int file_id = _top_file_id;
	for (Tree *t=tree; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (!t->directory)
		{
			AddFile(filerootdir, prefix, t->name, file_id++, alignmask);
		}
	}

	// add subdirectories
	for (Tree *t=tree; t; t=t->next)
	{
		//printf("*2* %s\n", t->name);

		if (t->directory)
		{
			char strbuf[MAXPATHLEN];
			strcpy(strbuf, prefix);
			strcat(strbuf, t->name);
			strcat(strbuf, "/");
			AddDirectory(t->directory, strbuf, t->dir_id, this_dir_id, alignmask);
		}
	}
}
