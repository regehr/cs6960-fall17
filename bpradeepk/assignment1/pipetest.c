#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"
#include "fcntl.h"

// #define MB 1024*1024// It takes too long with each iteration of size utmost 8KB
#define MB 1024*2 // For saving time
#define STSZ 1024*1		//Max stack size seems to be 4KB, and reader requires read data as well as original data.
						//So the above value is set little less than half of max stack size

static char data[STSZ];//data is the random data generated
//Max size could 256*1024 to generate max of MB data
//Not allowing non-multiples of 4 as it could mess up the integrity check
//This random generator facilitates the integrity check

unsigned int rand_int32(unsigned int num){
	num ^=num<<11;
	num ^=num>>7;
	num ^=num<<13;
	return num;
}


//x is to differentiate the sizes between readers and writers
unsigned int getSize(unsigned int x){

	static unsigned int sfeed=0;//for generating size of data
	if(sfeed==0)sfeed=x;	
	//generate a new size
	sfeed=rand_int32(sfeed);
	return ((sfeed<<24)>>24);//to make sure the stack size doesn't exceed approx 4KB
}

void getData(int size){
	static unsigned int feed=1;//for generating data
	unsigned int tFeed=feed;//temp variable for storing feed in data array
	int j=0;
	if(size==0){
		printf(1,"Stopping due to size\n");
		exit();
	}
	for(j=0;j<size;j++){

			//mess up the feed value
			feed=rand_int32(feed);
			tFeed=feed;
			//Feed the unsigned int into char array
			
			for(int i=0;i<4;i++){
				data[4*j+i]=tFeed;
				tFeed=tFeed>>8;
			}
	}
	return;
}

void singleTest(void){

int child;
int fd[2];
pipe(fd);
unsigned int size;
child=fork();

if(child<0){

	printf(1,"Fork error\n");
	exit();
} else if(child ==0){

	char rtext[STSZ];
	//for reading
	close(fd[1]);
	
	int rem=MB/4;
	
	//Loop for reading just above 1MB data
	//same randomizer makes sure the size between reader and writer is same
	while(rem>0){
			//Seems like there is some stack size constraint which is preventing me from allocating more than 8KB in a process
			size=getSize(1);//anything more than 14 would get data of less than 8 KB
			rem-=size;
			//Ensuring just 1MB of data is written to maintain consistency between reader and writer
			if(rem<0){
				//just write the remaining data
				size+=rem;//bcz rem is -ve
			}
			
			int done=0;
			
			//printf(1,"About to Read: %d %d\n", size, rem);
			//should fix the short read issue
			while(done!=4*size){
				done+=read(fd[0],rtext+done,4*size-done);
			}
			
			//Now verify data
			getData(size);//random generator ensures the same data
			for(int i=0; i<size*4; i++){
				if(data[i]!=rtext[i]){
					printf(1,"Integrity check Failed: %d %d %d\n",i, data[i], rtext[i]);
					exit();
				} 
			}
	}

	close(fd[0]);
	exit();
} else if(child >0){

	close(fd[0]);//for writing
	
	//Now need a loop for writing 1 MB of data
	int rem=MB/4;
	while(rem >0){
			//Seems like there is some stack size constraint which is preventing me from allocating more than 4KB in a process
			size=getSize(2);
			rem-=size;
			//Ensuring just 1MB of data is written to maintain consistency between reader and writer
			if(rem<0){
				//just write the remaining data
				size+=rem;//bcz rem is -ve
			}
			getData(size);//generate data of the mentioned size

			write(fd[1],data,4*size);//No Short writes, so wont return until finished
	}
	
	close(fd[1]);
	wait();
	printf(1, " Single Reader/Writer Successfully Tested\n");
	return;

	}

}

//This function is to independently calculate the reduce written size and is randomly chosen
unsigned int deduct(unsigned int x){ 		

	x^=rand_int32(x);
	x^=rand_int32(x);
	x^=rand_int32(x);
	
	//Now reduce the number to less than 8KB for now
	x=((x<<24)>>24);//would max out at 2048 and then its 4 times would be less than 8KB
	return x;
}
	
