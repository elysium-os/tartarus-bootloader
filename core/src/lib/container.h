#pragma once

#define CONTAINER_OF(PTR, TYPE, MEMBER)                                \
    ({                                                                 \
        const typeof(((TYPE *) 0)->MEMBER) *__mptr = (PTR);            \
        (TYPE *) ((char *) __mptr - __builtin_offsetof(TYPE, MEMBER)); \
    })
