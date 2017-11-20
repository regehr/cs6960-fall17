Proof of Inability to Crash xv6 with `read` System Call
=======================================================

The goal is to crash xv6 using `read` system call by getting
the *unexpected trap* message by making the following if statement true.

```
if(myproc() == 0 || (tf->cs&3) == 0){
  // In kernel, it must be our mistake.
  cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
          tf->trapno, cpuid(), tf->eip, rcr2());
  panic("trap");
}
```

There are two ways to make the above statement true:

- trap with current process being zero
- read to unmapped page and change the `cs` in trap frame to ring 0

The first way is impossible unless we are able to alter the code of kernel itself.
The reason is that when current process pointer is zero, we must be at the start
of `scheduler` function. Inside the infinite loop the interrupts are
enabled (`sti()`) and the `c->proc` is set to zero after returning back to scheduler process.
Unfortunately, all interrupts that could occur during scheduler process are handled
properly (IDE, keyboard, uart).

The second approach would be to either overwrite kernel or the `cs` value in trap
frame. Since trap frame is stored on kernel stack, the above are functionaly equivalent.
Therefore we have to call `read` with pointer either pointing to kernel memory or
make the read go over valid range.

The system call `read` takes file descriptor, pointer to where to read, and number
of bytes to read. Assume all arguments can attain any values before entering the
function `sys_read`.

`argfd`

- read valid integer with `argint`
- check if file descriptor is within `ofile` array range and if the entry is set

`argptr`

- read valid integer with `argint` which becomes pointer
- check if pointer `i` is within address space by comparing with `sz` (have to do separately to avoid potential wrap around if `size` was added)
- check if array lies within current process address space; can overflow the unsigned integer range if the pointer has high value (can't happen due to memory limits imposed by xv6); TODO: why is the `>` instead of `>=` in last subexpression


`argint`

- call `fetchint` with user space stack pointer
- check the passed stack address if it is in process's address space
- check if there are 4 bytes to read in process's address space
- read the integer from address


Since we can't overflow the array and even if we could, we would still be unable all pages up to the
kernel boundary due to the size of available memory for user - `PHYSTOP`. Thus even invoking page fault
in trap handler would still not panic kernel as the `cs` in trap frame would still be ring 3.

