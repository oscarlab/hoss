/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ERROR_H
#define JOS_INC_ERROR_H

enum {
	// Kernel error codes -- keep in sync with list in lib/printfmt.c.
	E_UNSPECIFIED	= 1,	// Unspecified or unknown problem
	E_BAD_ENV	= 2,	// Environment doesn't exist or otherwise
				// cannot be used in requested action
	E_INVAL		= 3,	// Invalid parameter
	E_NO_MEM	= 4,	// Request failed due to memory shortage
	E_NO_FREE_ENV	= 5,	// Attempt to create a new environment beyond
				// the maximum allowed
	E_FAULT		= 6,	// Memory fault
	E_NO_SYS	= 7,	// Unimplemented system call

	E_IPC_NOT_RECV	= 8,	// Attempt to send to env that is not recving
	E_EOF		= 9,	// Unexpected end of file

	// File system error codes -- only seen in user-level
	E_NO_DISK	= 10,	// No free space left on disk
	E_MAX_OPEN	= 11,	// Too many files are open
	E_NOT_FOUND	= 12, 	// File or block not found
	E_BAD_PATH	= 13,	// Bad path
	E_FILE_EXISTS	= 14,	// File already exists
	E_NOT_EXEC	= 15,	// File not a valid executable
	E_NOT_SUPP	= 16,	// Operation not supported
	// VMM error codes.
	E_NO_VMX = 17,    // The processor doesn't support VMX or 
	// is turned off in the BIOS
	E_NO_EPT = 18,
	E_VMX_ON = 19,    // Couldn't transition the cpu to VMX root mode
	E_VMCS_INIT = 20, // Couldn't init the VMCS region
	E_NO_ENT = 21,
	MAXERROR
};

#endif	// !JOS_INC_ERROR_H */
