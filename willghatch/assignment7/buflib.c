#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "mmu.h"
#include "memlayout.h"

char *buf_addr = 0;

int buf_setup(){
  if(buf_addr) return -1;
  buf_addr = buf_setup_syscall();
  // I have this panicking in the kernel if this fails, so this shouldn't happen.
  if(!buf_addr) return -1;
  int *head_offset, *tail_offset;
  head_offset = (int *)buf_addr;
  tail_offset = (int *)buf_addr + sizeof(int *);
  *head_offset = 0;
  *tail_offset = 0;
  return 0;
}

// TODO - thread-safety -- I think I really need a read lock and a write lock
//        if there is any chance of two readers or two writers.

/*
  Record format:
  int n
  char[n] bytes
  */

int buf_get(char *data, int *len, int maxlen){
  if(!buf_addr) return -1;
  int *head_offset, *tail_offset;
  head_offset = (int *)buf_addr;
  tail_offset = (int *)buf_addr + sizeof(int *);
  char *data_begin = buf_addr + PGSIZE;
  int data_wrap_size = PGSIZE*NUMBER_SHARED_PAGES;

  // no message case
  if (*head_offset == *tail_offset) return -1;

  int msgsize = *((int *)data_begin + *head_offset);
  char *msgstart;
  msgstart = data_begin + (*head_offset) + sizeof(int);
  // return the message length whether the get is successful or not
  *len = msgsize;
  if(msgsize > maxlen) return -2;

  memmove(data, msgstart, msgsize);
  *head_offset += msgsize + sizeof(int);
  // wrap the offset to be below wrap size
  if(*head_offset > data_wrap_size){
    *head_offset -= data_wrap_size;
  }

  //TODO - atomic loads and stores
  // TODO - return -2 if there is not enough space in the provided buffer
  return 0;
}

int buf_put(char *data, int len){
  if(!buf_addr) return -1;
  int *head_offset, *tail_offset;
  head_offset = (int *)buf_addr;
  tail_offset = (int *)buf_addr + sizeof(int *);
  char *data_begin = buf_addr + PGSIZE;
  int data_wrap_size = PGSIZE*NUMBER_SHARED_PAGES;

  // not enough room.
  if((*head_offset) + data_wrap_size - (*tail_offset) < len+sizeof(int)){
    return -1;
  }

  *((int *)(data_begin + *tail_offset)) = len;
  memmove(data_begin + *tail_offset + sizeof(int), data, len);
  *tail_offset += len + sizeof(int);
  // wrap the offset to be below wrap size
  if(*tail_offset > data_wrap_size){
    *tail_offset -= data_wrap_size;
  }
  //TODO - atomic loads and stores
  // TODO - return -1 if there is not enough room
  return 0;
}
