#include <inc/stab.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>

#include <kern/kdebug.h>
#include <kern/dwarf.h>
#include <kern/dwarf_api.h>
#include <kern/dwarf_elf.h>
#include <kern/dwarf_define.h>
#include <kern/dwarf_error.h>

#include <kern/pmap.h>
#include <kern/env.h>

extern int _dwarf_init(Dwarf_Debug dbg, void *obj);
extern int _get_next_cu(Dwarf_Debug dbg, Dwarf_CU *cu);
extern int dwarf_siblingof(Dwarf_Debug dbg, Dwarf_Die *die, Dwarf_Die *ret_die, 
			   Dwarf_CU *cu);
extern Dwarf_Attribute * _dwarf_attr_find(Dwarf_Die *, uint16_t);
extern int dwarf_child(Dwarf_Debug dbg, Dwarf_CU *cu, Dwarf_Die *die, 
		       Dwarf_Die *ret_die);
extern int dwarf_offdie(Dwarf_Debug dbg, uint64_t offset, Dwarf_Die *ret_die, 
			Dwarf_CU cu);
extern Dwarf_Section * _dwarf_find_section(const char *name);

struct _Dwarf_Debug mydebug;
Dwarf_Debug dbg = &mydebug;
#ifdef X86_64

static const char *const dwarf_regnames_x86_64[] =
{
	"rax", "rdx", "rcx", "rbx",
	"rsi", "rdi", "rbp", "rsp",
	"r8",  "r9",  "r10", "r11",
	"r12", "r13", "r14", "r15",
	"rip",
	"xmm0",  "xmm1",  "xmm2",  "xmm3",
	"xmm4",  "xmm5",  "xmm6",  "xmm7",
	"xmm8",  "xmm9",  "xmm10", "xmm11",
	"xmm12", "xmm13", "xmm14", "xmm15",
	"st0", "st1", "st2", "st3",
	"st4", "st5", "st6", "st7",
	"mm0", "mm1", "mm2", "mm3",
	"mm4", "mm5", "mm6", "mm7",
	"rflags",
	"es", "cs", "ss", "ds", "fs", "gs", NULL, NULL,
	"fs.base", "gs.base", NULL, NULL,
	"tr", "ldtr",
	"mxcsr", "fcw", "fsw"
};

#define reg_names_ptr dwarf_regnames_x86_64

#else
static const char *const dwarf_regnames_i386[] =
{
	"eax", "ecx", "edx", "ebx",
	"esp", "ebp", "esi", "edi",
	"eip", "eflags", NULL,
	"st0", "st1", "st2", "st3",
	"st4", "st5", "st6", "st7",
	NULL, NULL,
	"xmm0", "xmm1", "xmm2", "xmm3",
	"xmm4", "xmm5", "xmm6", "xmm7",
	"mm0", "mm1", "mm2", "mm3",
	"mm4", "mm5", "mm6", "mm7",
	"fcw", "fsw", "mxcsr",
	"es", "cs", "ss", "ds", "fs", "gs", NULL, NULL,
	"tr", "ldtr"
};

#define reg_names_ptr dwarf_regnames_i386

#endif


struct UserStabData {
	const struct Stab *stabs;
	const struct Stab *stab_end;
	const char *stabstr;
	const char *stabstr_end;
};

