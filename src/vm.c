#include <stdio.h>
#include <stddef.h>
#include "lang.h"

typedef struct Machine {
    memory_t mem;
    stack_t vars;
    stack_t stack;
} vm_t;

typedef struct VMConfig {
    size_t stack_size;
    size_t stack_cap;

    size_t vars_size;
    size_t max_vars;
} vmconfig_t;

int vm_init(vm_t *vm, vmconfig_t cfg) {
    size_t total = cfg.stack_size + cfg.vars_size;
    value_t *start = malloc(total * sizeof(value_t));
    if (start == 0) {
        return -1;
    }

    vm->mem.start = start;
    vm->mem.end = start + total;

    memory_t smem;
    smem.start = start + cfg.vars_size;
    smem.end = start + total;
    int err = stack_init(&vm->stack, cfg.stack_cap);
    if (err != 0) { return err; }

    memory_t vmem;
    vmem.start = start;
    vmem.end = smem.start;
    err = stack_init(&vm->vars, cfg.max_vars);
    return err;
}

int main() {
    vmconfig_t cfg;
    cfg.stack_size = 1024;
    cfg.stack_cap = 1024;
    cfg.vars_size = 512;
    cfg.max_vars = 256;

    vm_t vm;
    int err = vm_init(&vm, cfg);
    if (err != 0) {
        printf("vm init failed\n");
        return -1;
    }

    printf("vm initialized\n");

    err = stack_push(&vm.stack, vm.mem.start);

    return 0;
}
