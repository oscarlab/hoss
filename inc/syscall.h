#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_trapframe,
	SYS_env_set_pgfault_upcall,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_time_msec,
	SYS_ept_map,
	SYS_env_mkguest,
#ifndef VMM_GUEST
	SYS_vmx_list_vms,
	SYS_vmx_sel_resume,
	SYS_vmx_get_vmdisk_number,
	SYS_vmx_incr_vmdisk_number,
#endif
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
