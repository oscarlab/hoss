#include <inc/types.h>
#include <inc/string.h>
#include <inc/assert.h>
#include "dwarf_error.h"
#include "dwarf_define.h"
#include "dwarf.h"

static int _dwarf_get_next_fde(Dwarf_Debug, int , Dwarf_Error *, Dwarf_Fde);

Dwarf_Section debug_frame_sec = {".eh_frame", 0, 0, 0};

Dwarf_Regtable3 global_rt_table = {{0}};
Dwarf_Regtable_Entry3 global_rules[DW_FRAME_LAST_REG_NUM];

Dwarf_Regtable3 global_rt_table_shadow = {{0}};
Dwarf_Regtable_Entry3 global_rules_shadow[DW_FRAME_LAST_REG_NUM];

uint64_t
_dwarf_read_lsb(uint8_t *data, uint64_t *offsetp, int bytes_to_read);
uint64_t
_dwarf_decode_lsb(uint8_t **data, int bytes_to_read);
uint64_t
_dwarf_read_msb(uint8_t *data, uint64_t *offsetp, int bytes_to_read);
uint64_t
_dwarf_decode_msb(uint8_t **data, int bytes_to_read);
int64_t
_dwarf_read_sleb128(uint8_t *data, uint64_t *offsetp);
uint64_t
_dwarf_read_uleb128(uint8_t *data, uint64_t *offsetp);
int64_t
_dwarf_decode_sleb128(uint8_t **dp);
uint64_t
_dwarf_decode_uleb128(uint8_t **dp);

static int
_dwarf_frame_set_fde(Dwarf_Debug dbg, Dwarf_Fde retfde, Dwarf_Section *ds,
		     Dwarf_Unsigned *off, int eh_frame, Dwarf_Cie cie, Dwarf_Error *error);

int _dwarf_frame_section_load_eh(Dwarf_Debug dbg, Dwarf_Error *error);

extern int  _dwarf_find_section_enhanced(Dwarf_Section *ds);

void
_dwarf_frame_params_init(Dwarf_Debug dbg)
{
	/* Initialise call frame related parameters. */
	dbg->dbg_frame_rule_table_size = DW_FRAME_LAST_REG_NUM;
	dbg->dbg_frame_rule_initial_value = DW_FRAME_REG_INITIAL_VALUE;
	dbg->dbg_frame_cfa_value = DW_FRAME_CFA_COL3;
	dbg->dbg_frame_same_value = DW_FRAME_SAME_VAL;
	dbg->dbg_frame_undefined_value = DW_FRAME_UNDEFINED_VAL;
}


int
dwarf_get_fde_at_pc(Dwarf_Debug dbg, Dwarf_Addr pc,
		    struct _Dwarf_Fde *ret_fde, Dwarf_Cie cie,
		    Dwarf_Error *error)
{
	Dwarf_Fde fde = ret_fde;
	memset(fde, 0, sizeof(struct _Dwarf_Fde));
	fde->fde_cie = cie;
	
	if (ret_fde == NULL)
		return (DW_DLV_ERROR);

	while(dbg->curr_off_eh < dbg->dbg_eh_size) {
		if (_dwarf_get_next_fde(dbg, true, error, fde) < 0)
		{
			return DW_DLV_NO_ENTRY;
		}
		if (pc >= fde->fde_initloc && pc < fde->fde_initloc +
		    fde->fde_adrange)
			return (DW_DLV_OK);
	}

	DWARF_SET_ERROR(dbg, error, DW_DLE_NO_ENTRY);
	return (DW_DLV_NO_ENTRY);
}

int
_dwarf_frame_regtable_copy(Dwarf_Debug dbg, Dwarf_Regtable3 **dest,
			   Dwarf_Regtable3 *src, Dwarf_Error *error)
{
	int i;

	assert(dest != NULL);
	assert(src != NULL);

	if (*dest == NULL) {
		*dest = &global_rt_table_shadow;
		(*dest)->rt3_reg_table_size = src->rt3_reg_table_size;
		(*dest)->rt3_rules = global_rules_shadow;
	}

	memcpy(&(*dest)->rt3_cfa_rule, &src->rt3_cfa_rule,
	       sizeof(Dwarf_Regtable_Entry3));

	for (i = 0; i < (*dest)->rt3_reg_table_size &&
		     i < src->rt3_reg_table_size; i++)
		memcpy(&(*dest)->rt3_rules[i], &src->rt3_rules[i],
		       sizeof(Dwarf_Regtable_Entry3));

	for (; i < (*dest)->rt3_reg_table_size; i++)
		(*dest)->rt3_rules[i].dw_regnum =
			dbg->dbg_frame_undefined_value;

	return (DW_DLE_NONE);
}

