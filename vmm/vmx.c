
#include <vmm/vmx.h>
#include <vmm/vmx_asm.h>
#include <vmm/ept.h>
#include <vmm/vmexits.h>

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <kern/sched.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/kclock.h>
#include <kern/console.h>



void vmx_list_vms() {
	//findout how many VMs there
	int i;
	int vm_count = 0;
	for (i = 0; i < NENV; ++i) {
		if (envs[i].env_type == ENV_TYPE_GUEST) {
			if (vm_count == 0) {
				cprintf("Running VMs:\n");
			}
			vm_count++;
			cprintf("%d.[%x]vm%d\n", vm_count, envs[i].env_id, vm_count);
		}
	}
}

bool vmx_sel_resume(int num) {
	int i;
	int vm_count = 0;
	for (i = 0; i < NENV; ++i) {
		if (envs[i].env_type == ENV_TYPE_GUEST) {
			vm_count++;
			if (vm_count == num) {
				cprintf("Resume vm.%d\n", num);	
				envs[i].env_status = ENV_RUNNABLE;
				return true;
			}
		}
	}
	cprintf("Selected VM(No.%d VM) not found.\n", num);	
	return false;
}
/* static uintptr_t *msr_bitmap; */

/* Checks VMX processor support using CPUID.
 * See Section 23.6 of the Intel manual.
 * 
 * Hint: the TA solution uses the BIT() macro 
 *  to simplify the implementation.
 */
inline bool vmx_check_support() {
    uint32_t eax, ebx, ecx, edx;
    cpuid( 1, &eax, &ebx, &ecx, &edx );
    /* Your code here */ 
    panic ("vmx check not implemented\n");
    cprintf("[VMM] VMX extension not supported.\n");
    return false;
}

/* This function reads the VMX-specific MSRs
 * to determine whether EPT is supported.
 * See section 24.6.2 and Appenix A.3.3 of the Intel manual.
 * 
 * Hint: the TA solution uses the read_msr() helper function.
 * 
 * Hint: As specified in the appendix, the values in the tables
 *  are actually offset by 32 bits.
 *
 * Hint: This needs to check two MSR bits---first verifying
 *   that secondary VMX controls are enabled, and then that
 *   EPT is available.
 */
inline bool vmx_check_ept() {
    /* Your code here */
    panic ("ept check not implemented\n");
    cprintf("[VMM] EPT extension not supported.\n");
    return false;
}

/* Checks if curr_val is compatible with fixed0 and fixed1 
* (allowed values read from the MSR). This is to ensure current processor
* operating mode meets the required fixed bit requirement of VMX.  
*/
bool check_fixed_bits( uint64_t curr_val, uint64_t fixed0, uint64_t fixed1 ) {
    // TODO: Simplify this code.
    int i;
    for( i = 0 ; i < sizeof( curr_val ) * 8 ; ++i ) {
        int bit = BIT( curr_val, i );
        if ( bit == 1 ) {
            // Check if this bit is fixed to 0.
            if ( BIT( fixed1, i ) == 0 ) {
                return false;
            }
        } else if ( bit == 0 ) {
            // Check if this bit is fixed to 1.
            if ( BIT( fixed0, i ) == 1 ) {
                return false;
            }
        } else {
            assert(false);
        }
    }
    return true;
}

/* 
 * Allocate a page for the VMCS region and write the VMCS Rev. ID in the first 
 * 31 bits.
 */
struct PageInfo * vmx_init_vmcs() {
    // Read the VMX_BASIC MSR.
    uint64_t vmx_basic_msr =  read_msr( IA32_VMX_BASIC );
    uint32_t vmcs_rev_id = (uint32_t) vmx_basic_msr; // Bits 30:0, Bit 31 is always 0.

    uint32_t vmcs_num_bytes =  ( vmx_basic_msr >> 32 ) & 0xfff; // Bits 44:32.
    assert( vmcs_num_bytes <= 4096 ); // VMCS can have a max size of 4096.

    //Alocate mem for VMCS region.
    struct PageInfo *p_vmxon_region = page_alloc( ALLOC_ZERO );
    if(!p_vmxon_region) {
        return NULL;
    }
    p_vmxon_region->pp_ref += 1; 
    
    unsigned char* vmxon_region = (unsigned char *) page2kva( p_vmxon_region );
    memcpy( vmxon_region, &vmcs_rev_id, sizeof( vmcs_rev_id ) );

    return p_vmxon_region;
}

