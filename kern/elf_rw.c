#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/memlayout.h>
#include <inc/string.h>
#include "dwarf_elf.h"
#include "dwarf.h"

#include <kern/pmap.h>

#define SECTSIZE	512
#define OFFSET_CORRECT(x) (x - ROUNDDOWN(x, SECTSIZE))
#define KELFHDR (0x10000+KERNBASE)

enum {
	DEBUG_INFO,
	DEBUG_ABBREV,
	DEBUG_FRAME,
	DEBUG_LINE,
	DEBUG_STR,
	NDEBUG_SECT,
};

Dwarf_Section section_info[NDEBUG_SECT] = { 
	{.ds_name=".debug_info", .ds_data=NULL, .ds_addr=0, .ds_size=0},
	{.ds_name=".debug_abbrev", .ds_data=NULL, .ds_addr=0, .ds_size=0},
	{.ds_name=".eh_frame", .ds_data=NULL, .ds_addr=0, .ds_size=0},
	{.ds_name=".debug_line", .ds_data=NULL, .ds_addr=0, .ds_size=0},
	{.ds_name=".debug_str", .ds_data=NULL, .ds_addr=0, .ds_size=0},
};

void readsect(void*, uint64_t);
void readseg(uint64_t, uint64_t, uint64_t, uint64_t*);

uintptr_t
read_section_headers(uintptr_t, uintptr_t);

Dwarf_Section *
_dwarf_find_section(const char *name)
{
	Dwarf_Section *ret=NULL;
	int i;

	for(i=0; i < NDEBUG_SECT; i++) {
		if(!strcmp(section_info[i].ds_name, name)) {
			ret = (section_info + i);
			break;
		}
	}

	return ret;
}

void find_debug_sections(uintptr_t elf) 
{
	Elf *ehdr = (Elf *)elf;
	uintptr_t debug_address = USTABDATA;
	Secthdr *sh = (Secthdr *)(((uint8_t *)ehdr + ehdr->e_shoff));
	Secthdr *shstr_tab = sh + ehdr->e_shstrndx;
	Secthdr* esh = sh + ehdr->e_shnum;
	for(;sh < esh; sh++) {
		char* name = (char*)((uint8_t*)elf + shstr_tab->sh_offset) + sh->sh_name;
		if(!strcmp(name, ".debug_info")) {
			section_info[DEBUG_INFO].ds_data = (uint8_t*)debug_address;
			section_info[DEBUG_INFO].ds_addr = debug_address;
			section_info[DEBUG_INFO].ds_size = sh->sh_size;
			debug_address += sh->sh_size;
		} else if(!strcmp(name, ".debug_abbrev")) {
			section_info[DEBUG_ABBREV].ds_data = (uint8_t*)debug_address;
			section_info[DEBUG_ABBREV].ds_addr = debug_address;
			section_info[DEBUG_ABBREV].ds_size = sh->sh_size;
			debug_address += sh->sh_size;
		} else if(!strcmp(name, ".debug_line")){
			section_info[DEBUG_LINE].ds_data = (uint8_t*)debug_address;
			section_info[DEBUG_LINE].ds_addr = debug_address;
			section_info[DEBUG_LINE].ds_size = sh->sh_size;
			debug_address += sh->sh_size;
		} else if(!strcmp(name, ".eh_frame")){
			section_info[DEBUG_FRAME].ds_data = (uint8_t*)sh->sh_addr;
			section_info[DEBUG_FRAME].ds_addr = sh->sh_addr;
			section_info[DEBUG_FRAME].ds_size = sh->sh_size;
			debug_address += sh->sh_size;
		} else if(!strcmp(name, ".debug_str")) {
			section_info[DEBUG_STR].ds_data = (uint8_t*)debug_address;
			section_info[DEBUG_STR].ds_addr = debug_address;
			section_info[DEBUG_STR].ds_size = sh->sh_size;
			debug_address += sh->sh_size;
		}
	}

}