static int
_dwarf_frame_run_inst(Dwarf_Debug dbg, Dwarf_Regtable3 *rt, uint8_t *insts,
		      Dwarf_Unsigned len, Dwarf_Unsigned caf, Dwarf_Signed daf, Dwarf_Addr pc,
		      Dwarf_Addr pc_req, Dwarf_Addr *row_pc, Dwarf_Error *error)
{
	Dwarf_Regtable3 *init_rt, *saved_rt;
	uint8_t *p, *pe;
	uint8_t high2, low6;
	uint64_t reg, reg2, uoff, soff;
	int ret;

#define CFA     rt->rt3_cfa_rule
#define INITCFA init_rt->rt3_cfa_rule
#define RL      rt->rt3_rules
#define INITRL  init_rt->rt3_rules

#define CHECK_TABLE_SIZE(x)                                             \
	do {                                                            \
		if ((x) >= rt->rt3_reg_table_size) {                    \
			DWARF_SET_ERROR(dbg, error,                     \
					DW_DLE_DF_REG_NUM_TOO_HIGH);	\
			ret = DW_DLE_DF_REG_NUM_TOO_HIGH;               \
			goto program_done;                              \
		}                                                       \
	} while(0)

	ret = DW_DLE_NONE;
	init_rt = saved_rt = NULL;
	*row_pc = pc;

	/* Save a copy of the table as initial state. */
	_dwarf_frame_regtable_copy(dbg, &init_rt, rt, error);
	p = insts;
	pe = p + len;

	while (p < pe) {
		if (*p == DW_CFA_nop) {
			p++;
			continue;
		}

		high2 = *p & 0xc0;
		low6 = *p & 0x3f;
		p++;

		if (high2 > 0) {
			switch (high2) {
			case DW_CFA_advance_loc:
			        pc += low6 * caf;
			        if (pc_req < pc)
			                goto program_done;
			        break;
			case DW_CFA_offset:
			        *row_pc = pc;
			        CHECK_TABLE_SIZE(low6);
			        RL[low6].dw_offset_relevant = 1;
			        RL[low6].dw_value_type = DW_EXPR_OFFSET;
			        RL[low6].dw_regnum = dbg->dbg_frame_cfa_value;
			        RL[low6].dw_offset_or_block_len =
					_dwarf_decode_uleb128(&p) * daf;
			        break;
			case DW_CFA_restore:
			        *row_pc = pc;
			        CHECK_TABLE_SIZE(low6);
			        memcpy(&RL[low6], &INITRL[low6],
				       sizeof(Dwarf_Regtable_Entry3));
			        break;
			default:
			        DWARF_SET_ERROR(dbg, error,
						DW_DLE_FRAME_INSTR_EXEC_ERROR);
			        ret = DW_DLE_FRAME_INSTR_EXEC_ERROR;
			        goto program_done;
			}

			continue;
		}

		switch (low6) {
		case DW_CFA_set_loc:
			pc = dbg->decode(&p, dbg->dbg_pointer_size);
			if (pc_req < pc)
			        goto program_done;
			break;
		case DW_CFA_advance_loc1:
			pc += dbg->decode(&p, 1) * caf;
			if (pc_req < pc)
			        goto program_done;
			break;
		case DW_CFA_advance_loc2:
			pc += dbg->decode(&p, 2) * caf;
			if (pc_req < pc)
			        goto program_done;
			break;
		case DW_CFA_advance_loc4:
			pc += dbg->decode(&p, 4) * caf;
			if (pc_req < pc)
			        goto program_done;
			break;
		case DW_CFA_offset_extended:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			uoff = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 1;
			RL[reg].dw_value_type = DW_EXPR_OFFSET;
			RL[reg].dw_regnum = dbg->dbg_frame_cfa_value;
			RL[reg].dw_offset_or_block_len = uoff * daf;
			break;
		case DW_CFA_restore_extended:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			memcpy(&RL[reg], &INITRL[reg],
			       sizeof(Dwarf_Regtable_Entry3));
			break;
		case DW_CFA_undefined:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 0;
			RL[reg].dw_regnum = dbg->dbg_frame_undefined_value;
			break;
		case DW_CFA_same_value:
			reg = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 0;
			RL[reg].dw_regnum = dbg->dbg_frame_same_value;
			break;
		case DW_CFA_register:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			reg2 = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 0;
			RL[reg].dw_regnum = reg2;
			break;
		case DW_CFA_remember_state:
			_dwarf_frame_regtable_copy(dbg, &saved_rt, rt, error);
			break;
		case DW_CFA_restore_state:
			*row_pc = pc;
			_dwarf_frame_regtable_copy(dbg, &rt, saved_rt, error);
			break;
		case DW_CFA_def_cfa:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			uoff = _dwarf_decode_uleb128(&p);
			CFA.dw_offset_relevant = 1;
			CFA.dw_value_type = DW_EXPR_OFFSET;
			CFA.dw_regnum = reg;
			CFA.dw_offset_or_block_len = uoff;
			break;
		case DW_CFA_def_cfa_register:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			CFA.dw_regnum = reg;
			/*
			 * Note that DW_CFA_def_cfa_register change the CFA
			 * rule register while keep the old offset. So we
			 * should not touch the CFA.dw_offset_relevant flag
			 * here.
			 */
			break;
		case DW_CFA_def_cfa_offset:
			*row_pc = pc;
			uoff = _dwarf_decode_uleb128(&p);
			CFA.dw_offset_relevant = 1;
			CFA.dw_value_type = DW_EXPR_OFFSET;
			CFA.dw_offset_or_block_len = uoff;
			break;
		case DW_CFA_def_cfa_expression:
			*row_pc = pc;
			CFA.dw_offset_relevant = 0;
			CFA.dw_value_type = DW_EXPR_EXPRESSION;
			CFA.dw_offset_or_block_len = _dwarf_decode_uleb128(&p);
			CFA.dw_block_ptr = p;
			p += CFA.dw_offset_or_block_len;
			break;
		case DW_CFA_expression:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 0;
			RL[reg].dw_value_type = DW_EXPR_EXPRESSION;
			RL[reg].dw_offset_or_block_len =
				_dwarf_decode_uleb128(&p);
			RL[reg].dw_block_ptr = p;
			p += RL[reg].dw_offset_or_block_len;
			break;
		case DW_CFA_offset_extended_sf:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			soff = _dwarf_decode_sleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 1;
			RL[reg].dw_value_type = DW_EXPR_OFFSET;
			RL[reg].dw_regnum = dbg->dbg_frame_cfa_value;
			RL[reg].dw_offset_or_block_len = soff * daf;
			break;
		case DW_CFA_def_cfa_sf:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			soff = _dwarf_decode_sleb128(&p);
			CFA.dw_offset_relevant = 1;
			CFA.dw_value_type = DW_EXPR_OFFSET;
			CFA.dw_regnum = reg;
			CFA.dw_offset_or_block_len = soff * daf;
			break;
		case DW_CFA_def_cfa_offset_sf:
			*row_pc = pc;
			soff = _dwarf_decode_sleb128(&p);
			CFA.dw_offset_relevant = 1;
			CFA.dw_value_type = DW_EXPR_OFFSET;
			CFA.dw_offset_or_block_len = soff * daf;
			break;
		case DW_CFA_val_offset:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			uoff = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 1;
			RL[reg].dw_value_type = DW_EXPR_VAL_OFFSET;
			RL[reg].dw_regnum = dbg->dbg_frame_cfa_value;
			RL[reg].dw_offset_or_block_len = uoff * daf;
			break;
		case DW_CFA_val_offset_sf:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			soff = _dwarf_decode_sleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 1;
			RL[reg].dw_value_type = DW_EXPR_VAL_OFFSET;
			RL[reg].dw_regnum = dbg->dbg_frame_cfa_value;
			RL[reg].dw_offset_or_block_len = soff * daf;
			break;
		case DW_CFA_val_expression:
			*row_pc = pc;
			reg = _dwarf_decode_uleb128(&p);
			CHECK_TABLE_SIZE(reg);
			RL[reg].dw_offset_relevant = 0;
			RL[reg].dw_value_type = DW_EXPR_VAL_EXPRESSION;
			RL[reg].dw_offset_or_block_len =
				_dwarf_decode_uleb128(&p);
			RL[reg].dw_block_ptr = p;
			p += RL[reg].dw_offset_or_block_len;
			break;
		default:
			DWARF_SET_ERROR(dbg, error,
					DW_DLE_FRAME_INSTR_EXEC_ERROR);
			ret = DW_DLE_FRAME_INSTR_EXEC_ERROR;
			goto program_done;
		}
	}

program_done:
	return (ret);

#undef  CFA
#undef  INITCFA
#undef  RL
#undef  INITRL
#undef  CHECK_TABLE_SIZE
}

