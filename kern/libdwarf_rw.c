/*-
 * Copyright (c) 2007 John Birrell (jb@freebsd.org)
 * Copyright (c) 2010 Kai Wang
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
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/types.h>
#include <inc/string.h>
#include "dwarf_elf.h"
#include "dwarf_define.h"
#include "dwarf_error.h"
#include "dwarf.h"


Dwarf_Attribute* _dwarf_attr_find(Dwarf_Die *die, uint16_t attr);
extern char* elf_base_ptr;

void _dwarf_frame_params_init(Dwarf_Debug dbg);

Dwarf_Section *
_dwarf_find_section(const char *name);

uint64_t
_dwarf_read_lsb(uint8_t *data, uint64_t *offsetp, int bytes_to_read)
{
	uint64_t ret;
	uint8_t *src;

	src = data + *offsetp;

	ret = 0;
	switch (bytes_to_read) {
	case 8:
		ret |= ((uint64_t) src[4]) << 32 | ((uint64_t) src[5]) << 40;
		ret |= ((uint64_t) src[6]) << 48 | ((uint64_t) src[7]) << 56;
	case 4:
		ret |= ((uint64_t) src[2]) << 16 | ((uint64_t) src[3]) << 24;
	case 2:
		ret |= ((uint64_t) src[1]) << 8;
	case 1:
		ret |= src[0];
		break;
	default:
		return (0);
	}

	*offsetp += bytes_to_read;

	return (ret);
}

uint64_t
_dwarf_decode_lsb(uint8_t **data, int bytes_to_read)
{
	uint64_t ret;
	uint8_t *src;

	src = *data;

	ret = 0;
	switch (bytes_to_read) {
	case 8:
		ret |= ((uint64_t) src[4]) << 32 | ((uint64_t) src[5]) << 40;
		ret |= ((uint64_t) src[6]) << 48 | ((uint64_t) src[7]) << 56;
	case 4:
		ret |= ((uint64_t) src[2]) << 16 | ((uint64_t) src[3]) << 24;
	case 2:
		ret |= ((uint64_t) src[1]) << 8;
	case 1:
		ret |= src[0];
		break;
	default:
		return (0);
	}

	*data += bytes_to_read;

	return (ret);
}

uint64_t
_dwarf_read_msb(uint8_t *data, uint64_t *offsetp, int bytes_to_read)
{
	uint64_t ret;
	uint8_t *src;

	src = data + *offsetp;

	switch (bytes_to_read) {
	case 1:
		ret = src[0];
		break;
	case 2:
		ret = src[1] | ((uint64_t) src[0]) << 8;
		break;
	case 4:
		ret = src[3] | ((uint64_t) src[2]) << 8;
		ret |= ((uint64_t) src[1]) << 16 | ((uint64_t) src[0]) << 24;
		break;
	case 8:
		ret = src[7] | ((uint64_t) src[6]) << 8;
		ret |= ((uint64_t) src[5]) << 16 | ((uint64_t) src[4]) << 24;
		ret |= ((uint64_t) src[3]) << 32 | ((uint64_t) src[2]) << 40;
		ret |= ((uint64_t) src[1]) << 48 | ((uint64_t) src[0]) << 56;
		break;
	default:
		return (0);
	}

	*offsetp += bytes_to_read;

	return (ret);
}

uint64_t
_dwarf_decode_msb(uint8_t **data, int bytes_to_read)
{
	uint64_t ret;
	uint8_t *src;

	src = *data;

	ret = 0;
	switch (bytes_to_read) {
	case 1:
		ret = src[0];
		break;
	case 2:
		ret = src[1] | ((uint64_t) src[0]) << 8;
		break;
	case 4:
		ret = src[3] | ((uint64_t) src[2]) << 8;
		ret |= ((uint64_t) src[1]) << 16 | ((uint64_t) src[0]) << 24;
		break;
	case 8:
		ret = src[7] | ((uint64_t) src[6]) << 8;
		ret |= ((uint64_t) src[5]) << 16 | ((uint64_t) src[4]) << 24;
		ret |= ((uint64_t) src[3]) << 32 | ((uint64_t) src[2]) << 40;
		ret |= ((uint64_t) src[1]) << 48 | ((uint64_t) src[0]) << 56;
		break;
	default:
		return (0);
		break;
	}

	*data += bytes_to_read;

	return (ret);
}

int64_t
_dwarf_read_sleb128(uint8_t *data, uint64_t *offsetp)
{
	int64_t ret = 0;
	uint8_t b;
	int shift = 0;
	uint8_t *src;

	src = data + *offsetp;

	do {
		b = *src++;
		ret |= ((b & 0x7f) << shift);
		(*offsetp)++;
		shift += 7;
	} while ((b & 0x80) != 0);

	if (shift < 32 && (b & 0x40) != 0)
		ret |= (-1 << shift);

	return (ret);
}

uint64_t
_dwarf_read_uleb128(uint8_t *data, uint64_t *offsetp)
{
	uint64_t ret = 0;
	uint8_t b;
	int shift = 0;
	uint8_t *src;

	src = data + *offsetp;

	do {
		b = *src++;
		ret |= ((b & 0x7f) << shift);
		(*offsetp)++;
		shift += 7;
	} while ((b & 0x80) != 0);

	return (ret);
}

int64_t
_dwarf_decode_sleb128(uint8_t **dp)
{
	int64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = *dp;

	do {
		b = *src++;
		ret |= ((b & 0x7f) << shift);
		shift += 7;
	} while ((b & 0x80) != 0);

	if (shift < 32 && (b & 0x40) != 0)
		ret |= (-1 << shift);

	*dp = src;

	return (ret);
}

uint64_t
_dwarf_decode_uleb128(uint8_t **dp)
{
	uint64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = *dp;

	do {
		b = *src++;
		ret |= ((b & 0x7f) << shift);
		shift += 7;
	} while ((b & 0x80) != 0);

	*dp = src;

	return (ret);
}

#define Dwarf_Unsigned uint64_t

char *
_dwarf_read_string(void *data, Dwarf_Unsigned size, uint64_t *offsetp)
{
	char *ret, *src;

	ret = src = (char *) data + *offsetp;

	while (*src != '\0' && *offsetp < size) {
		src++;
		(*offsetp)++;
	}

	if (*src == '\0' && *offsetp < size)
		(*offsetp)++;

	return (ret);
}

uint8_t *
_dwarf_read_block(void *data, uint64_t *offsetp, uint64_t length)
{
	uint8_t *ret, *src;

	ret = src = (uint8_t *) data + *offsetp;

	(*offsetp) += length;

	return (ret);
}

Dwarf_Endianness
_dwarf_elf_get_byte_order(void *obj)
{
	Elf *e;

	e = (Elf *)obj;
	assert(e != NULL);

//TODO: Need to check for 64bit here. Because currently Elf header for
//      64bit doesn't have any memeber e_ident. But need to see what is
//      similar in 64bit.
	switch (e->e_ident[EI_DATA]) {
	case ELFDATA2MSB:
		return (DW_OBJECT_MSB);

	case ELFDATA2LSB:
	case ELFDATANONE:
	default:
		return (DW_OBJECT_LSB);
	}
}

Dwarf_Small
_dwarf_elf_get_pointer_size(void *obj)
{
	Elf *e;

	e = (Elf *) obj;
	assert(e != NULL);

	if (e->e_ident[4] == ELFCLASS32)
		return (4);
	else
		return (8);
}

//Return 0 on success
int _dwarf_init(Dwarf_Debug dbg, void *obj)
{
	memset(dbg, 0, sizeof(struct _Dwarf_Debug));
	dbg->curr_off_dbginfo = 0;
	dbg->dbg_info_size = 0;
	dbg->dbg_pointer_size = _dwarf_elf_get_pointer_size(obj); 

	if (_dwarf_elf_get_byte_order(obj) == DW_OBJECT_MSB) {
		dbg->read = _dwarf_read_msb;
		dbg->decode = _dwarf_decode_msb;
	} else {
		dbg->read = _dwarf_read_lsb;
		dbg->decode = _dwarf_decode_lsb;
	}
	_dwarf_frame_params_init(dbg);
	return 0;
}

//Return 0 on success
int _get_next_cu(Dwarf_Debug dbg, Dwarf_CU *cu)
{
	uint32_t length;
	uint64_t offset;
	uint8_t dwarf_size;

	if(dbg->curr_off_dbginfo > dbg->dbg_info_size)
		return -1;

	offset = dbg->curr_off_dbginfo;
	cu->cu_offset = offset;

	length = dbg->read((uint8_t *)dbg->dbg_info_offset_elf, &offset,4);
	if (length == 0xffffffff) {
		length = dbg->read((uint8_t *)dbg->dbg_info_offset_elf, &offset, 8);
		dwarf_size = 8;
	} else {
		dwarf_size = 4;
	}

	cu->cu_dwarf_size = dwarf_size;

	/*
	 * Check if there is enough ELF data for this CU. This assumes
	 * that libelf gives us the entire section in one Elf_Data
	 * object.
	 *
	 if (length > ds->ds_size - offset) {
	 return (DW_DLE_CU_LENGTH_ERROR);
	 }*/

	/* Compute the offset to the next compilation unit: */
	dbg->curr_off_dbginfo = offset + length;
	cu->cu_next_offset   = dbg->curr_off_dbginfo;

	/* Initialise the compilation unit. */
	cu->cu_length = (uint64_t)length;

	cu->cu_length_size   = (dwarf_size == 4 ? 4 : 12);
	cu->version              = dbg->read((uint8_t *)dbg->dbg_info_offset_elf, &offset, 2);
	cu->debug_abbrev_offset  = dbg->read((uint8_t *)dbg->dbg_info_offset_elf, &offset, dwarf_size);
	//cu->cu_abbrev_offset_cur = cu->cu_abbrev_offset;
	cu->addr_size  = dbg->read((uint8_t *)dbg->dbg_info_offset_elf, &offset, 1);

	if (cu->version < 2 || cu->version > 4) {
		return -1;
	}

	cu->cu_die_offset = offset;

	return 0;
}

