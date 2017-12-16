// Mutual exclusion locks

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "proc.h"
#include "mylock.h"
#include "spinlock.h" //@ For acquiring ptable lock

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

struct mylock mutex[2]; //@ can maximum create two mutexes for now
int used;       //@ If mutex[0] is being used, then 1, else 2 
                //@ If both are being used then 3, and if none then 0
struct spinlock access; //@ This lock is to ensure other's 

//To-Do
//Make an init and intialise the variables
//Handle and modify the functions such that syscalls just send the int
//and these functions take care of the rest.

void
initmylock(){

  //@ Initiallise the mutexes
  for(int i=0;i<NELEMS(mutex);i++){
    mutex[i].unlocked = 0;
    mutex[i].pid = -1;
  }

  //Spinlock initiliastion
  initlock(&access,"mylock");
  //@ Initialise used var
  used =0;
}


//@ This same function will be called by mutex_create as well as mutex_destroy
//@ This basically re-initialises the lock, but the syscall should make sure
//@ that this lock was really not in use, or else it creates havoc
int
crtlock(int x)
{
  int lk;
  //@ Acquire the spinlock
  acquire(&access);
 
  if(x!=-1){
    //create_mutex called to acquire a specific lock
    //So, x should be 1 or 2
    if(!used & x){
      lk = 0;
    } else {
      release(&access);
      return -1;
    }
  
  } else {
    //check if any unused mutex is available
    if(!(used & 1)){
      //@ 1 is free
      lk = 0;
    } else if(!(used & 2)){
      //@ 2 is free
      lk = 1;
    } else {
      release(&access);
      return -1;
    }
  }

  mutex[lk].locked = 0;
  mutex[lk].pid = -1;

  used |= lk;
  release(&access);

  //@ return the index of the mutex, for the process to identify
  return lk;
}

// Acquire the lock.
// Loops (spins while sleeping for longer intervals) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
myacquire(int index)
{
  //@ Since its a user message, make sure the index is less than available index
  if(index >= NELEMS(mutex)){
    return -1;
  }
  
  struct mylock lk = mutex[index];
  
  //@ This is where we try to avoid acquiring locks already acquired
  if(lk.locked && lk.pid == myproc()->pid)
    panic("Acquiring already acquired lock");

  //@ While you don't get the lock, acquire ptable lock and go to sleep
  // The xchg is atomic.
  while(xchg(&lk.locked, 1) != 0){
    acquire(&ptable.lock);
    sleep(&lk,&ptable.lock);
  }


  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk.pid = myproc()->pid;
}

// Release the lock.
void
myrelease(int index)
{
  
  //@ Since its a user message, make sure the index is less than available index
  if(index >= NELEMS(mutex)){
    return -1;
  }

  struct mylock lk = mutex[index];

  //@ Check if the process hold's the lock
  if(!(lk.locked && lk.pid == myproc()->pid))
    panic("Releasing an unheld mylock");

  
  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (lk.locked) : );

  lk.pid = -1;
  
  //@ Now time to wakeup sleeping beauties.. ;)
  //@ First acquire ptable lock
  acquire(&ptable.lock);
  wakeup1(&lk);
  //@ Now release ptable lock
  release(&ptable.lock);

  return 0;
}

int
mydestroy(int index){

 
  //@ Since its a user message, make sure the index is less than available index
  if(index >= NELEMS(mutex)){
    return -1;
  }
  
  struct mylock lk = mutex[index];
  //@ Check if its unlocked
  
  if(!(lk.locked == 0 && lk.pid == -1)){
    return -1;
  } else {
  
    //acquire the spinlock
    acquire(&access);
    used |= index;
    release(&access);     
  }

  return 0;
}

//Syscall for creating  lock
int sys_create_mutex(void) {

  int index;

  //check if args are correct
  if (argint(0, &index) < 0)
         return -1;

 return crtlock(index); 

}
//Syscall for acquiring lock
int sys_acquire_mutex(void) {

  int index;

  //check if args are correct
  if (argint(0, &index) < 0)
         return -1;

 return  myacquire(index); 

}
//Syscall for releasing lock
int sys_release_mutex(void) {

  int index;

  //check if args are correct
  if (argint(0, &index) < 0)
         return -1;

  return myrelease(index); 

}
//Syscall for deleting lock
int sys_delete_mutex(void) {

  int index;

  //check if args are correct
  if (argint(0, &index) < 0)
         return -1;

  return mydestroy(index); 

}
