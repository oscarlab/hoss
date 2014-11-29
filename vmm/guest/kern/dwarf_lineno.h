#ifndef DWARF_LINENO_H
// External functions used
extern Dwarf_Attribute *_dwarf_attr_find(Dwarf_Die *, uint16_t);


//Functions declared in this module
static int _dwarf_lineno_add_file(Dwarf_LineInfo, uint8_t **, const char *, Dwarf_Error *, Dwarf_Debug);

static int
_dwarf_lineno_run_program(Dwarf_CU *cu, Dwarf_LineInfo li, uint8_t *p,
    uint8_t *pe, Dwarf_Addr pc, Dwarf_Error *error);

static int
_dwarf_lineno_add_file(Dwarf_LineInfo li, uint8_t **p, const char *compdir,
    Dwarf_Error *error, Dwarf_Debug dbg);

int
_dwarf_lineno_init(Dwarf_Die *die, uint64_t offset, Dwarf_LineInfo linfo, Dwarf_Addr pc, Dwarf_Error *error);

#endif
