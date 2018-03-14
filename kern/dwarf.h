#ifndef DWARF_H
#define DWARF_H

#define NATTRDEF 32		//Maximum number of Attribute forms in abbreviation.
#define NATTRVAL 120	//Maximum number of attributes a Die can have.

struct _Dwarf_Die;


typedef int             Dwarf_Bool;
typedef off_t           Dwarf_Off;
typedef uint64_t        Dwarf_Unsigned;
typedef uint16_t        Dwarf_Half;
typedef uint8_t         Dwarf_Small;
typedef int64_t         Dwarf_Signed;
typedef uint64_t        Dwarf_Addr;
typedef void            *Dwarf_Ptr;

typedef struct _Dwarf_Debug     *Dwarf_Debug;
typedef struct _Dwarf_Fde       *Dwarf_Fde;
typedef struct _Dwarf_Fde       *Dwarf_P_Fde;
typedef struct _Dwarf_Cie       *Dwarf_Cie;


#ifndef DW_FRAME_HIGHEST_NORMAL_REGISTER
#define DW_FRAME_HIGHEST_NORMAL_REGISTER 63
#endif

#define DW_FRAME_RA_COL         (DW_FRAME_HIGHEST_NORMAL_REGISTER + 1)
#define DW_FRAME_STATIC_LINK    (DW_FRAME_HIGHEST_NORMAL_REGISTER + 2)

#ifndef DW_FRAME_LAST_REG_NUM
#define DW_FRAME_LAST_REG_NUM   (DW_FRAME_HIGHEST_NORMAL_REGISTER + 3)
#endif

#ifndef DW_FRAME_REG_INITIAL_VALUE
#define DW_FRAME_REG_INITIAL_VALUE DW_FRAME_SAME_VAL
#endif

#define DW_FRAME_UNDEFINED_VAL          1034
#define DW_FRAME_SAME_VAL               1035
#define DW_FRAME_CFA_COL3               1436

#define DW_EXPR_OFFSET 0
#define DW_EXPR_VAL_OFFSET 1
#define DW_EXPR_EXPRESSION 2
#define DW_EXPR_VAL_EXPRESSION 3

#define DW_FRAME_CFA_COL 0

#ifndef DW_REG_TABLE_SIZE
#define DW_REG_TABLE_SIZE       66
#endif

#define DW_OP_fbreg             0x91

typedef struct {
	Dwarf_Small     dw_offset_relevant;
	Dwarf_Small     dw_value_type;
	Dwarf_Half      dw_regnum;
	Dwarf_Unsigned  dw_offset_or_block_len;
	Dwarf_Ptr       dw_block_ptr;
} Dwarf_Regtable_Entry3;

typedef struct {
	Dwarf_Regtable_Entry3   rt3_cfa_rule;
	Dwarf_Half              rt3_reg_table_size;
	Dwarf_Regtable_Entry3   *rt3_rules;
} Dwarf_Regtable3;

typedef struct {
	Dwarf_Small     dw_offset_relevant;
	Dwarf_Small     dw_value_type;
	Dwarf_Half      dw_regnum;
	Dwarf_Addr      dw_offset;
} Dwarf_Regtable_Entry;

typedef struct {
	Dwarf_Regtable_Entry cfa_rule;
	Dwarf_Regtable_Entry rules[DW_REG_TABLE_SIZE];
} Dwarf_Regtable;

typedef struct _Dwarf_AttrDef {
	uint64_t    ad_attrib;      /* DW_AT_XXX */
	uint64_t    ad_form;        /* DW_FORM_XXX */
	uint64_t    ad_offset;      /* Offset in abbrev section. */
}Dwarf_AttrDef;

typedef struct _Dwarf_Abbrev {
	uint64_t  ab_entry;   /* Abbrev entry. */
	uint64_t  ab_tag;     /* Tag: DW_TAG_ */
	uint8_t   ab_children;    /* DW_CHILDREN_no or DW_CHILDREN_yes */
	uint64_t  ab_offset;  /* Offset in abbrev section. */
	uint64_t  ab_length;  /* Length of this abbrev entry. */
	uint64_t  ab_atnum;   /* Number of attribute defines. */
	Dwarf_AttrDef ab_attrdef[NATTRDEF];
}Dwarf_Abbrev;

typedef struct _Dwarf_CU
{
	//Following are header fields;
	uint64_t cu_length;	/* Length of CU (not including header) */
	uint16_t version;	/* Dwarf version, currently should either be 2 or 3. */
	uint8_t  addr_size;	/* 4bytes (32bit) or 8bytes(64bit) address */
	uint64_t debug_abbrev_offset;	/* Offset in .debug_abbrev for this CU */

	//Following are extra book-keeping stuff.
	uint8_t  cu_length_size;	/* Size in bytes of the length field. */
	uint8_t  cu_dwarf_size;		/* CU section dwarf size. */
	uint64_t cu_next_offset;	/* Offset to the next CU. Note this offset could be 32bit, just typecast while assigning*/
	uint64_t cu_die_offset;		/* First DIE offset. */
	uint64_t cu_offset;		/* Offset of this CUi */
}Dwarf_CU;

