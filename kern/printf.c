// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>


static void
putch(int ch, int *cnt)
{
	cputchar(ch);
	*cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;
	va_list aq;
	va_copy(aq,ap);
	vprintfmt((void*)putch, &cnt, fmt, aq);
	va_end(aq);
	return cnt;

}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;
	va_start(ap, fmt);
	va_list aq;
	va_copy(aq,ap);
	cnt = vcprintf(fmt, aq);
	va_end(aq);

	return cnt;
}

