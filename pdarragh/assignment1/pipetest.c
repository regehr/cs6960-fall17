#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * rand_int
 *
 * Generate a random int up to the given value. This range includes 0 and the
 * given value.
 *
 * This method of producing a random number on a range does not guarantee an
 * equal distribution along the entire range, but for our purposes it should
 * be fine.
 */
int
rand_int(int max) {
    return rand() % (max + 1);    // Generate a random number on [0, max].
}

/*
 * rand_char
 *
 * Generate a random ASCII character in the decimal range [33, 126] (because
 * these are easy to read when printed).
 */
char
rand_char() {
    int dec = rand_int(93);
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
rand_chars(int num_chars, char* chars) {
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
test_pipe(int length) {
    if (length <= 0) {
        return 0;
    }
    printf("Testing pipe with %d bytes of data...\n", length);

    int result = 0;         // Result of the comparison.
    length = length + 1;    // Increased to give space for a terminating byte.

    // Make the pipe.
    int fds[2];
    if (pipe(fds) < 0) {
        printf("Error opening pipe.\n");
        return 1;
    }
    int rf = fds[0];    // Reader.
    int wf = fds[1];    // Writer.

    // Fork to move data through the pipe.
    if (fork() == 0) {
        // Use the child as the writer.
        // Write the data through the pipe.
        close(rf);
        // Get some chars to pass through.
        char chars[length];
        rand_chars(length - 1, chars);
        write(wf, chars, (size_t)length);
        close(wf);
    } else {
        // Use the parent as the reader.
        close(wf);
        char buf[length];
        read(rf, buf, (size_t)length);
        close(rf);
        // Check that the data matches.
        char expected[length];
        rand_chars(length - 1, expected);
        for (int i = 0; i < length - 1; ++i) {
            if  (expected[i] == buf[i]) {
                printf(".");
            } else {
                printf("F");
                result = 1;
            }
        }
        printf("\n");
        exit(result);
    }
    return result;
}

int
main() {
    int result = 0;
    int sizes[] = {1, 2, 4, 8, 32, 128, 512, 1024, 2048, 4095, 4096, 4097, 65535};
    int length = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < length; ++i) {
        srand( (unsigned) time(NULL) );
        result |= test_pipe(sizes[i]);
    }

    return result;
}
