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
test_pipe() {
    // Make the pipe.
    int fds[2];
    pipe(fds);

    // Fork to move data through the pipe.
    if (fork() == 0) {
        // Use the child as the writer.
        // Get some chars to pass through.
        char chars[8];
        rand_chars(7, chars);
        printf("wrote: %s\n", chars);
        // Write the data through the pipe.
        close(fds[0]);
        write(fds[1], chars, 8);
        close(fds[1]);
    } else {
        // Use the parent as the reader.
        close(fds[1]);
        char buf[8];
        read(fds[0], buf, 8);
        close(fds[0]);
        printf("read: %s\n", buf);
    }
    return 0;
}

int
main(int argc, char * argv[]) {
    printf("pipetest\n");
    srand( (unsigned) time(NULL) );

    test_pipe();

    return 0;
}