struct _Dwarf_Debug
{
	uint64_t  curr_off_dbginfo;		 /* Offset inside .debug_info currently using it for reading CU's */
	uint64_t  dbg_info_offset_elf;
	uint64_t  dbg_info_size;
	uint64_t  (*read)(uint8_t *, uint64_t *, int);
	uint64_t  (*decode)(uint8_t **, int);
	int       dbg_pointer_size;		/* Object address size. */

	uint64_t   curr_off_eh;
	uint64_t   dbg_eh_offset;
	uint64_t   dbg_eh_size;

	Dwarf_Half      dbg_frame_rule_table_size;
	Dwarf_Half      dbg_frame_rule_initial_value;
	Dwarf_Half      dbg_frame_cfa_value;
	Dwarf_Half      dbg_frame_same_value;
	Dwarf_Half      dbg_frame_undefined_value;

	Dwarf_Regtable3 *dbg_internal_reg_table;
};

typedef struct {
	uint8_t   lr_atom;
	uint64_t  lr_number;
	uint64_t  lr_number2;
	uint64_t  lr_offset;
} Dwarf_Loc;

typedef struct {
	uint64_t      ld_lopc;
	uint64_t      ld_hipc;
	uint16_t      ld_cents;
	Dwarf_Loc     ld_s;;
} Dwarf_Locdesc;

typedef struct {
	uint64_t   bl_len;
	void      *bl_data;
} Dwarf_Block;

typedef struct _Dwarf_Attribute {
	struct _Dwarf_Die  *at_die;     /* Ptr to containing DIE. */
	//Dwarf_Die       at_refdie;  /* Ptr to reference DIE. */
	uint64_t        at_offset;  /* Offset in info section. */
	uint64_t        at_attrib;  /* DW_AT_XXX */
	uint64_t        at_form;    /* DW_FORM_XXX */
	int         at_indirect;    /* Has indirect form. */
	union {
		uint64_t    u64;        /* Unsigned value. */
		int64_t     s64;        /* Signed value. */
		char        *s;         /* String. */
		uint8_t     *u8p;       /* Block data. */
	} u[2];                 /* Value. */
	Dwarf_Block     at_block;   /* Block. */
	Dwarf_Locdesc       *at_ld;     /* at value is locdesc. */
	uint64_t        at_relsym;  /* Relocation symbol index. */
	const char      *at_relsec; /* Rel. to dwarf section. */
}Dwarf_Attribute;

typedef struct _Dwarf_Die {
	uint64_t   die_offset;		/* DIE offset in section. */
	uint64_t   die_next_off;	/* Next DIE offset in section. */
	uint64_t   die_abnum;		/* Abbrev number. */
	uint64_t   die_tag;		/* DW_TAG_ */
	Dwarf_Abbrev    die_ab;		/* Abbrev pointer. */
	char        *die_name;		/* Ptr to the name string. */
	uint8_t die_attr_count;
	Dwarf_CU *cu_header;		/*Ptr to the CU header containing die*/
	struct _Dwarf_Die *cu_die;
	Dwarf_Attribute die_attr[NATTRVAL]; /* Array of attributes. */
}Dwarf_Die;

typedef enum {
	DW_OBJECT_MSB,
	DW_OBJECT_LSB
} Dwarf_Endianness;

struct _Dwarf_Cie {
	Dwarf_Debug     cie_dbg;        /* Ptr to containing dbg. */
	Dwarf_Unsigned  cie_index;      /* Index of the CIE. */
	Dwarf_Unsigned  cie_offset;     /* Offset of the CIE. */
	Dwarf_Unsigned  cie_length;     /* Length of the CIE. */
	Dwarf_Half      cie_version;    /* CIE version. */
	uint8_t         *cie_augment;   /* CIE augmentation (UTF-8). */
	Dwarf_Unsigned  cie_ehdata;     /* Optional EH Data. */
	Dwarf_Unsigned  cie_caf;        /* Code alignment factor. */
	Dwarf_Signed    cie_daf;        /* Data alignment factor. */
	Dwarf_Unsigned  cie_ra;         /* Return address register. */
	Dwarf_Unsigned  cie_auglen;     /* Augmentation length. */
	uint8_t         *cie_augdata;   /* Augmentation data; */
	uint8_t         cie_fde_encode; /* FDE PC start/range encode. */
	Dwarf_Ptr       cie_initinst;   /* Initial instructions. */
	Dwarf_Unsigned  cie_instlen;    /* Length of init instructions. */
};

