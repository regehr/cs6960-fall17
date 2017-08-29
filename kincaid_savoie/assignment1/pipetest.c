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
	printf(1, "Testing pipes!\n");

	// Create read/write buffer.
	unsigned int * buf = malloc(BYTES_TO_SEND);
	
	// Create pipe.
	int p[2];
	if (pipe(p) != 0) {
		printf(2, "Error creating pipe.");
		exit();
	}

	// Fork processes.
	int pid = fork();

	// States of numbers to push.
	int data_to_write = 1;

	if (pid == 0) {
		close(p[0]);

		// Number of bytes sent through the pipe.
		unsigned int bytes_sent = 0;

		// amount of bytes to push.
		unsigned int number_to_push = 1;

		while (bytes_sent < BYTES_TO_SEND) {
			// Write a random amount of bytes up 1 KB.
			number_to_push = (rand(number_to_push) % BUF_SIZE) / sizeof(int);

			// Populate the buffer.
			int i;
			for (i = 0; i < number_to_push; i++) {
				buf[i] = data_to_write;
				data_to_write = rand(data_to_write) % MAX_SEND;
			}

			// Write the data.
			int num_written = write(p[1], buf, number_to_push * sizeof(int));

			if (num_written < 0) {
				printf(2, "Failure writing to pipe.\n");
				exit();
			}

			bytes_sent += num_written;

		//	printf(1, "Have pushed %d bytes...\n", bytes_sent);
		}
		
		close(p[1]);

	} else if (pid > 0) {
		close(p[1]);

		// Number of bytes read.
		unsigned int bytes_read = 0;

		// Number of bytes to read.
		unsigned int number_to_read = 1;

		while (bytes_read < BYTES_TO_SEND) {
			// Read a random amount of bytes up to 1 KB.
			number_to_read = (rand(number_to_read) % BUF_SIZE) / sizeof(int);

			// Read the data.
			int num_read = read(p[0], buf, number_to_read * sizeof(int));

			if (num_read < 0) {
				printf(2, "Failure reading from pipe.\n");
				exit();
			}

			// Verify the received bytes
			int i;
			for (i = 0; i < num_read / sizeof(int); i++) {
				if (buf[i] != data_to_write) {
					printf(2, "Mismatch in pipe data. Total read: %d\nCurrent read: %d\n%d %d\n", bytes_read, num_read, buf[i], data_to_write);
					exit();
				} else {
					data_to_write = rand(data_to_write) % MAX_SEND;
				}
			}

			bytes_read += num_read;
		}

		close(p[0]);

		free(buf);


		exit();

	} else {
		printf(2, "Fork error");
		exit();
	}

	free(buf);

	wait();

	printf(1, "All done!\n");

	exit();
}
