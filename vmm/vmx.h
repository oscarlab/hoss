
#ifndef JOS_VMM_VMX_H
#define JOS_VMM_VMX_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <inc/vmx.h>
#include <inc/env.h>

static __inline uint8_t vmcs_write16( uint32_t field, uint16_t value) __attribute((always_inline));
static __inline uint8_t vmcs_write32( uint32_t field, uint32_t value ) __attribute((always_inline));
static __inline uint8_t vmcs_write64( uint32_t field, uint64_t value ) __attribute((always_inline));
static __inline uint16_t vmcs_read16(uint32_t field) __attribute((always_inline));

static __inline uint32_t vmcs_read32(uint32_t field) __attribute((always_inline));

static __inline uint64_t vmcs_read64(uint32_t field) __attribute((always_inline));


struct vmx_msr_entry {
    uint32_t msr_index;
    uint32_t res;
    uint64_t msr_value;
} __attribute__((__packed__));

inline bool vmx_check_ept();
inline bool vmx_check_support();
int vmx_get_vmdisk_number();
void vmx_incr_vmdisk_number();
int vmx_init_vmxon();
int vmx_vmrun( struct Env *e );
void vmx_list_vms();
bool vmx_sel_resume(int num);
struct PageInfo * vmx_init_vmcs();

/* VMX Capalibility MSRs */
#define IA32_VMX_BASIC 0X480
#define IA32_VMX_PINBASED_CTLS 0X481
#define IA32_VMX_PROCBASED_CTLS 0X482
#define IA32_VMX_PROCBASED_CTLS2 0X48B
#define IA32_VMX_EXIT_CTLS 0X483
#define IA32_VMX_ENTRY_CTLS 0X484
#define IA32_VMX_MISC 0x485
#define IA32_VMX_CR0_FIXED0 0x486
#define IA32_VMX_CR0_FIXED1 0x487
#define IA32_VMX_CR4_FIXED0 0x488
#define IA32_VMX_CR4_FIXED1 0x489
#define IA32_VMX_VMCS_ENUM 0x48A
#define IA32_VMX_EPT_VPID_CAP 0x48C
#define IA32_FEATURE_CONTROL 0x03A

#define BIT( val, x ) ( ( val >> x ) & 0x1 )

static __inline uint8_t vmcs_writel( uint32_t field, uint64_t value) {
	uint8_t error;

	__asm __volatile ( "clc; vmwrite %%rax, %%rdx; setna %0"
		       : "=q"( error ) : "a"( value ), "d"( field ) : "cc");
    return error;
}

static __inline uint8_t vmcs_write16( uint32_t field, uint16_t value) {
	return vmcs_writel( field, value );
}

static __inline uint8_t vmcs_write32( uint32_t field, uint32_t value ) {
	return vmcs_writel(field, value);
}

static __inline uint8_t vmcs_write64( uint32_t field, uint64_t value ) {
	return vmcs_writel( field, value );
}

static __inline uint64_t vmcs_readl(uint32_t field)
{
	uint64_t value;

	__asm __volatile ( "vmread %%rdx, %%rax;"
		      : "=a"(value) : "d"(field) : "cc");
	return value;
}

static __inline uint16_t vmcs_read16(uint32_t field)
{
	return vmcs_readl(field);
}

static __inline uint32_t vmcs_read32(uint32_t field)
{
	return vmcs_readl(field);
}

static __inline uint64_t vmcs_read64(uint32_t field)
{
	return vmcs_readl(field);
}


// =============
//  VMCS fields
// =============

/* VMCS 16-bit control fields */
/* binary 0000_00xx_xxxx_xxx0 */
#define VMCS_16BIT_CONTROL_VPID                            0x00000000 /* VPID */
#define VMCS_16BIT_CONTROL_POSTED_INTERRUPT_VECTOR         0x00000002 /* Posted Interrupts */
#define VMCS_16BIT_CONTROL_EPTP_INDEX                      0x00000004 /* #VE Exception */

