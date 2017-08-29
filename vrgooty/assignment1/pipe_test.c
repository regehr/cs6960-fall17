/*****
 * Attribution:
 * The design was simplified after looking at other submissions.
 * Initially I thought of using files instead of buf_write and buf_read.
 * Later after looking at others submissions I used buffers which
 * simplified my code drastically.
*****/

#include "types.h"
#include "user.h"
#include "stat.h"

#define SZ 1<<20

/***** 
 * A poor man's implementation of a random number generator.
 * A random number in the range [0 25] is returned
*****/
int rand_int(){
	static int a = 1234;
	static int b = 5678;
	int random;
	random = (a*b)%26;
	a = (a*random)%1234;
	b = (b*random)%5678;
	return random;
}

int main(){
	char* buf_write = malloc(SZ);
	char* buf_read = malloc(SZ);
	int pid;
	int p[2];

	//printf(1, "Size is %d\n", SZ);
	for(int i=0; i<SZ; i++){
		buf_write[i] = 'A' + rand_int();
	}
	
	if(pipe(p) != 0){
		printf(1, "Error while creating pipe\n");
		free(buf_write);
		free(buf_read);
		exit();
	}

	pid = fork();
	if(pid == 0){
		// Child process writes to the pipe.
		close(p[0]);
		if(write(p[1], buf_write, SZ) != SZ){
			printf(1, "Error while writing to the pipe\n");
			exit();
		}
		close(p[1]);
	}
	else if(pid > 0){
		// Parent process reads from the pipe.
		close(p[1]);

		int index = 0;
		int n;
		char buf[500];
		while((n = read(p[0], buf, 500))>0){
			for(int i=0; i<n; i++)
				buf_read[index++] = buf[i];
		}
	
		for(int i=0; i<SZ; i++){
			if(buf_read[i] != buf_write[i]){
				printf(1, "Failed at byte no:%d", i);
				exit();
			}
		}
		close(p[0]);
		printf(1, "Pipe test successful\n");
		wait();
	}
	exit();
}
