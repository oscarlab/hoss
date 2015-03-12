/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/monitor.h>
#include <kern/macro.h>
#include <kern/dwarf_api.h>
#include <kern/sched.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <vmm/vmx.h>
#include <vmm/ept.h>

struct Env *envs = NULL;		// All environments
static struct Env *env_free_list;	// Free environment list
// (linked by Env->env_link)

#define ENVGENSHIFT	12		// >= LOGNENV

// Global descriptor table.
//
// Set up global descriptor table (GDT) with separate segments for
// kernel mode and user mode.  Segments serve many purposes on the x86.
// We don't use any of their memory-mapping capabilities, but we need
// them to switch privilege levels. 
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// In particular, the last argument to the SEG macro used in the
// definition of gdt specifies the Descriptor Privilege Level (DPL)
// of that descriptor: 0 for kernel and 3 for user.
//
struct Segdesc gdt[2*NCPU + 5] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)

	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xffffffff,0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG64(STA_W, 0x0, 0xffffffff,0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xffffffff,3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG64(STA_W, 0x0, 0xffffffff,3),

	// Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
	// in trap_init_percpu()
	[GD_TSS0 >> 3] = SEG_NULL,

	[6] = SEG_NULL //last 8 bytes of the tss since tss is 16 bytes long
};

struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};
//
// Converts an envid to an env pointer.
// If checkperm is set, the specified environment must be either the
// current environment or an immediate child of the current environment.
//
// RETURNS
//   0 on success, -E_BAD_ENV on error.
//   On success, sets *env_store to the environment.
//   On error, sets *env_store to NULL.
//
int
envid2env(envid_t envid, struct Env **env_store, bool checkperm)
{
	struct Env *e;

	// If envid is zero, return the current environment.
	if (envid == 0) {
		*env_store = curenv;
		return 0;
	}

	// Look up the Env structure via the index part of the envid,
	// then check the env_id field in that struct Env
	// to ensure that the envid is not stale
	// (i.e., does not refer to a _previous_ environment
	// that used the same slot in the envs[] array).
	e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	// Check that the calling environment has legitimate permission
	// to manipulate the specified environment.
	// If checkperm is set, the specified environment
	// must be either the current environment
	// or an immediate child of the current environment.
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	*env_store = e;
	return 0;
}

// Mark all environments in 'envs' as free, set their env_ids to 0,
// and insert them into the env_free_list.
// Make sure the environments are in the free list in the same order
// they are in the envs array (i.e., so that the first call to
// env_alloc() returns envs[0]).
//
void
env_init(void)
{
	// Set up envs array
	// LAB 3: Your code here.

	// Per-CPU part of the initialization
	env_init_percpu();
}

// Load GDT and segment descriptors.
void
env_init_percpu(void)
{
	lgdt(&gdt_pd);

	// The kernel never uses GS or FS, so we leave those set to
	// the user data segment.
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	// The kernel does use ES, DS, and SS.  We'll change between
	// the kernel and user data segments as needed.
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	// Load the kernel text segment into CS.
	asm volatile("pushq %%rbx \n \t movabs $1f,%%rax \n \t pushq %%rax \n\t lretq \n 1:\n" :: "b" (GD_KT):"cc","memory");
	// For good measure, clear the local descriptor table (LDT),
	// since we don't use it.
	lldt(0);
}

//
// Initialize the kernel virtual memory layout for environment e.
// Allocate a page map level 4, set e->env_pml4e accordingly,
// and initialize the kernel portion of the new environment's address space.
// Do NOT (yet) map anything into the user portion
// of the environment's virtual address space.
//
// Returns 0 on success, < 0 on error.  Errors include:
//	-E_NO_MEM if page directory or table could not be allocated.
//
static int
env_setup_vm(struct Env *e)
{
	int r;
	int i;
	struct PageInfo *p = NULL;

	// Allocate a page for the page directory
	if (!(p = page_alloc(0)))
		return -E_NO_MEM;

	// Now, set e->env_pml4e and initialize the page directory.
	//
	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//	(except at UVPT, which we've set below).
	//	See inc/memlayout.h for permissions and layout.
	//	Hint: Figure out which entry in the pml4e maps addresses 
	//	      above UTOP.
	//	(Make sure you got the permissions right in Lab 2.)
	//    - The initial VA below UTOP is empty.
	//    - You do not need to make any more calls to page_alloc.
	//    - Note: In general, pp_ref is not maintained for
	//	physical pages mapped only above UTOP, but env_pml4e
	//	is an exception -- you need to increment env_pml4e's
	//	pp_ref for env_free to work correctly.
	//    - The functions in kern/pmap.h are handy.

	// LAB 3: Your code here.

	// UVPT maps the env's own page table read-only.
	// Permissions: kernel R, user R
	e->env_pml4e[PML4(UVPT)] = e->env_cr3 | PTE_P | PTE_U;

	return 0;
}