uint64_t
read_section_headers(uintptr_t elfhdr, uintptr_t to_va)
{
	Secthdr* secthdr_ptr[20] = {0};
	char* kvbase = ROUNDUP((char*)to_va, SECTSIZE);
	uint64_t kvoffset = 0;
	char *orig_secthdr = (char*)kvbase;
	char * secthdr = NULL;
	uint64_t offset;
	if(elfhdr == KELFHDR)
		offset = ((Elf*)elfhdr)->e_shoff;
	else
		offset = ((Elf*)elfhdr)->e_shoff + (elfhdr - KERNBASE);

	int numSectionHeaders = ((Elf*)elfhdr)->e_shnum;
	int sizeSections = ((Elf*)elfhdr)->e_shentsize;
	char *nametab;
	int i;
	uint64_t temp;
	char *name;

	Elf *ehdr = (Elf *)elfhdr;
	Secthdr *sec_name;  

	readseg((uint64_t)orig_secthdr , numSectionHeaders * sizeSections,
		offset, &kvoffset);
	secthdr = (char*)orig_secthdr + (offset - ROUNDDOWN(offset, SECTSIZE));
	for (i = 0; i < numSectionHeaders; i++)
	{
		secthdr_ptr[i] = (Secthdr*)(secthdr) + i;
	}
	
	sec_name = secthdr_ptr[ehdr->e_shstrndx]; 
	temp = kvoffset;
	readseg((uint64_t)((char *)kvbase + kvoffset), sec_name->sh_size,
		sec_name->sh_offset, &kvoffset);
	nametab = (char *)((char *)kvbase + temp) + OFFSET_CORRECT(sec_name->sh_offset);	

	for (i = 0; i < numSectionHeaders; i++)
	{
		name = (char *)(nametab + secthdr_ptr[i]->sh_name);
		assert(kvoffset % SECTSIZE == 0);
		temp = kvoffset;
#ifdef DWARF_DEBUG
		cprintf("SectName: %s\n", name);
#endif
		if(!strcmp(name, ".debug_info"))
		{
			readseg((uint64_t)((char *)kvbase + kvoffset), secthdr_ptr[i]->sh_size, 
				secthdr_ptr[i]->sh_offset, &kvoffset);	
			section_info[DEBUG_INFO].ds_data = (uint8_t *)((char *)kvbase + temp) + OFFSET_CORRECT(secthdr_ptr[i]->sh_offset);
			section_info[DEBUG_INFO].ds_addr = (uintptr_t)section_info[DEBUG_INFO].ds_data;
			section_info[DEBUG_INFO].ds_size = secthdr_ptr[i]->sh_size;
		}
		else if(!strcmp(name, ".debug_abbrev"))
		{
			readseg((uint64_t)((char *)kvbase + kvoffset), secthdr_ptr[i]->sh_size, 
				secthdr_ptr[i]->sh_offset, &kvoffset);	
			section_info[DEBUG_ABBREV].ds_data = (uint8_t *)((char *)kvbase + temp) + OFFSET_CORRECT(secthdr_ptr[i]->sh_offset);
			section_info[DEBUG_ABBREV].ds_addr = (uintptr_t)section_info[DEBUG_ABBREV].ds_data;
			section_info[DEBUG_ABBREV].ds_size = secthdr_ptr[i]->sh_size;
		}
		else if(!strcmp(name, ".debug_line"))
		{
			readseg((uint64_t)((char *)kvbase + kvoffset), secthdr_ptr[i]->sh_size, 
				secthdr_ptr[i]->sh_offset, &kvoffset);	
			section_info[DEBUG_LINE].ds_data = (uint8_t *)((char *)kvbase + temp) + OFFSET_CORRECT(secthdr_ptr[i]->sh_offset);
			section_info[DEBUG_LINE].ds_addr = (uintptr_t)section_info[DEBUG_LINE].ds_data;
			section_info[DEBUG_LINE].ds_size = secthdr_ptr[i]->sh_size;
		}
		else if(!strcmp(name, ".eh_frame"))
		{
			section_info[DEBUG_FRAME].ds_data = (uint8_t *)secthdr_ptr[i]->sh_addr;
			section_info[DEBUG_FRAME].ds_addr = (uintptr_t)section_info[DEBUG_FRAME].ds_data;
			section_info[DEBUG_FRAME].ds_size = secthdr_ptr[i]->sh_size;
		}
		else if(!strcmp(name, ".debug_str"))
		{
			readseg((uint64_t)((char *)kvbase + kvoffset), secthdr_ptr[i]->sh_size, 
				secthdr_ptr[i]->sh_offset, &kvoffset);	
			section_info[DEBUG_STR].ds_data = (uint8_t *)((char *)kvbase + temp) + OFFSET_CORRECT(secthdr_ptr[i]->sh_offset);
			section_info[DEBUG_STR].ds_addr = (uintptr_t)section_info[DEBUG_STR].ds_data;
			section_info[DEBUG_STR].ds_size = secthdr_ptr[i]->sh_size;
		}
	}
	
	return ((uintptr_t)kvbase + kvoffset);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked
void
readseg(uint64_t pa, uint64_t count, uint64_t offset, uint64_t* kvoffset)
{
	uint64_t end_pa;
	uint64_t orgoff = offset;

	end_pa = pa + count;

	assert(pa % SECTSIZE == 0);	
	// round down to sector boundary
	pa &= ~(SECTSIZE - 1);

	// translate from bytes to sectors, and kernel starts at sector 1
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (pa < end_pa) {
		readsect((uint8_t*) pa, offset);
		pa += SECTSIZE;
		*kvoffset += SECTSIZE;
		offset++;
	}

	if(((orgoff % SECTSIZE) + count) > SECTSIZE)
	{
		readsect((uint8_t*) pa, offset);
		*kvoffset += SECTSIZE;
	}
	assert(*kvoffset % SECTSIZE == 0);
}

void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
readsect(void *dst, uint64_t offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

