#include <stddef.h>
#include <stdio.h>
#include "util.h"
#include "lang_int.h"

Instruction lang_instr_make_push(Value v) {
    Instruction result = { .type = I_PUSH };
    result.data.push = v;
    return result;
}
Instruction lang_instr_make_pop(uint8_t count) {
    Instruction result = { .type = I_POP };
    result.data.pop = count;
    return result;
}
Instruction lang_instr_make_binop(BinopType op) {
    Instruction result = { .type = I_BINOP };
    result.data.binop = op;
    return result;
}
Instruction lang_instr_make_unop(UnopType op) {
    Instruction result = { .type = I_UNOP };
    result.data.unop = op;
    return result;
}
Instruction lang_instr_make_jmp(InstructionPtr to) {
    Instruction result = { .type = I_JMP };
    result.data.jmp = to;
    return result;
}
Instruction lang_instr_make_jmpz(InstructionPtr to) {
    Instruction result = { .type = I_JMPZ };
    result.data.jmpz = to;
    return result;
}
Instruction lang_instr_make_memr(MemPtr to) {
    Instruction result = { .type = I_MEMR };
    result.data.memr = to;
    return result;
}
Instruction lang_instr_make_memw(MemPtr to) {
    Instruction result = { .type = I_MEMW };
    result.data.memw = to;
    return result;
}

size_t lang_instr_get_size(InstructionType i) {
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

void lang_instr_to_string(Instruction instr, StringBuilder *sw) {
    const size_t buffer_size = 20;
    char buffer[20];

    switch (instr.type) {
    case I_HALT:
        strb_appendc(sw, "HALT");
        break;

    case I_PUSH:
        strb_appendc(sw, "PUSH ");
        lang_value_to_string(instr.data.push, sw);
        break;

    case I_POP:
        snprintf(buffer, buffer_size, "POP %u", instr.data.pop);
        strb_appendc(sw, buffer);
        break;

    case I_BINOP:
        strb_appendc(sw, "BINOP ");
        lang_binop_to_string(instr.data.binop, sw);
        break;

    case I_UNOP:
        strb_appendc(sw, "UNOP ");
        lang_unop_to_string(instr.data.unop, sw);
        break;

    case I_JMP:
        snprintf(buffer, buffer_size, "JMP %zu", instr.data.jmp);
        strb_appendc(sw, buffer);
        break;

    case I_JMPZ:
        snprintf(buffer, buffer_size, "JMPZ %zu", instr.data.jmpz);
        strb_appendc(sw, buffer);
        break;

    case I_MEMR:
        snprintf(buffer, buffer_size, "MEMR @%zu", instr.data.memr);
        strb_appendc(sw, buffer);
        break;

    case I_MEMW:
        snprintf(buffer, buffer_size, "MEMW @%zu", instr.data.memr);
        strb_appendc(sw, buffer);
        break;

    default:
        break;
    }
}