int
_dwarf_frame_get_internal_table(Dwarf_Debug dbg, Dwarf_Fde fde,
				Dwarf_Addr pc_req, Dwarf_Regtable3 **ret_rt,
				Dwarf_Addr *ret_row_pc,
				Dwarf_Error *error)
{
	//Dwarf_Debug dbg;
	Dwarf_Cie cie;
	Dwarf_Regtable3 *rt;
	Dwarf_Addr row_pc;
	int i, ret;

	assert(ret_rt != NULL);

	//dbg = fde->fde_dbg;
	assert(dbg != NULL);

	rt = dbg->dbg_internal_reg_table;

	/* Clear the content of regtable from previous run. */
	memset(&rt->rt3_cfa_rule, 0, sizeof(Dwarf_Regtable_Entry3));
	memset(rt->rt3_rules, 0, rt->rt3_reg_table_size *
	       sizeof(Dwarf_Regtable_Entry3));

	/* Set rules to initial values. */
	for (i = 0; i < rt->rt3_reg_table_size; i++)
		rt->rt3_rules[i].dw_regnum = dbg->dbg_frame_rule_initial_value;

	/* Run initial instructions in CIE. */
	cie = fde->fde_cie;
	assert(cie != NULL);
	ret = _dwarf_frame_run_inst(dbg, rt, cie->cie_initinst,
				    cie->cie_instlen, cie->cie_caf,
				    cie->cie_daf, 0, ~0ULL,
				    &row_pc, error);
	if (ret != DW_DLE_NONE)
		return (ret);
	/* Run instructions in FDE. */
	if (pc_req >= fde->fde_initloc) {
		ret = _dwarf_frame_run_inst(dbg, rt, fde->fde_inst,
					    fde->fde_instlen, cie->cie_caf,
					    cie->cie_daf,
					    fde->fde_initloc, pc_req,
					    &row_pc, error);
		if (ret != DW_DLE_NONE)
			return (ret);
	}

	*ret_rt = rt;
	*ret_row_pc = row_pc;

	return (DW_DLE_NONE);
}


