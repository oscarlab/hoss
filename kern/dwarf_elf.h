#ifndef ELF_H
#define ELF_H

#define ELF_MAGIC 0x464C457FU   /* "\x7FELF" in little endian */

#define EI_NIDENT 16
#define EI_DATA 5

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ELFCLASSNONE 0 
#define ELFCLASS32 1 
#define ELFCLASS64 2 

extern char *elf_base_ptr;

#define X86_64 1

#ifdef X86_64

typedef struct _Elf64 {
    //uint32_t e_magic;   //TODO: For Mr. VVkulkarni. must equal ELF_MAGIC
    //uint8_t e_elf[12];
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
}Elf64;

typedef struct _Proghdr64 {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_va;
    uint64_t p_pa;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
}Proghdr64;

typedef struct {
               uint32_t   sh_name;
               uint32_t   sh_type;
               uint64_t   sh_flags;
               uint64_t   sh_addr;
               uint64_t   sh_offset;
               uint64_t   sh_size;
               uint32_t   sh_link;
               uint32_t   sh_info;
               uint64_t   sh_addralign;
               uint64_t   sh_entsize;
} Secthdr64;

#define Elf Elf64
#define Proghdr Proghdr64
#define Secthdr Secthdr64

#else

typedef struct _Elf32 {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
}Elf32;

typedef struct Proghdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint32_t p_offset;
    uint32_t p_va;
    uint32_t p_pa;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_align;
}Proghdr32;

typedef struct Secthdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
}Secthdr32;

#define Elf Elf32
#define Proghdr Proghdr32
#define Secthdr Secthdr32

#endif

// Values for Proghdr::p_type
#define ELF_PROG_LOAD       1

// Flag bits for Proghdr::p_flags
#define ELF_PROG_FLAG_EXEC  1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ  4

// Values for Secthdr::sh_type
#define ELF_SHT_NULL        0
#define ELF_SHT_PROGBITS    1
#define ELF_SHT_SYMTAB      2
#define ELF_SHT_STRTAB      3

// Values for Secthdr::sh_name
#define ELF_SHN_UNDEF       0

/*
extern Secthdr* secthdr_ptr[];

#define SEC_PTR(index) (Secthdr *)secthdr_ptr[index] //(((Secthdr *)(elf_base_ptr+ehdr->e_shoff))+index)
*/
#endif
