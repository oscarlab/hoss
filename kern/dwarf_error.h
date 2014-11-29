#ifndef DWARF_ERROR_H_
#define DWARF_ERROR_H_

#define DW_DLV_NO_ENTRY  -1
#define DW_DLV_OK         0
#define DW_DLV_ERROR      1

enum {
        DW_DLE_NONE,                    /* No error. */
        DW_DLE_ERROR,                   /* An error! */
        DW_DLE_ARGUMENT,                /* Invalid argument. */
        DW_DLE_DEBUG_INFO_NULL,         /* Debug info NULL. */
        DW_DLE_NO_ENTRY,                /* No entry. */
        DW_DLE_MEMORY,                  /* Insufficient memory. */
        DW_DLE_ELF,                     /* ELF error. */
        DW_DLE_CU_LENGTH_ERROR,         /* Invalid compilation unit data. */
        DW_DLE_VERSION_STAMP_ERROR,     /* Invalid version. */
        DW_DLE_DEBUG_ABBREV_NULL,       /* Abbrev not found. */
        DW_DLE_DIE_NO_CU_CONTEXT,       /* No current compilation unit. */
        DW_DLE_LOC_EXPR_BAD,            /* Invalid location expression. */
        DW_DLE_EXPR_LENGTH_BAD,         /* Invalid DWARF expression. */
        DW_DLE_DEBUG_LOC_SECTION_SHORT, /* Loclist section too short. */
        DW_DLE_ATTR_FORM_BAD,           /* Invalid attribute form. */
        DW_DLE_DEBUG_LINE_LENGTH_BAD,   /* Line info section too short. */
        DW_DLE_LINE_FILE_NUM_BAD,       /* Invalid file number. */
        DW_DLE_DIR_INDEX_BAD,           /* Invalid dir index. */
        DW_DLE_DEBUG_FRAME_LENGTH_BAD,  /* Frame section too short. */
        DW_DLE_NO_CIE_FOR_FDE,          /* CIE not found for certain FDE. */
        DW_DLE_FRAME_AUGMENTATION_UNKNOWN, /* Unknown CIE augmentation. */
        DW_DLE_FRAME_INSTR_EXEC_ERROR,  /* Frame instruction exec error. */
        DW_DLE_FRAME_VERSION_BAD,       /* Invalid frame section version. */
        DW_DLE_FRAME_TABLE_COL_BAD,     /* Invalid table column. */
        DW_DLE_DF_REG_NUM_TOO_HIGH,     /* Insufficient regtable space. */
        DW_DLE_PC_NOT_IN_FDE_RANGE,     /* PC requested not in the FDE range. */
        DW_DLE_ARANGE_OFFSET_BAD,       /* Invalid arange offset. */
        DW_DLE_DEBUG_MACRO_INCONSISTENT,/* Invalid macinfo data. */
        DW_DLE_ELF_SECT_ERR,            /* Application callback failed. */
        DW_DLE_NUM                      /* Max error number. */
};

#define DWARF_SET_ERROR(x, y ,z)  {  }

#endif
