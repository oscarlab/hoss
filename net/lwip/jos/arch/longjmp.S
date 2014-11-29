#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 16, 0x90
#endif

#define ENTRY(x) \
        .text; _ALIGN_TEXT; .globl x; .type x,@function; x:


ENTRY(jos_setjmp)
	//movq	4(%esp), %ecx	// jos_jmp_buf

	movq	%rdi, %rcx	// jos_jmp_buf
	movq	0(%rsp), %rdx	// %rip as pushed by call
	movq	%rdx,  0(%rcx)

	leaq	8(%rsp), %rdx	// where %esp will point when we return
	movq	%rdx,  8(%rcx)

	movq	%rbp,  16(%rcx)
	movq	%rbx, 24(%rcx)
	movq	%rsi, 32(%rcx)
	movq	%rdi, 40(%rcx)
	movq	%r15, 48(%rcx)
	movq	%r14, 56(%rcx)
	movq	%r13, 64(%rcx)
	movq	%r12, 72(%rcx)
	movq	%r11, 80(%rcx)
	movq	%r10, 88(%rcx)
	movq	%r9, 96(%rcx)
	movq	%r8, 104(%rcx)

	movq	$0, %rax
	ret

ENTRY(jos_longjmp)
	// %eax is the jos_jmp_buf*
	// %edx is the return value
	movq     %rdi, %rax
	movq	 %rsi, %rdx
	movq	 0(%rax), %rcx	// %rip
	movq	 8(%rax), %rsp
	movq	 16(%rax), %rbp
	movq	24(%rax), %rbx
	movq	32(%rax), %rsi
	movq	40(%rax), %rdi
	movq	48(%rax), %r15
	movq	56(%rax), %r14
	movq	64(%rax), %r13
	movq	72(%rax), %r12
	movq	80(%rax), %r11
	movq	88(%rax), %r10
	movq	96(%rax), %r9
	movq	104(%rax), %r8

	movq	%rdx, %rax
	jmp	*%rcx

