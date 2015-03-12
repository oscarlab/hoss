#ifndef VMM_GUEST
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	char *buf;
	sys_vmx_list_vms();
	buf = readline("Please select a VM to resume: ");
	while (!(strlen(buf) == 1
		&& buf[0] >= '1' 
		&& buf[0] <= '9')) {
error:		cprintf("Please enter a correct vm number\n");
		buf = readline("Please select a VM to resume: ");
	}
	
	if (sys_vmx_sel_resume(buf[0] - '1' + 1)) {
		cprintf("Press Enter to Continue\n");
	}
	else {		
		goto error;
	}
	
}
#endif

