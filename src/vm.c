#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "lang.h"
#include "rt.h"

int main() {
    ERR_DECL

    VMConfig cfg = {
        .stack_size = 1024,
        .stack_cap = 1024,
        .vars_size = 512,
        .max_vars = 256
    };

    VMachine vm;
    rt_vm_init(&vm, cfg);
    printf("vm initialized\n");

    rt_memory_print(vm.smem, 16);

    ProgramWriter pw;
    lang_program_init_writer(&pw);

    lang_program_write_instr(&pw, lang_instr_make_push(lang_value_make_numi(30)));
    lang_program_write_instr(&pw, lang_instr_make_push(lang_value_make_numi(20)));
    lang_program_write_instr(&pw, lang_instr_make_binop(BINOP_IADD));
    lang_program_write_instr(&pw, lang_instr_make_push(lang_value_make_numi(10)));
    lang_program_write_instr(&pw, lang_instr_make_binop(BINOP_IDIV));

    Program p;
    lang_program_finish(&pw, &p);

    lang_program_print(p);

    err = rt_vm_execute(&vm, p);
    if (ERR_ISSET) {
        printf("Failed to execute (%d)\n", err);
    }

    rt_memory_print(vm.smem, 16);

    return 0;
}
