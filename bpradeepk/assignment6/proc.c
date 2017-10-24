#include "types.h" 		//@ Extra definitions for something like unsigned int as uint, etc
#include "defs.h"		//@ Contains all function definitions
#include "param.h"		//@ Contains all xv6 level params like #processes, #active inodes, #CPUs, etc
#include "memlayout.h"	//@ This contains all high level memory boundaries like start of external memory, KernelBase, v2p and p2v mapping functions
#include "mmu.h"		//@ All constants, tempSigs, structs related to memory managament unit
#include "x86.h"		//@ Some special assembly instructions of x86 provided to be used by c routines
#include "proc.h"		//@ This contains all process related structs used by this file. Structs like cpu, proc, context(used for swapping processes)
#include "spinlock.h"	//@ Simple spinlock struct

//@ Removing it, as its conflicting with defs.h
//@ Added malloc() defintion in defs.h
//#include "user.h"			//@ This has definition of malloc()

//@ Unable to use NULL, and saw some examples using directly 0, so using that instead of NULL
//@ Maybe I should have included stdio.h and got NULL, will try later

//@ Assignment 2, we have to implement ready Queue
//@ I started with malloc(), which wouldn't work as we are not having malloc()
//@ we have kalloc(), but that allocates 4096 B page
//@ Other's have implemented by includeing new fields in the process structure
//@ Even though my idea takes up extra space, I am going to submit as it is
//@ Next thought of Array idea, but that sucks, if someone tries to delete an entry in between
//@ it will leave holes will mess up my logic.
//@ So, finally doing it using what I have seen in class, what most of the students did.

//@ Process table struct
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct proc *head,*tail;
} ptable;

struct {
  struct spinlock lock;
  struct proc *curProc;
  int killFlag;
} killProc;

//@ Should be called holding lock
struct proc*  dequeue(){
  //@ Returns a ready process and updates head
  struct proc *res = ptable.head; 
  //@ Update head not required as scheduler will change state of the selected process immediately
  //@ So, this head will automatically get updated
  //cprintf("dequeue: %d \n",res->pid);
  return res;
}


//@ make a process runnable
//@ Call should be made holding the ptable lock
void makeRunnable( struct proc *p){

  //cprintf("makeRunnable: %d \n",p->pid);
  if(ptable.head == 0){
  //@ First process to our readyQueue
    ptable.head = p;
    ptable.tail = p;
  }
  else{
    ptable.tail->nextReady = p;
    ptable.tail = ptable.tail->nextReady;
  }
  //cprintf("Done makeRunnable: %d %d \n",ptable.head->pid,ptable.tail->pid);
}

//@ Removes from rQeueu
//@ Call should be made holding ptable lock
void remRunnable(struct proc *p){
	//@ We will not be updating state, as new state could be anything.
	//@ Leave it for the original code to make that change
  
  //cprintf("removeRunnablei: %d \n",p->pid);
  //Search for the process first
  struct proc *tmp, *prev;
  tmp = ptable.head;
  prev = 0;
  while(tmp != p && tmp !=0){
    //cprintf("In remRunnable Loop: %d \n",tmp->pid);
    prev = tmp;
    tmp = tmp->nextReady;
  }

  if(tmp == 0){
    if(ptable.head!=0){
      //cprintf("Process Not found on ReadyQueue: %d \n", p->pid);
      panic("Process not found on readyQueue");
     } else {
      //cprintf("ptable head 0 but remmovable request\n");
     }
  } else {
      if(prev == 0){
        //@Head is being removed
        ptable.head = tmp->nextReady;
        tmp->nextReady = 0;
        if (tmp == ptable.tail){
          //@head and tail are same, so update tail
          ptable.tail = 0;
        }
      } else{
        prev->nextReady = tmp->nextReady;
        tmp->nextReady = 0;
      }
  }
	//@ Remove it from rQueue
  //cprintf("Done %d %d \n",ptable.head->pid, ptable.tail->pid);
}

//@ Change state of a process
//@ A common place to any changes in the state of a process, so that I easily call any of the above
//@ two functions at this function instead of multiple places spread out in this file
//@ Call should be made holding ptable lock
void changeState(struct proc *p, enum procstate newState){

  //cprintf("changeState: %d: %d -> %d \n",p->pid,p->state,newState);
	if(p->state == newState){
		//@ Atleast this ould prevent having cycles in my logic
    panic("Old and New State of a process are same");	//@ Not Sure if this is an issue
	} else {
		if(newState == RUNNABLE){
			makeRunnable(p);
		} else if(p->state == RUNNABLE){
				remRunnable(p);
			}
	}

	p->state = newState;

}

void
markKilled(struct proc *p){
  
  p->killed = 1;
  //kill process Code

  if(p->state == SLEEPING)
    changeState(p,RUNNABLE);

}

//Kills the pid and all its children
void
killTree(struct proc *curProc){

 struct proc *p;
 for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
   if(p->parent != curProc)
     continue;
   else{
     killTree(p);
   }
 }
 markKilled(curProc);
}

void
makeKillable(){
  acquire(&ptable.lock);
  acquire(&killProc.lock);
    killTree(killProc.curProc);
  release(&killProc.lock);
  release(&ptable.lock);
}

int nextpid = 1;
//@ Creating a static proc struct for the initproc which is the first process to load while booting
//@ By default init proc has pid 0
static struct proc *initproc;

//@ extern here is used compiler to ignore the declaration check for this functions because its defined later. Though I don't see the reason behind doing this?? 
extern void forkret(void);
extern void trapret(void);

//@John said xv6 not having wakeup1.. just look into it
static void wakeup1(void *chan);