int
dwarf_get_fde_info_for_all_regs(Dwarf_Debug dbg, Dwarf_Fde fde,
				Dwarf_Addr pc_requested,
				Dwarf_Regtable *reg_table, Dwarf_Addr *row_pc,
				Dwarf_Error *error)
{
	//Dwarf_Debug dbg;
	Dwarf_Regtable3 *rt;
	Dwarf_Addr pc;
	Dwarf_Half cfa;
	int i, ret;

	if (fde == NULL || reg_table == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	assert(dbg != NULL);

	if (pc_requested < fde->fde_initloc ||
	    pc_requested >= fde->fde_initloc + fde->fde_adrange) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_PC_NOT_IN_FDE_RANGE);
		return (DW_DLV_ERROR);
	}

	ret = _dwarf_frame_get_internal_table(dbg, fde, pc_requested, &rt, &pc,
					      error);
	if (ret != DW_DLE_NONE)
		return (DW_DLV_ERROR);

	/*
	 * Copy the CFA rule to the column intended for holding the CFA,
	 * if it's within the range of regtable.
	 */
#define CFA rt->rt3_cfa_rule
	cfa = dbg->dbg_frame_cfa_value;
	if (cfa < DW_REG_TABLE_SIZE) {
		reg_table->rules[cfa].dw_offset_relevant =
			CFA.dw_offset_relevant;
		reg_table->rules[cfa].dw_value_type = CFA.dw_value_type;
		reg_table->rules[cfa].dw_regnum = CFA.dw_regnum;
		reg_table->rules[cfa].dw_offset = CFA.dw_offset_or_block_len;
		reg_table->cfa_rule = reg_table->rules[cfa];
	} else {
		reg_table->cfa_rule.dw_offset_relevant =
		    CFA.dw_offset_relevant;
		reg_table->cfa_rule.dw_value_type = CFA.dw_value_type;
		reg_table->cfa_rule.dw_regnum = CFA.dw_regnum;
		reg_table->cfa_rule.dw_offset = CFA.dw_offset_or_block_len;
	}

	/*
	 * Copy other columns.
	 */
	for (i = 0; i < DW_REG_TABLE_SIZE && i < dbg->dbg_frame_rule_table_size;
	     i++) {

		/* Do not overwrite CFA column */
		if (i == cfa)
			continue;

		reg_table->rules[i].dw_offset_relevant =
			rt->rt3_rules[i].dw_offset_relevant;
		reg_table->rules[i].dw_value_type =
			rt->rt3_rules[i].dw_value_type;
		reg_table->rules[i].dw_regnum = rt->rt3_rules[i].dw_regnum;
		reg_table->rules[i].dw_offset =
			rt->rt3_rules[i].dw_offset_or_block_len;
	}

	if (row_pc) *row_pc = pc;
	return (DW_DLV_OK);
}

