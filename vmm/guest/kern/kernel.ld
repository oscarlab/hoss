
/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_head64)
/* STARTUP(obj/kern/bootstrap.o) */

SECTIONS
{
    . = 0x100000;

    .bootstrap : {
        obj/kern/bootstrap.o (.text .data .bss)
    }
    
	/* Link the kernel at this address: "." means the current address */
	. = 0x8004200000;

	.text : AT(0x200000) {
		*(EXCLUDE_FILE(obj/kern/bootstrap.o) .text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : {
		*(EXCLUDE_FILE(obj/kern/bootstrap.o) .rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* The data segment */
	.data : {
		*(EXCLUDE_FILE(obj/kern/bootstrap.o) .data)
	}

	PROVIDE(edata = .);

	.bss : {
		*(EXCLUDE_FILE(obj/kern/bootstrap.o) .bss)
	}

	PROVIDE(end = .);

	/* "Borrowed" from Linux kernel */
	/* Include debugging information in kernel memory */
        /* DWARF 1 */                                           
	.debug          0 : { *(.debug) }
        .line           0 : { *(.line) }
        /* GNU DWARF 1 extensions */
        .debug_srcinfo  0 : { *(.debug_srcinfo) }
        .debug_sfnames  0 : { *(.debug_sfnames) }
        /* DWARF 1.1 and DWARF 2 */
        .debug_aranges  0 : { *(.debug_aranges) }
        .debug_pubnames 0 : { *(.debug_pubnames) }
        /* DWARF 2 */
        .debug_info     0 : { *(.debug_info
        .gnu.linkonce.wi.*) }
        .debug_abbrev   0 : { *(.debug_abbrev) }
        .debug_line     0 : { *(.debug_line) }
        .debug_frame    0 : { *(.debug_frame) }
        .debug_str      0 : { *(.debug_str) }
        .debug_loc      0 : { *(.debug_loc) }
        .debug_macinfo  0 : { *(.debug_macinfo) }
        /* SGI/MIPS DWARF 2 extensions */ 
        .debug_weaknames 0 : { *(.debug_weaknames) }
        .debug_funcnames 0 : { *(.debug_funcnames) }
        .debug_typenames 0 : { *(.debug_typenames) }
        .debug_varnames  0 : { *(.debug_varnames) }

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
