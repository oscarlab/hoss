/*-
 * Copyright (c) 2009,2011 Kai Wang
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <inc/types.h>
#include <inc/string.h>
#include <inc/assert.h>
#include "dwarf_elf.h"
#include "dwarf_define.h"
#include "dwarf_error.h"
#include "dwarf.h"

#include "dwarf_lineno.h"

extern Dwarf_Debug dbg;
Dwarf_Small global_std_op[512];

int64_t
_dwarf_read_sleb128(uint8_t *data, uint64_t *offsetp);
uint64_t
_dwarf_read_uleb128(uint8_t *data, uint64_t *offsetp);
uint64_t
_dwarf_decode_uleb128(uint8_t **dp);
int64_t
_dwarf_decode_sleb128(uint8_t **dp);
int  _dwarf_find_section_enhanced(Dwarf_Section *ds);

static int
_dwarf_lineno_run_program(Dwarf_CU *cu, Dwarf_LineInfo li, uint8_t *p,
			  uint8_t *pe, Dwarf_Addr pc, Dwarf_Error *error)
{
	Dwarf_Line ln, tln;
	uint64_t address, file, line, column, isa, opsize;
	int is_stmt, basic_block, end_sequence;
	int prologue_end, epilogue_begin;
	int ret;

	ln = &li->li_line;
#define RESET_REGISTERS					\
	do {						\
		address        = 0;			\
		file           = 1;			\
		line           = 1;			\
		column         = 0;			\
		is_stmt        = li->li_defstmt;        \
		basic_block    = 0;			\
		end_sequence   = 0;			\
		prologue_end   = 0;			\
		epilogue_begin = 0;			\
	} while(0)

#define APPEND_ROW				\
	do {					\
		if (pc < address) {		\
			return DW_DLE_NONE;	\
		}				\
		ln->ln_addr   = address;	\
		ln->ln_symndx = 0;              \
		ln->ln_fileno = file;		\
		ln->ln_lineno = line;		\
		ln->ln_column = column;		\
		ln->ln_bblock = basic_block;	\
		ln->ln_stmt   = is_stmt;	\
		ln->ln_endseq = end_sequence;	\
		li->li_lnlen++;                 \
	} while(0)

#define LINE(x) (li->li_lbase + (((x) - li->li_opbase) % li->li_lrange))
#define ADDRESS(x) ((((x) - li->li_opbase) / li->li_lrange) * li->li_minlen)

	/*
	 *   ln->ln_li     = li;             \
	 * Set registers to their default values.
	 */
	RESET_REGISTERS;

	/*
	 * Start line number program.
	 */
	while (p < pe) {
		if (*p == 0) {

			/*
			 * Extended Opcodes.
			 */

			p++;
			opsize = _dwarf_decode_uleb128(&p);
			switch (*p) {
			case DW_LNE_end_sequence:
				p++;
				end_sequence = 1;
				RESET_REGISTERS;
				break;
			case DW_LNE_set_address:
				p++;
				address = dbg->decode(&p, cu->addr_size);
				break;
			case DW_LNE_define_file:
				p++;
				ret = _dwarf_lineno_add_file(li, &p, NULL,
							     error, dbg);
				if (ret != DW_DLE_NONE)
					goto prog_fail;
				break;
			default:
				/* Unrecognized extened opcodes. */
				p += opsize;
			}

		} else if (*p > 0 && *p < li->li_opbase) {

			/*
			 * Standard Opcodes.
			 */

			switch (*p++) {
			case DW_LNS_copy:
				APPEND_ROW;
				basic_block = 0;
				prologue_end = 0;
				epilogue_begin = 0;
				break;
			case DW_LNS_advance_pc:
				address += _dwarf_decode_uleb128(&p) *
					li->li_minlen;
				break;
			case DW_LNS_advance_line:
				line += _dwarf_decode_sleb128(&p);
				break;
			case DW_LNS_set_file:
				file = _dwarf_decode_uleb128(&p);
				break;
			case DW_LNS_set_column:
				column = _dwarf_decode_uleb128(&p);
				break;
			case DW_LNS_negate_stmt:
				is_stmt = !is_stmt;
				break;
			case DW_LNS_set_basic_block:
				basic_block = 1;
				break;
			case DW_LNS_const_add_pc:
				address += ADDRESS(255);
				break;
			case DW_LNS_fixed_advance_pc:
				address += dbg->decode(&p, 2);
				break;
			case DW_LNS_set_prologue_end:
				prologue_end = 1;
				break;
			case DW_LNS_set_epilogue_begin:
				epilogue_begin = 1;
				break;
			case DW_LNS_set_isa:
				isa = _dwarf_decode_uleb128(&p);
				break;
			default:
				/* Unrecognized extened opcodes. What to do? */
				break;
			}

		} else {

			/*
			 * Special Opcodes.
			 */

			line += LINE(*p);
			address += ADDRESS(*p);
			APPEND_ROW;
			basic_block = 0;
			prologue_end = 0;
			epilogue_begin = 0;
			p++;
		}
	}

	return (DW_DLE_NONE);

