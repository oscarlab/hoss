#include <inc/lib.h>

int
pageref(void *v)
{
	pte_t pte;

	if (!(uvpd[VPD(v)] & PTE_P))
		return 0;
	pte = uvpt[PGNUM(v)];
	if (!(pte & PTE_P))
		return 0;
	return pages[PPN(pte)].pp_ref;
}
