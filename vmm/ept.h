
#ifndef JOS_VMX_EPT_H
#define JOS_VMX_EPT_H

#include <inc/mmu.h>
#include <vmm/vmx.h>
#include <inc/ept.h>

typedef uint64_t epte_t;

int ept_map_hva2gpa( epte_t* eptrt, void* hva, void* gpa, int perm, int overwrite );
int ept_alloc_static(epte_t *eptrt, struct VmxGuestInfo *ginfo);
void free_guest_mem(epte_t* eptrt);
void ept_gpa2hva(epte_t* eptrt, void *gpa, void **hva);
int ept_page_insert(epte_t* eptrt, struct PageInfo* pp, void* gpa, int perm);

#define EPT_LEVELS 4

#define VMX_EPT_FAULT_READ	0x01
#define VMX_EPT_FAULT_WRITE	0x02
#define VMX_EPT_FAULT_INS	0x04


#define EPTE_ADDR	(~(PGSIZE - 1))
#define EPTE_FLAGS	(PGSIZE - 1)

#define ADDR_TO_IDX(pa, n) \
    ((((uint64_t) (pa)) >> (12 + 9 * (n))) & ((1 << 9) - 1))

#endif
