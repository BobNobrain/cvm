#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "util.h"
#include "lang.h"
#include "rt_int.h"

error rt_memory_init(Memory *mem) {
    mem->content = malloc(mem->length * sizeof(uint8_t));
    if (mem->content == 0) {
        return E_MEMORY;
    }
    return E_NONE;
}

#define IMPLEMENT_MEMORY_READ_FN(SUFFIX, VALUE_TYPE) \
size_t rt_memory_read_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE *into) { \
    if (mem.length < at + sizeof(VALUE_TYPE)) { return 0; } \
    memcpy(into, &mem.content[at], sizeof(VALUE_TYPE)); \
    return sizeof(VALUE_TYPE); \
}

MEMORY_TYPES_LIST(IMPLEMENT_MEMORY_READ_FN)
#undef IMPLEMENT_MEMORY_READ_FN

size_t rt_memory_read(Memory mem, MemPtr at, Value *into) {
    switch (into->type) {
    case V_BOOL:
        return rt_memory_read_bool(mem, at, &into->data.boolv);
    case V_NUMI:
        return rt_memory_read_numi(mem, at, &into->data.numi);
    case V_NUMF:
        return rt_memory_read_numf(mem, at, &into->data.numf);

    default:
        return 0;
    }
}

#define IMPLEMENT_MEMORY_WRITE_FN(SUFFIX, VALUE_TYPE) \
size_t rt_memory_write_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE value) { \
    if (mem.length < at + sizeof(VALUE_TYPE)) { return 0; } \
    memcpy(&mem.content[at], &value, sizeof(VALUE_TYPE)); \
    return sizeof(VALUE_TYPE); \
}

MEMORY_TYPES_LIST(IMPLEMENT_MEMORY_WRITE_FN)
#undef IMPLEMENT_MEMORY_WRITE_FN

size_t rt_memory_write(Memory mem, MemPtr at, Value value) {
    switch (value.type) {
    case V_BOOL:
        return rt_memory_write_bool(mem, at, value.data.boolv);
    case V_NUMI:
        return rt_memory_write_numi(mem, at, value.data.numi);
    case V_NUMF:
        return rt_memory_write_numf(mem, at, value.data.numf);

    default:
        return 0;
    }
}

void rt_memory_print(Memory mem, size_t max) {
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
