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

int
main(int argc, char * argv[]) {
    printf("pipetest\n");
    srand( (unsigned) time(NULL) );

    char chars[8];
    rand_chars(8, chars);
    printf(chars);
    printf("\n");

    return 0;
}
