We want to prove that user space process gets eventually descheduled.

Assume timer interrupt periodically fires (interrupts are enabled by `sti` in `scheduler` function, `proc.c`).
Now, suppose there is a process which did not get descheduled. Then there can't
be a call to `swtch` with scheduler context. Since `trap` (`trap.c`) can only `exit`
or `yield` on timer interrupt without panicing, `sched` will be called and in turn
`swtch` will switch to scheduler context. QED, such process can't exist.
