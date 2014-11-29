#ifndef JOS_VMM_ASM_H
#define JOS_VMM_ASM_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

static __inline uint8_t vmxon( physaddr_t vmxon_region ) __attribute((always_inline));
static __inline uint8_t vmclear( physaddr_t vmcs_region ) __attribute((always_inline));
static __inline uint8_t vmptrld( physaddr_t vmcs_region ) __attribute((always_inline));


static __inline uint8_t
vmxon( physaddr_t vmxon_region ) {
	uint8_t error = 0;

    __asm __volatile("clc; vmxon %1; setna %0"
            : "=q"( error ) : "m" ( vmxon_region ): "cc" );
    return error;
}

static __inline uint8_t
vmclear( physaddr_t vmcs_region ) {
	uint8_t error = 0;

    __asm __volatile("clc; vmclear %1; setna %0"
            : "=q"( error ) : "m" ( vmcs_region ) : "cc");
    return error;
}

static __inline uint8_t
vmptrld( physaddr_t vmcs_region ) {
	uint8_t error = 0;

    __asm __volatile("clc; vmptrld %1; setna %0"
            : "=q"( error ) : "m" ( vmcs_region ) : "cc");
    return error;
}

static __inline uint8_t
vmlaunch() {
	uint8_t error = 0;

    __asm __volatile("clc; vmlaunch; setna %0"
            : "=q"( error ) :: "cc");
    return error;
}

#endif
