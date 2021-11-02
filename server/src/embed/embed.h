#ifndef EMBED_H
#define EMBED_H

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define inline_always __inline__ __attribute__((always_inline))

#define container_of(ptr, type, member)                              \
    ({                                                               \
        const typeof(((type *)0)->member) *__mptr = (ptr);           \
        (type *)((char *)__mptr - __builtin_offsetof(type, member)); \
    })

#endif