static int
_dwarf_frame_read_lsb_encoded(Dwarf_Debug dbg, uint64_t *val, uint8_t *data,
			      uint64_t *offsetp, uint8_t encode, Dwarf_Addr pc, Dwarf_Error *error)
{
	uint8_t application;

	if (encode == DW_EH_PE_omit)
		return (DW_DLE_NONE);

	application = encode & 0xf0;
	encode &= 0x0f;

	switch (encode) {
	case DW_EH_PE_absptr:
		*val = dbg->read(data, offsetp, dbg->dbg_pointer_size);
		break;
	case DW_EH_PE_uleb128:
		*val = _dwarf_read_uleb128(data, offsetp);
		break;
	case DW_EH_PE_udata2:
		*val = dbg->read(data, offsetp, 2);
		break;
	case DW_EH_PE_udata4:
		*val = dbg->read(data, offsetp, 4);
		break;
	case DW_EH_PE_udata8:
		*val = dbg->read(data, offsetp, 8);
		break;
	case DW_EH_PE_sleb128:
		*val = _dwarf_read_sleb128(data, offsetp);
		break;
	case DW_EH_PE_sdata2:
		*val = (int16_t) dbg->read(data, offsetp, 2);
		break;
	case DW_EH_PE_sdata4:
		*val = (int32_t) dbg->read(data, offsetp, 4);
		break;
	case DW_EH_PE_sdata8:
		*val = dbg->read(data, offsetp, 8);
		break;
	default:
		DWARF_SET_ERROR(dbg, error, DW_DLE_FRAME_AUGMENTATION_UNKNOWN);
		return (DW_DLE_FRAME_AUGMENTATION_UNKNOWN);
	}

	if (application == DW_EH_PE_pcrel) {
		/*
		 * Value is relative to .eh_frame section virtual addr.
		 */
		switch (encode) {
		case DW_EH_PE_uleb128:
		case DW_EH_PE_udata2:
		case DW_EH_PE_udata4:
		case DW_EH_PE_udata8:
			*val += pc;
			break;
		case DW_EH_PE_sleb128:
		case DW_EH_PE_sdata2:
		case DW_EH_PE_sdata4:
		case DW_EH_PE_sdata8:
			*val = pc + (int64_t) *val;
			break;
		default:
			/* DW_EH_PE_absptr is absolute value. */
			break;
		}
	}

	/* XXX Applications other than DW_EH_PE_pcrel are not handled. */

	return (DW_DLE_NONE);
}

