#include "types.h"
#include "user.h"

int
main(void) {
    char* inmsg = "Hello, world!";
    char* outmsg = 0;
    int* outlen = 0;
    int err;

    printf(1, "testing buffer\n");

    if (buf_setup()) {
        printf(2, "error setting up buffer\n");
        exit();
    }

    printf(1, "putting message of size %d: %s\n", strlen(inmsg), inmsg);
    err = buf_put(inmsg, strlen(inmsg) + 1);
    if (err) {
        printf(2, "did not put message correctly\n");
    }

    err = buf_get(outmsg, outlen, strlen(inmsg) + 2);
    if (err) {
        printf(2, "did not get message correctly\n");
    } else {
        printf(1, "retrieved message of size %d: %s\n", outlen, outmsg);
    }

    printf(1, "buffer test complete\n");
}