/* VMCS 16-bit guest-state fields */
/* binary 0000_10xx_xxxx_xxx0 */
#define VMCS_16BIT_GUEST_ES_SELECTOR                       0x00000800
#define VMCS_16BIT_GUEST_CS_SELECTOR                       0x00000802
#define VMCS_16BIT_GUEST_SS_SELECTOR                       0x00000804
#define VMCS_16BIT_GUEST_DS_SELECTOR                       0x00000806
#define VMCS_16BIT_GUEST_FS_SELECTOR                       0x00000808
#define VMCS_16BIT_GUEST_GS_SELECTOR                       0x0000080A
#define VMCS_16BIT_GUEST_LDTR_SELECTOR                     0x0000080C
#define VMCS_16BIT_GUEST_TR_SELECTOR                       0x0000080E
#define VMCS_16BIT_GUEST_INTERRUPT_STATUS                  0x00000810 /* Virtual Interrupt Delivery */

/* VMCS 16-bit host-state fields */
/* binary 0000_11xx_xxxx_xxx0 */
#define VMCS_16BIT_HOST_ES_SELECTOR                        0x00000C00
#define VMCS_16BIT_HOST_CS_SELECTOR                        0x00000C02
#define VMCS_16BIT_HOST_SS_SELECTOR                        0x00000C04
#define VMCS_16BIT_HOST_DS_SELECTOR                        0x00000C06
#define VMCS_16BIT_HOST_FS_SELECTOR                        0x00000C08
#define VMCS_16BIT_HOST_GS_SELECTOR                        0x00000C0A
#define VMCS_16BIT_HOST_TR_SELECTOR                        0x00000C0C

/* VMCS 64-bit control fields */
/* binary 0010_00xx_xxxx_xxx0 */
#define VMCS_64BIT_CONTROL_IO_BITMAP_A                     0x00002000
#define VMCS_64BIT_CONTROL_IO_BITMAP_A_HI                  0x00002001
#define VMCS_64BIT_CONTROL_IO_BITMAP_B                     0x00002002
#define VMCS_64BIT_CONTROL_IO_BITMAP_B_HI                  0x00002003
#define VMCS_64BIT_CONTROL_MSR_BITMAPS                     0x00002004
#define VMCS_64BIT_CONTROL_MSR_BITMAPS_HI                  0x00002005
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR           0x00002006
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR_HI        0x00002007
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR            0x00002008
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR_HI         0x00002009
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR           0x0000200A
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR_HI        0x0000200B
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR              0x0000200C
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR_HI           0x0000200D
#define VMCS_64BIT_CONTROL_TSC_OFFSET                      0x00002010
#define VMCS_64BIT_CONTROL_TSC_OFFSET_HI                   0x00002011
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR          0x00002012 /* TPR shadow */
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR_HI       0x00002013
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR                0x00002014 /* APIC virtualization */
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR_HI             0x00002015
#define VMCS_64BIT_CONTROL_POSTED_INTERRUPT_DESC_ADDR      0x00002016 /* Posted Interrupts */
#define VMCS_64BIT_CONTROL_POSTED_INTERRUPT_DESC_ADDR_HI   0x00002017
#define VMCS_64BIT_CONTROL_VMFUNC_CTRLS                    0x00002018 /* VM Functions */
#define VMCS_64BIT_CONTROL_VMFUNC_CTRLS_HI                 0x00002019
#define VMCS_64BIT_CONTROL_EPTPTR                          0x0000201A /* EPT */
#define VMCS_64BIT_CONTROL_EPTPTR_HI                       0x0000201B
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP0                0x0000201C /* Virtual Interrupt Delivery */
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP0_HI             0x0000201D
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP1                0x0000201E
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP1_HI             0x0000201F
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP2                0x00002020
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP2_HI             0x00002021
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP3                0x00002022
#define VMCS_64BIT_CONTROL_EOI_EXIT_BITMAP3_HI             0x00002023
#define VMCS_64BIT_CONTROL_EPTP_LIST_ADDRESS               0x00002024 /* VM Functions - EPTP switching */
#define VMCS_64BIT_CONTROL_EPTP_LIST_ADDRESS_HI            0x00002025
#define VMCS_64BIT_CONTROL_VMREAD_BITMAP_ADDR              0x00002026 /* VMCS Shadowing */
#define VMCS_64BIT_CONTROL_VMREAD_BITMAP_ADDR_HI           0x00002027
#define VMCS_64BIT_CONTROL_VMWRITE_BITMAP_ADDR             0x00002028 /* VMCS Shadowing */
#define VMCS_64BIT_CONTROL_VMWRITE_BITMAP_ADDR_HI          0x00002029
#define VMCS_64BIT_CONTROL_VE_EXCEPTION_INFO_ADDR          0x0000202A /* #VE Exception */
#define VMCS_64BIT_CONTROL_VE_EXCEPTION_INFO_ADDR_HI       0x0000202B