prog_fail:

	return (ret);

#undef  RESET_REGISTERS
#undef  APPEND_ROW
#undef  LINE
#undef  ADDRESS
}

static int
_dwarf_lineno_add_file(Dwarf_LineInfo li, uint8_t **p, const char *compdir,
		       Dwarf_Error *error, Dwarf_Debug dbg)
{
	char *fname;
	//const char *dirname;
	uint8_t *src;
	int slen;

	src = *p;
/*
  if ((lf = malloc(sizeof(struct _Dwarf_LineFile))) == NULL) {
  DWARF_SET_ERROR(dbg, error, DW_DLE_MEMORY);
  return (DW_DLE_MEMORY);
  }
*/  
	//lf->lf_fullpath = NULL;
	fname = (char *) src;
	src += strlen(fname) + 1;
	_dwarf_decode_uleb128(&src);

	/* Make full pathname if need. 
	   if (*lf->lf_fname != '/') {
	   dirname = compdir;
	   if (lf->lf_dirndx > 0)
	   dirname = li->li_incdirs[lf->lf_dirndx - 1];
	   if (dirname != NULL) {
	   slen = strlen(dirname) + strlen(lf->lf_fname) + 2;
	   if ((lf->lf_fullpath = malloc(slen)) == NULL) {
	   free(lf);
	   DWARF_SET_ERROR(dbg, error, DW_DLE_MEMORY);
	   return (DW_DLE_MEMORY);
	   }
	   snprintf(lf->lf_fullpath, slen, "%s/%s", dirname,
	   lf->lf_fname);
	   }
	   }
	*/
	_dwarf_decode_uleb128(&src);
	_dwarf_decode_uleb128(&src);
	//STAILQ_INSERT_TAIL(&li->li_lflist, lf, lf_next);
	//li->li_lflen++;

	*p = src;

	return (DW_DLE_NONE);
}