/* 
 * Sets up a VMXON region and executes VMXON to put the processor in VMX root 
 * operation. Returns a >=0 value if VMX root operation is achieved.
 */
int vmx_init_vmxon() {
    
    //Alocate mem and init the VMXON region.
    struct PageInfo *p_vmxon_region = vmx_init_vmcs();
    if(!p_vmxon_region)
        return -E_NO_MEM;

    uint64_t cr0 = rcr0();
    uint64_t cr4 = rcr4();
    // Paging and protected mode are enabled in JOS.
    
    // FIXME: Workaround for CR0.NE (bochs needs this to be set to 1)
    cr0 = cr0 | CR0_NE;
    lcr0( cr0 );

    bool ret =  check_fixed_bits( cr0,
                                  read_msr( IA32_VMX_CR0_FIXED0 ), 
                                  read_msr( IA32_VMX_CR0_FIXED1 ) );
    if ( !ret ) {
        page_decref( p_vmxon_region );
        return -E_VMX_ON;
    }
    // Enable VMX in CR4.
    cr4 = cr4 | CR4_VMXE;
    lcr4( cr4 );
    ret =  check_fixed_bits( cr4,
                             read_msr( IA32_VMX_CR4_FIXED0 ), 
                             read_msr( IA32_VMX_CR4_FIXED1 ) );
    if ( !ret ) {
        page_decref( p_vmxon_region );
        return -E_VMX_ON;
    }
    // Ensure that IA32_FEATURE_CONTROL MSR has been properly programmed and 
    // and that it's lock bit has been set.
    uint64_t feature_control = read_msr( IA32_FEATURE_CONTROL );
    if ( !BIT( feature_control, 2 )) {
        page_decref( p_vmxon_region );
        // VMX disabled in BIOS.
        return -E_NO_VMX;
    }
    if ( !BIT( feature_control, 0 )) {
        // Lock bit not set, try setting it.
        feature_control |= 0x1;
        write_msr( IA32_FEATURE_CONTROL, feature_control );   
    }
    
    uint8_t error = vmxon( (physaddr_t) page2pa( p_vmxon_region ) );
    if ( error ) { 
        page_decref( p_vmxon_region );
        return -E_VMX_ON; 
    }

    thiscpu->is_vmx_root = true;
    thiscpu->vmxon_region = (uintptr_t) page2kva( p_vmxon_region );

    return 0;
}

void vmcs_host_init() {
    vmcs_write64( VMCS_HOST_CR0, rcr0() ); 
    vmcs_write64( VMCS_HOST_CR3, rcr3() ); 
    vmcs_write64( VMCS_HOST_CR4, rcr4() );

    vmcs_write16( VMCS_16BIT_HOST_ES_SELECTOR, GD_KD );
    vmcs_write16( VMCS_16BIT_HOST_SS_SELECTOR, GD_KD );
    vmcs_write16( VMCS_16BIT_HOST_DS_SELECTOR, GD_KD );
    vmcs_write16( VMCS_16BIT_HOST_FS_SELECTOR, GD_KD );
    vmcs_write16( VMCS_16BIT_HOST_GS_SELECTOR, GD_KD );
    vmcs_write16( VMCS_16BIT_HOST_CS_SELECTOR, GD_KT );

	int gd_tss = (GD_TSS0 >> 3) + thiscpu->cpu_id*2;
    vmcs_write16( VMCS_16BIT_HOST_TR_SELECTOR, gd_tss << 3 );
    
    uint16_t xdtr_limit;
    uint64_t xdtr_base;
    read_idtr( &xdtr_base, &xdtr_limit );
    vmcs_write64( VMCS_HOST_IDTR_BASE, xdtr_base );

    read_gdtr( &xdtr_base, &xdtr_limit );
    vmcs_write64( VMCS_HOST_GDTR_BASE, xdtr_base );

    vmcs_write64( VMCS_HOST_FS_BASE, 0x0 );
    vmcs_write64( VMCS_HOST_GS_BASE, 0x0 );
    vmcs_write64( VMCS_HOST_TR_BASE, (uint64_t) &thiscpu->cpu_ts );

    uint64_t tmpl;
	asm("movabs $.Lvmx_return, %0" : "=r"(tmpl));
	vmcs_writel(VMCS_HOST_RIP, tmpl);
}

