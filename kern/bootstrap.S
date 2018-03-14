#include <inc/mmu.h>
#include <inc/memlayout.h>

#ifdef VMM_GUEST
#include <inc/vmx.h>
#endif

#define	RELOC(x) ((x) - KERNBASE)

# Translation: cr3->pml4->pdpe->pgdir
# We set the PTE_MBZ bit in pgdir to skip the last step	
.code32
.set pml4, pml4phys
.set pdpt1,  pml4 + 0x1000
.set pdpt2,  pml4 + 2*0x1000
.set pde1, pml4 + 3*0x1000
.set pde2, pml4 + 4*0x1000

.text
.globl _head64
_head64:

# Save multiboot_info addr passed by bootloader
#ifdef VMM_GUEST
// VMCALL to the VMM to get a multiboot map.
    movl $VMX_VMCALL_MBMAP, %eax
    vmcall
#endif
	
    movl $multiboot_info, %eax
    movl %ebx, (%eax)

#ifndef VMM_GUEST
    movw $0x1234,0x472			# warm boot
#endif
	
# Reset the stack pointer in case we didn't come from the loader
    movl $0x7c00,%esp

    call verify_cpu   #check if CPU supports long mode
#ifndef VMM_GUEST
    movl $CR4_PAE,%eax
#else
    movl $(CR4_PAE|CR4_VMXE),%eax
#endif
    movl %eax,%cr4

# build an early boot pml4 at physical address pml4phys 

    #initializing the page tables
    movl $pml4,%edi
    xorl %eax,%eax
    movl $((4096/4)*5),%ecx  # moving these many words to the 6 pages with 4 second level pages + 1 3rd level + 1 4th level pages 
    rep stosl
    # creating a 4G boot page table
    # setting the 4th level page table only the second entry needed (PML4)
    movl $pml4,%eax
    movl $pdpt1, %ebx
    orl $PTE_P,%ebx
    orl $PTE_W,%ebx
    movl %ebx,(%eax)

    movl $pdpt2, %ebx
    orl $PTE_P,%ebx
    orl $PTE_W,%ebx
    movl %ebx,0x8(%eax)

    # setting the 3rd level page table (PDPE)
    # 4 entries (counter in ecx), point to the next four physical pages (pgdirs)
    # pgdirs in 0xa0000--0xd000
    movl $pdpt1,%edi
    movl $pde1,%ebx
    orl $PTE_P,%ebx
    orl $PTE_W,%ebx
    movl %ebx,(%edi)

    movl $pdpt2,%edi
    movl $pde2,%ebx
    orl $PTE_P,%ebx
    orl $PTE_W,%ebx
    movl %ebx,(%edi)
    
    # setting the pgdir so that the LA=PA
    # mapping first 1G of mem at KERNBASE
    movl $128,%ecx
    # Start at the end and work backwards
    #leal (pml4 + 5*0x1000 - 0x8),%edi
    movl $pde1,%edi
    movl $pde2,%ebx
    #64th entry - 0x8004000000
    addl $256,%ebx 
    # PTE_P|PTE_W|PTE_MBZ
    movl $0x00000183,%eax
  1:
     movl %eax,(%edi)
     movl %eax,(%ebx)
     addl $0x8,%edi
     addl $0x8,%ebx
     addl $0x00200000,%eax
     subl $1,%ecx
     cmp $0x0,%ecx
     jne 1b

    #setting the last 2nd level page table such that LA - KERNBASE = PA
    #leal (pml4 + 5*0x1000 + 8*384),%edi  #start with the 384th entry
    /* movl $iter2,%edi */
    /* movl $128,%ecx  # 512 - 384 = 128 */
    /* movl $0x00000183,%eax */
 /* 1: */
 /*    movl %eax, (%edi) */
 /*    addl $0x8, %edi */
 /*    addl $0x00200000, %eax */
 /*    subl $1,%ecx */
 /*    cmp $0x0,%ecx */
 /*    jne 1b */

    # set the cr3 register
    movl $pml4,%eax
    movl %eax, %cr3

	
    # enable the long mode in MSR
    movl $EFER_MSR,%ecx
    rdmsr
    btsl $EFER_LME,%eax
    wrmsr
    
    # enable paging 
    movl %cr0,%eax
    orl $CR0_PE,%eax
    orl $CR0_PG,%eax
    orl $CR0_AM,%eax
    orl $CR0_WP,%eax
    orl $CR0_MP,%eax
    movl %eax,%cr0
    #jump to long mode with CS=0 and

    movl $gdtdesc_64,%eax
    lgdt (%eax)
    pushl $0x8
    movl $_start,%eax
    pushl %eax
    
    .globl jumpto_longmode
    .type jumpto_longmode,@function
jumpto_longmode:
    lret

/* long_mode: */
/* .code64 */
/*     pushq $0x08 */
/*     movabs $_back_from_head64, %rax */
/*     pushq %rax */
/*     lretq */

verify_cpu:
    pushfl                   # get eflags in eax -- standardard way to check for cpuid
    popl %eax
    movl %eax,%ecx
    xorl $0x200000, %eax
    pushl %eax
    popfl
    pushfl
    popl %eax
    cmpl %eax,%ebx
    jz verify_cpu_no_longmode   # no cpuid -- no long mode

    movl $0x0,%eax              # see if cpuid 1 is implemented
    cpuid
    cmpl $0x1,%eax
    jb verify_cpu_no_longmode    # cpuid 1 is not implemented


    mov $0x80000001, %eax
    cpuid                 
    test $(1 << 29),%edx                 #Test if the LM-bit, is set or not.
    jz verify_cpu_no_longmode

    ret

verify_cpu_no_longmode:
    jmp verify_cpu_no_longmode
 
   
.data 
    .p2align 2

gdt_64:
    SEG_NULL
    .quad  0x00af9a000000ffff            #64 bit CS
    .quad  0x00cf92000000ffff            #64 bit DS

gdtdesc_64:
    .word 0x17 
    .quad gdt_64

    .p2align PGSHIFT

.globl pml4phys
pml4phys:
    .space 5*PGSIZE

.globl multiboot_info
multiboot_info:
    .long 0

