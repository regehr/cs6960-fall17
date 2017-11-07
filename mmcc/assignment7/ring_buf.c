#include <types.h>
#include <defs.h>
#include <mmu.h>

static char *pages[9];

int
buf_setup(void)
{
	int i;

	for (i = 0; i < 9; i++) {
		if ((pages[i] = kalloc()) == 0)
			return -1;
	}

	// unnecessary in any real Unix, but just to be safe
	for (i = 0; i < PGSIZE; i++)
		pages[0][i] = 0;

	// number of messages is the first four bytes (init: 0)
	// ptr to next message is next four bytes (init: NULL)

	return 0;
}

int
buf_put(char *p, int len)
{
	if (len > PGSIZE)
		return -1

	(int *)pages[0]++;
}

int
buf_get(char *p, int len, int maxlen)
{
	char *cur_msg = &pages[0][4];
	int cur_msg_sz = *(int *)cur_msg;
	char *msg_ptr;

	// there are no messages waiting
	if (!cur_msg)
		return -1;

	if (cur_msg_sz > maxlen)
		return -2;

	for (i = 0; i < len; i++)
		p[i] = cur_msg[i+4]

	(int *)pages[0]--;

	return 0;
}