void multipleTest(void){

	//First create resources
	unsigned int size=0;
	int fd[2];
	pipe(fd);
	int pid;
	unsigned int writerFeed[2]={4,5};

	pid=fork();

	if(pid<0){
		
		printf(1, "Error Forking\n");
		exit();

	} else if (pid==0){		//Readers
		
		close(fd[1]);
		
		//Readers resources
		int initfeed[2]={2,3};
		char rtext[STSZ];
		int reader;
		int done=1,rem=1, diff=1;	//to handle short reads

		//Creating Readers
		reader=fork();
		
		if(reader<0){

			printf(1,"Error Forking Reader\n");
			exit();
		} else if (reader==0){ 		//Reader 1 -- child
		
			rem=MB/4;
			while(rem>0 && diff!=0){
				size=getSize(initfeed[0]);
				rem-=size;
				if(rem<0){
					size+=rem;
					rem=0;
				}
				done=0;
				while(done!=4*size){
					
					diff=read(fd[0], rtext+done,4*size-done);
					done+=diff;
					if(diff==0){	//Most certainly write fd is closed
						break;
					}
				}
			}

			//We are not going to check the content this time, as its a tedious task due to scheduling
			//Instead we check if the count still remains consistent.

			//Now open a file and write the following content to it.
			int leftReader1=4*(rem+size)-done;//bcz done might not be multiple of 4
			int fl=open("res.txt",O_CREATE|O_RDWR);
			if(fl<0){
				printf(1, "Error creating file\n");
			} else{
				if(write(fl,&leftReader1,sizeof(int))!=sizeof(int))printf(1, "Error writing to file\n");
				else{
					close(fl);
					}	
			}
				
			exit();
				
		}else if (reader>0){		//Reader 2 -- Parent
			
			rem=MB/4;
			while(rem>0 && diff!=0){
				size=getSize(initfeed[0]);
				rem-=size;
				if(rem<0){
					size+=rem;
					rem=0;
				}
				done=0;
				while(done!=4*size){
					
					diff=read(fd[0], rtext+done,4*size-done);
					done+=diff;
					if(diff==0){	//Most certainly write fd is closed
						break;
					}
				}
			}

			//We are not going to check the content this time, as its a tedious task due to scheduling
			//Instead we check if the count still remains consistent.

			int leftReader2=4*(rem+size)-(done);
			
			//wait for Reader1 to exit then read the content of the file it created
			wait();

			int leftReader1;
			
			int fl=open("res.txt",O_RDONLY);
			if(fl<0){
				printf(1,"Error opening file in Parent Reader\n");
			} else {
				if(read(fl,&leftReader1,sizeof(int))!=sizeof(int))printf(1,"Error reading from the file\n");
				else{
					close(fl);
				}
			}
			//Now for the final count check
			int res=4*(deduct(writerFeed[0])+deduct(writerFeed[1])); 
			if(res==leftReader1+leftReader2){
				printf(1," Multiple Reader/Writer test successful:  %d Bytes \n", leftReader1+leftReader2);
			}else{
				printf(1, " Multiple Reader/Writer test failed\n");
			}
			
			exit();
		
		}

		exit();

	} else if (pid >0){		//Writers
		
		//Writers resources
		
		close(fd[0]);//for writing
			
		//Now need a loop for writing 1 MB of data
		int rem=MB/4;
		int writer=0;

		writer=fork();
		
		if(writer<0){
			printf(1,"Error Forking Writer\n");
			exit();
		} else if (writer==0){

			//first calculate the reduced size
			rem-=(deduct(writerFeed[0]));
			while(rem >0){
				//Seems like there is some stack size constraint which is preventing me from allocating more than 8KB in a process
				size=getSize(writerFeed[0]);//anything more than 14 would get data of less than 8 KB
				rem-=size;
				//Ensuring just 1MB of data is written to maintain consistency between reader and writer
				if(rem<0){
					//just write the remaining data
					size+=rem;//bcz rem is -ve
				}
				getData(size);//generate data of the mentioned size
			
				write(fd[1],data,4*size);//No Short writes, so wont return until finished
			}
			exit();

		} else if (writer >0){

			//first calculate the reduced size
			rem-=(deduct(writerFeed[1]));
			
			while(rem >0){
				//Seems like there is some stack size constraint which is preventing me from allocating more than 4KB in a process
				size=getSize(writerFeed[1]);
				rem-=size;
				//Ensuring just 1MB of data is written to maintain consistency between reader and writer
				if(rem<0){
					//just write the remaining data
					size+=rem;//bcz rem is -ve
				}
				getData(size);//generate data of the mentioned size
			
				write(fd[1],data,4*size);//No Short writes, so wont return until finished
			}
			
			int child=wait();
			while(child!=writer && child!=-1){
				child=wait();//atleast one of the readers will still be waiting for data, since they are trying to read 2MB of data which is not being written
			}
			close(fd[1]);	//Now the waiting reader(s) could exit with 0 bytes being read;
			child=wait();
			while(child!=-1)child=wait();	//wait for every child to exit

			exit();
		}
	}

}

int main(void){

	//Main function
	singleTest();
	multipleTest();
	return 1;

}
