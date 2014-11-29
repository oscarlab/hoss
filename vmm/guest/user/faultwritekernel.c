// buggy program - faults with a write to a kernel location

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned*)0x8004000000 = 0;
}

