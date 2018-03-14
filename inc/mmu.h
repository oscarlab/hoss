#ifndef JOS_INC_MMU_H
#define JOS_INC_MMU_H

/*
 * This file contains definitions for the x86 memory management unit (MMU),
 * including paging- and segmentation-related data structures and constants,
 * the %cr0, %cr4, and %eflags registers, and traps.
 */

/*
 *
 *	Part 1.  Paging data structures and constants.
 *
 */

// A linear address 'la' has a three-part structure as follows:
//
// +-------9--------+-------9--------+--------9-------+--------9-------+----------12---------+
// |Page Map Level 4|Page Directory  | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |  Pointer Index |      Index     |      Index     |                     |
// +----------------+----------------+----------------+--------------------------------------+
// \----PML4(la)----/\--- PDPE(la)---/\--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \------------------------------ VPN(la) -------------------------/
//
// The PML4, PDPE, PDX, PTX, PGOFF, and VPN macros decompose linear addresses as shown.
// To construct a linear address la from PML4(la), PDPE(la), PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PML4(la), PDPE(la), PDX(la), PTX(la), PGOFF(la)).

// page number field of address
#define PPN(pa)		(((uintptr_t) (pa)) >> PTXSHIFT)
#define VPN(la)		PPN(la)		// used to index into vpt[]
#define PGNUM(la)	PPN(la)		// used to index into vpt[]

// page directory index
#define PDX(la)		((((uintptr_t) (la)) >> PDXSHIFT) & 0x1FF)
#define VPD(la)		(((uintptr_t) (la)) >> PDXSHIFT)		// used to index into vpd[]
#define VPDPE(la)   (((uintptr_t) (la)) >> PDPESHIFT)
#define VPML4E(la)  (((uintptr_t) (la)) >> PML4SHIFT)

#define PML4(la)  ((((uintptr_t) (la)) >> PML4SHIFT) & 0x1FF)

// page table index
#define PTX(la)		((((uintptr_t) (la)) >> PTXSHIFT) & 0x1FF)
#define PDPE(la)   ((((uintptr_t) (la)) >> PDPESHIFT) & 0x1FF)


// offset in page
#define PGOFF(la)	(((uintptr_t) (la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(m,p,d, t, o)	((void*) ((m) << PML4SHIFT| (p) << PDPESHIFT | (d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPMLENTRIES	512		// page directory entries per page directory
#define NPDPENTRIES	512		// page table entries per page table
#define NPDENTRIES     512
#define NPTENTRIES     512

#define PGSIZE		4096		// bytes mapped by a page
#define PGSHIFT		12		// log2(PGSIZE)

#define PTSIZE		(PGSIZE*NPTENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT		21		// log2(PTSIZE)

#define PTXSHIFT	12		// offset of PTX in a linear address
#define PDXSHIFT	21		// offset of PDX in a linear address
#define PDPESHIFT    30
#define PML4SHIFT   39

// Page table/directory entry flags.
#define PTE_P		0x001	// Present
#define PTE_W		0x002	// Writeable
#define PTE_U		0x004	// User
#define PTE_PWT		0x008	// Write-Through
#define PTE_PCD		0x010	// Cache-Disable
#define PTE_A		0x020	// Accessed
#define PTE_D		0x040	// Dirty
#define PTE_PS		0x080	// Page Size
#define PTE_MBZ		0x180	// Bits must be zero

// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.
#define PTE_AVAIL	0xE00	// Available for software use

// Flags in PTE_SYSCALL may be used only in system calls. (Others may not.)
#define PTE_SYSCALL (PTE_AVAIL | PTE_P | PTE_W | PTE_U)

// Only flags in PTE_USER may be used in system calls.
#define PTE_USER	(PTE_AVAIL | PTE_P | PTE_W | PTE_U)

// Address in page table or page directory entry
#define PTE_ADDR(pte)	((physaddr_t) (pte) & ~0xFFF)