/* VMCS 64-bit read only data fields */
/* binary 0010_01xx_xxxx_xxx0 */
#define VMCS_64BIT_GUEST_PHYSICAL_ADDR                     0x00002400 /* EPT */
#define VMCS_64BIT_GUEST_PHYSICAL_ADDR_HI                  0x00002401

/* VMCS 64-bit guest state fields */
/* binary 0010_10xx_xxxx_xxx0 */
#define VMCS_64BIT_GUEST_LINK_POINTER                      0x00002800
#define VMCS_64BIT_GUEST_LINK_POINTER_HI                   0x00002801
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL                     0x00002802
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL_HI                  0x00002803
#define VMCS_64BIT_GUEST_IA32_PAT                          0x00002804 /* PAT */
#define VMCS_64BIT_GUEST_IA32_PAT_HI                       0x00002805
#define VMCS_64BIT_GUEST_IA32_EFER                         0x00002806 /* EFER */
#define VMCS_64BIT_GUEST_IA32_EFER_HI                      0x00002807
#define VMCS_64BIT_GUEST_IA32_PERF_GLOBAL_CTRL             0x00002808 /* Perf Global Ctrl */
#define VMCS_64BIT_GUEST_IA32_PERF_GLOBAL_CTRL_HI          0x00002809
#define VMCS_64BIT_GUEST_IA32_PDPTE0                       0x0000280A /* EPT */
#define VMCS_64BIT_GUEST_IA32_PDPTE0_HI                    0x0000280B
#define VMCS_64BIT_GUEST_IA32_PDPTE1                       0x0000280C
#define VMCS_64BIT_GUEST_IA32_PDPTE1_HI                    0x0000280D
#define VMCS_64BIT_GUEST_IA32_PDPTE2                       0x0000280E
#define VMCS_64BIT_GUEST_IA32_PDPTE2_HI                    0x0000280F
#define VMCS_64BIT_GUEST_IA32_PDPTE3                       0x00002810
#define VMCS_64BIT_GUEST_IA32_PDPTE3_HI                    0x00002811

/* VMCS 64-bit host state fields */
/* binary 0010_11xx_xxxx_xxx0 */
#define VMCS_64BIT_HOST_IA32_PAT                           0x00002C00 /* PAT */
#define VMCS_64BIT_HOST_IA32_PAT_HI                        0x00002C01
#define VMCS_64BIT_HOST_IA32_EFER                          0x00002C02 /* EFER */
#define VMCS_64BIT_HOST_IA32_EFER_HI                       0x00002C03
#define VMCS_64BIT_HOST_IA32_PERF_GLOBAL_CTRL              0x00002C04 /* Perf Global Ctrl */
#define VMCS_64BIT_HOST_IA32_PERF_GLOBAL_CTRL_HI           0x00002C05

