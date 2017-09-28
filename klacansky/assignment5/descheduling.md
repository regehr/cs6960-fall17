Proof of Process Descheduling
=============================

We want to prove that user space process gets eventually descheduled, that is it will reach following code in function `trap` (`trap.c`).

```
// Force process to give up CPU on clock tick.
// If interrupts were on while locks held, would need to check nlock.
if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
  yield();
```

First, we need to prove that interrupts are properly set up by loading *interrupt descriptor
table* (IDT) with `lidt` instruction. The function chain is as follows:

- `cli` (`bootasm.S`) disables interrupts
- `bootmain.c` and `entry.S` do not enable interrupts
- `main` (`main.c`)
- `lapicinit` (`lapic.c`) enables interrupts on local APIC
- `tvinit` (`trap.c`) sets up IDT entries in memory
- `startothers` (`main.c`) starts other cores
- `mpmain` (`main.c`) called on each core
- `idtinit` (`trap.c`) calls `lidt` instruction
- `scheduler` (`proc.c`) enables interrupts

We need to make sure each time interrupts are disabled they are reenabled. The enable/disable
operation is usually done through helper functions `popcli`/`pushcli` (`spinlock.c`).
This list contains all places where are interrupts touched:

- `myproc` (`proc.c`) disables and enables interrupts (no branching)
- `switchuvm` (`vm.c`) disables and enables interrupts (no branching)
- `acquire` (`spinlock.c`) disables interrupts thus must be matched by `release`

Now we need to check that all spinlock `acquire` are matched by `release` (stack based, so
we can do push, push matched by pop, pop):

- `bget` (`bio.c`) released in for loop before going to sleep; otherwise there is panic
- `brelse` (`bio.c`) matched release at the end
- `filealloc` (`file.c`) released before both returns
- `filedup` (`file.c`) matching release or panic
- `fileclose` (`file.c`) release before return or panic
- `iget` (`fs.c`) release before return or panic
- `idup` (`fs.c`) matching release
- `iput` (`fs.c`) matching releases (in if branch and outside)
- `ideintr` (`ide.c`) release before both returns
- `iderw` (`ide.c`) released temporarily by sleep or at the end of function
- `kfree`, `kalloc` (`kalloc.c`) acquired and released if `use_lock` is not zero
- `begin_op` (`log.c`) released and reaquired by sleep or released before breaking out
- `end_op` (`log.c`) both acquires matched with release
- `log_write` (`log.c`) matched before return
- `pipeclose` (`pipe.c`) either released in if or else branch (**TODO:** why not release before if?)
- `pipewrite`, `piperead` (`pipe.c`) released before return (or in sleep)
- `acquiresleep` (`sleeplock.c`) released before return or in sleep
- `releasesleep`, `holdingsleep` (`sleeplock.c`) matching release
- `sys_sleep` (`sysproc.c`) either released in while loop (if branch or sleep) or on exit
- `sys_uptime` (`sysproc.c`) matched release
- `trap` (`trap.c`) matched release in handling timer interrupt
- `allocproc` (`proc.c`) either released if no process is found or when found
- `userinit`, `fork` (`proc.c`) matched with release
- `exit` (`proc.c`) released in `scheduler` after if
- `wait` (`proc.c`) either release before returning or release in sleep
- `scheduler` (`proc.c`) interrupts are enabled and acquired lock is released not on scheduler thread (in yield or forkret)
- `yield` (`proc.c`) matched released in `scheduler` after if
- `forkret` (`proc.c`) releases lock (thus call `popcl`) after being scheduled (forked process)
- `sleep` (`proc.c`)
- `wakeup` (`proc.c`) matched with release
- `kill` (`proc.c`) released before each return
- `consoleread` (`console.c`) before each return released
- `consolewrite` (`console.c`) matched with release
- `cprintf` (`console.c`) matched with release (used only if `locking` is not zero)

There is really cool interplay of `forkret` (process creation), `yield`, and `exit` (process destruction). Symmetry between
`forkret` releasing `ptable` lock and `exit` acquiring the lock is necessary as we are entering `scheduler` for first time
or exiting it forever. The reason `yield` acquires and releases the lock is because first it is releasing the lock acquired
by scheduler process and then reacquiring it when returning back to scheduler process.