// Control Register flags
#define CR0_PE		0x00000001	// Protection Enable
#define CR0_MP		0x00000002	// Monitor coProcessor
#define CR0_EM		0x00000004	// Emulation
#define CR0_TS		0x00000008	// Task Switched
#define CR0_ET		0x00000010	// Extension Type
#define CR0_NE		0x00000020	// Numeric Errror
#define CR0_WP		0x00010000	// Write Protect
#define CR0_AM		0x00040000	// Alignment Mask
#define CR0_NW		0x20000000	// Not Writethrough
#define CR0_CD		0x40000000	// Cache Disable
#define CR0_PG		0x80000000	// Paging

#define CR4_PCE		0x00000100	// Performance counter enable
#define CR4_MCE		0x00000040	// Machine Check Enable
#define CR4_PSE		0x00000010	// Page Size Extensions
#define CR4_DE		0x00000008	// Debugging Extensions
#define CR4_TSD		0x00000004	// Time Stamp Disable
#define CR4_PVI		0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_VME		0x00000001	// V86 Mode Extensions
#define CR4_VMXE	0x00002000	// VMX 

// x86_64 related flags
#define CR4_PAE		0x00000020
#define EFER_MSR	0xC0000080
#define EFER_LME	8

// Eflags register
#define FL_CF		0x00000001	// Carry Flag
#define FL_PF		0x00000004	// Parity Flag
#define FL_AF		0x00000010	// Auxiliary carry Flag
#define FL_ZF		0x00000040	// Zero Flag
#define FL_SF		0x00000080	// Sign Flag
#define FL_TF		0x00000100	// Trap Flag
#define FL_IF		0x00000200	// Interrupt Flag
#define FL_DF		0x00000400	// Direction Flag
#define FL_OF		0x00000800	// Overflow Flag
#define FL_IOPL_MASK	0x00003000	// I/O Privilege Level bitmask
#define FL_IOPL_0	0x00000000	//   IOPL == 0
#define FL_IOPL_1	0x00001000	//   IOPL == 1
#define FL_IOPL_2	0x00002000	//   IOPL == 2
#define FL_IOPL_3	0x00003000	//   IOPL == 3
#define FL_NT		0x00004000	// Nested Task
#define FL_RF		0x00010000	// Resume Flag
#define FL_VM		0x00020000	// Virtual 8086 mode
#define FL_AC		0x00040000	// Alignment Check
#define FL_VIF		0x00080000	// Virtual Interrupt Flag
#define FL_VIP		0x00100000	// Virtual Interrupt Pending
#define FL_ID		0x00200000	// ID flag

// Page fault error codes
#define FEC_PR		0x1	// Page fault caused by protection violation
#define FEC_WR		0x2	// Page fault caused by a write
#define FEC_U		0x4	// Page fault occured while in user mode


/*
 *
 *	Part 2.  Segmentation data structures and constants.
 *
 */

#ifdef __ASSEMBLER__

/*
 * Macros to build GDT entries in assembly.
 */
#define SEG_NULL						\
	.word 0, 0;						\
	.byte 0, 0, 0, 0
