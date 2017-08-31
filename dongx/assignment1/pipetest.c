#include "types.h"
#include "stat.h"
#include "user.h"

#define N (1024*1024)

//Borrowed from usertests.c
unsigned long randstate = 1;
unsigned int
rand()
{
  randstate = randstate * 1664525 + 1013904223;
  return randstate;
}

//Write 1MB from one process and read from another.
void
pipe1(void) {
    int pipe_fds[2];
    int pid;
    char* bufin = malloc(N);
    char* bufout = malloc(N);

    int i;
    for (i = 0; i < N; ++i) {
        bufin[i] = (char)rand();
    }

    if (pipe(pipe_fds) != 0) {
        printf(1, "Failed to create pipe.\n");
        exit();
    }

    
    pid = fork();
    if (pid == -1) {
        //Fork failure;
        printf(1, "Fork is failed!!\n");
    } else if (pid == 0) {
        //Child process: writer
        close(pipe_fds[0]);
        if (write(pipe_fds[1], bufin, N) != N) {
            printf(1, "Failed to write %d bytes pipe\n", N);
        }
        exit();
    } else {
        //Parent process: reader
        close(pipe_fds[1]);
        int len, bufout_cnt;
        char buf[2048];
        bufout_cnt = 0;
        while ((len = read(pipe_fds[0], buf, N)) > 0)
            for (i = 0; i < len; ++i) 
                bufout[bufout_cnt++] = buf[i];

        if (bufout_cnt != N) {
            printf(1, "Bytes transfered does not match\n");
            exit();
        }
        
        for (i = 0; i < N; ++i)
            if (bufin[i] != bufout[i]) {
                printf(1, "A byte in the seqenece through pipe does not match\n");
                exit();
            }
            
        close(pipe_fds[0]);
        wait();
    }
    printf(1, "Test pipe1 passed....\n");
}


//Short read test.
void
pipe2(void) {
    int pipe_fds[2];
    int pid;
    char bufin[100];

    int i;
    for (i = 0; i < 100; ++i) {
        bufin[i] = (char)rand();
    }

    if (pipe(pipe_fds) != 0) {
        printf(1, "Failed to create pipe.\n");
        exit();
    }

    
    pid = fork();
    if (pid == -1) {
        //Fork failure;
        printf(1, "Fork is failed!!\n");
    } else if (pid == 0) {
        //Child process: writer
        close(pipe_fds[0]);
        if (write(pipe_fds[1], bufin, 100) != 100) {
            printf(1, "Failed to write %d bytes pipe\n", N);
        }
        exit();
    } else {
        //Parent process: reader
        close(pipe_fds[1]);
        char buf[512];

        if (read(pipe_fds[0], buf, 512) != 100) {
            printf(1, "Bytes transfered does not match\n");
            exit();
        }
        
        for (i = 0; i < 100; ++i)
            if (bufin[i] != buf[i]) {
                printf(1, "A byte in the seqenece through pipe does not match\n");
                exit();
            }
            
        close(pipe_fds[0]);
        wait();
    }
    printf(1, "Test pipe2 passed....\n");
}

//Read only a part of the pipe buffer.
void
pipe3(void) {
    int pipe_fds[2];
    int pid;
    char bufin[100];

    int i;
    for (i = 0; i < 100; ++i) {
        bufin[i] = (char)rand();
    }

    if (pipe(pipe_fds) != 0) {
        printf(1, "Failed to create pipe.\n");
        exit();
    }

    
    pid = fork();
    if (pid == -1) {
        //Fork failure;
        printf(1, "Fork is failed!!\n");
    } else if (pid == 0) {
        //Child process: writer
        close(pipe_fds[0]);
        if (write(pipe_fds[1], bufin, 100) != 100) {
            printf(1, "Failed to write %d bytes pipe\n", N);
        }
        exit();
    } else {
        //Parent process: reader
        close(pipe_fds[1]);
        char buf[50];

        if (read(pipe_fds[0], buf, 50) != 50) {
            printf(1, "Bytes transfered does not match\n");
            exit();
        }
        
        for (i = 0; i < 50; ++i)
            if (bufin[i] != buf[i]) {
                printf(1, "A byte in the seqenece through pipe does not match\n");
                exit();
            }
            
        close(pipe_fds[0]);
        wait();
    }
    printf(1, "Test pipe3 passed....\n");
}

//Single thread pipe read & write
void
pipe4(void) {
    int pipe_fds[2];
    char bufin[100];

    int i;
    for (i = 0; i < 100; ++i) {
        bufin[i] = (char)rand();
    }

    if (pipe(pipe_fds) != 0) {
        printf(1, "Failed to create pipe.\n");
        exit();
    }
    
    if (write(pipe_fds[1], bufin, 100) != 100) {
        printf(1, "Failed to write %d bytes pipe\n", N);
    }
    close(pipe_fds[1]);
    char buf[100];
    if (read(pipe_fds[0], buf, 200) != 100) {
        printf(1, "Bytes transfered does not match\n");
        exit();
    }
    
    for (i = 0; i < 100; ++i)
        if (bufin[i] != buf[i]) {
            printf(1, "A byte in the seqenece through pipe does not match\n");
            exit();
        }
    close(pipe_fds[0]);

    printf(1, "Test pipe4 passed....\n");
}

int
main(void) {
    pipe1();
    pipe2();
    pipe3();
    pipe4();
    exit();
}
