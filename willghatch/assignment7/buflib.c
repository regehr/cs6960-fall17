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
  head_offset = &(((int *)buf_addr)[0]);
  tail_offset = &(((int *)buf_addr)[1]);
  *head_offset = 0;
  *tail_offset = 0;
  //printf(1, "buf_setup with addr: %d\n", buf_addr);
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
  head_offset = &(((int *)buf_addr)[0]);
  tail_offset = &(((int *)buf_addr)[1]);
  char *data_begin = buf_addr + PGSIZE;
  int data_wrap_size = PGSIZE*NUMBER_SHARED_PAGES;

  //printf(1, "head offset: %d\n", *head_offset);
  //printf(1, "tail offset: %d\n", *tail_offset);
  // no message case
  if (*head_offset == *tail_offset) return -1;

  //printf(1, "head offset: %d\n", *head_offset);
  //printf(1, "tail offset: %d\n", *tail_offset);
  //int dataread = *((int *)data_begin);
  //printf(1, "here in buf_get with dataread: %d\n", dataread);
  int msgsize = *((int *)(data_begin + *head_offset));
  char *msgstart;
  msgstart = data_begin + (*head_offset) + sizeof(int);
  // return the message length whether the get is successful or not
  *len = msgsize;
  if(msgsize > maxlen) return -2;

  memmove(data, msgstart, msgsize);
  int newoffset = *head_offset + msgsize + sizeof(int);
  // wrap the offset to be below wrap size
  if(newoffset > data_wrap_size){
    newoffset -= data_wrap_size;
  }
  *head_offset = newoffset;

  //TODO - atomic loads and stores
  // TODO - return -2 if there is not enough space in the provided buffer
  return 0;
}

int buf_put(char *data, int len){
  if(!buf_addr) return -1;
  int *head_offset, *tail_offset;
  head_offset = &(((int *)buf_addr)[0]);
  tail_offset = &(((int *)buf_addr)[1]);
  char *data_begin = buf_addr + PGSIZE;
  int data_wrap_size = PGSIZE*NUMBER_SHARED_PAGES;
  //printf(1, "buf_put -- head %d tail %d\n", *head_offset, *tail_offset);

  // not enough room.
  if((*head_offset) + data_wrap_size - (*tail_offset) < len+sizeof(int)){
    return -1;
  }

  *((int *)(data_begin + *tail_offset)) = len;
  memmove(data_begin + *tail_offset + sizeof(int), data, len);
  int newoffset = *tail_offset + len + sizeof(int);
  // wrap the offset to be below wrap size
  if(newoffset > data_wrap_size){
    newoffset -= data_wrap_size;
  }
  *tail_offset = newoffset;
  //TODO - atomic loads and stores
  // TODO - return -1 if there is not enough room
  //printf(1, "buf_put -- head %d tail %d\n", *head_offset, *tail_offset);
  return 0;
}
