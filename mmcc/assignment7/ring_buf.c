#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"

const uint buf_sz = 8 * PGSIZE;

int *ntaken = (int *)0x0000f000;
char *buf = (char *)0x0000f000 + PGSIZE;

int
buf_put(char *data, uint len)
{
	if (len > buf_sz)
		return -1;
	
	// if we're full
	if (*ntaken == 8)
		return -1;

	return 0;
}

int
buf_get(char *data, uint len, uint maxlen)
{
	if (len > buf_sz)
		return -1;

	// if we're empty
	if (*ntaken == 0)
		return -1;

	return 0;
}