void vmcs_guest_init() {
    
    vmcs_write16( VMCS_16BIT_GUEST_CS_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_ES_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_SS_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_DS_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_FS_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_GS_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_TR_SELECTOR, 0x0 );
    vmcs_write16( VMCS_16BIT_GUEST_LDTR_SELECTOR, 0x0 );

    vmcs_write64( VMCS_GUEST_CS_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_ES_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_SS_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_DS_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_FS_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_GS_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_LDTR_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_GDTR_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_IDTR_BASE, 0x0 );
    vmcs_write64( VMCS_GUEST_TR_BASE, 0x0 );

    vmcs_write32( VMCS_32BIT_GUEST_CS_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_ES_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_SS_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_DS_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_FS_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_GS_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_LDTR_LIMIT, 0x0000FFFF );
    vmcs_write32( VMCS_32BIT_GUEST_TR_LIMIT, 0xFFFFF );
    vmcs_write32( VMCS_32BIT_GUEST_GDTR_LIMIT, 0x30 );
    vmcs_write32( VMCS_32BIT_GUEST_IDTR_LIMIT, 0x3FF );
    // FIXME: Fix access rights.
    vmcs_write32( VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS, 0x93 );
    vmcs_write32( VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS, 0x82 );
    vmcs_write32( VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS, 0x8b );

    vmcs_write32( VMCS_32BIT_GUEST_ACTIVITY_STATE, 0 );
    vmcs_write32( VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE, 0 );

    vmcs_write64( VMCS_GUEST_CR3, 0 );
    vmcs_write64( VMCS_GUEST_CR0, CR0_NE );
    vmcs_write64( VMCS_GUEST_CR4, CR4_VMXE );

    vmcs_write64( VMCS_64BIT_GUEST_LINK_POINTER, 0xffffffff );
    vmcs_write64( VMCS_64BIT_GUEST_LINK_POINTER_HI, 0xffffffff ); 
    vmcs_write64( VMCS_GUEST_DR7, 0x0 );
    vmcs_write64( VMCS_GUEST_RFLAGS, 0x2 );

}

void vmx_read_capability_msr( uint32_t msr, uint32_t* hi, uint32_t* lo ) {
    uint64_t msr_val = read_msr( msr );
    *hi = (uint32_t)( msr_val >> 32 );
    *lo = (uint32_t)( msr_val );
}

