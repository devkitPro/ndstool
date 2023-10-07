/*
 * Copyright (c) 2009 Forest Belton
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* System header files. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project header files. */
#include "ndstool.h"
#include "elf.h"

std::vector<std::pair<unsigned_int, unsigned_int>> overlay_fat_entries;

struct OvlTableEntry {
	unsigned_int overlay_id;
	unsigned_int ram_address;
	unsigned_int load_size;
	unsigned_int bss_size;
	unsigned_int ctors_start;
	unsigned_int ctors_end;
	unsigned_int file_id;
	unsigned_int reserved;
};

struct OvlTableEntryShort {
	unsigned_int ctors_start;
	unsigned_int ctors_end;
	unsigned_int reserved;
};

/* Simple assertion macro. */
#define die(...) \
	do {\
		fprintf(stderr, __VA_ARGS__);\
		exit(EXIT_FAILURE);\
	} while(0)

/* Function:    void ElfWriteData(size_t n, FILE *fp)
 * Description: Writes data from one file to another.
 * Parameters:  size_t n,   the number of bytes to write.
 *              FILE  *in,  the file pointer to read from.
 *              FILE  *out, the file pointer to write to.
 */
void ElfWriteData(size_t n, FILE *in, FILE *out) {
	unsigned char buffer[64*1024];

	while(n) {
		size_t cur_size = n > sizeof(buffer) ? sizeof(buffer) : n;

		size_t read = fread(buffer, 1, cur_size, in);
		if (read != cur_size)
			die("failed to read from input file\n");

		size_t written = fwrite(buffer, 1, cur_size, out);
		if (written != cur_size)
			die("failed to write to file\n");

		n -= cur_size;
	}
}

/* Function:    void ElfReadHdr(FILE *fp, Elf32_Ehdr *hdr)
 * Description: Read in the ELF header, and populate a list of program headers.
 * Parameters:  FILE        *fp,   the file pointer to read from.
 *              Elf32_Ehdr  *hdr,  a pointer to a header to fill.
 *              Elf32_Phdr **phdr, a pointer to pointer to place the list of
 *                                 program headers at.
 */
void ElfReadHdr(FILE *fp, Elf32_Ehdr *hdr, Elf32_Phdr **phdr) {
	/* Read in ELF header. */
	if(fread(hdr, 1, sizeof(Elf32_Ehdr), fp) != sizeof(Elf32_Ehdr))
		die("failed to read ELF header\n");

  	/* Check for magic number. */
	if(memcmp(&hdr->e_ident[EI_MAG0], ELF_MAGIC, 4))
		die("invalid ELF file\n");
  
	/* Check object type. */
	if(hdr->e_type != ET_EXEC)
		die("object type not executable\n");
  
	/* Check machine type. */
	if(hdr->e_machine != EM_ARM)
		die("machine type not ARM\n");
  
	/* Check ELF version. */
	if(hdr->e_version != EV_CURRENT)
		die("invalid ELF version\n");
  
	/* Check ELF size. */
	if(hdr->e_ehsize != sizeof(Elf32_Ehdr))
		die("invalid ELF header size\n");
  
	/* Make sure there is at least one program header. */
	if(!hdr->e_phnum)
		die("no program headers\n");
  
	/* Seek to program header table and read it. */
	if(fseek(fp, hdr->e_phoff, SEEK_SET))
		die("failed to seek to program header table\n");
  
	*phdr = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * hdr->e_phnum);
	if(!*phdr)
		die("failed to allocate memory\n");
  
	if(fread(*phdr, sizeof(Elf32_Phdr), hdr->e_phnum, fp) != hdr->e_phnum)
		die("failed to read program header table\n");
}

/* Function:    int CopyFromElf(char *elfFilename,         unsigned int *entry,
                                unsigned int *ram_address, unsigned int *size
                                bool is_twl)
 * Description: Copy the program data from an ELF file into fNDS.
 * Parameters:  char *elfFilename,         the filename of the ELF file to use.
 *              unsigned int *entry,       a pointer to place the entry point at.
 *              unsigned int *ram_address, a pointer to place the RAM address at.
 *              unsigned int *size,        a pointer to place the data size at.
 *              unsigned int *wram_address,a pointer to map DSi exclusive ARM7 WRAM at.
 *              bool *has_overlays,        a pointer to place the "has overlays" flag at.
 *              bool is_twl,               true if we want to copy TWL sections.
 */
