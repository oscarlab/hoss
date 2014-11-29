#ifndef JOS_INC_VMX_H
#define JOS_INC_VMX_H

#define GUEST_MEM_SZ 16 * 1024 * 1024
#define MAX_MSR_COUNT ( PGSIZE / 2 ) / ( 128 / 8 )

#ifndef __ASSEMBLER__

struct VmxGuestInfo {
    uint64_t phys_sz;
    uintptr_t *vmcs;

    // Exception bitmap.
    uint32_t exception_bmap;
    // I/O bitmap.
    uint64_t *io_bmap_a;
    uint64_t *io_bmap_b;
    // MSR load/store area.
    int msr_count;
    uintptr_t *msr_host_area;
    uintptr_t *msr_guest_area;
};

#endif

#if defined(VMM_GUEST) || defined(VMM_HOST)

// VMCALLs
#define VMX_VMCALL_MBMAP 0x1
#define VMX_VMCALL_IPCSEND 0x2
#define VMX_VMCALL_IPCRECV 0x3

#define VMX_HOST_FS_ENV 0x1

#endif
#endif
