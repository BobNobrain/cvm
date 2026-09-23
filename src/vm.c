#include <stdio.h>
#include <stddef.h>
#include "hoduli.h"
#include "program.h"
#include "vm.h"

int main() {
    ERR_DECL

    VMConfig cfg = {
        .stack_size = 1024,
        .stack_cap = 1024,
        .vars_size = 512,
        .max_vars = 256
    };

    VMachine vm;
    err = vm_init(&vm, cfg);
    if (err != E_NONE) {
        printf("vm init failed\n");
        return -1;
    }

    printf("vm initialized\n");

    memory_print(vm.smem, 16);

    ProgramWriter pw;
    err = program_init_writer(&pw);
    if (err != E_NONE) {
        printf("ProgramWriter init failed\n");
        return -1;
    }

    program_write_instr(&pw, instr_push(value_numi(3)));
    program_write_instr(&pw, instr_push(value_numi(2)));
    program_write_instr(&pw, instr_binop(BINOP_IADD));

    Program p;
    program_finish(&pw, &p);

    program_print(p);

    err = vm_execute(&vm, p);
    if (err != E_NONE) {
        printf("Failed to execute (%d)\n", err);
    }

    memory_print(vm.smem, 16);

    return 0;
}