#define SEG(type,base,lim)					\
	.word (((lim) >> 12) & 0xffff), ((base) & 0xffff);	\
	.byte (((base) >> 16) & 0xff), (0x90 | (type)),		\
		(0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#define SEG64(type,base,lim)					\
	.word (((lim) >> 12) & 0xffff), ((base) & 0xffff);	\
	.byte (((base) >> 16) & 0xff), (0x90 | (type)),		\
		(0xA0 | (((lim) >> 28) & 0xF)), (((base) >> 24) & 0xff)

#define SEG64USER(type,base,lim)					\
	.word (((lim) >> 12) & 0xffff), ((base) & 0xffff);	\
	.byte (((base) >> 16) & 0xff), (0xf0 | (type)),		\
		(0xA0 | (((lim) >> 28) & 0xF)), (((base) >> 24) & 0xff)
#else	// not __ASSEMBLER__

#include <inc/types.h>

// Segment Descriptors
struct Segdesc {
	unsigned sd_lim_15_0 : 16;  // Low bits of segment limit
	unsigned sd_base_15_0 : 16; // Low bits of segment base address
	unsigned sd_base_23_16 : 8; // Middle bits of segment base address
	unsigned sd_type : 4;       // Segment type (see STS_ constants)
	unsigned sd_s : 1;          // 0 = system, 1 = application
	unsigned sd_dpl : 2;        // Descriptor Privilege Level
	unsigned sd_p : 1;          // Present
	unsigned sd_lim_19_16 : 4;  // High bits of segment limit
	unsigned sd_avl : 1;        // Unused (available for software use)
	unsigned sd_l : 1;       // Reserved
	unsigned sd_db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
	unsigned sd_g : 1;          // Granularity: limit scaled by 4K when set
	unsigned sd_base_31_24 : 8; // High bits of segment base address
};
struct SystemSegdesc64{
    unsigned sd_lim_15_0 : 16;
    unsigned sd_base_15_0 : 16;
    unsigned sd_base_23_16 : 8;
    unsigned sd_type : 4;
	unsigned sd_s : 1;          // 0 = system, 1 = application
	unsigned sd_dpl : 2;        // Descriptor Privilege Level
	unsigned sd_p : 1;          // Present
	unsigned sd_lim_19_16 : 4;  // High bits of segment limit
	unsigned sd_avl : 1;        // Unused (available for software use)
	unsigned sd_rsv1 : 2;       // Reserved
	unsigned sd_g : 1;          // Granularity: limit scaled by 4K when set
	unsigned sd_base_31_24 : 8; // High bits of segment base address
    uint32_t sd_base_63_32;  
    unsigned sd_res1 : 8;
    unsigned sd_clear : 8;
    unsigned sd_res2 : 16;
};
// Null segment
#define SEG_NULL	(struct Segdesc){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
// Segment that is loadable but faults when used
#define SEG_FAULT	(struct Segdesc){ 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0 }
// Normal segment
#define SEG(type, base, lim, dpl) (struct Segdesc)			\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    type, 1, dpl, 1, (unsigned) (lim) >> 28, 0, 0, 1, 1,		\
    (unsigned) (base) >> 24 }
#define SEG64(type, base, lim, dpl) (struct Segdesc)			\
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,	\
    type, 1, dpl, 1, (unsigned) (lim) >> 28, 0, 1, 0, 1,		\
    (unsigned) (base) >> 24 }

#define SEG16(type, base, lim, dpl) (struct Segdesc)			\
{ (lim) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,		\
    type, 1, dpl, 1, (unsigned) (lim) >> 16, 0, 0, 1, 0,		\
    (unsigned) (base) >> 24 }

#endif /* !__ASSEMBLER__ */

// Application segment type bits
#define STA_X		0x8	    // Executable segment
#define STA_E		0x4	    // Expand down (non-executable segments)
#define STA_C		0x4	    // Conforming code segment (executable only)
#define STA_W		0x2	    // Writeable (non-executable segments)
#define STA_R		0x2	    // Readable (executable segments)
#define STA_A		0x1	    // Accessed

// System segment type bits
//#define STS_T16A	0x1	    // Available 16-bit TSS
#define STS_LDT		0x2	    // Local Descriptor Table
//#define STS_T16B	0x3	    // Busy 16-bit TSS
//#define STS_CG16	0x4	    // 16-bit Call Gate
//#define STS_TG		0x5	    // Task Gate / Coum Transmitions
//#define STS_IG16	0x6	    // 16-bit Interrupt Gate
//#define STS_TG16	0x7	    // 16-bit Trap Gate
#define STS_T64A	0x9	    // Available 64-bit TSS
#define STS_T64B	0xB	    // Busy 64-bit TSS
#define STS_CG64	0xC	    // 64-bit Call Gate
#define STS_IG64	0xE	    // 64-bit Interrupt Gate
#define STS_TG64	0xF	    // 64-bit Trap Gate


/*
 *
 *	Part 3.  Traps.
 *
 */

#ifndef __ASSEMBLER__

// Task state segment format (as described by the Pentium architecture book)
struct Taskstate {
	uint32_t ts_res1;	// -- reserved in long mode
	uintptr_t ts_esp0;	// Stack pointers and segment selectors
	uintptr_t ts_esp1;
	uintptr_t ts_esp2;
	uint64_t ts_res2;	// reserved in long mode
    uint64_t ts_ist1;
    uint64_t ts_ist2;
    uint64_t ts_ist3;
    uint64_t ts_ist4;
    uint64_t ts_ist5;
    uint64_t ts_ist6;
    uint64_t ts_ist7;
    uint64_t ts_res3;
    uint16_t ts_res4;
	uint16_t ts_iomb;	// I/O map base address
}__attribute__ ((packed));

