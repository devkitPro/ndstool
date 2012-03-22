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

#ifndef _ELF_H_
#define _ELF_H_

/* Expose fixed-width integral types. */
#include <stdint.h>

#include "little.h"

/* Types for use within ELF. */
typedef unsigned_int Elf32_Addr;  /* Unsigned program address. */
typedef unsigned_short Elf32_Half;  /* Unsigned medium integer.  */
typedef unsigned_int Elf32_Off;   /* Unsigned file offset.     */
typedef signed_int  Elf32_Sword; /* Signed large integer.     */
typedef unsigned_int Elf32_Word;  /* Unsigned large integer.   */

/* Identification indices. */
typedef enum
{
  EI_MAG0    = 0, /* File identification.    */
  EI_MAG1    = 1, /* File identification.    */
  EI_MAG2    = 2, /* File identification.    */
  EI_MAG3    = 3, /* File identification.    */
  EI_CLASS   = 4, /* File class.             */
  EI_DATA    = 5, /* Data encoding.          */
  EI_VERSION = 6, /* File version.           */
  EI_PAD     = 7, /* Start of padding bytes. */
  EI_NIDENT  = 16 /* Size of e_ident[].      */
} ELF_IDENT;

/* Header structure. */
typedef struct
{
  unsigned char e_ident[EI_NIDENT]; /* Identification bytes.       */
  Elf32_Half    e_type;             /* Object file type.           */
  Elf32_Half    e_machine;          /* Object architecture.        */
  Elf32_Word    e_version;          /* Object file version.        */
  Elf32_Addr    e_entry;            /* Object entry point.         */
  Elf32_Off     e_phoff;            /* Program header file offset. */
  Elf32_Off     e_shoff;            /* Section header file offset. */
  Elf32_Word    e_flags;            /* Processor-specific flags.   */
  Elf32_Half    e_ehsize;           /* ELF header size.            */
  Elf32_Half    e_phentsize;        /* Program header entry size.  */
  Elf32_Half    e_phnum;            /* Program header entries.     */
  Elf32_Half    e_shentsize;        /* Section header entry size.  */
  Elf32_Half    e_shnum;            /* Section header entries.     */
  Elf32_Half    e_shstrndx;         /* String table index.         */
} Elf32_Ehdr;

/* Program header structure. */
typedef struct
{
  Elf32_Word p_type;   /* Segment type.      */
  Elf32_Off  p_offset; /* File offset.       */
  Elf32_Addr p_vaddr;  /* Virtual address.   */
  Elf32_Addr p_paddr;  /* Physical address.  */
  Elf32_Word p_filesz; /* File image size.   */
  Elf32_Word p_memsz;  /* Memory image size. */
  Elf32_Word p_flags;  /* Segment flags.     */
  Elf32_Word p_align;  /* Alignment value.   */
} Elf32_Phdr;

/* Object file type. */
typedef enum
{
  ET_NONE   = 0,      /* No file type.       */
  ET_REL    = 1,      /* Relocatable file.   */
  ET_EXEC   = 2,      /* Executable file.    */
  ET_DYN    = 3,      /* Shared object file. */
  ET_CORE   = 4,      /* Core file.          */
} ELF_TYPE;

/* Object architecture. */
typedef enum
{
  EM_NONE  = 0, /* No machine.     */
  EM_M32   = 1, /* AT&T WE 32100.  */
  EM_SPARC = 2, /* SPARC.          */
  EM_386   = 3, /* Intel 80386.    */
  EM_68K   = 4, /* Motorola 68000. */
  EM_88K   = 5, /* Motorola 88000. */
  EM_860   = 7, /* Intel 80860.    */
  EM_MIPS  = 8, /* MIPS RS3000.    */
  EM_ARM   = 40 /* ARM.            */
} ELF_MACHINE;

/* Object file version. */
typedef enum
{
  EV_NONE    = 0, /* Invalid version. */
  EV_CURRENT = 1  /* Current version. */
} ELF_VERSION;

/* Magic number. */
#define ELF_MAGIC "\x7f" "ELF"

/* File class. */
typedef enum
{
  ELFCLASSNONE = 0, /* Invalid class.  */
  ELFCLASS32   = 1, /* 32-bit objects. */
  ELFCLASS64   = 2, /* 64-bit objects. */
} ELF_CLASS;

/* Data encoding. */
typedef enum
{
  ELFDATANONE = 0, /* Invalid data encoding. */
  ELFDATA2LSB = 1, /* Little endian.         */
  ELFDATA2MSB = 2, /* Big endian.            */
} ELF_DATA;

/* Program header segment type. */
typedef enum
{
  PT_NULL    = 0, /* Unused.                      */
  PT_LOAD    = 1, /* Loadable segment.            */
  PT_DYNAMIC = 2, /* Dynamic linking information. */
  PT_INTERP  = 3, /* Interpreter.                 */
  PT_NOTE    = 4, /* Auxiliary information.       */
  PT_SHLIB   = 5, /* Reserved.                    */
  PT_PHDR    = 6  /* Program header table.        */
} ELF_P_TYPE;

/* Program header flag. */
typedef enum
{
  PF_R = 4, /* Read flag.    */
  PF_W = 2, /* Write flag.   */
  PF_X = 1, /* Execute flag. */
} ELF_P_FLAG;

/* Function prototypes. */
int  CopyFromElf(char *elfFilename, 
                 unsigned int *entry,
                 unsigned int *ram_address,
                 unsigned int *size);
void ElfReadHdr(FILE *fp, Elf32_Ehdr *hdr, Elf32_Phdr **phdr);
void ElfWriteData(size_t n, FILE *in, FILE *out);
void ElfWriteZeros(size_t n, FILE *fp);

#endif