void print_cu(Dwarf_CU cu)
{
	cprintf("%ld---%du--%d\n",cu.cu_length,cu.version,cu.addr_size);
}

//Return 0 on success
int
_dwarf_abbrev_parse(Dwarf_Debug dbg, Dwarf_CU cu, Dwarf_Unsigned *offset,
		    Dwarf_Abbrev *abp, Dwarf_Section *ds)
{
	uint64_t attr;
	uint64_t entry;
	uint64_t form;
	uint64_t aboff;
	uint64_t adoff;
	uint64_t tag;
	uint8_t children;
	uint64_t abbr_addr;
	int ret;

	assert(abp != NULL);
	assert(ds != NULL);

	if (*offset >= ds->ds_size)
        	return (DW_DLE_NO_ENTRY);

	aboff = *offset;

	abbr_addr = (uint64_t)ds->ds_data; //(uint64_t)((uint8_t *)elf_base_ptr + ds->sh_offset);

	entry = _dwarf_read_uleb128((uint8_t *)abbr_addr, offset);

	if (entry == 0) {
		/* Last entry. */
		//Need to make connection from below function
		abp->ab_entry = 0;
		return DW_DLE_NONE;
	}

	tag = _dwarf_read_uleb128((uint8_t *)abbr_addr, offset);
	children = dbg->read((uint8_t *)abbr_addr, offset, 1);

	abp->ab_entry    = entry;
	abp->ab_tag      = tag;
	abp->ab_children = children;
	abp->ab_offset   = aboff;
	abp->ab_length   = 0;    /* fill in later. */
	abp->ab_atnum    = 0;

	/* Parse attribute definitions. */
	do {
		adoff = *offset;
		attr = _dwarf_read_uleb128((uint8_t *)abbr_addr, offset);
		form = _dwarf_read_uleb128((uint8_t *)abbr_addr, offset);
		if (attr != 0)
		{
			/* Initialise the attribute definition structure. */
			abp->ab_attrdef[abp->ab_atnum].ad_attrib = attr;
			abp->ab_attrdef[abp->ab_atnum].ad_form   = form;
			abp->ab_attrdef[abp->ab_atnum].ad_offset = adoff;
			abp->ab_atnum++;
		}
	} while (attr != 0);

	//(*abp)->ab_length = *offset - aboff;
	abp->ab_length = (uint64_t)(*offset - aboff);

	return DW_DLV_OK;
}