static int
_dwarf_frame_parse_lsb_cie_augment(Dwarf_Debug dbg, Dwarf_Cie cie,
				   Dwarf_Error *error)
{
	uint8_t *aug_p, *augdata_p;
	uint64_t val, offset;
	uint8_t encode;
	int ret;

	assert(cie->cie_augment != NULL && *cie->cie_augment == 'z');

	/*
	 * Here we're only interested in the presence of augment 'R'
	 * and associated CIE augment data, which describes the
	 * encoding scheme of FDE PC begin and range.
	 */
	aug_p = &cie->cie_augment[1];
	augdata_p = cie->cie_augdata;
	while (*aug_p != '\0') {
		switch (*aug_p) {
		case 'L':
			/* Skip one augment in augment data. */
			augdata_p++;
			break;
		case 'P':
			/* Skip two augments in augment data. */
			encode = *augdata_p++;
			offset = 0;
			ret = _dwarf_frame_read_lsb_encoded(dbg, &val,
							    augdata_p, &offset, encode, 0, error);
			if (ret != DW_DLE_NONE)
				return (ret);
			augdata_p += offset;
			break;
		case 'R':
			cie->cie_fde_encode = *augdata_p++;
			break;
		default:
			DWARF_SET_ERROR(dbg, error,
					DW_DLE_FRAME_AUGMENTATION_UNKNOWN);
			return (DW_DLE_FRAME_AUGMENTATION_UNKNOWN);
		}
		aug_p++;
	}

	return (DW_DLE_NONE);
}


static int
_dwarf_frame_set_cie(Dwarf_Debug dbg, Dwarf_Section *ds,
		     Dwarf_Unsigned *off, Dwarf_Cie ret_cie, Dwarf_Error *error)
{
	Dwarf_Cie cie;
	uint64_t length;
	int dwarf_size, ret;
	char *p;

	assert(ret_cie);
	cie = ret_cie;

	cie->cie_dbg = dbg;
	cie->cie_offset = *off;

	length = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 4);
	if (length == 0xffffffff) {
		dwarf_size = 8;
		length = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 8);
	} else
		dwarf_size = 4;

	if (length > dbg->dbg_eh_size - *off) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_DEBUG_FRAME_LENGTH_BAD);
		return (DW_DLE_DEBUG_FRAME_LENGTH_BAD);
	}

	(void) dbg->read((uint8_t *)dbg->dbg_eh_offset, off, dwarf_size); /* Skip CIE id. */
	cie->cie_length = length;

	cie->cie_version = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 1);
	if (cie->cie_version != 1 && cie->cie_version != 3 &&
	    cie->cie_version != 4) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_FRAME_VERSION_BAD);
		return (DW_DLE_FRAME_VERSION_BAD);
	}

	cie->cie_augment = (uint8_t *)dbg->dbg_eh_offset + *off;
	p = (char *)dbg->dbg_eh_offset;
	while (p[(*off)++] != '\0')
		;

	/* We only recognize normal .dwarf_frame and GNU .eh_frame sections. */
	if (*cie->cie_augment != 0 && *cie->cie_augment != 'z') {
		*off = cie->cie_offset + ((dwarf_size == 4) ? 4 : 12) +
			cie->cie_length;
		return (DW_DLE_NONE);
	}

	/* Optional EH Data field for .eh_frame section. */
	if (strstr((char *)cie->cie_augment, "eh") != NULL)
		cie->cie_ehdata = dbg->read((uint8_t *)dbg->dbg_eh_offset, off,
					    dbg->dbg_pointer_size);

	cie->cie_caf = _dwarf_read_uleb128((uint8_t *)dbg->dbg_eh_offset, off);
	cie->cie_daf = _dwarf_read_sleb128((uint8_t *)dbg->dbg_eh_offset, off);

	/* Return address register. */
	if (cie->cie_version == 1)
		cie->cie_ra = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 1);
	else
		cie->cie_ra = _dwarf_read_uleb128((uint8_t *)dbg->dbg_eh_offset, off);

	/* Optional CIE augmentation data for .eh_frame section. */
	if (*cie->cie_augment == 'z') {
		cie->cie_auglen = _dwarf_read_uleb128((uint8_t *)dbg->dbg_eh_offset, off);
		cie->cie_augdata = (uint8_t *)dbg->dbg_eh_offset + *off;
		*off += cie->cie_auglen;
		/*
		 * XXX Use DW_EH_PE_absptr for default FDE PC start/range,
		 * in case _dwarf_frame_parse_lsb_cie_augment fails to
		 * find out the real encode.
		 */
		cie->cie_fde_encode = DW_EH_PE_absptr;
		ret = _dwarf_frame_parse_lsb_cie_augment(dbg, cie, error);
		if (ret != DW_DLE_NONE)
			return (ret);
	}

	/* CIE Initial instructions. */
	cie->cie_initinst = (uint8_t *)dbg->dbg_eh_offset + *off;
	if (dwarf_size == 4)
		cie->cie_instlen = cie->cie_offset + 4 + length - *off;
	else
		cie->cie_instlen = cie->cie_offset + 12 + length - *off;

	*off += cie->cie_instlen;
	return (DW_DLE_NONE);
}

