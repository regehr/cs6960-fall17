#include "types.h"
#include "stat.h"
#include "user.h"

/*
 * "Random" number generator taken from usertests.c.
 *
 */
unsigned int
rand(unsigned int randstate)
{
  randstate = randstate * 1664525 + 1013904223;
  return randstate;
}


#define BYTES_TO_SEND 1000000
#define BUF_SIZE 1024
#define MAX_SEND 1024


int main() {
	printf(1, "Testing pipe!\n");

	// Create read/write buffer.
	unsigned int * buf = malloc(BUF_SIZE);
	
	// Create pipe.
	int p[2];
	if (pipe(p) != 0) {
		printf(2, "Error creating pipe.");
		exit();
	}

	// Fork processes.
	int pid = fork();

	// State of numbers to push.
	int current_data = 1;

    // If the process is the child, start pushing bytes through.
	if (pid == 0) {
        // Close read end of pipe.
		close(p[0]);

		// total number of bytes sent through the pipe
		unsigned int bytes_sent = 0;

		// current number of bytes to push
		unsigned int number_to_push = 1;

        // Keep sending bytes until we've sent the number we want
		while (bytes_sent < BYTES_TO_SEND) {
			// Write a random amount of bytes up to the size of the buffer
			number_to_push = (rand(number_to_push) % BUF_SIZE) / sizeof(int);

			// Populate the buffer.
			int i;
			for (i = 0; i < number_to_push; i++) {
				buf[i] = current_data;
				current_data = rand(current_data) % MAX_SEND;
			}

			// Write the data.
			int num_written = write(p[1], buf, number_to_push * sizeof(int));

			if (num_written < 0) {
				printf(2, "Failure writing to pipe.\n");
				exit();
			}

			bytes_sent += num_written;
		}
		
        // Close the write end of the pipe.
		close(p[1]);

        // Free the buffer.
        free(buf);

        // Exit.
        exit();

    // If this is the parent process, read and verify bytes.
	} else if (pid > 0) {
        // Close write end of pipe.
		close(p[1]);

		// total number of bytes read
		unsigned int bytes_read = 0;

		// Number of bytes to read.
		unsigned int number_to_read = 1;

        // Keep reading bytes until we've read the amount specified
		while (bytes_read < BYTES_TO_SEND) {
			// Read a random amount of bytes up to the buffer size
			number_to_read = (rand(number_to_read) % BUF_SIZE) / sizeof(int);

			// Read the data.
			int num_read = read(p[0], buf, number_to_read * sizeof(int));

            // If something went wrong with the pipe, exit.
			if (num_read < 0) {
				printf(2, "Failure reading from pipe.\n");
				exit();
			}

			// Verify the received bytes
			int i;
			for (i = 0; i < num_read / sizeof(int); i++) {
				if (buf[i] != current_data) {
					printf(2, "Mismatch in pipe data.\n");
                    printf(2, "Total read: %d\n", num_read);
                    printf(2, "Current read: %d\n", i * sizeof(int));
                    printf(2, "Byte expected: %d\n", current_data);
                    printf(2, "Byte received: %d\n", buf[i]);
					exit();
				} else {
					current_data = rand(current_data) % MAX_SEND;
				}
			}

			bytes_read += num_read;
		}

        // Close the read end of the buffer.
		close(p[0]);

	} else {
		printf(2, "Fork error");
		exit();
	}

    // Wait for the child process to exit.
	wait();

	printf(1, "Pipe test successful!\n");

	exit();
}
