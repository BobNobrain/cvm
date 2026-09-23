#ifndef PROGRAM_H
#define PROGRAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "str.h"
#include "value.h"
#include "ops.h"
#include "mem.h"

typedef enum InstructionType {
    I_HALT,
    I_PUSH,
    I_POP,
    I_BINOP,
    I_UNOP,
    I_JMP,
    I_JMPZ,
    I_MEMR,
    I_MEMW,

    I_INVALID
} InstructionType;

typedef unsigned int InstructionPtr;

typedef Value IPushData;
typedef ValueType IPopData;
typedef BinopType IBinopData;
typedef UnopType IUnopData;
typedef InstructionPtr IJmpData;
typedef InstructionPtr IJmpzData;
typedef MemPtr IMemRData;
typedef MemPtr IMemWData;

typedef struct Instruction {
    InstructionType type;
    union {
        IPushData push;
        IPopData pop;
        IBinopData binop;
        IUnopData unop;
        IJmpData jmp;
        IJmpzData jmpz;
        IMemRData memr;
        IMemWData memw;
    } data;
} Instruction;

Instruction instr_push(Value v) {
    Instruction result = { .type = I_PUSH };
    result.data.push = v;
    return result;
}
Instruction instr_pop(uint8_t count) {
    Instruction result = { .type = I_POP };
    result.data.pop = count;
    return result;
}
Instruction instr_binop(BinopType op) {
    Instruction result = { .type = I_BINOP };
    result.data.binop = op;
    return result;
}
Instruction instr_unop(UnopType op) {
    Instruction result = { .type = I_UNOP };
    result.data.unop = op;
    return result;
}
Instruction instr_jmp(InstructionPtr to) {
    Instruction result = { .type = I_JMP };
    result.data.jmp = to;
    return result;
}
Instruction instr_jmpz(InstructionPtr to) {
    Instruction result = { .type = I_JMPZ };
    result.data.jmpz = to;
    return result;
}
Instruction instr_memr(MemPtr to) {
    Instruction result = { .type = I_MEMR };
    result.data.memr = to;
    return result;
}
Instruction instr_memw(MemPtr to) {
    Instruction result = { .type = I_MEMW };
    result.data.memw = to;
    return result;
}

size_t instr_get_size(InstructionType i) {
    switch (i) {
    case I_HALT:
        return 0;
    case I_PUSH:
        return sizeof(IPushData);
    case I_POP:
        return sizeof(IPopData);
    case I_BINOP:
        return sizeof(IBinopData);
    case I_UNOP:
        return sizeof(IUnopData);
    case I_JMP:
        return sizeof(IJmpData);
    case I_JMPZ:
        return sizeof(IJmpzData);
    case I_MEMR:
        return sizeof(IMemRData);
    case I_MEMW:
        return sizeof(IMemWData);

    default:
        return 0;
    }
}

error instr_to_string(Instruction instr, StringWriter *sw) {
    ERR_DECL
    const size_t buffer_size = 20;
    char buffer[20];

    switch (instr.type) {
    case I_HALT:
        return strw_appendc(sw, "HALT");

    case I_PUSH:
        ERR_PASS( strw_appendc(sw, "PUSH ") )
        return value_to_string(instr.data.push, sw);

    case I_POP:
        snprintf(buffer, buffer_size, "POP %u", instr.data.pop);
        return strw_appendc(sw, buffer);

    case I_BINOP:
        ERR_PASS( strw_appendc(sw, "BINOP ") )
        return binop_to_string(instr.data.binop, sw);

    case I_UNOP:
        ERR_PASS( strw_appendc(sw, "UNOP ") )
        return unop_to_string(instr.data.unop, sw);

    case I_JMP:
        snprintf(buffer, buffer_size, "JMP %d", instr.data.jmp);
        return strw_appendc(sw, buffer);

    case I_JMPZ:
        snprintf(buffer, buffer_size, "JMPZ %d", instr.data.jmpz);
        return strw_appendc(sw, buffer);

    case I_MEMR:
        snprintf(buffer, buffer_size, "MEMR @%zu", instr.data.memr);
        return strw_appendc(sw, buffer);

    case I_MEMW:
        snprintf(buffer, buffer_size, "MEMW @%zu", instr.data.memr);
        return strw_appendc(sw, buffer);

    default:
        return E_NONE;
    }
}

typedef struct Program {
    uint8_t *code;
    size_t length;
} Program;

size_t program_read_instr(Program p, InstructionPtr at, Instruction *into) {
    if (p.length <= at) {
        return 0;
    }

    InstructionType type;
    memcpy(&type, &p.code[at], sizeof(InstructionType));
    size_t data_size = instr_get_size(type);

    if (at + data_size + sizeof(InstructionType) > p.length) {
        return 0;
    }

    if (type >= I_INVALID) {
        return 0;
    }

    into->type = type;
    if (data_size > 0) {
        memcpy(&into->data, &p.code[at + sizeof(InstructionType)], data_size);
    }

    return sizeof(InstructionType) + data_size;
}

typedef struct ProgramWriter {
    uint8_t *code;
    size_t length;
    size_t capacity;
} ProgramWriter;

int program_init_writer(ProgramWriter *w) {
    w->length = 0;
    w->capacity = 256;
    w->code = malloc(w->capacity * sizeof(uint8_t));
    if (w->code == 0) {
        return -1;
    }
    return 0;
}

size_t program_write_instr(ProgramWriter *w, Instruction next) {
    if (w->length >= w->capacity) {
        w->capacity *= 2;
        w->code = realloc(w->code, w->capacity);
    }

    size_t type_size = sizeof(next.type);
    memcpy(&w->code[w->length], &next.type, type_size);
    w->length += type_size;

    size_t data_size = instr_get_size(next.type);
    if (data_size > 0) {
        memcpy(&w->code[w->length], &next.data, data_size);
        w->length += data_size;
    }

    return type_size + data_size;
}

void program_finish(ProgramWriter *from, Program *into) {
    into->length = from->length;
    into->code = realloc(from->code, from->length);

    from->code = 0;
    from->length = 0;
    from->capacity = 0;
}

void program_print(Program p) {
    ERR_DECL

    InstructionPtr ptr = 0;
    Instruction instr;
    const size_t buffer_size = 128;
    char buffer[128];

    StringWriter sw;
    strw_init(&sw, 64);

    while (ptr < p.length) {
        size_t ilen = program_read_instr(p, ptr, &instr);
        if (ilen == 0) {
            printf("failed to read next instruction at %d", ptr);
            return;
        }

        snprintf(buffer, buffer_size, "%3d ", ptr);
        err = strw_appendc(&sw, buffer);
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        err = instr_to_string(instr, &sw);
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        err = strw_appendc(&sw, "\n");
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        ptr += ilen;
    }

    String result = strw_render(&sw);
    str_print(result);
}

#endif