/* VMCS 32_bit control fields */
/* binary 0100_00xx_xxxx_xxx0 */
#define VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS         0x00004000
#define VMCS_32BIT_CONTROL_PROCESSOR_BASED_VMEXEC_CONTROLS 0x00004002
#define VMCS_32BIT_CONTROL_EXCEPTION_BITMAP                0x00004004
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK        0x00004006
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH       0x00004008
#define VMCS_32BIT_CONTROL_CR3_TARGET_COUNT                0x0000400A
#define VMCS_32BIT_CONTROL_VMEXIT_CONTROLS                 0x0000400C
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT          0x0000400E
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT           0x00004010
#define VMCS_32BIT_CONTROL_VMENTRY_CONTROLS                0x00004012
#define VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT          0x00004014
#define VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO       0x00004016
#define VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE      0x00004018
#define VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH      0x0000401A
#define VMCS_32BIT_CONTROL_TPR_THRESHOLD                   0x0000401C /* TPR shadow */
#define VMCS_32BIT_CONTROL_SECONDARY_VMEXEC_CONTROLS       0x0000401E
#define VMCS_32BIT_CONTROL_PAUSE_LOOP_EXITING_GAP          0x00004020 /* PAUSE loop exiting */
#define VMCS_32BIT_CONTROL_PAUSE_LOOP_EXITING_WINDOW       0x00004022 /* PAUSE loop exiting */

/* VMCS 32-bit read only data fields */
/* binary 0100_01xx_xxxx_xxx0 */
#define VMCS_32BIT_INSTRUCTION_ERROR                       0x00004400
#define VMCS_32BIT_VMEXIT_REASON                           0x00004402
#define VMCS_32BIT_VMEXIT_INTERRUPTION_INFO                0x00004404
#define VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE            0x00004406
#define VMCS_32BIT_IDT_VECTORING_INFO                      0x00004408
#define VMCS_32BIT_IDT_VECTORING_ERR_CODE                  0x0000440A
#define VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH               0x0000440C
#define VMCS_32BIT_VMEXIT_INSTRUCTION_INFO                 0x0000440E

/* VMCS 32-bit guest-state fields */
/* binary 0100_10xx_xxxx_xxx0 */
#define VMCS_32BIT_GUEST_ES_LIMIT                          0x00004800
#define VMCS_32BIT_GUEST_CS_LIMIT                          0x00004802
#define VMCS_32BIT_GUEST_SS_LIMIT                          0x00004804
#define VMCS_32BIT_GUEST_DS_LIMIT                          0x00004806
#define VMCS_32BIT_GUEST_FS_LIMIT                          0x00004808
#define VMCS_32BIT_GUEST_GS_LIMIT                          0x0000480A
#define VMCS_32BIT_GUEST_LDTR_LIMIT                        0x0000480C
#define VMCS_32BIT_GUEST_TR_LIMIT                          0x0000480E
#define VMCS_32BIT_GUEST_GDTR_LIMIT                        0x00004810
#define VMCS_32BIT_GUEST_IDTR_LIMIT                        0x00004812
#define VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS                  0x00004814
#define VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS                  0x00004816
#define VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS                  0x00004818
#define VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS                  0x0000481A
#define VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS                  0x0000481C
#define VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS                  0x0000481E
#define VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS                0x00004820
#define VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS                  0x00004822
#define VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE            0x00004824
#define VMCS_32BIT_GUEST_ACTIVITY_STATE                    0x00004826
#define VMCS_32BIT_GUEST_SMBASE                            0x00004828
#define VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR              0x0000482A
#define VMCS_32BIT_GUEST_PREEMPTION_TIMER_VALUE            0x0000482E /* VMX preemption timer */

/* VMCS 32-bit host-state fields */
/* binary 0100_11xx_xxxx_xxx0 */
#define VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR               0x00004C00

/* VMCS natural width control fields */
/* binary 0110_00xx_xxxx_xxx0 */
#define VMCS_CONTROL_CR0_GUEST_HOST_MASK                   0x00006000
#define VMCS_CONTROL_CR4_GUEST_HOST_MASK                   0x00006002
#define VMCS_CONTROL_CR0_READ_SHADOW                       0x00006004
#define VMCS_CONTROL_CR4_READ_SHADOW                       0x00006006
#define VMCS_CR3_TARGET0                                   0x00006008
#define VMCS_CR3_TARGET1                                   0x0000600A
#define VMCS_CR3_TARGET2                                   0x0000600C
#define VMCS_CR3_TARGET3                                   0x0000600E
                                                           
