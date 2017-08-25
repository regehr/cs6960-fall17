#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

/* Xorshift cause all the cool people use that nowadays,
 * and we're not doing crypto. */
static uint32_t random() {
    static uint32_t state = 0x1337;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

int main(int argc, char **argv) {
    while(1) {
        printf("%08X\n", random()); 
        sleep(1);
    }
}