static int
_dwarf_frame_set_fde(Dwarf_Debug dbg, Dwarf_Fde ret_fde, Dwarf_Section *ds,
		     Dwarf_Unsigned *off, int eh_frame, Dwarf_Cie cie, Dwarf_Error *error)
{
	Dwarf_Fde fde;
	Dwarf_Unsigned cieoff;
	uint64_t length, val;
	int dwarf_size, ret;

	fde = ret_fde;
	fde->fde_dbg = dbg;
	fde->fde_addr = (uint8_t *)dbg->dbg_eh_offset + *off;
	fde->fde_offset = *off;

	length = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 4);
	if (length == 0xffffffff) {
		dwarf_size = 8;
		length = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 8);
	} else
		dwarf_size = 4;

	if (length > dbg->dbg_eh_size - *off) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_DEBUG_FRAME_LENGTH_BAD);
		return (DW_DLE_DEBUG_FRAME_LENGTH_BAD);
	}

	fde->fde_length = length;

	if (eh_frame) {
		fde->fde_cieoff = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, 4);
		cieoff = *off - (4 + fde->fde_cieoff);
		/* This delta should never be 0. */
		if (cieoff == fde->fde_offset) {
			DWARF_SET_ERROR(dbg, error, DW_DLE_NO_CIE_FOR_FDE);
			return (DW_DLE_NO_CIE_FOR_FDE);
		}
	} else {
		fde->fde_cieoff = dbg->read((uint8_t *)dbg->dbg_eh_offset, off, dwarf_size);
		cieoff = fde->fde_cieoff;
	}

	if (eh_frame) {
		/*
		 * The FDE PC start/range for .eh_frame is encoded according
		 * to the LSB spec's extension to DWARF2.
		 */
		ret = _dwarf_frame_read_lsb_encoded(dbg, &val,
						    (uint8_t *)dbg->dbg_eh_offset,
						    off, cie->cie_fde_encode, ds->ds_addr + *off, error);
		if (ret != DW_DLE_NONE)
			return (ret);
		fde->fde_initloc = val;
		/*
		 * FDE PC range should not be relative value to anything.
		 * So pass 0 for pc value.
		 */
		ret = _dwarf_frame_read_lsb_encoded(dbg, &val,
						    (uint8_t *)dbg->dbg_eh_offset,
						    off, cie->cie_fde_encode, 0, error);
		if (ret != DW_DLE_NONE)
			return (ret);
		fde->fde_adrange = val;
	} else {
		fde->fde_initloc = dbg->read((uint8_t *)dbg->dbg_eh_offset, off,
					     dbg->dbg_pointer_size);
		fde->fde_adrange = dbg->read((uint8_t *)dbg->dbg_eh_offset, off,
					     dbg->dbg_pointer_size);
	}

	/* Optional FDE augmentation data for .eh_frame section. (ignored) */
	if (eh_frame && *cie->cie_augment == 'z') {
		fde->fde_auglen = _dwarf_read_uleb128((uint8_t *)dbg->dbg_eh_offset, off);
		fde->fde_augdata = (uint8_t *)dbg->dbg_eh_offset + *off;
		*off += fde->fde_auglen;
	}

	fde->fde_inst = (uint8_t *)dbg->dbg_eh_offset + *off;
	if (dwarf_size == 4)
		fde->fde_instlen = fde->fde_offset + 4 + length - *off;
	else
		fde->fde_instlen = fde->fde_offset + 12 + length - *off;

	*off += fde->fde_instlen;
	return (DW_DLE_NONE);
}


