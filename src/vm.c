#include <stdio.h>
#include <stddef.h>
#include "hoduli.h"
#include "lang.h"
#include "mem.h"
#include "program.h"

typedef struct Machine {
    Memory vmem;
    Memory smem;
    Stack vars;
    Stack stack;
} VMachine;

typedef struct VMConfig {
    size_t stack_size;
    size_t stack_cap;

    size_t vars_size;
    size_t max_vars;
} VMConfig;

int vm_init(VMachine *vm, VMConfig cfg) {
    ERR_DECL

    Memory mem;
    size_t total = cfg.stack_size + cfg.vars_size;
    mem.length = total;
    ERR_PASS( memory_init(&mem) )

    Memory vmem, smem;
    vmem.length = cfg.vars_size;
    vmem.content = mem.content;
    smem.length = cfg.stack_size;
    smem.content = &mem.content[cfg.vars_size];

    vm->vmem = vmem;
    vm->smem = smem;

    ERR_PASS( stack_init(&vm->stack, cfg.stack_cap) )
    ERR_PASS( stack_init(&vm->vars, cfg.max_vars) )

    return 0;
}

int write_and_push_numi(Stack *stack, Memory mem, NumIValue value) {
    ERR_DECL

    MemPtr addr;
    ERR_PASS( stack_get_next(*stack, &addr) )

    size_t size = memory_write_numi(mem, addr, value);
    if (size == 0) { return -1; }

    ERR_PASS( stack_push(stack, addr, size) )

    return 0;
}

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
    if (err != 0) {
        printf("vm init failed\n");
        return -1;
    }

    printf("vm initialized\n");

    for (int i = 2; i < 50; i += 10) {
        err = write_and_push_numi(&vm.stack, vm.smem, i);
        if (err != 0) {
            printf("vm init failed\n");
            return -1;
        }
    }

    memory_print(vm.smem, 32);

    ProgramWriter pw;
    err = program_init_writer(&pw);
    if (err != 0) {
        printf("ProgramWriter init failed\n");
        return -1;
    }

    program_write_instr(&pw, instr_push(value_numi(3)));
    program_write_instr(&pw, instr_push(value_numi(2)));
    program_write_instr(&pw, instr_binop(BINOP_IADD));

    Program p;
    program_finish(&pw, &p);

    program_print(p);

    return 0;
}