static void 
vmcs_ctls_init( struct Env* e ) {
    // Set pin based vm exec controls.
    uint32_t pinbased_ctls_or, pinbased_ctls_and;
    vmx_read_capability_msr( IA32_VMX_PINBASED_CTLS, 
            &pinbased_ctls_and, &pinbased_ctls_or );

	//enable the guest external interrupt exit    
	pinbased_ctls_or |= VMCS_PIN_BASED_VMEXEC_CTL_EXINTEXIT;
    vmcs_write32( VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS, 
            pinbased_ctls_or & pinbased_ctls_and );

    // Set proc-based controls.
    uint32_t procbased_ctls_or, procbased_ctls_and;
    vmx_read_capability_msr( IA32_VMX_PROCBASED_CTLS, 
            &procbased_ctls_and, &procbased_ctls_or );
    // Make sure there are secondary controls.
    assert( BIT( procbased_ctls_and, 31 ) == 0x1 ); 
   
    procbased_ctls_or |= VMCS_PROC_BASED_VMEXEC_CTL_ACTIVESECCTL; 
    procbased_ctls_or |= VMCS_PROC_BASED_VMEXEC_CTL_HLTEXIT;
    procbased_ctls_or |= VMCS_PROC_BASED_VMEXEC_CTL_USEIOBMP;
    /* CR3 accesses and invlpg don't need to cause VM Exits when EPT
       enabled */
    procbased_ctls_or &= ~( VMCS_PROC_BASED_VMEXEC_CTL_CR3LOADEXIT |
            VMCS_PROC_BASED_VMEXEC_CTL_CR3STOREXIT | 
            VMCS_PROC_BASED_VMEXEC_CTL_INVLPGEXIT );

    vmcs_write32( VMCS_32BIT_CONTROL_PROCESSOR_BASED_VMEXEC_CONTROLS, 
            procbased_ctls_or & procbased_ctls_and );

    // Set Proc based secondary controls.
    uint32_t procbased_ctls2_or, procbased_ctls2_and;
    vmx_read_capability_msr( IA32_VMX_PROCBASED_CTLS2, 
            &procbased_ctls2_and, &procbased_ctls2_or );
    
    // Enable EPT.
    procbased_ctls2_or |= VMCS_SECONDARY_VMEXEC_CTL_ENABLE_EPT;
    procbased_ctls2_or |= VMCS_SECONDARY_VMEXEC_CTL_UNRESTRICTED_GUEST;
    vmcs_write32( VMCS_32BIT_CONTROL_SECONDARY_VMEXEC_CONTROLS, 
            procbased_ctls2_or & procbased_ctls2_and );

    // Set VM exit controls.
    uint32_t exit_ctls_or, exit_ctls_and;
    vmx_read_capability_msr( IA32_VMX_EXIT_CTLS, 
            &exit_ctls_and, &exit_ctls_or );

    exit_ctls_or |= VMCS_VMEXIT_HOST_ADDR_SIZE;
    exit_ctls_or |= VMCS_VMEXIT_GUEST_ACK_INTR_ON_EXIT;	
    vmcs_write32( VMCS_32BIT_CONTROL_VMEXIT_CONTROLS, 
            exit_ctls_or & exit_ctls_and );

    vmcs_write64( VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR,
            PADDR(e->env_vmxinfo.msr_guest_area));
    vmcs_write32( VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT,
            e->env_vmxinfo.msr_count);
    vmcs_write64( VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR,
            PADDR(e->env_vmxinfo.msr_host_area));
    vmcs_write32( VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT,
            e->env_vmxinfo.msr_count);

    // Set VM entry controls.
    uint32_t entry_ctls_or, entry_ctls_and;
    vmx_read_capability_msr( IA32_VMX_ENTRY_CTLS, 
            &entry_ctls_and, &entry_ctls_or );

    vmcs_write64( VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR,
            PADDR(e->env_vmxinfo.msr_guest_area));
    vmcs_write32( VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT,
            e->env_vmxinfo.msr_count);

    vmcs_write32( VMCS_32BIT_CONTROL_VMENTRY_CONTROLS, 
            entry_ctls_or & entry_ctls_and );
    
    uint64_t ept_ptr = e->env_cr3 | ( ( EPT_LEVELS - 1 ) << 3 );
    vmcs_write64( VMCS_64BIT_CONTROL_EPTPTR, ept_ptr );

    vmcs_write32( VMCS_32BIT_CONTROL_EXCEPTION_BITMAP, 
            e->env_vmxinfo.exception_bmap);
    vmcs_write64( VMCS_64BIT_CONTROL_IO_BITMAP_A,
            PADDR(e->env_vmxinfo.io_bmap_a));
    vmcs_write64( VMCS_64BIT_CONTROL_IO_BITMAP_B,
            PADDR(e->env_vmxinfo.io_bmap_b));

}

void vmcs_dump_cpu() {
	uint64_t flags = vmcs_readl(VMCS_GUEST_RFLAGS);

    // TODO: print all the regs.
	cprintf( "vmx: --- Begin VCPU Dump ---\n");
	cprintf( "vmx: RIP 0x%016llx RSP 0x%016llx RFLAGS 0x%016llx\n",
	       vmcs_read64( VMCS_GUEST_RIP ) , vmcs_read64( VMCS_GUEST_RSP ), flags);
	cprintf( "vmx: CR0 0x%016llx CR3 0x%016llx\n",
            vmcs_read64( VMCS_GUEST_CR0 ), vmcs_read64( VMCS_GUEST_CR3 ) );
    cprintf( "vmx: CR4 0x%016llx \n",
            vmcs_read64( VMCS_GUEST_CR4 ) );

    cprintf( "vmx: --- End VCPU Dump ---\n");

}