#ifndef VMM_GUEST
int
env_guest_alloc(struct Env **newenv_store, envid_t parent_id)
{
	int32_t generation;
	struct Env *e;

	if (!(e = env_free_list))
		return -E_NO_FREE_ENV;

	memset(&e->env_vmxinfo, 0, sizeof(struct VmxGuestInfo));

	// allocate a page for the EPT PML4..
	struct PageInfo *p = NULL;

	if (!(p = page_alloc(ALLOC_ZERO)))
		return -E_NO_MEM;

	memset(p, 0, sizeof(struct PageInfo));
	p->pp_ref       += 1;
	e->env_pml4e    = page2kva(p);
	e->env_cr3      = page2pa(p);

	// Allocate a VMCS.
	struct PageInfo *q = vmx_init_vmcs();
	if (!q) {
		page_decref(p);
		return -E_NO_MEM;
	}
	q->pp_ref += 1;
	e->env_vmxinfo.vmcs = page2kva(q);

	// Allocate a page for msr load/store area.
	struct PageInfo *r = NULL;
	if (!(r = page_alloc(ALLOC_ZERO))) {
		page_decref(p);
		page_decref(q);
		return -E_NO_MEM;
	}
	r->pp_ref += 1;
	e->env_vmxinfo.msr_host_area = page2kva(r);
	e->env_vmxinfo.msr_guest_area = page2kva(r) + PGSIZE / 2;

	// Allocate pages for IO bitmaps.
	struct PageInfo *s = NULL;
	if (!(s = page_alloc(ALLOC_ZERO))) {
		page_decref(p);
		page_decref(q);
		page_decref(r);
		return -E_NO_MEM;
	}
	s->pp_ref += 1;
	e->env_vmxinfo.io_bmap_a = page2kva(s);

	struct PageInfo *t = NULL;
	if (!(t = page_alloc(ALLOC_ZERO))) {
		page_decref(p);
		page_decref(q);
		page_decref(r);
		page_decref(s);
		return -E_NO_MEM;
	}
	t->pp_ref += 1;
	e->env_vmxinfo.io_bmap_b = page2kva(t);

	// Generate an env_id for this environment.
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)	// Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
	e->env_id = generation | (e - envs);

	// Set the basic status variables.
	e->env_parent_id = parent_id;
	e->env_type = ENV_TYPE_GUEST;
	e->env_status = ENV_RUNNABLE;
	e->env_runs = 0;

	memset(&e->env_tf, 0, sizeof(e->env_tf));

	e->env_pgfault_upcall = 0;
	e->env_ipc_recving = 0;

	// commit the allocation
	env_free_list = e->env_link;
	*newenv_store = e;

	return 0;
}

void env_guest_free(struct Env *e) {
	// Free the VMCS.
	page_decref(pa2page(PADDR(e->env_vmxinfo.vmcs)));
	// Free msr load/store area.
	page_decref(pa2page(PADDR(e->env_vmxinfo.msr_host_area)));
	// Free IO bitmaps page.
	page_decref(pa2page(PADDR(e->env_vmxinfo.io_bmap_a)));
	page_decref(pa2page(PADDR(e->env_vmxinfo.io_bmap_b)));
    
	// Free the host pages that were allocated for the guest and 
	// the EPT tables itself.
	free_guest_mem(e->env_pml4e);

	// Free the EPT PML4 page.
	page_decref(pa2page(e->env_cr3));
	e->env_pml4e = 0;
	e->env_cr3 = 0;

	// return the environment to the free list
	e->env_status = ENV_FREE;
	e->env_link = env_free_list;
	env_free_list = e;

	cprintf("[%08x] free vmx guest env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
}
#endif

//
// Allocates and initializes a new environment.
// On success, the new environment is stored in *newenv_store.
//
// Returns 0 on success, < 0 on failure.  Errors include:
//	-E_NO_FREE_ENV if all NENVS environments are allocated
//	-E_NO_MEM on memory exhaustion
//
int
env_alloc(struct Env **newenv_store, envid_t parent_id)
{
	int32_t generation;
	int r;
	struct Env *e;

	if (!(e = env_free_list))
		return -E_NO_FREE_ENV;

	// Allocate and set up the page directory for this environment.
	if ((r = env_setup_vm(e)) < 0)
		return r;

	// Generate an env_id for this environment.
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)	// Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
	e->env_id = generation | (e - envs);

	// Set the basic status variables.
	e->env_parent_id = parent_id;
	e->env_type = ENV_TYPE_USER;
	e->env_status = ENV_RUNNABLE;
	e->env_runs = 0;

	// Clear out all the saved register state,
	// to prevent the register values
	// of a prior environment inhabiting this Env structure
	// from "leaking" into our new environment.
	memset(&e->env_tf, 0, sizeof(e->env_tf));

	// Set up appropriate initial values for the segment registers.
	// GD_UD is the user data segment selector in the GDT, and
	// GD_UT is the user text segment selector (see inc/memlayout.h).
	// The low 2 bits of each segment register contains the
	// Requestor Privilege Level (RPL); 3 means user mode.  When
	// we switch privilege levels, the hardware does various
	// checks involving the RPL and the Descriptor Privilege Level
	// (DPL) stored in the descriptors themselves.
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;
	e->env_tf.tf_rsp = USTACKTOP;
	e->env_tf.tf_cs = GD_UT | 3;
	// You will set e->env_tf.tf_rip later.

	// Enable interrupts while in user mode.
	// LAB 4: Your code here.

	// Clear the page fault handler until user installs one.
	e->env_pgfault_upcall = 0;

	// Also clear the IPC receiving flag.
	e->env_ipc_recving = 0;

	// commit the allocation
	env_free_list = e->env_link;
	*newenv_store = e;

	// cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
	return 0;
}

