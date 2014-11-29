#ifndef DWARF_API_H
#define DWARF_API_H
#include <kern/kdebug.h>

extern uintptr_t read_section_headers(uintptr_t, uintptr_t);
extern void find_debug_sections(uintptr_t);
extern int dwarf_get_pc_info(uintptr_t addr, struct Ripdebuginfo *info);

#endif