void vmexit() {
    int exit_reason = -1;
    bool exit_handled = false;
    static uint32_t host_vector;
    // Get the reason for VMEXIT from the VMCS.
    // Your code here.

     //cprintf( "---VMEXIT Reason: %d---\n", exit_reason ); 
    /* vmcs_dump_cpu(); */
 
    switch(exit_reason & EXIT_REASON_MASK) {
    	case EXIT_REASON_EXTERNAL_INT:
    		host_vector = vmcs_read32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO);
    		exit_handled = handle_interrupts(&curenv->env_tf, &curenv->env_vmxinfo, host_vector);
    		break;
    	case EXIT_REASON_INTERRUPT_WINDOW:
    		exit_handled = handle_interrupt_window(&curenv->env_tf, &curenv->env_vmxinfo, host_vector);
    		break;
        case EXIT_REASON_RDMSR:
            exit_handled = handle_rdmsr(&curenv->env_tf, &curenv->env_vmxinfo);
            break;
        case EXIT_REASON_WRMSR:
            exit_handled = handle_wrmsr(&curenv->env_tf, &curenv->env_vmxinfo);
            break;
        case EXIT_REASON_EPT_VIOLATION:
            exit_handled = handle_eptviolation(curenv->env_pml4e, &curenv->env_vmxinfo);
            break;
        case EXIT_REASON_IO_INSTRUCTION:
            exit_handled = handle_ioinstr(&curenv->env_tf, &curenv->env_vmxinfo);
            break;
        case EXIT_REASON_CPUID:
            exit_handled = handle_cpuid(&curenv->env_tf, &curenv->env_vmxinfo);
            break;
        case EXIT_REASON_VMCALL:
            exit_handled = handle_vmcall(&curenv->env_tf, &curenv->env_vmxinfo,
                    curenv->env_pml4e);
            break;
        case EXIT_REASON_HLT:
            cprintf("\nHLT in guest, exiting guest.\n");
            env_destroy(curenv);
            exit_handled = true;
            break;
    }

    if(!exit_handled) {
        cprintf( "Unhandled VMEXIT, aborting guest.\n" );
        vmcs_dump_cpu();
        env_destroy(curenv);
    }
    
    sched_yield();
}

