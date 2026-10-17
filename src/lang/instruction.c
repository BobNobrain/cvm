#include <stddef.h>
#include <stdio.h>
#include "util.h"
#include "lang_int.h"

size_t lang_instr_get_size(InstructionType i) {
    switch (i) {
    case I_HALT:
        return 0;
    case I_PUSH32:
        return sizeof(uint32_t);
    case I_POP32:
        return 0;
    case I_BINOP:
        return sizeof(BinopType);
    case I_UNOP:
        return sizeof(UnopType);
    case I_JMP:
        return sizeof(InstructionPtr);
    case I_JMPZ:
        return sizeof(InstructionPtr);
    case I_IOC1:
        return 0;

    default:
        return 0;
    }
}

void lang_instr_print(Instruction instr) {
    switch (instr.type) {
    case I_HALT:    printf("HALT");                             break;
    case I_PUSH32:  printf("PUSH32 %#010x", instr.data.push32); break;
    case I_POP32:   printf("POP32");                            break;
    case I_JMP:     printf("JMP %zu", instr.data.jmp);          break;
    case I_JMPZ:    printf("JMPZ %zu", instr.data.jmpz);        break;
    case I_IOC1:    printf("IOC1");                             break;

    case I_BINOP:
        printf("BINOP ");
        lang_print_binop(instr.data.binop);
        break;

    case I_UNOP:
        printf("UNOP ");
        lang_print_unop(instr.data.unop);
        break;

    default: break;
    }
}