//
// Allocate len bytes of physical memory for environment env,
// and map it at virtual address va in the environment's address space.
// Does not zero or otherwise initialize the mapped pages in any way.
// Pages should be writable by user and kernel.
// Panic if any allocation attempt fails.
//
static void
region_alloc(struct Env *e, void *va, size_t len)
{
	// LAB 3: Your code here.
	// (But only if you need it for load_icode.)
	//
	// Hint: It is easier to use region_alloc if the caller can pass
	//   'va' and 'len' values that are not page-aligned.
	//   You should round va down, and round (va + len) up.
	//   (Watch out for corner-cases!)
}

//
// Set up the initial program binary, stack, and processor flags
// for a user process.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
//
// This function loads all loadable segments from the ELF binary image
// into the environment's user memory, starting at the appropriate
// virtual addresses indicated in the ELF program header.
// At the same time it clears to zero any portions of these segments
// that are marked in the program header as being mapped
// but not actually present in the ELF file - i.e., the program's bss section.
//
// All this is very similar to what our boot loader does, except the boot
// loader also needs to read the code from disk.  Take a look at
// boot/main.c to get ideas.
//
// Finally, this function maps one page for the program's initial stack.
//
// load_icode panics if it encounters problems.
//  - How might load_icode fail?  What might be wrong with the given input?
//
void
load_icode(struct Env *e, uint8_t *binary)
{
	// Hints:
	//  Load each program segment into virtual memory
	//  at the address specified in the ELF section header.
	//  You should only load segments with ph->p_type == ELF_PROG_LOAD.
	//  Each segment's virtual address can be found in ph->p_va
	//  and its size in memory can be found in ph->p_memsz.
	//  The ph->p_filesz bytes from the ELF binary, starting at
	//  'binary + ph->p_offset', should be copied to virtual address
	//  ph->p_va.  Any remaining memory bytes should be cleared to zero.
	//  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
	//  Use functions from the previous lab to allocate and map pages.
	//
	//  All page protection bits should be user read/write for now.
	//  ELF segments are not necessarily page-aligned, but you can
	//  assume for this function that no two segments will touch
	//  the same virtual page.
	//
	//  You may find a function like region_alloc useful.
	//
	//  Loading the segments is much simpler if you can move data
	//  directly into the virtual addresses stored in the ELF binary.
	//  So which page directory should be in force during
	//  this function?
	//
	//  You must also do something with the program's entry point,
	//  to make sure that the environment starts executing there.
	//  What?  (See env_run() and env_pop_tf() below.)

	// LAB 3: Your code here
	// Now map one page for the program's initial stack
	// at virtual address USTACKTOP - PGSIZE.

	// LAB 3: Your code here.
	e->elf = binary;
}

//
// Allocates a new env with env_alloc, loads the named elf
// binary into it with load_icode, and sets its env_type.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
// The new env's parent ID is set to 0.
//
void
env_create(uint8_t *binary, enum EnvType type)
{
	// LAB 3: Your code here.

	// If this is the file server (type == ENV_TYPE_FS) give it I/O privileges.
	// LAB 5: Your code here.
}