void asm_vmrun(struct Trapframe *tf) {

    /* cprintf("VMRUN\n"); */
    // NOTE: Since we re-use Trapframe structure, tf.tf_err contains the value
    // of cr2 of the guest.
    tf->tf_ds = curenv->env_runs;
    tf->tf_es = 0;
    asm(
            "push %%rdx; push %%rbp;"
            "push %%rcx \n\t" /* placeholder for guest rcx */
            "push %%rcx \n\t"
	    /* Set the VMCS rsp to the current top of the frame. */
            /* Your code here */
            "1: \n\t"
            /* Reload cr2 if changed */
            "mov %c[cr2](%0), %%rax \n\t"
            "mov %%cr2, %%rdx \n\t"
            "cmp %%rax, %%rdx \n\t"
            "je 2f \n\t"
            "mov %%rax, %%cr2 \n\t"
            "2: \n\t"
            /* Check if vmlaunch of vmresume is needed, set the condition code
	     * appropriately for use below.  
	     * 
	     * Hint: We store the number of times the VM has run in tf->tf_ds
	     * 
	     * Hint: In this function, 
	     *       you can use register offset addressing mode, such as '%c[rax](%0)' 
	     *       to simplify the pointer arithmetic.
	     */
	    /* Your code here */
            /* Load guest general purpose registers from the trap frame.  Don't clobber flags. 
	     *
	     */
	    /* Your code here */
            /* Enter guest mode */
	    /* Your code here:
	     * 
	     * Test the condition code from rflags
	     * to see if you need to execute a vmlaunch
	     * instruction, or just a vmresume.
	     * 
	     * Note: be careful in loading the guest registers
	     * that you don't do any compareison that would clobber the condition code, set
	     * above.
	     */
            ".Lvmx_return: "

	    /* POST VM EXIT... */
            "mov %0, %c[wordsize](%%rsp) \n\t"
            "pop %0 \n\t"
            /* Save general purpose guest registers and cr2 back to the trapframe.
	     *
	     * Be careful that the number of pushes (above) and pops are symmetrical.
	     */
	    /* Your code here */
            "pop  %%rbp; pop  %%rdx \n\t"

            "setbe %c[fail](%0) \n\t"
            : : "c"(tf), "d"((unsigned long)VMCS_HOST_RSP), 
            [launched]"i"(offsetof(struct Trapframe, tf_ds)),
            [fail]"i"(offsetof(struct Trapframe, tf_es)),
            [rax]"i"(offsetof(struct Trapframe, tf_regs.reg_rax)),
            [rbx]"i"(offsetof(struct Trapframe, tf_regs.reg_rbx)),
            [rcx]"i"(offsetof(struct Trapframe, tf_regs.reg_rcx)),
            [rdx]"i"(offsetof(struct Trapframe, tf_regs.reg_rdx)),
            [rsi]"i"(offsetof(struct Trapframe, tf_regs.reg_rsi)),
            [rdi]"i"(offsetof(struct Trapframe, tf_regs.reg_rdi)),
            [rbp]"i"(offsetof(struct Trapframe, tf_regs.reg_rbp)),
            [r8]"i"(offsetof(struct Trapframe, tf_regs.reg_r8)),
            [r9]"i"(offsetof(struct Trapframe, tf_regs.reg_r9)),
            [r10]"i"(offsetof(struct Trapframe, tf_regs.reg_r10)),
            [r11]"i"(offsetof(struct Trapframe, tf_regs.reg_r11)),
            [r12]"i"(offsetof(struct Trapframe, tf_regs.reg_r12)),
            [r13]"i"(offsetof(struct Trapframe, tf_regs.reg_r13)),
            [r14]"i"(offsetof(struct Trapframe, tf_regs.reg_r14)),
            [r15]"i"(offsetof(struct Trapframe, tf_regs.reg_r15)),
            [cr2]"i"(offsetof(struct Trapframe, tf_err)),
            [wordsize]"i"(sizeof(uint64_t))
                : "cc", "memory"
                    , "rax", "rbx", "rdi", "rsi"
                        , "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    );

    if(tf->tf_es) {
        cprintf("Error during VMLAUNCH/VMRESUME\n");
    } else {
        curenv->env_tf.tf_rsp = vmcs_read64(VMCS_GUEST_RSP);
        curenv->env_tf.tf_rip = vmcs_read64(VMCS_GUEST_RIP);
        vmexit();
    }
}

void
msr_setup(struct VmxGuestInfo *ginfo) {
    struct vmx_msr_entry *entry;
    uint32_t idx[] = { EFER_MSR };
    int i, count = sizeof(idx) / sizeof(idx[0]);

    assert(count <= MAX_MSR_COUNT);
    ginfo->msr_count = count;
    
    for(i=0; i<count; ++i) {
        entry = ((struct vmx_msr_entry *)ginfo->msr_host_area) + i;
        entry->msr_index = idx[i];
        entry->msr_value = read_msr(idx[i]);
        
        entry = ((struct vmx_msr_entry *)ginfo->msr_guest_area) + i;
        entry->msr_index = idx[i];
    }
}

void
bitmap_setup(struct VmxGuestInfo *ginfo) {
    unsigned int io_ports[] = { IO_RTC, IO_RTC+1 };
    int i, count = sizeof(io_ports) / sizeof(io_ports[0]);
    
    for(i=0; i<count; ++i) {
        int idx = io_ports[i] / (sizeof(uint64_t) * 8);
        if(io_ports[i] < 0x7FFF) {
            ginfo->io_bmap_a[idx] |= ((0x1uL << (io_ports[i] & 0x3F)));
        } else if (io_ports[i] < 0xFFFF) {
            ginfo->io_bmap_b[idx] |= ((0x1uL << (io_ports[i] & 0x3F)));
        } else {
            assert(false);
        }
    }
}

/* 
 * Processor must be in VMX root operation before executing this function.
 */
int vmx_vmrun( struct Env *e ) {

    if ( e->env_type != ENV_TYPE_GUEST ) {
        return -E_INVAL;
    }
    uint8_t error;

    if( e->env_runs == 1 ) {
        physaddr_t vmcs_phy_addr = PADDR(e->env_vmxinfo.vmcs);

        // Call VMCLEAR on the VMCS region.
        error = vmclear(vmcs_phy_addr);
        // Check if VMCLEAR succeeded. ( RFLAGS.CF = 0 and RFLAGS.ZF = 0 )
        if ( error )
            return -E_VMCS_INIT; 

        // Make this VMCS working VMCS.
        error = vmptrld(vmcs_phy_addr);
        if ( error )
            return -E_VMCS_INIT; 

        vmcs_host_init();
        vmcs_guest_init();
        // Setup IO and exception bitmaps.
        bitmap_setup(&e->env_vmxinfo);
        // Setup the msr load/store area
        msr_setup(&e->env_vmxinfo);
        vmcs_ctls_init(e);

        /* ept_alloc_static(e->env_pml4e, &e->env_vmxinfo); */

    } else {
        // Make this VMCS working VMCS.
        error = vmptrld(PADDR(e->env_vmxinfo.vmcs));
        if ( error ) {
            return -E_VMCS_INIT; 
        }
    }

    vmcs_write64( VMCS_GUEST_RSP, curenv->env_tf.tf_rsp  );
    vmcs_write64( VMCS_GUEST_RIP, curenv->env_tf.tf_rip );
    panic ("asm vmrun incomplete\n");
    asm_vmrun( &e->env_tf );    
    return 0;
}
