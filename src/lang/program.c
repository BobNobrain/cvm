#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "lang_int.h"

size_t lang_program_read_instr(Program p, InstructionPtr at, Instruction *into) {
    if (p.length <= at) {
        return 0;
    }

    InstructionType type;
    memcpy(&type, &p.code[at], sizeof(InstructionType));
    size_t data_size = lang_instr_get_size(type);

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

int lang_program_init_writer(ProgramWriter *w) {
    w->length = 0;
    w->capacity = 256;
    w->code = malloc(w->capacity * sizeof(uint8_t));
    if (w->code == 0) {
        return -1;
    }
    return 0;
}

size_t lang_program_write_instr(ProgramWriter *w, Instruction next) {
    if (w->length >= w->capacity) {
        w->capacity *= 2;
        w->code = realloc(w->code, w->capacity);
    }

    size_t type_size = sizeof(next.type);
    memcpy(&w->code[w->length], &next.type, type_size);
    w->length += type_size;

    size_t data_size = lang_instr_get_size(next.type);
    if (data_size > 0) {
        memcpy(&w->code[w->length], &next.data, data_size);
        w->length += data_size;
    }

    return type_size + data_size;
}

void lang_program_finish(ProgramWriter *from, Program *into) {
    into->length = from->length;
    into->code = realloc(from->code, from->length);

    from->code = 0;
    from->length = 0;
    from->capacity = 0;
}

void lang_program_print(Program p) {
    ERR_DECL

    InstructionPtr ptr = 0;
    Instruction instr;
    const size_t buffer_size = 128;
    char buffer[128];

    StringBuilder *sw = strb_new(64);

    while (ptr < p.length) {
        size_t ilen = lang_program_read_instr(p, ptr, &instr);
        if (ilen == 0) {
            printf("failed to read next instruction at %d", ptr);
            return;
        }

        snprintf(buffer, buffer_size, "%3d ", ptr);
        err = strb_appendc(&sw, buffer);
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        err = lang_instr_to_string(instr, &sw);
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        err = strb_appendc(&sw, "\n");
        if (err != E_NONE) {
            printf("failed to render: %d\n", err);
            return;
        }

        ptr += ilen;
    }

    String result = strb_render(&sw);
    printf(STR_FMT, STR_FMT_VAL(result));
}