//
// Frees env e and all memory it uses.
//
void
env_free(struct Env *e)
{
	pte_t *pt;
	uint64_t pdeno, pteno;
	physaddr_t pa;

#ifndef VMM_GUEST 
	if(e->env_type == ENV_TYPE_GUEST) {
		env_guest_free(e);
		return;
	}
#endif

	// If freeing the current environment, switch to kern_pgdir
	// before freeing the page directory, just in case the page
	// gets reused.
	if (e == curenv)
		lcr3(boot_cr3);

	// Note the environment's demise.
	// cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	// Flush all mapped pages in the user portion of the address space
	pdpe_t *env_pdpe = KADDR(PTE_ADDR(e->env_pml4e[0]));
	int pdeno_limit;
	uint64_t pdpe_index;
	// using 3 instead of NPDPENTRIES as we have only first three indices
	// set for 4GB of address space.
	for(pdpe_index=0;pdpe_index<=3;pdpe_index++){
		if(!(env_pdpe[pdpe_index] & PTE_P))
			continue;
		pde_t *env_pgdir = KADDR(PTE_ADDR(env_pdpe[pdpe_index]));
		pdeno_limit  = pdpe_index==3?PDX(UTOP):PDX(0xFFFFFFFF);
		static_assert(UTOP % PTSIZE == 0);
		for (pdeno = 0; pdeno < pdeno_limit; pdeno++) {

			// only look at mapped page tables
			if (!(env_pgdir[pdeno] & PTE_P))
				continue;
			// find the pa and va of the page table
			pa = PTE_ADDR(env_pgdir[pdeno]);
			pt = (pte_t*) KADDR(pa);

			// unmap all PTEs in this page table
			for (pteno = 0; pteno < PTX(~0); pteno++) {
				if (pt[pteno] & PTE_P){
					page_remove(e->env_pml4e, PGADDR((uint64_t)0,pdpe_index,pdeno, pteno, 0));
				}
			}

			// free the page table itself
			env_pgdir[pdeno] = 0;
			page_decref(pa2page(pa));
		}
		// free the page directory
		pa = PTE_ADDR(env_pdpe[pdpe_index]);
		env_pdpe[pdpe_index] = 0;
		page_decref(pa2page(pa));
	}
	// free the page directory pointer
	page_decref(pa2page(PTE_ADDR(e->env_pml4e[0])));
	// free the page map level 4 (PML4)
	e->env_pml4e[0] = 0;
	pa = e->env_cr3;
	e->env_pml4e = 0;
	e->env_cr3 = 0;
	page_decref(pa2page(pa));

	// return the environment to the free list
	e->env_status = ENV_FREE;
	e->env_link = env_free_list;
	env_free_list = e;
}

//
// Frees environment e.
// If e was the current env, then runs a new environment (and does not return
// to the caller).
//
void
env_destroy(struct Env *e)
{
	// If e is currently running on other CPUs, we change its state to
	// ENV_DYING. A zombie environment will be freed the next time
	// it traps to the kernel.
	if (e->env_status == ENV_RUNNING && curenv != e) {
		e->env_status = ENV_DYING;
		return;
	}

	env_free(e);
	if (curenv == e) {
		curenv = NULL;
		sched_yield();
	}
}


//
// Restores the register values in the Trapframe with the 'iret' instruction.
// This exits the kernel and starts executing some environment's code.
//
// This function does not return.
//
void
env_pop_tf(struct Trapframe *tf)
{
	// Record the CPU we are running on for user-space debugging
	curenv->env_cpunum = cpunum();
	__asm __volatile("movq %0,%%rsp\n"
			 POPA
			 "movw (%%rsp),%%es\n"
			 "movw 8(%%rsp),%%ds\n"
			 "addq $16,%%rsp\n"
			 "\taddq $16,%%rsp\n" /* skip tf_trapno and tf_errcode */
			 "\tiretq"
			 : : "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}

//
// Context switch from curenv to env e.
// Note: if this is the first call to env_run, curenv is NULL.
//
// This function does not return.
//
void
env_run(struct Env *e)
{
	// Step 1: If this is a context switch (a new environment is running):
	//	   1. Set the current environment (if any) back to
	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
	//	      what other states it can be in),
	//	   2. Set 'curenv' to the new environment,
	//	   3. Set its status to ENV_RUNNING,
	//	   4. Update its 'env_runs' counter,
	//	   5. Use lcr3() to switch to its address space.
	// Step 2: Use env_pop_tf() to restore the environment's
	//	   registers and drop into user mode in the
	//	   environment.

	// Hint: This function loads the new environment's state from
	//	e->env_tf.  Go back through the code you wrote above
	//	and make sure you have set the relevant parts of
	//	e->env_tf to sensible values.

	// LAB 3: Your code here.

	panic("env_run not yet implemented");
}

