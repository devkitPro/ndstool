#include "ndstool.h"


unsigned int free_file_id = 0x0000;
unsigned int _entry_start;	// current position in name entry table
unsigned int file_top;	// current position to write new file to
unsigned int free_dir_id = 0xF001;
unsigned int directory_count = 0;
unsigned int file_count = 0;
unsigned int total_name_size = 0;


void WalkTree(Tree *tree, char *prefix, unsigned int this_dir_id, unsigned int _parent_id)
{
	// skip dummy node
	tree = tree->next;

	// write directory info
	fseek(fNDS, header.fnt_offset + 8*(this_dir_id & 0xFFF), SEEK_SET);
	unsigned_int entry_start = _entry_start;	// reference location of entry name
	fwrite(&entry_start, 1, sizeof(entry_start), fNDS);
	unsigned int _top_file_id = free_file_id;
	unsigned_short top_file_id = _top_file_id;	// file ID of top entry 
	fwrite(&top_file_id, 1, sizeof(top_file_id), fNDS);
	unsigned_short parent_id = _parent_id;	// ID of parent directory or directory count (root)
	fwrite(&parent_id, 1, sizeof(parent_id), fNDS);

	//printf("%04X %04X+ ", this_dir_id, (int)top_file_id);
	printf("%s\n", prefix);

	char strbuf[MAXPATHLEN];

	// write names and allocate IDs
	fseek(fNDS, header.fnt_offset + _entry_start, SEEK_SET);
	for (Tree *t=tree; t; t=t->next)
	{
		int namelen = strlen(t->name);
		fputc(t->directory ? namelen | 128 : namelen, fNDS); _entry_start += 1;
		fwrite(t->name, 1, namelen, fNDS); _entry_start += namelen;
		if (t->directory)
		{
			//printf("[ %s -> %04X ]\n", t->name, t->dir_id);

			unsigned_short _dir_id_tmp = t->dir_id;
			fwrite(&_dir_id_tmp, 1, sizeof(_dir_id_tmp), fNDS);
			_entry_start += sizeof(_dir_id_tmp);
		}
		else
		{
			//printf("[ %s -> %04X ]\n", t->name, free_file_id);

			free_file_id++;
		}
	}
	fputc(0, fNDS); _entry_start += 1;	// end of directory

	// recurse
	for (Tree *t=tree; t; t=t->next)
	{
		if (t->directory)
		{
			strcpy(strbuf, prefix);
			strcat(strbuf, t->name);
			strcat(strbuf, "/");
			WalkTree(t->directory, strbuf, t->dir_id, this_dir_id);
		}
		else
		{
			file_top = (file_top + 0x1FF) &~ 0x1F;		// align to 512 bytes
			fseek(fNDS, file_top, SEEK_SET);

			// open file
			strcpy(strbuf, filerootdir);
			strcat(strbuf, prefix);
			strcat(strbuf, t->name);
			//printf("%04X %s\n", _top_file_id, strbuf);
			FILE *fi = fopen(strbuf, "rb");
			if (!fi) { fprintf(stderr, "Cannot open file '%s'.\n", strbuf); exit(1); }
			fseek(fi, 0, SEEK_END);
			unsigned int size = ftell(fi);
			unsigned int file_bottom = file_top + size;
			fseek(fi, 0, SEEK_SET);

			// write data
			while (size > 0)
			{
				unsigned char copybuf[1024];
				unsigned int size2 = (size >= sizeof(copybuf)) ? sizeof(copybuf) : size;
				fread(copybuf, 1, size2, fi);
				fwrite(copybuf, 1, size2, fNDS);
				size -= size2;
			}
			fclose(fi);

			// write fat
			fseek(fNDS, header.fat_offset + 8*_top_file_id, SEEK_SET);
			unsigned_int top = file_top;
			fwrite(&top, 1, sizeof(top), fNDS);
			unsigned_int bottom = file_bottom;
			fwrite(&bottom, 1, sizeof(bottom), fNDS);
			
			_top_file_id++;
			file_top = file_bottom;
		}
	}
}


/*
 * ReadDirectory
 */
void ReadDirectory(Tree *tree, char *path)
{
	directory_count++;

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

		tree->next = new Tree();
		tree = tree->next;
		tree->name = strdup(de->d_name);

		total_name_size += strlen(de->d_name);

		if (S_ISDIR(st.st_mode))
		{
			//strcat(strbuf, "/");
			tree->dir_id = free_dir_id++;
			tree->directory = new Tree();
			ReadDirectory(tree->directory, strbuf);
		}
		else if (S_ISREG(st.st_mode))
		{
			//tree->id = free_file_id++;
			//printf("%s\n", path);
			//file_id++;
			file_count++;
		}
	}
	closedir(dir);
}