int
_dwarf_frame_interal_table_init(Dwarf_Debug dbg, Dwarf_Error *error)
{
	Dwarf_Regtable3 *rt = &global_rt_table;

	if (dbg->dbg_internal_reg_table != NULL)
		return (DW_DLE_NONE);

	rt->rt3_reg_table_size = dbg->dbg_frame_rule_table_size;
	rt->rt3_rules = global_rules;

	dbg->dbg_internal_reg_table = rt;

	return (DW_DLE_NONE);
}

static int
_dwarf_get_next_fde(Dwarf_Debug dbg,
		    int eh_frame, Dwarf_Error *error, Dwarf_Fde ret_fde)
{
	Dwarf_Section *ds = &debug_frame_sec; 
	uint64_t length, offset, cie_id, entry_off;
	int dwarf_size, i, ret=-1;

	offset = dbg->curr_off_eh;
	if (offset < dbg->dbg_eh_size) {
		entry_off = offset;
		length = dbg->read((uint8_t *)dbg->dbg_eh_offset, &offset, 4);
		if (length == 0xffffffff) {
			dwarf_size = 8;
			length = dbg->read((uint8_t *)dbg->dbg_eh_offset, &offset, 8);
		} else
			dwarf_size = 4;

		if (length > dbg->dbg_eh_size - offset || (length == 0 && !eh_frame)) {
			DWARF_SET_ERROR(dbg, error,
					DW_DLE_DEBUG_FRAME_LENGTH_BAD);
			return (DW_DLE_DEBUG_FRAME_LENGTH_BAD);
		}

		/* Check terminator for .eh_frame */
		if (eh_frame && length == 0)
			return(-1);

		cie_id = dbg->read((uint8_t *)dbg->dbg_eh_offset, &offset, dwarf_size);

		if (eh_frame) {
			/* GNU .eh_frame use CIE id 0. */
			if (cie_id == 0)
				ret = _dwarf_frame_set_cie(dbg, ds,
							   &entry_off, ret_fde->fde_cie, error);
			else
				ret = _dwarf_frame_set_fde(dbg,ret_fde, ds,
							   &entry_off, 1, ret_fde->fde_cie, error);
		} else {
			/* .dwarf_frame use CIE id ~0 */
			if ((dwarf_size == 4 && cie_id == ~0U) ||
			    (dwarf_size == 8 && cie_id == ~0ULL))
				ret = _dwarf_frame_set_cie(dbg, ds,
							   &entry_off, ret_fde->fde_cie, error);
			else
				ret = _dwarf_frame_set_fde(dbg, ret_fde, ds,
							   &entry_off, 0, ret_fde->fde_cie, error);
		}

		if (ret != DW_DLE_NONE)
			return(-1);

		offset = entry_off;
		dbg->curr_off_eh = offset;
	}

	return (0);
}

Dwarf_Half
dwarf_set_frame_cfa_value(Dwarf_Debug dbg, Dwarf_Half value)
{
	Dwarf_Half old_value;

	old_value = dbg->dbg_frame_cfa_value;
	dbg->dbg_frame_cfa_value = value;

	return (old_value);
}

int dwarf_init_eh_section(Dwarf_Debug dbg, Dwarf_Error *error)
{
	Dwarf_Section *section;

	if (dbg == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	if (dbg->dbg_internal_reg_table == NULL) {
		if (_dwarf_frame_interal_table_init(dbg, error) != DW_DLE_NONE)
			return (DW_DLV_ERROR);
	}

	_dwarf_find_section_enhanced(&debug_frame_sec);

	dbg->curr_off_eh = 0;
	dbg->dbg_eh_offset = debug_frame_sec.ds_addr;
	dbg->dbg_eh_size = debug_frame_sec.ds_size;

	return (DW_DLV_OK);
}
