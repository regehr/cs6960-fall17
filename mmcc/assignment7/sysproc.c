#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int mappages(pde_t *, void *, uint, uint, int);

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_buf_setup(void)
{
	int i, j, e;
	char *va;

	if (myproc()->bufpages[0] != 0)
		return -1;

	for (i = 0; i < 9; i++) {
		if ((myproc()->bufpages[i] = kalloc()) == 0) {
			for (j = 0; j < i; j++)
				kfree(myproc()->bufpages[j]);
		}
	}
	for (i = 0, va = (char *)0x0000f000; i < 9; i++, va += PGSIZE) {
		e = mappages(myproc()->pgdir, va, PGSIZE, V2P(myproc()->bufpages[i]), PTE_P | PTE_W | PTE_U);
		if (e == -1)
			return -1;	// TODO: unmap and deallocate all eight pages
	}

	return 0;
}