struct _Dwarf_Fde {
	Dwarf_Debug     fde_dbg;        /* Ptr to containing dbg. */
	Dwarf_Cie       fde_cie;        /* Ptr to associated CIE. */
	Dwarf_Ptr       fde_addr;       /* Ptr to start of the FDE. */
	Dwarf_Unsigned  fde_offset;     /* Offset of the FDE. */
	Dwarf_Unsigned  fde_length;     /* Length of the FDE. */
	Dwarf_Unsigned  fde_cieoff;     /* Offset of associated CIE. */
	Dwarf_Unsigned  fde_initloc;    /* Initial location. */
	Dwarf_Unsigned  fde_adrange;    /* Address range. */
	Dwarf_Unsigned  fde_auglen;     /* Augmentation length. */
	uint8_t         *fde_augdata;   /* Augmentation data. */
	uint8_t         *fde_inst;      /* Instructions. */
	Dwarf_Unsigned  fde_instlen;    /* Length of instructions. */
	Dwarf_Unsigned  fde_instcap;    /* Capacity of inst buffer. */
	Dwarf_Unsigned  fde_symndx;     /* Symbol index for relocation. */
	Dwarf_Unsigned  fde_esymndx;    /* End symbol index for relocation. */
	Dwarf_Addr      fde_eoff;       /* Offset from the end symbol. */
};

typedef struct _Dwarf_Error {
	int             err_error;      /* DWARF error. */
	int             err_elferror;   /* ELF error. */
	const char      *err_func;      /* Function name where error occurred. */
	int             err_line;       /* Line number where error occurred. */
	char            err_msg[1024];  /* Formatted error message. */
} Dwarf_Error;

typedef struct _Dwarf_Section {
	const char      *ds_name;       /* Section name. */
	Dwarf_Small     *ds_data;       /* Section data. */
	Dwarf_Unsigned  ds_addr;        /* Section virtual addr. */
	Dwarf_Unsigned  ds_size;        /* Section size. */
} Dwarf_Section;

typedef struct _Dwarf_Line_ {
	//Dwarf_LineInfo  ln_li;      /* Ptr to line info. */
	Dwarf_Addr  ln_addr;    /* Line address. */
	Dwarf_Unsigned  ln_symndx;  /* Symbol index for relocation. */
	Dwarf_Unsigned  ln_fileno;  /* File number. */
	Dwarf_Unsigned  ln_lineno;  /* Line number. */
	Dwarf_Signed    ln_column;  /* Column number. */
	Dwarf_Bool  ln_bblock;  /* Basic block flag. */
	Dwarf_Bool  ln_stmt;    /* Begin statement flag. */
	Dwarf_Bool  ln_endseq;  /* End sequence flag. */
	//STAILQ_ENTRY(_Dwarf_Line) ln_next; /* Next line in list. */
}_Dwarf_Line;

typedef struct _Dwarf_LineInfo_ {
	Dwarf_Unsigned  li_length;  /* Length of line info data. */
	Dwarf_Half  li_version; /* Version of line info. */
	Dwarf_Unsigned  li_hdrlen;  /* Length of line info header. */
	Dwarf_Small li_minlen;  /* Minimum instrutction length. */
	Dwarf_Small li_defstmt; /* Default value of is_stmt. */
	int8_t      li_lbase;       /* Line base for special opcode. */
	Dwarf_Small li_lrange;      /* Line range for special opcode. */
	Dwarf_Small li_opbase;  /* Fisrt std opcode number. */
	Dwarf_Small *li_oplen;  /* Array of std opcode len. */
	char        **li_incdirs;   /* Array of include dirs. */
	Dwarf_Unsigned  li_inclen;  /* Length of inc dir array. */
	char        **li_lfnarray;  /* Array of file names. */
	Dwarf_Unsigned  li_lflen;   /* Length of filename array. */
	//STAILQ_HEAD(, _Dwarf_LineFile) li_lflist; /* List of files. */
	_Dwarf_Line  li_line;    /* Array of lines. */
	Dwarf_Unsigned  li_lnlen;   /* Length of the line array. */
	//STAILQ_HEAD(, _Dwarf_Line) li_lnlist; /* List of lines. */
}_Dwarf_LineInfo;

typedef _Dwarf_LineInfo  *Dwarf_LineInfo;
typedef _Dwarf_Line     *Dwarf_Line;

int dwarf_srclines(Dwarf_Die *die, Dwarf_Line linebuf, Dwarf_Addr pc, Dwarf_Error *error);

#endif