int CopyFromElf(char *elfFilename,         unsigned int *entry,
                unsigned int *ram_address, unsigned int *size,
                unsigned int *wram_address, bool *has_overlays,
                bool is_twl)
{
	FILE        *in;
	Elf32_Ehdr   header;
	Elf32_Phdr  *p_headers;
	unsigned int i;
	unsigned int expected_address = 0;

	*ram_address = 0;
	if (has_overlays) *has_overlays = false;

	/* Open ELF file. */
	in = fopen(elfFilename, "rb");
	if(!in)
		die("failed to open input file: '%s'\n",elfFilename);

	/* Read in header. */
	ElfReadHdr(in, &header, &p_headers);
  
	if(entry) *entry = header.e_entry;
	*size  = 0;
  	/* Iterate over each program header. */
	for(i = 0; i < header.e_phnum; i++) {
		/* Skip non-loadable segments. */
		if(p_headers[i].p_type != PT_LOAD)
			continue;

		/* Skip overlay sections. */
		if(p_headers[i].p_flags&0x200000) {
			if (has_overlays) *has_overlays = true;
			continue;
		}

		/* Skip non-TWL/non-NTR sections. */
		if(is_twl == !(p_headers[i].p_flags&0x100000))
			continue;

		/* Detect address of DSi exclusive ARM7 WRAM bank. */
		if(wram_address && !*wram_address && p_headers[i].p_vaddr >= 0x03000000 && p_headers[i].p_vaddr < 0x037F8000)
			*wram_address = p_headers[i].p_vaddr;

		/* Skip BSS segments. */
		if(!p_headers[i].p_filesz)
			continue;

		/* Use first found address. */
		if(!*ram_address)
			*ram_address = p_headers[i].p_paddr;
		else if(p_headers[i].p_paddr != expected_address) {
			die("PHDR %u paddr expected at 0x%08X, got 0x%08X\n", i, expected_address, p_headers[i].p_paddr);
		}

		/* Seek to segment offset. */
		if(fseek(in, p_headers[i].p_offset, SEEK_SET))
			die("failed to seek to program header segment\n");

		/* Write file image. */
		ElfWriteData(p_headers[i].p_filesz, in, fNDS);

		*size += p_headers[i].p_filesz;
		expected_address = p_headers[i].p_paddr + p_headers[i].p_filesz;
	}
  
	/* Clean up. */
	free(p_headers);
	fclose(in);
  
	return 0;
}

void CopyOverlaysFromElf(const char* elfFilename, bool is_arm9)
{
	/* Open ELF file. */
	FILE* in = fopen(elfFilename, "rb");
	if(!in)
		die("failed to open input file: '%s'\n",elfFilename);

	/* Read in header. */
	Elf32_Ehdr elf_header;
	Elf32_Phdr* p_headers;
	ElfReadHdr(in, &elf_header, &p_headers);

	int ovl_id = -1;
	unsigned num_ovls = 0;
	OvlTableEntry* ovl_table = NULL;

	uint32_t cur_offset = (ftell(fNDS) + 0x1ff) &~ 0x1ff;
	uint32_t table_offset = cur_offset;

	/* Iterate over each program header. */
	for (unsigned i = 0; i < elf_header.e_phnum; i ++) {
		/* Skip non-loadable segments. */
		if (p_headers[i].p_type != PT_LOAD)
			continue;

		/* Skip non-overlay sections. */
		if (!(p_headers[i].p_flags&0x200000))
			continue;

		uint32_t cur_size = 0;

		if (ovl_id < 0) {
			num_ovls = p_headers[i].p_filesz / sizeof(OvlTableEntryShort);
			if (!num_ovls) die("overlay table is empty!\n");

			ovl_table = (OvlTableEntry*)calloc(num_ovls, sizeof(OvlTableEntry));
			if (!ovl_table) die("out of mem\n");

			OvlTableEntryShort* tbl_short = (OvlTableEntryShort*)malloc(p_headers[i].p_filesz);
			if (!tbl_short) die("out of mem\n");

			/* Seek to segment offset. */
			if (fseek(in, p_headers[i].p_offset, SEEK_SET))
				die("failed to seek to overlay table data\n");

			if (fread(tbl_short, p_headers[i].p_filesz, 1, in) != 1)
				die("failed to read overlay table\n");

			for (unsigned j = 0; j < num_ovls; j ++) {
				ovl_table[j].overlay_id = j;
				ovl_table[j].ctors_start = tbl_short[j].ctors_start;
				ovl_table[j].ctors_end = tbl_short[j].ctors_end;
				ovl_table[j].reserved = tbl_short[j].reserved;
			}

			free(tbl_short);

			cur_size = num_ovls*sizeof(OvlTableEntry);
			if (is_arm9) {
				header.arm9_overlay_offset = table_offset;
				header.arm9_overlay_size = cur_size;
			} else {
				header.arm7_overlay_offset = table_offset;
				header.arm7_overlay_size = cur_size;
			}

		} else if (ovl_id < num_ovls) {
			ovl_table[ovl_id].ram_address = p_headers[i].p_vaddr;
			ovl_table[ovl_id].load_size = p_headers[i].p_filesz;
			ovl_table[ovl_id].bss_size = p_headers[i].p_memsz - p_headers[i].p_filesz;

			if (p_headers[i].p_filesz) {
				/* Add FAT entry */
				cur_size = p_headers[i].p_filesz;
				ovl_table[ovl_id].file_id = overlay_fat_entries.size();
				overlay_fat_entries.emplace_back(cur_offset, cur_offset + cur_size);

				/* Seek to segment offset. */
				if(fseek(in, p_headers[i].p_offset, SEEK_SET))
					die("failed to seek to overlay %d data\n", ovl_id);

				/* Write file image. */
				fseek(fNDS, cur_offset, SEEK_SET);
				ElfWriteData(p_headers[i].p_filesz, in, fNDS);
			}
		} else
			die("too many overlay PHDRs\n");

		cur_offset += cur_size;
		cur_offset = (cur_offset + 0x1ff) &~ 0x1ff;

		ovl_id ++;
	}

	if (ovl_table) {
		fseek(fNDS, table_offset, SEEK_SET);
		fwrite(ovl_table, sizeof(OvlTableEntry), num_ovls, fNDS);
		fseek(fNDS, cur_offset, SEEK_SET);
	}

	/* Clean up. */
	free(ovl_table);
	free(p_headers);
	fclose(in);
}