/* VMCS natural width read only data fields */
/* binary 0110_01xx_xxxx_xxx0 */
#define VMCS_VMEXIT_QUALIFICATION                          0x00006400
#define VMCS_IO_RCX                                        0x00006402
#define VMCS_IO_RSI                                        0x00006404
#define VMCS_IO_RDI                                        0x00006406
#define VMCS_IO_RIP                                        0x00006408
#define VMCS_GUEST_LINEAR_ADDR                             0x0000640A

/* VMCS natural width guest state fields */
/* binary 0110_10xx_xxxx_xxx0 */
#define VMCS_GUEST_CR0                                     0x00006800
#define VMCS_GUEST_CR3                                     0x00006802
#define VMCS_GUEST_CR4                                     0x00006804
#define VMCS_GUEST_ES_BASE                                 0x00006806
#define VMCS_GUEST_CS_BASE                                 0x00006808
#define VMCS_GUEST_SS_BASE                                 0x0000680A
#define VMCS_GUEST_DS_BASE                                 0x0000680C
#define VMCS_GUEST_FS_BASE                                 0x0000680E
#define VMCS_GUEST_GS_BASE                                 0x00006810
#define VMCS_GUEST_LDTR_BASE                               0x00006812
#define VMCS_GUEST_TR_BASE                                 0x00006814
#define VMCS_GUEST_GDTR_BASE                               0x00006816
#define VMCS_GUEST_IDTR_BASE                               0x00006818
#define VMCS_GUEST_DR7                                     0x0000681A
#define VMCS_GUEST_RSP                                     0x0000681C
#define VMCS_GUEST_RIP                                     0x0000681E
#define VMCS_GUEST_RFLAGS                                  0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS                  0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP_MSR                   0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP_MSR                   0x00006826

/* VMCS natural width host state fields */
/* binary 0110_11xx_xxxx_xxx0 */
#define VMCS_HOST_CR0                                      0x00006C00
#define VMCS_HOST_CR3                                      0x00006C02
#define VMCS_HOST_CR4                                      0x00006C04
#define VMCS_HOST_FS_BASE                                  0x00006C06
#define VMCS_HOST_GS_BASE                                  0x00006C08
#define VMCS_HOST_TR_BASE                                  0x00006C0A
#define VMCS_HOST_GDTR_BASE                                0x00006C0C
#define VMCS_HOST_IDTR_BASE                                0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP_MSR                    0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP_MSR                    0x00006C12
#define VMCS_HOST_RSP                                      0x00006C14
#define VMCS_HOST_RIP                                      0x00006C16

// VMCS control field bits.

#define VMCS_PIN_BASED_VMEXEC_CTL_EXINTEXIT	0x1
#define VMCS_PIN_BASED_VMEXEC_CTL_NMIEXIT	0x8
#define VMCS_PIN_BASED_VMEXEC_CTL_VIRTNMIS	0x20

#define VMCS_PROC_BASED_VMEXEC_CTL_INTRWINEXIT  0x4
#define VMCS_PROC_BASED_VMEXEC_CTL_USETSCOFF	0x8
#define VMCS_PROC_BASED_VMEXEC_CTL_HLTEXIT		0x80
#define VMCS_PROC_BASED_VMEXEC_CTL_INVLPGEXIT	0x200
#define VMCS_PROC_BASED_VMEXEC_CTL_MWAITEXIT	0x400
#define VMCS_PROC_BASED_VMEXEC_CTL_RDPMCEXIT	0x800
#define VMCS_PROC_BASED_VMEXEC_CTL_RDTSCEXIT	0x1000
#define VMCS_PROC_BASED_VMEXEC_CTL_CR3LOADEXIT	0x8000
#define VMCS_PROC_BASED_VMEXEC_CTL_CR3STOREXIT	0x10000
#define VMCS_PROC_BASED_VMEXEC_CTL_CR8LOADEXIT	0x80000
#define VMCS_PROC_BASED_VMEXEC_CTL_CR8STOREEXIT	0x100000
#define VMCS_PROC_BASED_VMEXEC_CTL_USETPRSHADOW	0x200000
#define VMCS_PROC_BASED_VMEXEC_CTL_NMIWINEXIT	0x400000
#define VMCS_PROC_BASED_VMEXEC_CTL_MOVDREXIT	0x800000
#define VMCS_PROC_BASED_VMEXEC_CTL_UNCONDIOEXIT	0x1000000
#define VMCS_PROC_BASED_VMEXEC_CTL_USEIOBMP		0x2000000
#define VMCS_PROC_BASED_VMEXEC_CTL_MTF		    0x8000000
#define VMCS_PROC_BASED_VMEXEC_CTL_USEMSRBMP	0x10000000
#define VMCS_PROC_BASED_VMEXEC_CTL_MONITOREXIT	0x20000000
#define VMCS_PROC_BASED_VMEXEC_CTL_PAUSEEXIT	0x40000000
#define VMCS_PROC_BASED_VMEXEC_CTL_ACTIVESECCTL	0x80000000

