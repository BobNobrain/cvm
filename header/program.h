#ifndef PROGRAM_H
#define PROGRAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "lang.h"

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

typedef enum BinopType {
    // common ops
    BINOP_EQ,
    BINOP_NEQ,

    // int ops
    BINOP_IADD,
    BINOP_IMUL,
    BINOP_ISUB,
    BINOP_IDIV,
    BINOP_IREM,
    BINOP_IGT,
    BINOP_ILT,
    BINOP_IGTE,
    BINOP_ILTE,

    // float ops
    BINOP_FADD,
    BINOP_FMUL,
    BINOP_FSUB,
    BINOP_FDIV,
    BINOP_FREM,

    // bool ops
    BINOP_BAND,
    BINOP_BOR,

    BINOP_INVALID
} BinopType;

typedef enum UnopType {
    UNOP_BNOT,
    UNOP_INEG,
    UNOP_FNEG,
    UNOP_ITOF,

    UNOP_INVALID
} UnopType;

typedef Value IPushData;
typedef uint8_t IPopData;
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

int instr_to_string(Instruction instr, char *str, size_t str_size) {
    char value_str[20];
    int written;

    switch (instr.type) {
    case I_HALT:
        return snprintf(str, str_size, "%s", "HALT");
    case I_PUSH:
        written = value_to_string(instr.data.push, value_str, 20);
        if (written < 0) {
            return snprintf(str, str_size, "PUSH %s", "??");
        }

        if (written >= 20) {
            value_str[17] = '.';
            value_str[18] = '.';
            value_str[19] = '.';
        }

        return snprintf(str, str_size, "PUSH %s", value_str);
    case I_POP:
        return snprintf(str, str_size, "POP %d", instr.data.pop);
    case I_BINOP:
        return snprintf(str, str_size, "BINOP %d", instr.data.binop); // TODO: instr_binop_to_string
    case I_UNOP:
        return snprintf(str, str_size, "UNOP %d", instr.data.unop); // TODO: instr_unop_to_string
    case I_JMP:
        return snprintf(str, str_size, "JMP %d", instr.data.jmp);
    case I_JMPZ:
        return snprintf(str, str_size, "JMPZ %d", instr.data.jmpz);
    case I_MEMR:
        return snprintf(str, str_size, "MEMR @%zu", instr.data.memr);
    case I_MEMW:
        return snprintf(str, str_size, "MEMW @%zu", instr.data.memr);

    default:
        return 0;
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
    InstructionPtr ptr = 0;
    Instruction instr;
    char buffer[128];

    while (ptr < p.length) {
        size_t ilen = program_read_instr(p, ptr, &instr);
        if (ilen == 0) {
            printf("failed to read next instruction at %d", ptr);
            return;
        }

        instr_to_string(instr, buffer, 128);
        printf("%3d %s\n", ptr, buffer);

        ptr += ilen;
    }
}

#endif