// Gate descriptors for interrupts and traps
struct Gatedesc {
	unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
	unsigned gd_ss : 16;         // segment selector
	unsigned gd_ist : 3;        // # args, 0 for interrupt/trap gates
	unsigned gd_rsv1 : 5;        // reserved(should be zero I guess)
	unsigned gd_type : 4;        // type(STS_{TG,IG32,TG32})
	unsigned gd_s : 1;           // must be 0 (system)
	unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
	unsigned gd_p : 1;           // Present
	unsigned gd_off_31_16 : 16;  // high bits of offset in segment
    uint32_t gd_off_32_63;       
    uint32_t gd_rsv2;                   
};

#define SETTSS(desc,type,base,lim,dpl)   \
{         \
    (desc)->sd_lim_15_0 = (uint64_t) (lim) & 0xffff; \
    (desc)->sd_base_15_0 = (uint64_t)(base) & 0xffff; \
    (desc)->sd_base_23_16 = ((uint64_t)(base)>>16) & 0xff;   \
    (desc)->sd_type = type;  \
    (desc)->sd_s = 0; \
    (desc)->sd_dpl = 0; \
    (desc)->sd_p = 1; \
    (desc)->sd_lim_19_16 = ((uint64_t)(lim) >> 16) & 0xf;  \
    (desc)->sd_avl = 0; \
    (desc)->sd_rsv1 = 0; \
    (desc)->sd_g = 0;    \
    (desc)->sd_base_31_24 = ((uint64_t)(base)>>24)& 0xff; \
    (desc)->sd_base_63_32 = ((uint64_t)(base)>>32) & 0xffffffff; \
    (desc)->sd_res1 = 0; \
    (desc)->sd_clear = 0; \
    (desc)->sd_res2 = 0; \
}
// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
    //   see section 9.6.1.3 of the i386 reference: "The difference between
    //   an interrupt gate and a trap gate is in the effect on IF (the
    //   interrupt-enable flag). An interrupt that vectors through an
    //   interrupt gate resets IF, thereby preventing other interrupts from
    //   interfering with the current interrupt handler. A subsequent IRET
    //   instruction restores IF to the value in the EFLAGS image on the
    //   stack. An interrupt through a trap gate does not change IF."
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//	  the privilege level required for software to invoke
//	  this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, dpl)			\
{	                        \
    (gate).gd_off_15_0 = (uint64_t) (off) & 0xffff;		\
	(gate).gd_ss = (sel);					\
	(gate).gd_ist = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = (istrap) ? STS_TG64 : STS_IG64;	\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uint64_t) (off) >> 16) & 0xffff;		\
    (gate).gd_off_32_63 = ((uint64_t) (off) >> 32) & 0xffffffff;       \
    (gate).gd_rsv2 = 0;               \
}

// Set up a call gate descriptor.
#define SETCALLGATE(gate, ss, off, dpl)           	        \
{								\
	(gate).gd_off_15_0 = (uint32_t) (off) & 0xffff;		\
	(gate).gd_ss = (ss);					\
	(gate).gd_ist = 0;					\
	(gate).gd_rsv1 = 0;					\
	(gate).gd_type = STS_CG64;				\
	(gate).gd_s = 0;					\
	(gate).gd_dpl = (dpl);					\
	(gate).gd_p = 1;					\
	(gate).gd_off_31_16 = ((uint32_t) (off) >> 16) & 0xffff;		\
    (gate).gd_off_32_63 = ((uint64_t) (off) >> 32) & 0xffffffff;       \
    (gate).gd_rsv2 = 0;               \
}

// Pseudo-descriptors used for LGDT, LLDT and LIDT instructions.
struct Pseudodesc {
	uint16_t pd_lim;		// Limit
	uint64_t pd_base;		// Base address
} __attribute__ ((packed));

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_INC_MMU_H */
