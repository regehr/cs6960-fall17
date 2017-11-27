#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"

// we use the first page for metadata
const char *base = (char *)0x0000f000 + PGSIZE;

static int *head = (int *)0x0000f000;
static int *tail = head + sizeof(*head);
static uint *sizes = (uint *)tail + sizeof(*tail);
static int *lock = (int *)sizes + (sizeof(uint) * 8);

int
buf_put(char *data, uint len)
{
	__sync_synchronize();
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE) == 1)
		continue;

	if (len > PGSIZE)
		goto fail;
	
	// if we're full
	if (((*tail + 1) % 8) == *head)
		goto fail;

	*tail = (*tail + 1) % 8;

	memcpy(base + *tail * PGSIZE, data, len);
	sizes[*tail] = len;

	__sync_synchronize();
	__atomic_clear(lock, __ATOMIC_RELEASE);
	return 0;

fail:
	__sync_synchronize();
	__atomic_clear(lock, __ATOMIC_RELEASE);
	return -1;
}

int
buf_get(char *data, uint len, uint maxlen)
{
	uint clen = len;

	__sync_synchronize();
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE) == 1)
		continue;

	if (len > PGSIZE)
		goto fail;

	if (maxlen < len)
		goto fail;

	// if we're empty
	if (*head == *tail)
		goto fail;

	if (sizes[*head] < clen)
		clen = sizes[*head];

	if (clen > maxlen)
		clen = maxlen;

	memcpy(data, base + *head * PGSIZE, clen);

	*head = (*head + 1) % 8;

	__sync_synchronize();
	__atomic_clear(lock, __ATOMIC_RELEASE);
	return 0;

fail:
	__sync_synchronize();
	__atomic_clear(lock, __ATOMIC_RELEASE);
	return -1;
}
