#include <inc/mmu.h>

# Start the CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.set PROT_MODE_CSEG, 0x8         # kernel code segment selector
.set PROT_MODE_DSEG, 0x10        # kernel data segment selector
.set CR0_PE_ON,      0x1         # protected mode enable flag

.set multiboot_info, 0x7000 // After the boot block
.set e820_map, multiboot_info + 52
.set e820_map4, multiboot_info + 56
	
.set MB_flag, multiboot_info
.set MB_mmap_len, multiboot_info + 44
.set MB_mmap_addr, multiboot_info + 48

.globl multiboot_info
.globl start
start:
  .code16                     # Assemble for 16-bit mode
  cli                         # Disable interrupts
  cld                         # String operations increment

  # Set up the important data segment registers (DS, ES, SS).
  xorw    %ax,%ax             # Segment number zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment

  # Enable A20:
  #   For backwards compatibility with the earliest PCs, physical
  #   address line 20 is tied low, so that addresses higher than
  #   1MB wrap around to zero by default.  This code undoes this.
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60

  # get the E820 memory map from the BIOS
do_e820:
  movl $0xe820, %eax
  movl $e820_map4, %edi
  xorl %ebx, %ebx
  movl $0x534D4150, %edx
  movl $24, %ecx
  int $0x15
  jc failed
  cmpl %eax, %edx
  jne failed
  testl %ebx, %ebx
  je failed
  movl $24, %ebp

next_entry:
  #increment di
  movl %ecx, -4(%edi)
  addl $24, %edi
  movl $0xe820, %eax
  movl $24, %ecx
  int $0x15
  jc done
  addl $24, %ebp
  testl %ebx, %ebx
  jne next_entry

done:
  movl %ecx, -4(%edi)
  movw $0x40, (MB_flag) #multiboot info flags
  movl $e820_map, (MB_mmap_addr)
  movl %ebp, (MB_mmap_len)
  
failed:
 
  # Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses 
  # identical to their physical addresses, so that the 
  # effective memory map does not change during the switch.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0
 
  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $PROT_MODE_CSEG, $protcseg

  .code32                     # Assemble for 32-bit mode
protcseg:
  # Set up the protected-mode data segment registers
  movw    $PROT_MODE_DSEG, %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS
  movw    %ax, %ss                # -> SS: Stack Segment
  
  # Set up the stack pointer and call into C.
  movl    $start, %esp
  movl $multiboot_info, %ebx
 # call bootmain
   call bootmain

  # If bootmain returns (it shouldn't), loop.
spin:
  jmp spin

# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
  SEG_NULL				# null seg
  SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg
  SEG(STA_W, 0x0, 0xffffffff)	        # data seg
gdtdesc:
  .word   0x17                           # sizeof(gdt) - 1
  .long   gdt                             # address gdt