#define VMCS_SECONDARY_VMEXEC_CTL_ENABLE_EPT          0x2
#define VMCS_SECONDARY_VMEXEC_CTL_UNRESTRICTED_GUEST  0x80

#define VMCS_VMEXIT_HOST_ADDR_SIZE ( 0x1 << 9 )
#define VMCS_VMEXIT_GUEST_ACK_INTR_ON_EXIT ( 0x1 << 15 )

#define VMCS_VMENTRY_x64_GUEST ( 0x1 << 9 )

// VMEXIT reasons.
#define EXIT_REASON_MASK		0xFFFF

#define EXIT_REASON_EXCEPTION_OR_NMI	0x0
#define EXIT_REASON_EXTERNAL_INT        0x1
#define EXIT_REASON_TRIPLE_FAULT	0x2
#define EXIT_REASON_INIT_SIGNAL		0x3
#define EXIT_REASON_STARTUP_IPI		0x4
#define EXIT_REASON_IO_SMI		0x5
#define EXIT_REASON_OTHER_SMI		0x6
#define EXIT_REASON_INTERRUPT_WINDOW	0x7
#define EXIT_REASON_TASK_SWITCH		0x9
#define EXIT_REASON_CPUID		0xA
#define EXIT_REASON_HLT			0xC
#define EXIT_REASON_INVD		0xD
#define EXIT_REASON_INVLPG		0xE
#define EXIT_REASON_RDPMC		0xF
#define EXIT_REASON_RDTSC		0x10
#define EXIT_REASON_RSM			0x11
#define EXIT_REASON_VMCALL		0x12
#define EXIT_REASON_VMCLEAR		0x13
#define EXIT_REASON_VMLAUNCH		0x14
#define EXIT_REASON_VMPTRLD		0x15
#define EXIT_REASON_VMPTRST		0x16
#define EXIT_REASON_VMREAD		0x17
#define EXIT_REASON_VMRESUME		0x18
#define EXIT_REASON_VMWRITE		0x19
#define EXIT_REASON_VMXOFF		0x1A
#define EXIT_REASON_VMXON		0x1B
#define EXIT_REASON_MOV_CR		0x1C
#define EXIT_REASON_MOV_DR		0x1D
#define EXIT_REASON_IO_INSTRUCTION	0x1E
#define EXIT_REASON_RDMSR		0x1F
#define EXIT_REASON_WRMSR		0x20
#define EXIT_REASON_ENTFAIL_GUEST_STATE	0x21
#define EXIT_REASON_ENTFAIL_MSR_LOADING	0x22
#define EXIT_REASON_MWAIT		0x24
#define EXIT_REASON_MTF			0x25
#define EXIT_REASON_MONITOR		0x27
#define EXIT_REASON_PAUSE		0x28
#define EXIT_REASON_ENTFAIL_MACHINE_CHK	0x29
#define EXIT_REASON_TPR_BELOW_THRESHOLD	0x2B
#define EXIT_REASON_VMEXIT_FROM_VMX_ROOT_OPERATION_BIT	0x20000000
#define EXIT_REASON_VMENTRY_FAILURE_BIT	0x80000000
#define EXIT_REASON_APIC_ACCESS		0x2C
#define EXIT_REASON_ACCESS_GDTR_OR_IDTR	0x2E
#define EXIT_REASON_ACCESS_LDTR_OR_TR	0x2F
#define EXIT_REASON_EPT_VIOLATION	0x30
#define EXIT_REASON_EPT_MISCONFIG	0x31
#define EXIT_REASON_INVEPT		0x32
#define EXIT_REASON_RDTSCP		0x33
#define EXIT_REASON_VMX_PREEMPT_TIMER	0x34
#define EXIT_REASON_INVVPID		0x35
#define EXIT_REASON_WBINVD		0x36
#define EXIT_REASON_XSETBV		0x37

