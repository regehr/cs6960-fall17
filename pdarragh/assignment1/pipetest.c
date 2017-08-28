#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * rand_char
 *
 * Generate a random ASCII character in the decimal range [33, 126] (because
 * these are easy to read when printed).
 *
 * This method does not produce a perfectly equal distribution among all
 * possible characters, but that's okay for this program.
 */
char
rand_char() {
    int dec = rand() % 94;  // Generate a random number on [0, 93].
    dec += 33;              // Add 33 to make the range [33, 126].
    char c = (char) dec;
    return c;
}

/*
 * rand_chars
 *
 * Populate an existing char array with random characters.
 */
void
rand_chars(size_t num_chars, char* chars) {
    for (int i = 0; i < num_chars; ++i) {
        chars[i] = rand_char();
    }
}

/*
 * test_pipe
 *
 * Build a pipe, and feed random bytes through it to the other side.
 */
int
test_pipe(size_t length) {
    int result = 0;         // Result of the comparison.
    length = length + 1;    // Increased to give space for a terminating byte.

    // Make the pipe.
    int fds[2];
    pipe(fds);
    int rf = fds[0];    // Reader.
    int wf = fds[1];    // Writer.

    // Fork to move data through the pipe.
    if (fork() == 0) {
        // Use the child as the writer.
        // Get some chars to pass through.
        char chars[length];
        rand_chars(length - 1, chars);
        printf("wrote: %s\n", chars);
        // Write the data through the pipe.
        close(rf);
        write(wf, chars, length);
        close(wf);
    } else {
        // Use the parent as the reader.
        close(wf);
        char buf[length];
        read(rf, buf, length);
        close(rf);
        printf("read:  %s\n", buf);
        // Check the data matches.
        char expected[length];
        rand_chars(length - 1, expected);
        printf("rgen:  %s\n", expected);
        for (int i = 0; i < length; ++i) {
            if (expected[i] != buf[i]) {
                printf("%c != %c\n", expected[i], buf[i]);
                result = 1;
            }
        }
    }
    return result;
}

int
main() {
    printf("pipetest\n");
    srand( (unsigned) time(NULL) );

    int result = test_pipe(8);

    return result;
}
