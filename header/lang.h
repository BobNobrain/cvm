#ifndef LANG_H
#define LANG_H

#include <stdlib.h>
#include <stddef.h>

#define I_HALT  0
#define I_PUSH  1
#define I_POP   2
#define I_BINOP 3
#define I_UNOP  4
#define I_JMP   5
#define I_JMPZ  6
#define I_MEMR  7
#define I_MEMW  8

typedef char instr_t;
typedef int memptr_t;
typedef int instrptr_t;
typedef char value_t;

typedef struct Program {
    instr_t *code;
    unsigned int length;
} program_t;

typedef struct Memory {
    value_t *start;
    value_t *end;
} memory_t;

#define V_NULL 0
#define V_BOOL 1
#define V_NUMI 2
#define V_NUMF 3
#define V_STR  4
#define V_CPTR 5
#define V_ARR  6
#define V_HASH 7

typedef char value_t;

typedef char boolval_t;
typedef int numival_t;
typedef float numfval_t;

typedef struct StrVal {
    size_t length;
} strval_t;

typedef struct ArrVal {
    size_t length;
    value_t value_type;
} arrval_t;

typedef struct Stack {
    value_t **start;
    size_t size;
    size_t capacity;
} stack_t;

void value_debug_print(value_t v) {
    switch (v) {
    case V_NULL:
        printf("null"); break;
    case V_BOOL:
        printf("bool"); break;
    }
}

size_t value_get_size(memory_t mem) {
    switch (*(mem.start)) {
    case V_NULL:
        return 0;
    case V_BOOL:
        return sizeof(boolval_t);
    case V_NUMI:
        return sizeof(numival_t);
    }

    return 0;
}

memory_t memory_get_tail(memory_t mem) {
    mem.start += get_value_size(mem);
    if (mem.start > mem.end) {
        mem.start = mem.end;
    }
    return mem;
}

int stack_init(stack_t *s, size_t cap) {
    s->start = malloc(sizeof(value_t*) * cap);

    if (s->start == 0) { return -1 ; }

    s->capacity = cap;
    s->size = 0;

    return 0;
}

int stack_push(stack_t *s, value_t *v) {
    if (s->size >= s->capacity) {
        return -1;
    }

    s->start[s->size] = v;
    s->size += 1;

    return 0;
}

int stack_pop(stack_t *s, size_t n) {
    if (s->size < n) { return -1; }

    s->size -= n;

    return 0;
}

value_t* stack_get(stack_t s, int at) {
    if (at < 0) {
        at += s.size;
    }

    if (s.size <= at) {
        return 0;
    }

    return s.start[at];
}

#endif
