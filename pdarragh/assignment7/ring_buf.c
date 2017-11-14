#include "types.h"
#include "user.h"

#define PAGE_SIZE 4096  // size of pages in xv6 -- do not change!
#define PAGE_COUNT 8    // number of pages to use in the buffer
#define BUF_SIZE ((PAGE_COUNT * PAGE_SIZE) - (3 * (sizeof(int))))   // the size of the ring buffer

#define SUCCESS 0
#define FAILURE -1
#define NOROOM -2

#define FALSE 0
#define TRUE 1

/**
 * An atomic store, using GCC macros.
 *
 * @param ptr where to store the data
 * @param val the value to store
 */
static void
store(int *ptr, int val) {
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * An atomic load, using GCC macros.
 *
 * @param ptr data to load
 * @return the data
 */
static int
load(int *ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

/**
 * The ring buffer struct is composed of two pointers (one indicating the read progress, the other the write progress)
 * and the array of data stored in the buffer. There is also a flag to indicate whether the buffer was already set up.
 */
static struct ring_buf {
    int allocated;  // whether the buffer has been set up
    int r;  // count of bytes read
    int w;  // count of bytes written
            // w - r = 0    => empty buffer
            // r > w        => not allowed
    char data[BUF_SIZE];    // data storage
} buf;

void
set_buf_r(int new_r) {
    store(&(buf.r), new_r);
}

void
set_buf_w(int new_w) {
    store(&(buf.w), new_w);
}

int
get_buf_r(void) {
    return load(&(buf.r));
}

int
get_buf_w(void) {
    return load(&(buf.w));
}

/**
 * Initial setup of the shared ring buffer -- a FIFO-like memory storage system.
 *
 * It is assumed that there is one producer of information and one consumer, and that the producer and consumer are in
 * cahoots (i.e. they discuss with one another about the quantity of data sent and when).
 *
 * If the ring buffer has not been allocated, it is allocated here. An error is generated if the ring buffer has
 * previously been allocated or if the allocation fails.
 *
 * @return 0 on success, -1 on failure
 */
int
buf_setup(void) {
    // We can only allocate the buffer once.
    if (buf.allocated) {
        return FAILURE;
    }
    int dest = (int)allocbuf();
    if (!dest) {
        // The buffer's space was not allocated correctly.
        return FAILURE;
    }
    buf = *(struct ring_buf*)dest;
    buf.allocated = TRUE;
    return SUCCESS;
}

/**
 * Put data into the buffer, if space is available.
 *
 * @param data the message to enqueue
 * @param len the amount of data in the message (how much data to copy)
 * @return 0 on success, -1 on failure
 */
int
buf_put(char* data, int len) {
    if (!buf.allocated) {
        // Buffer is not allocated. Tsk tsk.
        return FAILURE;
    }
    if (len == 0) {
        // Nothing to write, so we can't have failed!
        return SUCCESS;
    }

    int read = get_buf_r();
    int writ = get_buf_w();

    // Check if we're trying to write too much.
    if (len > (BUF_SIZE - writ + read)) {
        return FAILURE;
    }

    buf.data[writ % BUF_SIZE] = len;   // Write down the size of the message as a header.
    writ += sizeof(int);                // Move the write offset for the header.
    int remaining = BUF_SIZE - (writ % BUF_SIZE);
    if (len < remaining) {
        // The message will fit in the remainder of the buffer.
        memmove(&buf.data[writ % BUF_SIZE], data, len);
        writ += len;
    } else {
        // The message is too long, so we'll have to wrap around.
        memmove(&buf.data[writ % BUF_SIZE], data, remaining);
        writ += remaining;
        memmove(&buf.data[writ % BUF_SIZE], data + remaining, len - remaining);
        writ += len - remaining;
    }

    set_buf_w(writ);

    return SUCCESS;
}

/**
 * Retrieve data from the buffer.
 *
 * @param data the destination for the message
 * @param len the length of the message (returned to the user)
 * @param maxlen the maximum size of the destination
 * @return 0 on success, -1 on generic failure, -2 if the message is longer than `maxlen`
 */
int
buf_get(char* data, int* len, int maxlen) {
    if (maxlen < *len) {
        // Not allowed to return enough data.
        return FAILURE;
    }
    if (*len == 0) {
        // Nothing to return, so no failure!
        return SUCCESS;
    }

    int read = get_buf_r();
    int writ = get_buf_w();

    if (read >= writ) {
        // Somehow our read offset is ahead of the write, which shouldn't be possible.
        return FAILURE;
    }

    int message_len = buf.data[read % BUF_SIZE];

    if (message_len > maxlen) {
        // Not enough space to return this message.
        return NOROOM;
    }

    read += sizeof(int);    // Move the read offset for the header.

    int remaining = BUF_SIZE - (read % BUF_SIZE);
    if (message_len < remaining) {
        // The message is contained within the remainder of the buffer (no wraparound).
        memmove(data, &buf.data[read % BUF_SIZE], message_len);
        read += message_len;
    } else {
        // The message is wrapped through the buffer's boundaries.
        memmove(data, &buf.data[read % BUF_SIZE], remaining);
        read += remaining;
        memmove(data + remaining, &buf.data[read % BUF_SIZE], message_len - remaining);
        read += message_len - remaining;
    }

    set_buf_r(read);
    *len = message_len;

    return SUCCESS;
}
