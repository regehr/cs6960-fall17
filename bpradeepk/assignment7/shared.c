#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"

int sys_shared_mem() {
  //Just like Assignment 4, its just a system call, which actually calls the original function  
  return mp_shared(myproc()->pgdir);
}
