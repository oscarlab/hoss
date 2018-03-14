#ifndef JOS_INC_X86_H
#define JOS_INC_X86_H

#include <inc/types.h>
#include <inc/mmu.h>

static inline uint32_t xchg(volatile uint32_t *addr,uint32_t newval);
static __inline void breakpoint(void) __attribute__((always_inline));
static __inline uint8_t inb(int port) __attribute__((always_inline));
static __inline void insb(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline uint16_t inw(int port) __attribute__((always_inline));
static __inline void insw(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline uint32_t inl(int port) __attribute__((always_inline));
static __inline void insl(int port, void *addr, int cnt) __attribute__((always_inline));
static __inline void outb(int port, uint8_t data) __attribute__((always_inline));
static __inline void outsb(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outw(int port, uint16_t data) __attribute__((always_inline));
static __inline void outsw(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outsl(int port, const void *addr, int cnt) __attribute__((always_inline));
static __inline void outl(int port, uint32_t data) __attribute__((always_inline));
static __inline void invlpg(void *addr) __attribute__((always_inline));
static __inline void lidt(void *p) __attribute__((always_inline));
static __inline void lgdt(void *p) __attribute__((always_inline));
static __inline void lldt(uint16_t sel) __attribute__((always_inline));
static __inline void ltr(uint16_t sel) __attribute__((always_inline));
static __inline void lcr0(uint64_t val) __attribute__((always_inline));
static __inline uint64_t rcr0(void) __attribute__((always_inline));
static __inline uint64_t rcr2(void) __attribute__((always_inline));
static __inline void lcr3(uint64_t val) __attribute__((always_inline));
static __inline uint64_t rcr3(void) __attribute__((always_inline));
static __inline void lcr4(uint64_t val) __attribute__((always_inline));
static __inline uint64_t rcr4(void) __attribute__((always_inline));
static __inline void tlbflush(void) __attribute__((always_inline));
static __inline uint64_t read_eflags(void) __attribute__((always_inline));
static __inline void write_eflags(uint64_t eflags) __attribute__((always_inline));
static __inline uint64_t read_rbp(void) __attribute__((always_inline));
static __inline uint64_t read_rsp(void) __attribute__((always_inline));
static __inline void cpuid(uint32_t info, uint32_t *eaxp, uint32_t *ebxp, uint32_t *ecxp, uint32_t *edxp);
static __inline uint64_t read_tsc(void) __attribute__((always_inline));
static __inline uint64_t read_msr(uint32_t ecx) __attribute__((always_inline));
static __inline void write_msr( uint32_t ecx, uint64_t val ) __attribute__((always_inline));
static __inline void read_idtr (uint64_t *idtbase, uint16_t *idtlimit) __attribute__((always_inline));
static __inline void read_gdtr (uint64_t *gdtbase, uint16_t *gdtlimit) __attribute__((always_inline));

static __inline void
breakpoint(void)
{
	__asm __volatile("int3");
}

static __inline uint8_t
inb(int port)
{
	uint8_t data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insb(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			"=D" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"memory", "cc");
}

static __inline uint16_t
inw(int port)
{
	uint16_t data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insw(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			"=D" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"memory", "cc");
}

static __inline uint32_t
inl(int port)
{
	uint32_t data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insl(int port, void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			"=D" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"memory", "cc");
}

static __inline void
outb(int port, uint8_t data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsb(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsb"		:
			"=S" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"cc");
}

static __inline void
outw(int port, uint16_t data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsw(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsw"		:
			"=S" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"cc");
}

static __inline void
outsl(int port, const void *addr, int cnt)
{
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			"=S" (addr), "=c" (cnt)			:
			"d" (port), "0" (addr), "1" (cnt)	:
			"cc");
}

static __inline void
outl(int port, uint32_t data)
{
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static __inline void 
invlpg(void *addr)
{
	__asm __volatile("invlpg (%0)" : : "r" (addr) : "memory");
}  

static __inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static __inline void
lldt(uint16_t sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
lgdt(void *p)
{
	__asm __volatile("lgdt (%0)" : : "r" (p));
}
static __inline void
ltr(uint16_t sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline void
lcr0(uint64_t val)
{
	__asm __volatile("movq %0,%%cr0" : : "r" (val));
}

static __inline uint64_t
rcr0(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr0,%0" : "=r" (val));
	return val;
}

static __inline uint64_t
rcr2(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr2,%0" : "=r" (val));
	return val;
}

static __inline void
lcr3(uint64_t val)
{
	__asm __volatile("movq %0,%%cr3" : : "r" (val));
}

static __inline uint64_t
rcr3(void)
{
	uint64_t val;
	__asm __volatile("movq %%cr3,%0" : "=r" (val));
	return val;
}

static __inline void
lcr4(uint64_t val)
{
	__asm __volatile("movq %0,%%cr4" : : "r" (val));
}

static __inline uint64_t
rcr4(void)
{
	uint64_t cr4;
	__asm __volatile("movq %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static __inline void
tlbflush(void)
{
	uint64_t cr3;
	__asm __volatile("movq %%cr3,%0" : "=r" (cr3));
	__asm __volatile("movq %0,%%cr3" : : "r" (cr3));
}

static __inline uint64_t
read_eflags(void)
{
	uint64_t rflags;
	__asm __volatile("pushfq; popq %0" : "=r" (rflags));
	return rflags;
}

static __inline void
write_eflags(uint64_t eflags)
{
	__asm __volatile("pushq %0; popfq" : : "r" (eflags));
}

static __inline uint64_t
read_rbp(void)
{
	uint64_t rbp;
	__asm __volatile("movq %%rbp,%0" : "=r" (rbp)::"cc","memory");
	return rbp;
}

static __inline uint64_t
read_rsp(void)
{
	uint64_t esp;
	__asm __volatile("movq %%rsp,%0" : "=r" (esp));
	return esp;
}

#define read_rip(var) __asm __volatile("leaq (%%rip), %0" : "=r" (var)::"cc","memory")

static __inline void
cpuid(uint32_t info, uint32_t *eaxp, uint32_t *ebxp, uint32_t *ecxp, uint32_t *edxp)
{
	uint32_t eax, ebx, ecx, edx;
	asm volatile("cpuid" 
			 : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
			 : "a" (info));
	if (eaxp)
		*eaxp = eax;
	if (ebxp)
		*ebxp = ebx;
	if (ecxp)
		*ecxp = ecx;
	if (edxp)
		*edxp = edx;
}

static inline uint32_t
xchg(volatile uint32_t *addr,uint32_t newval){
	uint32_t result;
	__asm __volatile("lock; xchgl %0, %1":
			 "+m" (*addr), "=a" (result):
			 "1"(newval):
			 "cc");
	return result;
}

static __inline uint64_t
read_tsc(void)
{
	uint64_t tsc;
	__asm __volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

static __inline uint64_t
read_msr( uint32_t ecx ) {
	uint32_t edx, eax;
	__asm __volatile("rdmsr"
			: "=d" (edx), "=a" (eax)
			: "c" (ecx));
	uint64_t ret = 0;
	ret = edx;
	ret = ret << 32;
	ret |= eax;

	return ret;
}

static __inline void
write_msr( uint32_t ecx, uint64_t val ) {
	uint32_t edx, eax;
	eax = (uint32_t) val;
	edx = (uint32_t) ( val >> 32 );
	__asm __volatile("wrmsr"
			:: "c" (ecx), "d" (edx), "a" (eax) );
}

static __inline void
read_idtr (uint64_t *idtbase, uint16_t *idtlimit)
{
	struct Pseudodesc idtr;

	asm volatile ("sidt %0"
			: "=m" (idtr));
	*idtbase = idtr.pd_base;
	*idtlimit = idtr.pd_lim;
}

static __inline void
read_gdtr (uint64_t *gdtbase, uint16_t *gdtlimit)
{
	struct Pseudodesc gdtr;

	asm volatile ("sgdt %0"
			: "=m" (gdtr));
	*gdtbase = gdtr.pd_base;
	*gdtlimit = gdtr.pd_lim;
}

#endif /* !JOS_INC_X86_H */
