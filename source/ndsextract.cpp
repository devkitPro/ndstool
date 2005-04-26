#include "ndstool.h"

/*
 * ExtractFile
 * if filename==0 nothing will be written
 */
void ExtractFile(char *filename, unsigned int file_id)
{
	unsigned int save_filepos = ftell(fNDS);
	
	// read FAT data
	fseek(fNDS, header.fat_offset + 8*file_id, SEEK_SET);
	unsigned_int top;
	fread(&top, 1, sizeof(top), fNDS);
	unsigned_int bottom;
	fread(&bottom, 1, sizeof(bottom), fNDS);
	unsigned int size = bottom - top;

	// print file size
	if (!filerootdir || verbose)
	{
		printf("%8u\n", size);
	}

	// extract file
	if (filename)
	{
		fseek(fNDS, top, SEEK_SET);
		FILE *fo = fopen(filename, "wb");
		if (!fo) { fprintf(stderr, "Cannot create file '%s'.\n", filename); exit(1); }
		while (size > 0)
		{
			unsigned char copybuf[1024];
			unsigned int size2 = (size >= sizeof(copybuf)) ? sizeof(copybuf) : size;
			fread(copybuf, 1, size2, fNDS);
			fwrite(copybuf, 1, size2, fo);
			size -= size2;
		}
		fclose(fo);
	}
	
	fseek(fNDS, save_filepos, SEEK_SET);	
}

/*
 * ExtractDirectory
 * filerootdir can be 0 for just listing files
 */
void ExtractDirectory(char *prefix, unsigned int dir_id)
{
	char strbuf[MAXPATHLEN];
	unsigned int save_filepos = ftell(fNDS);

	fseek(fNDS, header.fnt_offset + 8*(dir_id & 0xFFF), SEEK_SET);
	unsigned_int entry_start;	// reference location of entry name
	fread(&entry_start, 1, sizeof(entry_start), fNDS);
	unsigned_short top_file_id;	// file ID of top entry 
	fread(&top_file_id, 1, sizeof(top_file_id), fNDS);
	unsigned_short parent_id;	// ID of parent directory or directory count (root)
	fread(&parent_id, 1, sizeof(parent_id), fNDS);

	fseek(fNDS, header.fnt_offset + entry_start, SEEK_SET);

	// print directory name
	//printf("%04X ", dir_id);
	if (!filerootdir || verbose)
	{
		printf("%s\n", prefix);
	}

	for (unsigned int file_id=top_file_id; ; file_id++)
	{
		unsigned char entry_type_name_length;
		fread(&entry_type_name_length, 1, sizeof(entry_type_name_length), fNDS);
		unsigned int name_length = entry_type_name_length & 127;
		bool entry_type_directory = (entry_type_name_length & 128) ? true : false;
		if (name_length == 0) break;
	
		char entry_name[128];
		memset(entry_name, 0, 128);
		fread(entry_name, 1, entry_type_name_length & 127, fNDS);
		if (entry_type_directory)
		{
			unsigned_short dir_id;
			fread(&dir_id, 1, sizeof(dir_id), fNDS);

			if (filerootdir)
			{
				strcpy(strbuf, filerootdir);
				strcat(strbuf, prefix);
				strcat(strbuf, entry_name);

#ifdef __MINGW32__
				if (mkdir(strbuf))
#else
				if (mkdir(strbuf, S_IRWXU))
#endif
				{
					fprintf(stderr, "Cannot create directory '%s'.\n", strbuf);
					exit(1);
				}
			}

			strcpy(strbuf, prefix);
			strcat(strbuf, entry_name);
			strcat(strbuf, "/");
			ExtractDirectory(strbuf, dir_id);
		}
		else
		{
			if (!filerootdir || verbose)
			{
				//printf("%04X ", file_id);
				printf("%s%s", prefix, entry_name);
				int len = strlen(prefix) + strlen(entry_name);
				for (; len<70; len++) putchar(' ');
			}

			if (filerootdir)
			{
				strcpy(strbuf, filerootdir);
				strcat(strbuf, prefix);
				strcat(strbuf, entry_name);
				ExtractFile(strbuf, file_id);
			}
			else
			{
				ExtractFile(0, file_id);
			}
		}
	}

	fseek(fNDS, save_filepos, SEEK_SET);
}

/*
 * ExtractFiles
 */
void ExtractFiles()
{
	fNDS = fopen(ndsfilename, "rb");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	fread(&header, 512, 1, fNDS);

	if (filerootdir)
	{
#ifdef __MINGW32__
		if (mkdir(filerootdir))
#else
		if (mkdir(filerootdir, S_IRWXU))
#endif
		{
			fprintf(stderr, "Cannot create directory '%s'.\n", filerootdir);
			exit(1);
		}
	}

	ExtractDirectory("/", 0xF000);

	fclose(fNDS);
}

/*
 * Extract
 */
void Extract(char *outfilename, bool indirect_offset, unsigned int offset, bool indirect_size, unsigned size)
{
	fNDS = fopen(ndsfilename, "rb");
	if (!fNDS) { fprintf(stderr, "Cannot open file '%s'.\n", ndsfilename); exit(1); }
	fread(&header, 512, 1, fNDS);

	if (indirect_offset) offset = *((unsigned int *)&header + offset/4);
	if (indirect_size) size = *((unsigned int *)&header + size/4);
	
	fseek(fNDS, offset, SEEK_SET);

	FILE *fo = fopen(outfilename, "wb");
	if (!fo) { fprintf(stderr, "Cannot create file '%s'.\n", outfilename); exit(1); }
	
	unsigned char copybuf[1024];
	while (size > 0)
	{
		unsigned int size2 = (size >= sizeof(copybuf)) ? sizeof(copybuf) : size;
		fread(copybuf, 1, size2, fNDS);
		fwrite(copybuf, 1, size2, fo);
		size -= size2;
	}

	fclose(fo);
	fclose(fNDS);
}
