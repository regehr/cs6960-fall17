/* I go back and forth on testing on dev Linux box (use -DLINUX to test on linux) */
/* interestingly the partial reads and writes fire under system load */
/* TODO: how does one deal with short reads (app level lock?)? For example let say I want to read int, then if some other thread reads part of it, I get wrong result. */

/*
http://man7.org/linux/man-pages/man7/pipe.7.html
pipe semantics:
- read blocks on empty pipe
- write blocks on full pipe (more importantly it blocks until full write completes; TODO: what about short writes?)
*/

#ifdef LINUX
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#else
#include "fcntl.h"
#include "syscall.h"
#include "types.h"
#include "user.h"

#define assert(x) {if (!(x)) exit();}
#endif

#include "std.h"



/* TODO: not atomic */
static int
read_count(int const fd, void *buffer, int count)
{
        for (int remaining = count; remaining > 0;) {
                int const ret = read(fd, &((char *)buffer)[count - remaining], remaining);
                if (ret == -1)
                        return -1;
                if (ret == 0)
                        return 0;
                remaining -= ret;
        }
        return count;
}


/* TODO: not atomic */
static int
write_count(int const fd, void *buffer, int count)
{
        for (int remaining = count; remaining > 0;) {
                int const ret = write(fd, &((char *)buffer)[count - remaining], remaining);
                if (ret == -1)
                        return -1;
                remaining -= ret;
        }
        return count;
}




/* trivial test of writing a stream of single bytes and reading them */
/* NOTE: we can assume only a pipe with buffer size at least 1, so writes must be 1 byte to avoid deadlocks
         as full pipe blocks on write */
static int
serial_stream(int const size)
{
        int p[2];
        pipe(p);

        unsigned int number_state = 1;
        for (int i = 0; i < size; ++i) {
                char const number = s_rand(&number_state);

                char in_number = number;
                int ret = write(p[1], &in_number, sizeof in_number);
                assert(ret == 1);
                in_number = -1;

                char out_number;
                ret = read(p[0], &out_number, sizeof out_number);
                assert(ret == 1);

                if (number != out_number)
                        return -1;
        }
        return 0;
}


/* forked child writes random stream of characters which are read and compared by parent */
static int
parallel_stream(int const size)
{
        assert(size%sizeof (int) == 0);

        int const num_count = size/sizeof (int);

        int p[2];
        pipe(p);

        unsigned int number_state = 1;

        if (fork() == 0) {
                close(p[0]);

                unsigned int buffer_state = 1;

                /* seems stack is 4k so we need to be careful here */
                enum {buffer_count = 128};
                int buffer[buffer_count];
                for (int i = 0; i < num_count; i += buffer_count) {
                        /* fill buffer */
                        for (int j = 0; j < buffer_count; ++j)
                                buffer[j] = s_rand(&number_state);

                        /* in bytes */
                        int const remaining_count = (i + buffer_count > num_count) ? (num_count%buffer_count)*sizeof *buffer : sizeof buffer;
                        int const count = s_rand(&buffer_state)%remaining_count;

                        int ret = write_count(p[1], buffer, count);
                        assert(ret != -1);
                        assert(ret == count);
                        memset(buffer, -1, count);

                        ret = write_count(p[1], &((char *)buffer)[count], remaining_count - count);
                        assert(ret != -1);
                        assert(ret == remaining_count - count);
                        memset(&((char *)buffer)[count], -1, ret);
                }

                close(p[1]);
#ifdef LINUX
                exit(EXIT_SUCCESS);
#else
                exit();
#endif
        } else {
                close(p[1]);

                unsigned int buffer_state = 42;

                int all_monoid = 1;
                for (int i = 0; i < num_count && all_monoid; ++i) {
                        int number;
                        int const count = s_rand(&buffer_state)%sizeof number;

                        int ret = read_count(p[0], &number, count);
                        assert(ret != -1 && ret == count);

                        ret = read_count(p[0], &((char *)&number)[ret], sizeof number - count);
                        assert(ret != -1 && ret + count == sizeof number);

                        int random = s_rand(&number_state);
                        all_monoid &= number == random;
                }

                close(p[0]);
#ifdef LINUX
                wait(NULL);
#else
                wait();
#endif

                return (all_monoid) ? 0 : -1;
        }

        return -1;
}




/* using files is ugly; we can't use pipe to send back result as we are testing pipes */
static int
multiple_readers(int const size, int const reader_count)
{
        if (reader_count > 10) {
#ifdef LINUX
                fprintf(stderr, "At most 10 readers\n");
#else
                printf(2, "At most 10 readers\n");
#endif
                return -1;
        }

        int p[2];
        pipe(p);

        unsigned int number_state = 2;

        for (int i = 0; i < reader_count; ++i) {
                if (fork() == 0) {
                        close(p[1]);

                        char filename[] = {'r', i + '0', '\0'};
#ifdef LINUX
                        int const fd = open(filename, O_CREAT | O_WRONLY, 0660);
#else
                        int const fd = open(filename, O_CREATE | O_WRONLY);
#endif
                        assert(fd != -1);

                        /* use char so we do not split numbers as in case of int in case of short read */
                        char number;
                        while (read(p[0], &number, sizeof number) > 0)
                                write(fd, &number, sizeof number);
#ifdef LINUX
                        exit(EXIT_SUCCESS);
#else
                        exit();
#endif
                }
        }

        int ret = 0;

        close(p[0]);
        int writer_sum = 0;
        for (int i = 0; i < size; ++i) {
                char number = s_rand(&number_state);
                int const ret = write(p[1], &number, sizeof number);
                assert(ret != -1);
                writer_sum += number;
                number = -1;
        }
        close(p[1]);

        for (int i = 0; i < reader_count; ++i)
#ifdef LINUX
                if (wait(NULL) == -1)
#else
                if (wait() == -1)
#endif
                        ret = -1;

        /* verify if the sum of numbers is same */
        /* TODO: stronger verification */
        int sum = 0;
        for (int i = 0; i < reader_count; ++i) {
                char filename[] = {'r', i + '0', '\0'};
                int const fd = open(filename, O_RDONLY);

                char number;
                while (read(fd, &number, sizeof number) > 0)
                        sum += number;

                close(fd);
                unlink(filename);
        }
        if (sum != writer_sum)
                ret = -1;

        return ret;
}




int
main(void)
{
        int const stream_size = 1024*1024;

#ifdef LINUX
        printf("Serial stream: %d\n", serial_stream(stream_size));
        printf("Parallel stream: %d\n", parallel_stream(stream_size));
        printf("Multiple readers: %d\n", multiple_readers(stream_size, 10));
#else
        printf(1, "Serial stream: %d\n", serial_stream(stream_size));
        printf(1, "Parallel stream: %d\n", parallel_stream(stream_size));
        /* NOTE: small as the multiple readers test takes forever */
        printf(1, "Multiple readers: %d\n", multiple_readers(stream_size/20, 10));
        exit();
#endif
}
