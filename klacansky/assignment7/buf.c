// shared memory ring buffer
// uint aligned slots

#include "types.h"
#include "memlayout.h"
#include "mmu.h"
#include "user.h"


static char *
s_memcpy(char *const restrict dst, char const *const restrict src, int const count)
{
        for (int i = 0; i < count; ++i)
                dst[i] = src[i];
        return dst;
}


int
buf_put(char *data, size_t len)
{
        uint *const nread = (uint *)BUF_BASE;
        uint *const nwrite = (uint *)(BUF_BASE + sizeof *nread);

        uint const capacity = BUF_SIZE - (*nwrite - *nread) - sizeof (uint);
        if (capacity < len)
                return -1;

        uint *const ptr = (uint *)(BUF_DATA + *nwrite%BUF_SIZE);
        ptr[0] = len;
        s_memcpy((char *)&ptr[1], data, len);
        __atomic_fetch_add(nwrite, sizeof (uint) + (len + sizeof (uint) - 1)/sizeof (uint)*sizeof (uint), __ATOMIC_RELAXED);

        return 0;
}


int
buf_get(char *data, size_t *len, size_t maxlen)
{
        uint *const nread = (uint *)BUF_BASE;
        uint *const nwrite = (uint *)(BUF_BASE + sizeof *nread);

        if (*nread == *nwrite)
                return -1;

        uint const *const ptr = (uint *)(BUF_DATA + *nread%BUF_SIZE);
        if (ptr[0] > maxlen)
                return -2;

        *len = ptr[0];
        s_memcpy(data, (char *)&ptr[1], *len);
        __atomic_fetch_add(nread, sizeof (uint) + (*len + sizeof (uint) - 1)/sizeof (uint)*sizeof (uint), __ATOMIC_RELAXED);

        return 0;
}
