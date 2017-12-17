# Proof of CPU Yielding in xv6

## Introduction

I wasn't sure where to start with this proof, so I looked at Pavol's for ideas.
But his is written really well; it was too easy to read, and I feel like
whatever I write now would be essentially the same thing. So I will run through
his proof and add some commentary to show that I didn't just copy/past it.

## Claim

He starts with rewording the claim of the proof: that all processes will at some
point run through the code in `trap.c` which forces a process to yield the CPU.
(This code is located on lines 103-106.) Putting the claim into these terms
makes constructing the proof much easier, because now everything can be laid out
in terms of execution paths and whether they reach this point in the code.

## Interrupts are enabled

Pavol then shows that interrupts are set up correctly in the first place. After
all, without interrupts we can't very well expect to run through the code which
handles the interrupts! This includes the process of loading the interrupt
descriptor table, since interrupts certainly wouldn't work (would not be "set up
correctly") if this step were to go undone.

The path of execution is traced from boot until the scheduler, and at each point
we see that interrupts have been handled correctly. Specifically:

* Starting with `bootasm.S`, we see that interrupts start in a disabled state
  with a call to `cli`. This is done to allow other setup to take place
  uninterrupted.
* The boot process moves to `bootmain.c` and then to `entry.S` without even
  thinking about interrupts.
* `main.c` is reached, which performs the rest of the setup before initializing
  things for the user. From `main()` we see:
  * `lapicinit()` is called, which sets up the local interrupt controller. This
    setup finishes on line 97 of `lapic.c`, which enables the interrupt on the
    interrupt controller ("but not on the processor", as the comments note).
  * The interrupt descriptor table is set up in memory by a call to `tvinit()`.
    This still does not enable interrupts on the CPU.
  * We continue to not enable interrupts for a while as each core is spun up.
  * `mpmain()` is called to setup the CPU cores.
    * A call goes out to `idtinit()`, which loads the interrupt descriptor
      table for that processor.
    * Processes begin to run by a call to `scheduler()`, which calls `sti()` to
      enable interrupts on the processor.

Therefore, each processor which boots correctly will enable interrupts.

## Disabled interrupts are always re-enabled

The proof then moves to show that any time interrupts are disabled, they will be
enabled again. That is, there is no valid path of execution that leaves the
interrupts in a disabled state. Pavol notes that generally the toggling of the
state of interrupts is done through helper methods `popcli()` and `pushcli()`.

These methods are provided in `spinlock.c` and are used in:

* `myproc()` in `proc.c`, which disables and then enables the interrupts.
* `switchuvm()` in `vm.c`, which disables and then enables interrupts.
* `spinlock.c`, where `acquire()` disables interrupts (to avoid deadlock), and
  `release()` enables the interrupts again.

Since all disablings exist in a duality with re-enablings, we are okay.
However...

## Acquired spinlocks are always released

The only loose end left here is that we require xv6 to guarantee that whenever
a lock is acquired via `acquire()`, it must also be released via `release()` (or
otherwise re-enable the interrupts).

Pavol has a long list of every time a lock is acquired and then released. The
list seems complete (I don't see missing entries), so I will defer to his list
for this part.

## Conclusion

xv6 will not complete the boot process without successfully enabling interrupts.
If interrupts are ever disabled, they are always re-enabled -- so long as any
spinlock `acquire`/`release` duality is handled correctly (i.e. every `acquire`
is paired with a `release` call). Since all of these conditions hold in xv6, we
can safely claim that we will always reach that bit of code mentioned up in the
introduction, which specifically means that all processes will be forced to
yield their time to the CPU when it is demanded of them. This is incredibly
important, since it is a necessity to have a successful multi-process OS.