//@ maybe a special lock for init process, which shouldn't exit and probably doesn't unlock??
//@ Nope, it's just initiating the spinlock
//@ This is called from the bootstrapper code as mentioned in main.c file
void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  //@ Initialise our rQueue,tail as well
  //rQueue = 0; Avoiding this, so that multiple checks for checking if there is a ready process, is avoided
	//Remember, Kalloc() assigned a kernel stack page of 4KB and that shouldn't be called for small memory calls. Using malloc instead
  //Not Sure, if the default values for a pointer is 0 
  ptable.head = 0;
  ptable.tail = 0;
  //cprintf("pinit hi");
  killProc.curProc = 0;
  killProc.killFlag = 0;
  //@ Updating a proc's nextReady will be done in fork instead and exit too.
}

// Must be called with interrupts disabled
//@ cpus comes from proc.h
//@ This will return cpudaddress which is *int - *int of start address i.e &cpus[0] which would give the index of this cpu
int
cpuid() {
  return mycpu()-cpus;
}

/*
void
obekilled()
*/

// Must be called with interrupts disabled to avoid the caller being rescheduled
// between reading lapicid and running through the loop.
//@ Since, a process could be taken up by any free cpu after being re-scheduled
struct cpu*
mycpu(void)
{
  int apicid, i;
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();	//@ Look at what this is doing??
	//@ This is just returning the local advanced programmable interrupt controller's ID. This will be 0 for a 
	//@ unicore process
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();	//@ This probably you will get to know after reading locking chapter
	//@ This basically just disables interrupts on this core/cedu
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//@ Find out what exactly this PAGEBREAK comment mean?? If adding comments to these pages affect anything
//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;	//@address pointer used for setting all the address pointers within *proc

  acquire(&ptable.lock);

  //@ you can implement empty queue to speed up this loop!
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  //@ If unused slot not found
  release(&ptable.lock);
  return 0;

found:
  //p->state = EMBRYO;	//@ Once in EMBRYO state, no other process touches that particular process, until the state changes
						//@ This is basically to avoid anyone using this process until its properly initialised
  changeState(p, EMBRYO);
	p->pid = nextpid++;	//@ initialised to 1 after creating init process

  release(&ptable.lock);

  //@ Check kalloc() method to see how much memory is allocated or how is the memory mapped??
  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    //p->state = UNUSED;
    changeState(p, UNUSED);
		return 0;
  }
  sp = p->kstack + KSTACKSIZE;	//@ here is the answer for first part of the above question

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;	//@ Trapframe as part of Kernel Stack

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;	//@ If the scheduler schedules this function it will return to this address
  p->nextReady = 0; //@Updating while allocation, need not repeat that at exit
  return p;
}

//@Continued...
//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;	//@ Interrupts are enabled
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  //p->state = RUNNABLE;
	changeState(p, RUNNABLE);	

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
	//@ np is still in EMBRYO state

  // Copy process state from proc.
	//@ This is the cloning process which makes sure, parent an child process have same stack
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    //np->state = UNUSED;
		changeState(np, UNUSED);
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);	//@ We are not directly copying the contents, because iget() in this indirect call, needs to increment the reference counter
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  //np->state = RUNNABLE;
	changeState(np, RUNNABLE);

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
//@ This does not free the resource, wait() does
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  //curproc->state = ZOMBIE;
	changeState(curproc, ZOMBIE);
  sched();
  panic("zombie exit");	//@ Since we have already woken up parent process, if sched runs, then it will 
						//@ clean up all the resources and won't print this panic line unless parent had already exited
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
//@ One tricky point is that, this call waits for only child to exit and not all.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        //p->state = UNUSED;
				changeState(p, UNUSED);
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
  
/**OLDCODE
	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
**/

		while(ptable.head != 0){
			
      p = ptable.head;
      //@ Assignment 5 check if a process needs to be killed;
      acquire(&killProc.lock);
      killProc.curProc = p;
      release(&killProc.lock);
      //cprintf("scheduler while loop %d %d\n", ptable.head->pid, ptable.tail->pid);
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      //p->state = RUNNING;
			changeState(p,RUNNING); //@ ptable.head will be updated here
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)	//@ So, a process shouldn't be in a running state when scheduler is called
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");	//@ Not sure here, because exit() calls sched but never makes it uninterruptible, then it should cause this panic right??
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);//@ This is the context switch betwn scheduler and selected process
  mycpu()->intena = intena;
}

//@ It just gives up it slice while remaining in RUNNABLE state.
//@ Normally usage of yield is because of improper logic in the code
// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  //myproc()->state = RUNNABLE;
  changeState(myproc(),RUNNABLE);
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  //@ IF we lock a lock in xv6 we will have a single process deadlock
  //@ No problem if its ptable lock, because it will be released on he other side
  //@ But if its some other lock, then release it before going to sleep

  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  //p->state = SLEEPING;
	changeState(p, SLEEPING);
  sched();	//@ I am not sure, as to when does a call to sched actually returns for the remainder part of this function??

  // Tidy up.
  p->chan = 0;

  //@ Why should this be in this order??
  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      	changeState(p,RUNNABLE);
				//p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
	  //@ Even if the condition is not met, we are waking up but its ok, since we always check the kill
	  //@ flag before doing some process or scheduling it
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        changeState(p,RUNNABLE);
				//p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
//@ This is the function that gets called when we press ctrl + p in xv6 console

void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
	[RUNNABLE]  "runble",
	[RUNNING]   "run   ",
	[ZOMBIE]    "zombie"
	 };
	
  int i;
	struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
     }
     cprintf("\n");
  }
}