int     
_dwarf_lineno_init(Dwarf_Die *die, uint64_t offset, Dwarf_LineInfo linfo, Dwarf_Addr pc, Dwarf_Error *error)
{   
	Dwarf_Section myds = {.ds_name = ".debug_line"};
	Dwarf_Section *ds = &myds;
	Dwarf_CU *cu;
	Dwarf_Attribute at;
	Dwarf_LineInfo li;
	//Dwarf_LineFile lf, tlf;
	uint64_t length, hdroff, endoff;
	uint8_t *p;
	int dwarf_size, i, ret;
            
	cu = die->cu_header;
	assert(cu != NULL); 
	assert(dbg != NULL);

	if ((_dwarf_find_section_enhanced(ds)) != 0)
		return (DW_DLE_NONE);

	li = linfo;
	/*
	 * Try to find out the dir where the CU was compiled. Later we
	 * will use the dir to create full pathnames, if need.
	 compdir = NULL;
	 at = _dwarf_attr_find(die, DW_AT_comp_dir);
	 if (at != NULL) {
	 switch (at->at_form) {
	 case DW_FORM_strp:
	 compdir = at->u[1].s;
	 break;
	 case DW_FORM_string:
	 compdir = at->u[0].s;
	 break;
	 default:
	 break;
	 }
	 }
	*/

	length = dbg->read(ds->ds_data, &offset, 4);
	if (length == 0xffffffff) {
		dwarf_size = 8;
		length = dbg->read(ds->ds_data, &offset, 8);
	} else
		dwarf_size = 4;

	if (length > ds->ds_size - offset) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_DEBUG_LINE_LENGTH_BAD);
		return (DW_DLE_DEBUG_LINE_LENGTH_BAD);
	}
	/*
	 * Read in line number program header.
	 */
	li->li_length = length;
	endoff = offset + length;
	li->li_version = dbg->read(ds->ds_data, &offset, 2); /* FIXME: verify version */
	li->li_hdrlen = dbg->read(ds->ds_data, &offset, dwarf_size);
	hdroff = offset;
	li->li_minlen = dbg->read(ds->ds_data, &offset, 1);
	li->li_defstmt = dbg->read(ds->ds_data, &offset, 1);
	li->li_lbase = dbg->read(ds->ds_data, &offset, 1);
	li->li_lrange = dbg->read(ds->ds_data, &offset, 1);
	li->li_opbase = dbg->read(ds->ds_data, &offset, 1);
	//STAILQ_INIT(&li->li_lflist);
	//STAILQ_INIT(&li->li_lnlist);

	if ((int)li->li_hdrlen - 5 < li->li_opbase - 1) {
		ret = DW_DLE_DEBUG_LINE_LENGTH_BAD;
		DWARF_SET_ERROR(dbg, error, ret);
		goto fail_cleanup;
	}

	li->li_oplen = global_std_op;
	/*if ((li->li_oplen = malloc(li->li_opbase)) == NULL) {
	  ret = DW_DLE_MEMORY;
	  DWARF_SET_ERROR(dbg, error, ret);
	  goto fail_cleanup;
	  }*/

	/*
	 * Read in std opcode arg length list. Note that the first
	 * element is not used.
	 */
	for (i = 1; i < li->li_opbase; i++)
		li->li_oplen[i] = dbg->read(ds->ds_data, &offset, 1);

	/*
	 * Check how many strings in the include dir string array.
	 */
	length = 0;
	p = ds->ds_data + offset;
	while (*p != '\0') {
		while (*p++ != '\0')
			;
		length++;
	}
	li->li_inclen = length;

	/* Sanity check. */
	if (p - ds->ds_data > (int) ds->ds_size) {
		ret = DW_DLE_DEBUG_LINE_LENGTH_BAD;
		DWARF_SET_ERROR(dbg, error, ret);
		goto fail_cleanup;
	}
	p++;

	/*
	 * Process file list.
	 */
	while (*p != '\0') {
		ret = _dwarf_lineno_add_file(li, &p, NULL, error, dbg);
		//p++;
	}

	p++;
	/* Sanity check. */
	if (p - ds->ds_data - hdroff != li->li_hdrlen) {
		ret = DW_DLE_DEBUG_LINE_LENGTH_BAD;
		DWARF_SET_ERROR(dbg, error, ret);
		goto fail_cleanup;
	}

	/*
	 * Process line number program.
	 */
	ret = _dwarf_lineno_run_program(cu, li, p, ds->ds_data + endoff, pc,
					error);
	if (ret != DW_DLE_NONE)
		goto fail_cleanup;

	//cu->cu_lineinfo = li;

	return (DW_DLE_NONE);

fail_cleanup:

	/*if (li->li_oplen)
	  free(li->li_oplen);*/

	return (ret);
}

int
dwarf_srclines(Dwarf_Die *die, Dwarf_Line linebuf, Dwarf_Addr pc, Dwarf_Error *error)
{
	_Dwarf_LineInfo li;
	Dwarf_Attribute *at;

	assert(die);
	assert(linebuf);

	memset(&li, 0, sizeof(_Dwarf_LineInfo));

	if ((at = _dwarf_attr_find(die, DW_AT_stmt_list)) == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_NO_ENTRY);
		return (DW_DLV_NO_ENTRY);
	}

	if (_dwarf_lineno_init(die, at->u[0].u64, &li, pc, error) !=
	    DW_DLE_NONE)
	{
		return (DW_DLV_ERROR);
	}
	*linebuf = li.li_line;

	return (DW_DLV_OK);
}