int list_func_die(struct Ripdebuginfo *info, Dwarf_Die *die, uint64_t addr)
{
	_Dwarf_Line ln;
	Dwarf_Attribute *low;
	Dwarf_Attribute *high;
	Dwarf_CU *cu = die->cu_header;
	Dwarf_Die *cudie = die->cu_die; 
	Dwarf_Die ret, sib=*die; 
	Dwarf_Attribute *attr;
	uint64_t offset;
	uint64_t ret_val=8;
	
	if(die->die_tag != DW_TAG_subprogram)
		return 0;

	memset(&ln, 0, sizeof(_Dwarf_Line));

	low  = _dwarf_attr_find(die, DW_AT_low_pc);
	high = _dwarf_attr_find(die, DW_AT_high_pc);

	if((low && (low->u[0].u64 < addr)) && (high && (high->u[0].u64 > addr)))
	{
		info->rip_file = die->cu_die->die_name;

		info->rip_fn_name = die->die_name;
		info->rip_fn_namelen = strlen(die->die_name);

		info->rip_fn_addr = (uintptr_t)low->u[0].u64;

		assert(die->cu_die);	
		dwarf_srclines(die->cu_die, &ln, addr, NULL); 

		info->rip_line = ln.ln_lineno;
		info->rip_fn_narg = 0;

		Dwarf_Attribute* attr;

		if(dwarf_child(dbg, cu, &sib, &ret) != DW_DLE_NO_ENTRY)
		{
			if(ret.die_tag != DW_TAG_formal_parameter)
				goto last;

			attr = _dwarf_attr_find(&ret, DW_AT_type);
	
		try_again:
			if(attr != NULL)
			{
				offset = (uint64_t)cu->cu_offset + attr->u[0].u64;
				dwarf_offdie(dbg, offset, &sib, *cu);
				attr = _dwarf_attr_find(&sib, DW_AT_byte_size);
		
				if(attr != NULL)
				{
					ret_val = attr->u[0].u64;
				}
				else
				{
					attr = _dwarf_attr_find(&sib, DW_AT_type);
					goto try_again;
				}
			}
			info->size_fn_arg[info->rip_fn_narg] = ret_val;
			info->rip_fn_narg++;
			sib = ret; 

			while(dwarf_siblingof(dbg, &sib, &ret, cu) == DW_DLV_OK)	
			{
				if(ret.die_tag != DW_TAG_formal_parameter)
					break;

				attr = _dwarf_attr_find(&ret, DW_AT_type);
    
				if(attr != NULL)
				{	   
					offset = (uint64_t)cu->cu_offset + attr->u[0].u64;
					dwarf_offdie(dbg, offset, &sib, *cu);
					attr = _dwarf_attr_find(&sib, DW_AT_byte_size);
        
					if(attr != NULL)
					{
						ret_val = attr->u[0].u64;
					}
				}
	
				info->size_fn_arg[info->rip_fn_narg]=ret_val;// _get_arg_size(ret);
				info->rip_fn_narg++;
				sib = ret; 
			}
		}
	last:	
		return 1;
	}

	return 0;
}

// debuginfo_rip(addr, info)
//
//	Fill in the 'info' structure with information about the specified
//	instruction address, 'addr'.  Returns 0 if information was found, and
//	negative if not.  But even if it returns negative it has stored some
//	information into '*info'.
//
int
debuginfo_rip(uintptr_t addr, struct Ripdebuginfo *info)
{
	static struct Env* lastenv = NULL;
	void* elf;    
	Dwarf_Section *sect;
	Dwarf_CU cu;
	Dwarf_Die die, cudie, die2;
	Dwarf_Regtable *rt = NULL;
	//Set up initial pc
	uint64_t pc  = (uintptr_t)addr;

    
	// Initialize *info
	info->rip_file = "<unknown>";
	info->rip_line = 0;
	info->rip_fn_name = "<unknown>";
	info->rip_fn_namelen = 9;
	info->rip_fn_addr = addr;
	info->rip_fn_narg = 0;
    
	// Find the relevant set of stabs
	if (addr >= ULIM) {
		elf = (void *)0x10000 + KERNBASE;
	} else {
		if(curenv != lastenv) {
			find_debug_sections((uintptr_t)curenv->elf);
			lastenv = curenv;
		}
		elf = curenv->elf;
	}
	_dwarf_init(dbg, elf);

	sect = _dwarf_find_section(".debug_info");	
	dbg->dbg_info_offset_elf = (uint64_t)sect->ds_data; 
	dbg->dbg_info_size = sect->ds_size;
    
	assert(dbg->dbg_info_size);
	while(_get_next_cu(dbg, &cu) == 0)
	{
		if(dwarf_siblingof(dbg, NULL, &cudie, &cu) == DW_DLE_NO_ENTRY)
		{
			continue;
		}	
		cudie.cu_header = &cu;
		cudie.cu_die = NULL;
	    
		if(dwarf_child(dbg, &cu, &cudie, &die) == DW_DLE_NO_ENTRY)
		{
			continue;
		}	
		die.cu_header = &cu;
		die.cu_die = &cudie;
		while(1)
		{
			if(list_func_die(info, &die, addr))
				goto find_done;
			if(dwarf_siblingof(dbg, &die, &die2, &cu) < 0)
				break; 
			die = die2;
			die.cu_header = &cu;
			die.cu_die = &cudie;
		}
	}
    
	return -1;

find_done:
	return 0;

}
