#include "types.h"
#include "user.h"

#define FAILED 1

#define STDOUT 1
#define STDERR 2

/* Xorshift 'cause all the cool people use that nowadays,
 * and we're not doing crypto. */
static uint random() {
    static uint state = 0x1337;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

/* A second RNG with a different seed */
static uint randomC() {
    static uint state = 0xBEEF;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

int pipeTest(char *stream);

int main(int argc, char **argv) {
    /* Initialize random stream */
    char stream[1024];

    for (int i = 0; i < 256; i++) {
        uint *intOffset = (uint*) (&stream[i*4]);
        *intOffset = random();
    }

    // TODO: Make snazzy testing framework with more tests. 

    printf(STDOUT, pipeTest(stream) ? "Tests have failed\n" : "All tests have passed\n");
    exit();
}

/* Tests random sized reads and writes */
int pipeTest(char *stream) {
    int fds[2];
    int pid;

    /* Error checking */
    if (pipe(fds) != 0) {
        printf(STDERR, "Failed to create pipe.\n");
        exit();
    }

    /* Parent */
    if ((pid = fork())) {
        int bytesLeft = 1024;
        char buf[1024];

        int randRead;
        while (!(randRead = (random() & 31))); // Avoid reading 0 bytes
        while (bytesLeft > 0) {
            int bytesRead = read(fds[0], &buf[1024-bytesLeft], randRead > bytesLeft ? bytesLeft : randRead);  
            if (bytesRead == -1) { break; }

            bytesLeft -= bytesRead;
        }

        close(fds[0]);
        wait();

        /* Test: Correct number of bytes transfered */
        if (bytesLeft > 0) {
                printf(STDOUT, "Failed: The numbers of bytes read did not match\n");
                printf(STDOUT, "bytesLeft: %d\n", bytesLeft);
                return FAILED;
        }

        /* Test: Data was written correctly and read out in order */
        for (int i = 0; i < 1024; i++) {
            if (buf[i] != stream[i]) {
                printf(STDOUT, "Failed: Bytes written to buffer did not match the initial stream\n");
                return FAILED;
            }
        }
    }

    /* Child 
     * 
     * The child will end up doing the writing so the parent can handle 
     * verification without the child having to communicate anything else. */
    else {
        int bytesLeft = 1024;

        int randWrite;
        while (!(randWrite = (randomC() & 31))); // Avoid writing 0 bytes
        while (bytesLeft > 0) {
            bytesLeft -= write(fds[1], &stream[1024-bytesLeft], randWrite > bytesLeft ? bytesLeft : randWrite);
        }

        close(fds[1]);
        exit();
    }

    return 0;
}