#define VMEXIT_CR0_READ			0x0
#define VMEXIT_CR1_READ			0x1
#define VMEXIT_CR2_READ			0x2
#define VMEXIT_CR3_READ			0x3
#define VMEXIT_CR4_READ			0x4
#define VMEXIT_CR5_READ			0x5
#define VMEXIT_CR6_READ			0x6
#define VMEXIT_CR7_READ			0x7
#define VMEXIT_CR8_READ			0x8
#define VMEXIT_CR9_READ			0x9
#define VMEXIT_CR10_READ		0xA
#define VMEXIT_CR11_READ		0xB
#define VMEXIT_CR12_READ		0xC
#define VMEXIT_CR13_READ		0xD
#define VMEXIT_CR14_READ		0xE
#define VMEXIT_CR15_READ		0xF
#define VMEXIT_CR0_WRITE		0x10
#define VMEXIT_CR1_WRITE		0x11
#define VMEXIT_CR2_WRITE		0x12
#define VMEXIT_CR3_WRITE		0x13
#define VMEXIT_CR4_WRITE		0x14
#define VMEXIT_CR5_WRITE		0x15
#define VMEXIT_CR6_WRITE		0x16
#define VMEXIT_CR7_WRITE		0x17
#define VMEXIT_CR8_WRITE		0x18
#define VMEXIT_CR9_WRITE		0x19
#define VMEXIT_CR10_WRITE		0x1A
#define VMEXIT_CR11_WRITE		0x1B
#define VMEXIT_CR12_WRITE		0x1C
#define VMEXIT_CR13_WRITE		0x1D
#define VMEXIT_CR14_WRITE		0x1E
#define VMEXIT_CR15_WRITE		0x1F
#define VMEXIT_DR0_READ			0x20
#define VMEXIT_DR1_READ			0x21
#define VMEXIT_DR2_READ			0x22
#define VMEXIT_DR3_READ			0x23
#define VMEXIT_DR4_READ			0x24
#define VMEXIT_DR5_READ			0x25
#define VMEXIT_DR6_READ			0x26
#define VMEXIT_DR7_READ			0x27
#define VMEXIT_DR8_READ			0x28
#define VMEXIT_DR9_READ			0x29
#define VMEXIT_DR10_READ		0x2A
#define VMEXIT_DR11_READ		0x2B
#define VMEXIT_DR12_READ		0x2C
#define VMEXIT_DR13_READ		0x2D
#define VMEXIT_DR14_READ		0x2E
#define VMEXIT_DR15_READ		0x2F
#define VMEXIT_DR0_WRITE		0x30
#define VMEXIT_DR1_WRITE		0x31
#define VMEXIT_DR2_WRITE		0x32
#define VMEXIT_DR3_WRITE		0x33
#define VMEXIT_DR4_WRITE		0x34
#define VMEXIT_DR5_WRITE		0x35
#define VMEXIT_DR6_WRITE		0x36
#define VMEXIT_DR7_WRITE		0x37
#define VMEXIT_DR8_WRITE		0x38
#define VMEXIT_DR9_WRITE		0x39
#define VMEXIT_DR10_WRITE		0x3A
#define VMEXIT_DR11_WRITE		0x3B
#define VMEXIT_DR12_WRITE		0x3C
#define VMEXIT_DR13_WRITE		0x3D
#define VMEXIT_DR14_WRITE		0x3E
#define VMEXIT_DR15_WRITE		0x3F
#define VMEXIT_EXCP0			0x40
#define VMEXIT_EXCP1			0x41
#define VMEXIT_EXCP2			0x42
#define VMEXIT_EXCP3			0x43
#define VMEXIT_EXCP4			0x44
#define VMEXIT_EXCP5			0x45
#define VMEXIT_EXCP6			0x46
#define VMEXIT_EXCP7			0x47
#define VMEXIT_EXCP8			0x48
#define VMEXIT_EXCP9			0x49
#define VMEXIT_EXCP10			0x4A
#define VMEXIT_EXCP11			0x4B
#define VMEXIT_EXCP12			0x4C
#define VMEXIT_EXCP13			0x4D
#define VMEXIT_EXCP14			0x4E
#define VMEXIT_EXCP15			0x4F
#define VMEXIT_EXCP16			0x50
#define VMEXIT_EXCP17			0x51
#define VMEXIT_EXCP18			0x52
#define VMEXIT_EXCP19			0x53
#define VMEXIT_EXCP20			0x54
#define VMEXIT_EXCP21			0x55
#define VMEXIT_EXCP22			0x56
#define VMEXIT_EXCP23			0x57
#define VMEXIT_EXCP24			0x58
#define VMEXIT_EXCP25			0x59
#define VMEXIT_EXCP26			0x5A
#define VMEXIT_EXCP27			0x5B
#define VMEXIT_EXCP28			0x5C
#define VMEXIT_EXCP29			0x5D
#define VMEXIT_EXCP30			0x5E
#define VMEXIT_EXCP31			0x5F
#define VMEXIT_INTR			0x60
#define VMEXIT_NMI			0x61
#define VMEXIT_SMI			0x62
#define VMEXIT_INIT			0x63
#define VMEXIT_VINTR			0x64
#define VMEXIT_CR0_SEL_WRITE		0x65
#define VMEXIT_IDTR_READ		0x66
#define VMEXIT_GDTR_READ		0x67
#define VMEXIT_LDTR_READ		0x68
#define VMEXIT_TR_READ			0x69
#define VMEXIT_IDTR_WRITE		0x6A
#define VMEXIT_GDTR_WRITE		0x6B
#define VMEXIT_LDTR_WRITE		0x6C
#define VMEXIT_TR_WRITE			0x6D
#define VMEXIT_RDTSC			0x6E
#define VMEXIT_RDPMC			0x6F
#define VMEXIT_PUSHF			0x70
#define VMEXIT_POPF			0x71
#define VMEXIT_CPUID			0x72
#define VMEXIT_RSM			0x73
#define VMEXIT_IRET			0x74
#define VMEXIT_SWINT			0x75
#define VMEXIT_INVD			0x76
#define VMEXIT_PAUSE			0x77
#define VMEXIT_HLT			0x78
#define VMEXIT_INVLPG			0x79
#define VMEXIT_INVLPGA			0x7A
#define VMEXIT_IOIO			0x7B
#define VMEXIT_MSR			0x7C
#define VMEXIT_TASK_SWITCH		0x7D
#define VMEXIT_FERR_FREEZE		0x7E
#define VMEXIT_SHUTDOWN			0x7F
#define VMEXIT_VMRUN			0x80
#define VMEXIT_VMMCALL			0x81
#define VMEXIT_VMLOAD			0x82
#define VMEXIT_VMSAVE			0x83
#define VMEXIT_STGI			0x84
#define VMEXIT_CLGI			0x85
#define VMEXIT_SKINIT			0x86
#define VMEXIT_RDTSCP			0x87
#define VMEXIT_ICEBP			0x88
#define VMEXIT_WBINVD			0x89
#define VMEXIT_MONITOR			0x8A
#define VMEXIT_MWAIT			0x8B
#define VMEXIT_MWAIT_CONDITIONAL	0x8C
#define VMEXIT_NPF			0x400
#define VMEXIT_INVALID			-1

#endif

