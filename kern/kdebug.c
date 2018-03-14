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

struct _Dwarf_Fde _fde;
Dwarf_Fde fde = &_fde;
struct _Dwarf_Cie _cie;
Dwarf_Cie cie = &_cie;

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

extern int
dwarf_loclist(Dwarf_Attribute * attr,
    Dwarf_Locdesc * locdesc,
    Dwarf_Signed * listlen, Dwarf_Error * error);

extern int dwarf_init_eh_section(Dwarf_Debug dbg, Dwarf_Error *error);
extern int
dwarf_get_fde_at_pc(Dwarf_Debug dbg, Dwarf_Addr pc,
		    Dwarf_Fde ret_fde, Dwarf_Cie cie,
		    Dwarf_Error *error);
extern int
dwarf_get_fde_info_for_all_regs(Dwarf_Debug dbg, Dwarf_Fde fde,
				Dwarf_Addr pc_requested,
				Dwarf_Regtable *reg_table, Dwarf_Addr *row_pc,
				Dwarf_Error *error);

int64_t
_dwarf_read_sleb128(uint8_t *data, uint64_t *offsetp);
uint64_t
_dwarf_read_uleb128(uint8_t *data, uint64_t *offsetp);
int64_t
_dwarf_decode_sleb128(uint8_t **dp);
uint64_t
_dwarf_decode_uleb128(uint8_t **dp);

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
	/* "mxcsr", "fcw", "fsw" */
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
	uint64_t ret_offset=0;

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

			ret_offset = 0;
			attr = _dwarf_attr_find(&ret, DW_AT_location);
			if (attr != NULL)
			{
				Dwarf_Unsigned loc_len = attr->at_block.bl_len;
				Dwarf_Small *loc_ptr = attr->at_block.bl_data;
				Dwarf_Small atom;
				Dwarf_Unsigned op1, op2;

				switch(attr->at_form) {
					case DW_FORM_block1:
					case DW_FORM_block2:
					case DW_FORM_block4:
						offset = 0;
						atom = *(loc_ptr++);
						offset++;
						if (atom == DW_OP_fbreg) {
							uint8_t *p = loc_ptr;
							ret_offset = _dwarf_decode_sleb128(&p);
							offset += p - loc_ptr;
							loc_ptr = p;
						}
						break;
				}
			}

			info->size_fn_arg[info->rip_fn_narg] = ret_val;
			info->offset_fn_arg[info->rip_fn_narg] = ret_offset;
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
	
				ret_offset = 0;
				attr = _dwarf_attr_find(&ret, DW_AT_location);
				if (attr != NULL)
				{
					Dwarf_Unsigned loc_len = attr->at_block.bl_len;
					Dwarf_Small *loc_ptr = attr->at_block.bl_data;
					Dwarf_Small atom;
					Dwarf_Unsigned op1, op2;

					switch(attr->at_form) {
						case DW_FORM_block1:
						case DW_FORM_block2:
						case DW_FORM_block4:
							offset = 0;
							atom = *(loc_ptr++);
							offset++;
							if (atom == DW_OP_fbreg) {
								uint8_t *p = loc_ptr;
								ret_offset = _dwarf_decode_sleb128(&p);
								offset += p - loc_ptr;
								loc_ptr = p;
							}
							break;
					}
				}

				info->size_fn_arg[info->rip_fn_narg]=ret_val;// _get_arg_size(ret);
				info->offset_fn_arg[info->rip_fn_narg]=ret_offset;
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
			continue;

		cudie.cu_header = &cu;
		cudie.cu_die = NULL;

		if(dwarf_child(dbg, &cu, &cudie, &die) == DW_DLE_NO_ENTRY)
			continue;

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

	if (dwarf_init_eh_section(dbg, NULL) == DW_DLV_ERROR)
		return -1;

	if (dwarf_get_fde_at_pc(dbg, addr, fde, cie, NULL) == DW_DLV_OK) {
		dwarf_get_fde_info_for_all_regs(dbg, fde, addr,
						&info->reg_table,
						NULL, NULL);

#if 0
		cprintf("CFA: reg %s off %d\n",
			reg_names_ptr[info->reg_table.cfa_rule.dw_regnum],
			info->reg_table.cfa_rule.dw_offset);

		for (i = 0; i < sizeof(reg_names_ptr) / sizeof(reg_names_ptr[0]); i++) {
			if (!reg_names_ptr[i])
				continue;
			switch(info->reg_table.rules[i].dw_regnum) {
				case DW_FRAME_UNDEFINED_VAL:
					cprintf("%s: \n", reg_names_ptr[i]);
					break;
				case DW_FRAME_CFA_COL3:
					cprintf("%s: off %d\n", reg_names_ptr[i],
						info->reg_table.rules[i].dw_offset);
					break;
				case DW_FRAME_SAME_VAL:
					break;
				default:
					cprintf("%s: reg %s\n", reg_names_ptr[i],
						reg_names_ptr[info->reg_table.rules[i].dw_regnum]);
					break;
			}
		}
#endif
	}
	return 0;
}