//Return 0 on success
int
_dwarf_abbrev_find(Dwarf_Debug dbg, Dwarf_CU cu, uint64_t entry, Dwarf_Abbrev *abp)
{
	Dwarf_Section *ds;
	uint64_t offset;
	int ret;

	if (entry == 0)
	{
		return (DW_DLE_NO_ENTRY);
	}

	/* Load and search the abbrev table. */
	ds = _dwarf_find_section(".debug_abbrev");
	assert(ds != NULL);

	//TODO: We are starting offset from 0, however libdwarf logic
	//      is keeping a counter for current offset. Ok. let use
	//      that. I relent, but this will be done in Phase 2. :)
	//offset = 0; //cu->cu_abbrev_offset_cur;
	offset = cu.debug_abbrev_offset; //cu->cu_abbrev_offset_cur;
	while (offset < ds->ds_size) {
		ret = _dwarf_abbrev_parse(dbg, cu, &offset, abp, ds);
		if (ret != DW_DLE_NONE)
			return (ret);
		if (abp->ab_entry == entry) {
			//cu->cu_abbrev_offset_cur = offset;
			return DW_DLE_NONE;
		}
		if (abp->ab_entry == 0) {
			//cu->cu_abbrev_offset_cur = offset;
			//cu->cu_abbrev_loaded = 1;
			break;
		}
	}

	return DW_DLE_NO_ENTRY;
}

