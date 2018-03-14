// test register restore on user-level page fault return

#include <inc/lib.h>

struct regs
{
	struct PushRegs regs;
	uintptr_t eip;
	uint64_t eflags;
	uintptr_t esp;
};

#define SAVE_REGS(base)				\
	"\tmovq %%r14, 0x8("base")\n"		\
	"\tmovq %%r13, 0x10("base")\n"		\
	"\tmovq %%r12, 0x18("base")\n"		\
	"\tmovq %%r11, 0x20("base")\n"		\
	"\tmovq %%r10, 0x28("base")\n"		\
	"\tmovq %%r9, 0x30("base")\n"		\
	"\tmovq %%r8, 0x38("base")\n"		\
	"\tmovq %%rsi, 0x40("base")\n"		\
	"\tmovq %%rdi, 0x48("base")\n"		\
	"\tmovq %%rbp, 0x50("base")\n"		\
	"\tmovq %%rdx, 0x58("base")\n"		\
	"\tmovq %%rcx, 0x60("base")\n"		\
	"\tmovq %%rbx, 0x68("base")\n"		\
	"\tmovq %%rax, 0x70("base")\n"		\
	"\tmovq %%rsp, 0x88("base")\n"

#define LOAD_REGS(base)				\
	"\tmovq 0x8("base"), %%r14\n"		\
	"\tmovq 0x10("base"), %%r13\n"		\
	"\tmovq 0x18("base"), %%r12 \n"		\
	"\tmovq 0x20("base"), %%r11\n"		\
	"\tmovq 0x28("base"), %%r10\n"		\
	"\tmovq 0x30("base"), %%r9\n"		\
	"\tmovq 0x38("base"), %%r8\n"		\
	"\tmovq 0x40("base"), %%rsi\n"		\
	"\tmovq 0x48("base"), %%rdi\n"		\
	"\tmovq 0x50("base"), %%rbp\n"		\
	"\tmovq 0x58("base"), %%rdx\n"		\
	"\tmovq 0x60("base"), %%rcx\n"		\
	"\tmovq 0x68("base"), %%rbx\n"		\
	"\tmovq 0x70("base"), %%rax\n"		\
	"\tmovq 0x88("base"), %%rsp\n"

static struct regs before, during, after;

static void
check_regs(struct regs* a, const char *an, struct regs* b, const char *bn,
	   const char *testname)
{
	int mismatch = 0;

	cprintf("%-6s %-8s %-8s\n", "", an, bn);

#define CHECK(name, field)						\
	do {								\
		cprintf("%-6s %08x %08x ", #name, a->field, b->field);	\
		if (a->field == b->field)				\
			cprintf("OK\n");				\
		else {							\
			cprintf("MISMATCH\n");				\
			mismatch = 1;					\
		}							\
	} while (0)

	CHECK(edi, regs.reg_rdi);
	CHECK(esi, regs.reg_rsi);
	CHECK(ebp, regs.reg_rbp);
	CHECK(ebx, regs.reg_rbx);
	CHECK(edx, regs.reg_rdx);
	CHECK(ecx, regs.reg_rcx);
	CHECK(eax, regs.reg_rax);
	CHECK(eip, eip);
	CHECK(eflags, eflags);
	CHECK(esp, esp);

#undef CHECK


	if (!mismatch)
		cprintf("Registers %s OK\n", testname);
	else
		cprintf("Registers %s MISMATCH\n", testname);
}

static void
pgfault(struct UTrapframe *utf)
{
	int r;

	if (utf->utf_fault_va != (uint64_t)UTEMP)
		panic("pgfault expected at UTEMP, got 0x%08x (eip %08x)",
		      utf->utf_fault_va, utf->utf_rip);

	// Check registers in UTrapframe
	during.regs = utf->utf_regs;
	during.eip = utf->utf_rip;
	during.eflags = utf->utf_eflags & 0xfff;
	during.esp = utf->utf_rsp;
	check_regs(&before, "before", &during, "during", "in UTrapframe");

	// Map UTEMP so the write succeeds
	if ((r = sys_page_alloc(0, UTEMP, PTE_U|PTE_P|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(pgfault);

	__asm __volatile(
		// Light up eflags to catch more errors
		"\tpushq %0\n"
		"\tpushq %1\n"
		"\tpushq %%rax\n"
		"\tpushfq\n"
		"\tpopq %%rax\n"
		"\torq $0x8d4, %%rax\n"
		"\tpushq %%rax\n"
		"\tpopfq\n"

		// Save before registers
		// eflags
		"\tmovq 0x10(%%rsp), %%r15\n"
		"\tmovq %%rax, 0x80(%%r15)\n"
		// eip
		"\tleaq 0f, %%rax\n"
		"\tmovq %%rax, 0x78(%%r15)\n"
		"\tpopq %%rax\n"
		// others
		SAVE_REGS("%%r15")
		//"\t1: jmp 1b\n"
		// Fault at UTEMP
		"\t0: movl $42, 0x400000\n"

		// Save after registers (except eip and eflags)
		"\t movq (%%rsp),%%r15\n"
		SAVE_REGS("%%r15")
		// Restore registers (except eip and eflags).  This
		// way, the test will run even if EIP is the *only*
		// thing restored correctly.
		"\tmovq 0x8(%%rsp), %%r15\n"
		LOAD_REGS("%%r15")
		// Save after eflags (now that stack is back); note
		// that we were very careful not to modify eflags in
		// since we saved it
		"\tpushq %%rax\n"
		"\tpushfq\n"
		"\tpopq %%rax\n"
		"\tmovq 0x8(%%rsp), %%r15\n"
		"\tmovq %%rax, 0x80(%%r15)\n"
		"\tpopq %%rax\n"
		: : "r" (&before), "r" (&after) : "memory", "cc");

	// Check UTEMP to roughly determine that EIP was restored
	// correctly (of course, we probably wouldn't get this far if
	// it weren't)
	if (*(int*)UTEMP != 42)
		cprintf("EIP after page-fault MISMATCH\n");
	after.eip = before.eip;

	check_regs(&before, "before", &after, "after", "after page-fault");
}
