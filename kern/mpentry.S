/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>

###################################################################
# entry point for APs
###################################################################

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must run from an address in
# the low 2^16 bytes of physical memory.
#
# boot_aps() (in init.c) copies this code to MPENTRY_PADDR (which
# satisfies the above restrictions).  Then, for each AP, it stores the
# address of the pre-allocated per-core stack in mpentry_kstack, sends
# the STARTUP IPI, and waits for this code to acknowledge that it has
# started (which happens in mp_main in init.c).
#
# This code is similar to boot/boot.S except that
#    - it does not need to enable A20
#    - it uses MPBOOTPHYS to calculate absolute addresses of its
#      symbols, rather than relying on the linker to fill them

#define MPBOOTPHYS(s) ((s) - mpentry_start + MPENTRY_PADDR)

.set LONG_MODE_CSEG, 0x8	# long mode kernel code segment selector
.set LONG_MODE_DSEG, 0x10	# long mode kernel data segment selector
.set PROT_MODE_CSEG, 0x8	# protected mode kernel code segment selector
.set PROT_MODE_DSEG, 0x10	# protected mode kernel data segment selector

.set pml4, pml4phys

.code16           
.globl mpentry_start
mpentry_start:
	cli
	xorw    %ax, %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss
	
	
	lgdt    MPBOOTPHYS(gdt32desc)   // load 32 bit global descritor table
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0
	ljmp	$(PROT_MODE_CSEG),$(MPBOOTPHYS(start32))

.code32
start32:
	
	movw    $(PROT_MODE_DSEG), %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss
	movw    $0, %ax
	movw    %ax, %fs
	movw    %ax, %gs

#ifndef VMM_GUEST
	movl $CR4_PAE,%eax
#else
	movl $(CR4_PAE|CR4_VMXE),%eax
#endif

	movl %eax,%cr4
	
	lgdt    MPBOOTPHYS(gdtdesc)
	movl    %cr0, %eax
	orl     $CR0_PE, %eax
	movl    %eax, %cr0
	
	# Set up initial page table. We cannot use kern_pgdir yet because
	# we are still running at a low EIP.
	movl    $pml4, %eax
	movl    %eax, %cr3
	
	# enable the long mode in MSR
	movl $EFER_MSR,%ecx
	rdmsr
	btsl $EFER_LME,%eax
	wrmsr
	
	# Turn on paging.
	movl    %cr0, %eax
	orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
	movl    %eax, %cr0
	

	ljmpl   $(LONG_MODE_CSEG), $(MPBOOTPHYS(start64))

.code64
start64:
	movw    $(LONG_MODE_DSEG), %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss
	movw    $0, %ax
	movw    %ax, %fs
	movw    %ax, %gs

	# Switch to the per-cpu stack allocated in boot_aps()
	movabs    mpentry_kstack, %rax
	movq    %rax,%rsp
	movq    $0x0, %rbp       # nuke frame pointer

	# Call mp_main().  (Exercise for the reader: why the indirect call?)
	movabs    $mp_main, %rax
	call    *%rax

	# If mp_main returns (it shouldn't), loop.
spin:
	jmp     spin

# Bootstrap GDT
.p2align 3					# force 8 byte alignment
gdt:
	SEG_NULL
	SEG64(STA_X|STA_R,0x0,0xffffffff)    #64 bit CS
	SEG64(STA_R|STA_W,0x0,0xffffffff)    #64 bit DS

gdtdesc:
	.word   0x17				# sizeof(gdt) - 1
	.long   MPBOOTPHYS(gdt)			# address gdt

.p2align 2
gdt32:
	SEG_NULL
	SEG(STA_X|STA_R,0x0,0xffffffff)      #32 bit CS
	SEG(STA_R|STA_W,0x0,0xffffffff)      #32 bit DS

gdt32desc:
	.word   0x17				# sizeof(gdt) - 1
	.long   MPBOOTPHYS(gdt32)			# address gdt


.globl mpentry_end
mpentry_end:
	nop