//Return 0 on success
int
_dwarf_attr_init(Dwarf_Debug dbg, uint64_t *offsetp, Dwarf_CU *cu, Dwarf_Die *ret_die, Dwarf_AttrDef *ad,
		 uint64_t form, int indirect)
{
	struct _Dwarf_Attribute atref;
	Dwarf_Section *str;
	int ret;
	Dwarf_Section *ds = _dwarf_find_section(".debug_info");
	uint8_t *ds_data = (uint8_t *)ds->ds_data; //(uint8_t *)dbg->dbg_info_offset_elf;
	uint8_t dwarf_size = cu->cu_dwarf_size;

	ret = DW_DLE_NONE;
	memset(&atref, 0, sizeof(atref));
	atref.at_die = ret_die;
	atref.at_attrib = ad->ad_attrib;
	atref.at_form = ad->ad_form;
	atref.at_indirect = indirect;
	atref.at_ld = NULL;

	switch (form) {
	case DW_FORM_addr:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, cu->addr_size);
		break;
	case DW_FORM_block:
	case DW_FORM_exprloc:
		atref.u[0].u64 = _dwarf_read_uleb128(ds_data, offsetp);
		atref.u[1].u8p = (uint8_t*)_dwarf_read_block(ds_data, offsetp, atref.u[0].u64);
		break;
	case DW_FORM_block1:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 1);
		atref.u[1].u8p = (uint8_t*)_dwarf_read_block(ds_data, offsetp, atref.u[0].u64);
		break;
	case DW_FORM_block2:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 2);
		atref.u[1].u8p = (uint8_t*)_dwarf_read_block(ds_data, offsetp, atref.u[0].u64);
		break;
	case DW_FORM_block4:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 4);
		atref.u[1].u8p = (uint8_t*)_dwarf_read_block(ds_data, offsetp, atref.u[0].u64);
		break;
	case DW_FORM_data1:
	case DW_FORM_flag:
	case DW_FORM_ref1:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 1);
		break;
	case DW_FORM_data2:
	case DW_FORM_ref2:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 2);
		break;
	case DW_FORM_data4:
	case DW_FORM_ref4:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 4);
		break;
	case DW_FORM_data8:
	case DW_FORM_ref8:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, 8);
		break;
	case DW_FORM_indirect:
		form = _dwarf_read_uleb128(ds_data, offsetp);
		return (_dwarf_attr_init(dbg, offsetp, cu, ret_die, ad, form, 1));
	case DW_FORM_ref_addr:
		if (cu->version == 2)
			atref.u[0].u64 = dbg->read(ds_data, offsetp, cu->addr_size);
		else if (cu->version == 3)
			atref.u[0].u64 = dbg->read(ds_data, offsetp, dwarf_size);
		break;
	case DW_FORM_ref_udata:
	case DW_FORM_udata:
		atref.u[0].u64 = _dwarf_read_uleb128(ds_data, offsetp);
		break;
	case DW_FORM_sdata:
		atref.u[0].s64 = _dwarf_read_sleb128(ds_data, offsetp);
		break;
	case DW_FORM_sec_offset:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, dwarf_size);
		break;
	case DW_FORM_string:
		atref.u[0].s =(char*) _dwarf_read_string(ds_data, (uint64_t)ds->ds_size, offsetp);
		break;
	case DW_FORM_strp:
		atref.u[0].u64 = dbg->read(ds_data, offsetp, dwarf_size);
		str = _dwarf_find_section(".debug_str");
		assert(str != NULL);
		//atref.u[1].s = (char *)(elf_base_ptr + str->sh_offset) + atref.u[0].u64;
		atref.u[1].s = (char *)str->ds_data + atref.u[0].u64;
		break;
	case DW_FORM_ref_sig8:
		atref.u[0].u64 = 8;
		atref.u[1].u8p = (uint8_t*)(_dwarf_read_block(ds_data, offsetp, atref.u[0].u64));
		break;
	case DW_FORM_flag_present:
		/* This form has no value encoded in the DIE. */
		atref.u[0].u64 = 1;
		break;
	default:
		//DWARF_SET_ERROR(dbg, error, DW_DLE_ATTR_FORM_BAD);
		ret = DW_DLE_ATTR_FORM_BAD;
		break;
	}

	if (ret == DW_DLE_NONE) {
		if (form == DW_FORM_block || form == DW_FORM_block1 ||
		    form == DW_FORM_block2 || form == DW_FORM_block4) {
			atref.at_block.bl_len = atref.u[0].u64;
			atref.at_block.bl_data = atref.u[1].u8p;
		}
		//ret = _dwarf_attr_add(die, &atref, NULL, error);
		if (atref.at_attrib == DW_AT_name) {
			switch (atref.at_form) {
			case DW_FORM_strp:
				ret_die->die_name = atref.u[1].s;
				break;
			case DW_FORM_string:
				ret_die->die_name = atref.u[0].s;
				break;
			default:
				break;
			}
		}
		ret_die->die_attr[ret_die->die_attr_count++] = atref;
	}

	return (ret);
}

