#ifndef JOS_KERN_KDEBUG_H
#define JOS_KERN_KDEBUG_H

#include <inc/types.h>
#include <kern/dwarf.h>

// Debug information about a particular instruction pointer
struct Ripdebuginfo {
	const char *rip_file;		// Source code filename for RIP
	int rip_line;			// Source code linenumber for RIP

	const char *rip_fn_name;	// Name of function containing RIP
					//  - Note: not null terminated!
	int rip_fn_namelen;		// Length of function name
	uintptr_t rip_fn_addr;		// Address of start of function
	int rip_fn_narg;		// Number of function arguments
	int size_fn_arg[10];		// Sizes of each of function arguments
	uintptr_t offset_fn_arg[10];	// Offset of each of function arguments (from CFA)
	Dwarf_Regtable reg_table;
};

int debuginfo_rip(uintptr_t rip, struct Ripdebuginfo *info);

#endif
