#define PUSHA        \
    subq $120,%rsp  ; \
    movq %rax,112(%rsp); \
    movq %rbx,104(%rsp); \
    movq %rcx,96(%rsp); \
    movq %rdx,88(%rsp); \
    movq %rbp,80(%rsp); \
    movq %rdi,72(%rsp); \
    movq %rsi,64(%rsp); \
    movq %r8,56(%rsp); \
    movq %r9,48(%rsp); \
    movq %r10,40(%rsp); \
    movq %r11,32(%rsp); \
    movq %r12,24(%rsp); \
    movq %r13,16(%rsp); \
    movq %r14,8(%rsp); \
    movq %r15,0(%rsp);

#define POPA \
	"\tmovq 0(%%rsp),%%r15\n" \
	"\tmovq 8(%%rsp),%%r14\n" \
	"\tmovq 16(%%rsp),%%r13\n" \
	"\tmovq 24(%%rsp),%%r12\n" \
	"\tmovq 32(%%rsp),%%r11\n" \
	"\tmovq 40(%%rsp),%%r10\n" \
	"\tmovq 48(%%rsp),%%r9\n" \
	"\tmovq 56(%%rsp),%%r8\n" \
	"\tmovq 64(%%rsp),%%rsi\n" \
	"\tmovq 72(%%rsp),%%rdi\n" \
	"\tmovq 80(%%rsp),%%rbp\n" \
	"\tmovq 88(%%rsp),%%rdx\n" \
	"\tmovq 96(%%rsp),%%rcx\n" \
	"\tmovq 104(%%rsp),%%rbx\n" \
	"\tmovq 112(%%rsp),%%rax\n" \
	"\taddq $120,%%rsp\n"

#define POPA_ \
    movq 0(%rsp),%r15;    \
    movq 8(%rsp),%r14;  \
    movq 16(%rsp),%r13; \
    movq 24(%rsp),%r12;  \
    movq 32(%rsp),%r11; \
    movq 40(%rsp),%r10; \
    movq 48(%rsp),%r9; \
    movq 56(%rsp),%r8; \
    movq 64(%rsp),%rsi; \
    movq 72(%rsp),%rdi; \
    movq 80(%rsp),%rbp; \
    movq 88(%rsp),%rdx; \
    movq 96(%rsp),%rcx; \
    movq 104(%rsp),%rbx; \
    movq 112(%rsp),%rax; \
    addq $120,%rsp;