int
dwarf_search_die_within_cu(Dwarf_Debug dbg, Dwarf_CU cu, uint64_t offset, Dwarf_Die *ret_die, int search_sibling)
{
	Dwarf_Abbrev ab;
	Dwarf_AttrDef ad;
	uint64_t abnum;
	uint64_t die_offset;
	int ret, level;
	int i;

	assert(dbg);
	//assert(cu);
	assert(ret_die);

	level = 1;

	while (offset < cu.cu_next_offset && offset < dbg->dbg_info_size) {

		die_offset = offset;

		abnum = _dwarf_read_uleb128((uint8_t *)dbg->dbg_info_offset_elf, &offset);

		if (abnum == 0) {
			if (level == 0 || !search_sibling) {
				//No more entry
				return (DW_DLE_NO_ENTRY);
			}
			/*
			 * Return to previous DIE level.
			 */
			level--;
			continue;
		}

		if ((ret = _dwarf_abbrev_find(dbg, cu, abnum, &ab)) != DW_DLE_NONE)
			return (ret);
		ret_die->die_offset = die_offset;
		ret_die->die_abnum  = abnum;
		ret_die->die_ab  = ab;
		ret_die->die_attr_count = 0;
		ret_die->die_tag = ab.ab_tag;
		//ret_die->die_cu  = cu;
		//ret_die->die_dbg = cu->cu_dbg;

		for(i=0; i < ab.ab_atnum; i++)
		{
			if ((ret = _dwarf_attr_init(dbg, &offset, &cu, ret_die, &ab.ab_attrdef[i], ab.ab_attrdef[i].ad_form, 0)) != DW_DLE_NONE)
				return (ret);
		}

		ret_die->die_next_off = offset;
		if (search_sibling && level > 0) {
			//dwarf_dealloc(dbg, die, DW_DLA_DIE);
			if (ab.ab_children == DW_CHILDREN_yes) {
				/* Advance to next DIE level. */
				level++;
			}
		} else {
			//*ret_die = die;
			return (DW_DLE_NONE);
		}
	}

	return (DW_DLE_NO_ENTRY);
}

