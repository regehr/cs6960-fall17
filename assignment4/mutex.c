#include "types.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

#define NO_OF_MUTEXES 64

struct mutex{
	int								acquired_pid;
	struct spinlock		lock;
};

// alloted_pid array holds the pid of the process that created the corresponding mutex. If not alloted, a 0 is stored.

struct {
	struct mutex		table[NO_OF_MUTEXES];
	int							alloted_pid[NO_OF_MUTEXES];
	struct spinlock lock;
} mutex_table;

#define is_mutex_acquired(mtx) ( (mtx).acquired_pid != 0 ) 
#define is_mutex_alloted(mtx_id) ( (mutex_table.alloted_pid[mtx_id] != 0) )
#define is_mutex_valid(mtx_id) ( (mtx_id >= 0) && (mtx_id < NO_OF_MUTEXES) )

struct mutex* get_mutex(int mtx_id){
	if( !is_mutex_valid(mtx_id) ){
		return 0;
	}
	struct mutex* mtx;
	mtx = &mutex_table.table[mtx_id];

	return mtx;
}

void mutex_init(void){
	
	initlock(&(mutex_table.lock), "mutex_table");
	
	struct mutex* mtx;
	for(int i=0; i<NO_OF_MUTEXES; i++){
		mtx = get_mutex(i);
		mtx->acquired_pid = 0;
		initlock(&(mtx->lock), "mutex");

		mutex_table.alloted_pid[i] = 0;
	}

}

int mutex_create(void){

	acquire(&(mutex_table.lock));
	for(int i=0; i<NO_OF_MUTEXES; i++){
		if( !is_mutex_alloted(i) ){
			mutex_table.alloted_pid[i] = myproc()->pid;
			release(&(mutex_table.lock));
			return 0;
		}
	}

	release(&(mutex_table.lock));
	return -1;
}

int mutex_destroy(int mtx_id){

	if( !is_mutex_valid(mtx_id) ||
			(mutex_table.alloted_pid[mtx_id] != myproc()->pid)){
		return -1;
	}

	struct mutex* mtx = get_mutex(mtx_id);
	acquire(&mtx->lock);

	acquire(&(mutex_table.lock));
	
	mutex_table.alloted_pid[mtx_id] = 0;
	mtx->acquired_pid = 0;
	release(&mtx->lock);

	release(&(mutex_table.lock));

	return 0;
}

int mutex_acquire(int mtx_id){

	if( !is_mutex_valid(mtx_id) ||
				(mutex_table.table[mtx_id].acquired_pid == myproc()->pid)){
		return -1;
	}

	struct mutex* mtx = get_mutex(mtx_id);
	acquire(&mtx->lock);
	
	while( is_mutex_acquired(*mtx) ){
		sleep(mtx, &(mtx->lock));							// sleep releases the lock and puts process in waiting until mtx_release is called
	}

	mtx->acquired_pid = myproc()->pid;
	release(&mtx->lock);
	return 0;
}

int mutex_release(int mtx_id){
	
	if( !is_mutex_valid(mtx_id) ||
				(mutex_table.table[mtx_id].acquired_pid != myproc()->pid)){
		return -1;
	}
	
	struct mutex* mtx = &(mutex_table.table[mtx_id]);

	acquire(&mtx->lock);
	mtx->acquired_pid = 0;
	wakeup(mtx);
	release(&mtx->lock);

	return 0;
}

