#ifndef MEM_H
#define MEM_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "hoduli.h"
#include "value.h"

typedef size_t MemPtr;
typedef struct Memory {
    uint8_t *content;
    size_t length;
} Memory;

error memory_init(Memory *mem) {
    mem->content = malloc(mem->length * sizeof(uint8_t));
    if (mem->content == 0) {
        return E_MEMORY;
    }
    return E_NONE;
}

#define DECLARE_MEMORY_READ_FN(SUFFIX, VALUE_TYPE) \
size_t memory_read_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE *into) { \
    if (mem.length < at + sizeof(VALUE_TYPE)) { return 0; } \
    memcpy(into, &mem.content[at], sizeof(VALUE_TYPE)); \
    return sizeof(VALUE_TYPE); \
}

DECLARE_MEMORY_READ_FN(bool, BoolValue)
DECLARE_MEMORY_READ_FN(numi, NumIValue)
DECLARE_MEMORY_READ_FN(numf, NumFValue)

#undef DECLARE_MEMORY_READ_FN

size_t memory_read(Memory mem, MemPtr at, Value *into) {
    switch (into->type) {
    case V_BOOL:
        return memory_read_bool(mem, at, &into->data.boolv);
    case V_NUMI:
        return memory_read_numi(mem, at, &into->data.numi);
    case V_NUMF:
        return memory_read_numf(mem, at, &into->data.numf);

    default:
        return 0;
    }
}

#define DECLARE_MEMORY_WRITE_FN(SUFFIX, VALUE_TYPE) \
size_t memory_write_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE value) { \
    if (mem.length < at + sizeof(VALUE_TYPE)) { return 0; } \
    memcpy(&mem.content[at], &value, sizeof(VALUE_TYPE)); \
    return sizeof(VALUE_TYPE); \
}

DECLARE_MEMORY_WRITE_FN(bool, BoolValue)
DECLARE_MEMORY_WRITE_FN(numi, NumIValue)
DECLARE_MEMORY_WRITE_FN(numf, NumFValue)

#undef DECLARE_MEMORY_WRITE_FN

size_t memory_write(Memory mem, MemPtr at, Value value) {
    switch (value.type) {
    case V_BOOL:
        return memory_write_bool(mem, at, value.data.boolv);
    case V_NUMI:
        return memory_write_numi(mem, at, value.data.numi);
    case V_NUMF:
        return memory_write_numf(mem, at, value.data.numf);

    default:
        return 0;
    }
}

void memory_print(Memory mem, size_t max) {
    if (max == 0 || max > mem.length) {
        max = mem.length;
    }

    for (size_t i = 0; i < max; i++) {
        printf("%02X ", mem.content[i]);

        if (i % 16 == 15) {
            printf("\n");
        }
    }

    printf("\n");
}

/*
typedef struct Stack {
    MemPtr *ptrs;
    size_t size;
    size_t capacity;
} Stack;

error stack_init(Stack *s, size_t cap) {
    if (cap == 0) {
        return -1;
    }

    s->ptrs = malloc(sizeof(MemPtr) * cap);

    if (s->ptrs == 0) { return E_MEMORY ; }

    s->capacity = cap;
    s->size = 0;
    s->ptrs[0] = 0;

    return E_NONE;
}

error stack_push(Stack *s, MemPtr ptr, size_t size) {
    if (s->size >= s->capacity) {
        return E_OUT_OF_RANGE;
    }

    s->ptrs[s->size] = ptr;
    s->size += 1;
    s->ptrs[s->size] = ptr + size;

    return E_NONE;
}

error stack_pop(Stack *s, size_t n) {
    if (s->size < n) { return E_OUT_OF_RANGE; }

    s->size -= n;

    return E_NONE;
}

error stack_get(Stack s, int at, MemPtr *into) {
    if (at < 0) {
        at += s.size;
    }

    if ((int)(s.size) <= at) {
        return E_OUT_OF_RANGE;
    }

    *into = s.ptrs[at];
    return E_NONE;
}

error stack_get_next(Stack s, MemPtr *into) {
    if (s.size >= s.capacity) {
        return E_OUT_OF_RANGE;
    }

    *into = s.ptrs[s.size];
    return E_NONE;
}
*/

#endif