//Return 0 on success
int
dwarf_offdie(Dwarf_Debug dbg, uint64_t offset, Dwarf_Die *ret_die, Dwarf_CU cu)
{
	int ret;

	assert(dbg);
	assert(ret_die);

	/* First search the current CU. */
	if (offset < cu.cu_next_offset) {
		ret = dwarf_search_die_within_cu(dbg, cu, offset, ret_die, 0);
		return ret;
	}

	/*TODO: Search other CU*/
	return DW_DLV_OK;
}

Dwarf_Attribute*
_dwarf_attr_find(Dwarf_Die *die, uint16_t attr)
{
	Dwarf_Attribute *myat = NULL;
	int i;
    
	for(i=0; i < die->die_attr_count; i++)
	{
		if (die->die_attr[i].at_attrib == attr)
		{
			myat = &(die->die_attr[i]);
			break;
		}
	}

	return myat;
}

//Return 0 on success
int
dwarf_siblingof(Dwarf_Debug dbg, Dwarf_Die *die, Dwarf_Die *ret_die,
		Dwarf_CU *cu)
{
	Dwarf_Attribute *at;
	uint64_t offset;
	int ret, search_sibling;

	assert(dbg);
	assert(ret_die);
	assert(cu);

	/* Application requests the first DIE in this CU. */
	if (die == NULL)
		return (dwarf_offdie(dbg, cu->cu_die_offset, ret_die, *cu));

	/*
	 * If the DIE doesn't have any children, its sibling sits next
	 * right to it.
	 */
	search_sibling = 0;
	if (die->die_ab.ab_children == DW_CHILDREN_no)
		offset = die->die_next_off;
	else {
		/*
		 * Look for DW_AT_sibling attribute for the offset of
		 * its sibling.
		 */
		if ((at = _dwarf_attr_find(die, DW_AT_sibling)) != NULL) {
			if (at->at_form != DW_FORM_ref_addr)
				offset = at->u[0].u64 + cu->cu_offset;
			else
				offset = at->u[0].u64;
		} else {
			offset = die->die_next_off;
			search_sibling = 1;
		}
	}

	ret = dwarf_search_die_within_cu(dbg, *cu, offset, ret_die, search_sibling);


	if (ret == DW_DLE_NO_ENTRY) {
		return (DW_DLV_NO_ENTRY);
	} else if (ret != DW_DLE_NONE)
		return (DW_DLV_ERROR);


	return (DW_DLV_OK);
}

int
dwarf_child(Dwarf_Debug dbg, Dwarf_CU *cu, Dwarf_Die *die, Dwarf_Die *ret_die)
{
	int ret;

	assert(die);
	assert(ret_die);
	assert(dbg);
	assert(cu);

	if (die->die_ab.ab_children == DW_CHILDREN_no)
		return (DW_DLE_NO_ENTRY);

	ret = dwarf_search_die_within_cu(dbg, *cu, die->die_next_off, ret_die, 0);

	if (ret == DW_DLE_NO_ENTRY) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_NO_ENTRY);
		return (DW_DLV_NO_ENTRY);
	} else if (ret != DW_DLE_NONE)
		return (DW_DLV_ERROR);

	return (DW_DLV_OK);
}


int  _dwarf_find_section_enhanced(Dwarf_Section *ds)
{
	Dwarf_Section *secthdr = _dwarf_find_section(ds->ds_name);
	ds->ds_data = secthdr->ds_data;//(Dwarf_Small*)((uint8_t *)elf_base_ptr + secthdr->sh_offset);
	ds->ds_addr = secthdr->ds_addr;
	ds->ds_size = secthdr->ds_size;
	return 0;
}

