#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "str.h"
#include "value.h"
#include "ops.h"
#include "program.h"

typedef enum VMState {
    VMState_READY,
    VMState_RUNNING,
    VMState_HALTED,
    VMState_CRASHED,
} VMState;

typedef struct Machine {
    Memory vmem;
    Memory smem;
    MemPtr stack_ptr;

    InstructionPtr current;
    VMState state;
} VMachine;

typedef struct VMConfig {
    size_t stack_size;
    size_t stack_cap;

    size_t vars_size;
    size_t max_vars;
} VMConfig;

error vm_init(VMachine *vm, VMConfig cfg) {
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
    vm->stack_ptr = 0;

    vm->current = 0;
    vm->state = VMState_READY;

    return E_NONE;
}

error vm_stack_push(VMachine *vm, Value value) {
    size_t size = memory_write(vm->smem, vm->stack_ptr, value);
    if (size == 0) {
        return E_OUT_OF_RANGE;
    }

    vm->stack_ptr += size;
    return E_NONE;
}

error vm_stack_pop(VMachine *vm, Value *into) {
    size_t data_size = value_get_data_size(into->type);
    if (vm->stack_ptr < data_size) {
        return E_OUT_OF_RANGE;
    }

    vm->stack_ptr -= data_size;
    size_t bytes_read = memory_read(vm->smem, vm->stack_ptr, into);
    if (bytes_read != data_size) {
        return E_UNKNOWN;
    }

    return E_NONE;
}

#define BINOP_HANDLE(LTYPE, RTYPE, RSETFUN, RESULT) \
    left.type = LTYPE; \
    right.type = RTYPE; \
    ERR_PASS( vm_stack_pop(vm, &right) ) \
    ERR_PASS( vm_stack_pop(vm, &left) ) \
    value_set_##RSETFUN(&result, (RESULT));

error vm_exec_binop(VMachine *vm, IBinopData op) {
    ERR_DECL

    Value left, right;
    Value result = { .type = V_NULL };

    switch (op) {
    case BINOP_IADD:
        BINOP_HANDLE(V_NUMI, V_NUMI, numi, left.data.numi + right.data.numi)
        break;
    case BINOP_IMUL:
        BINOP_HANDLE(V_NUMI, V_NUMI, numi, left.data.numi * right.data.numi)
        break;
    case BINOP_ISUB:
        BINOP_HANDLE(V_NUMI, V_NUMI, numi, left.data.numi - right.data.numi)
        break;
    case BINOP_IDIV:
        BINOP_HANDLE(V_NUMI, V_NUMI, numi, left.data.numi / right.data.numi)
        break;
    case BINOP_IREM:
        BINOP_HANDLE(V_NUMI, V_NUMI, numi, left.data.numi % right.data.numi)
        break;
    case BINOP_IGT:
        BINOP_HANDLE(V_NUMI, V_NUMI, boolv, left.data.numi > right.data.numi)
        break;
    case BINOP_ILT:
        BINOP_HANDLE(V_NUMI, V_NUMI, boolv, left.data.numi < right.data.numi)
        break;
    case BINOP_IGTE:
        BINOP_HANDLE(V_NUMI, V_NUMI, boolv, left.data.numi >= right.data.numi)
        break;
    case BINOP_ILTE:
        BINOP_HANDLE(V_NUMI, V_NUMI, boolv, left.data.numi <= right.data.numi)
        break;

    default:
        return E_BAD_DATA;
    }

    printf("LEFT: ");
    value_print(left);
    printf(" / RIGHT: ");
    value_print(right);
    printf("\nOP RESULT: ");
    value_print(result);
    printf("\n");
    return vm_stack_push(vm, result);
}
#undef BINOP_HANDLE

#define UNOP_HANDLE(ARG_TYPE, RSETFUN, RESULT) \
    arg.type = ARG_TYPE; \
    ERR_PASS( vm_stack_pop(vm, &result) ) \
    value_set_##RSETFUN(&result, RESULT);

error vm_exec_unop(VMachine *vm, IUnopData op) {
    ERR_DECL

    Value arg;
    Value result = { .type = V_NULL };

    switch (op) {
    case UNOP_BNOT:
        UNOP_HANDLE(V_BOOL, boolv, !arg.data.boolv)
        break;
    case UNOP_INEG:
        UNOP_HANDLE(V_NUMI, numi, -arg.data.numi)
        break;
    case UNOP_FNEG:
        UNOP_HANDLE(V_NUMF, numf, -arg.data.numf)
        break;

    case UNOP_ITOF:
        UNOP_HANDLE(V_NUMI, numf, (float)arg.data.numi)
        break;
    case UNOP_ITOB:
        UNOP_HANDLE(V_NUMI, boolv, arg.data.numi != 0)
        break;
    case UNOP_FTOB:
        UNOP_HANDLE(V_NUMF, boolv, arg.data.numf != 0.0)
        break;

    default:
        return E_BAD_DATA;
    }

    return vm_stack_push(vm, result);
}
#undef UNOP_HANDLE

error vm_exec_instr(VMachine *vm, Instruction instr) {
    switch (instr.type) {
    case I_HALT:
        vm->state = VMState_HALTED;
        return E_NONE;

    case I_PUSH:
        return vm_stack_push(vm, instr.data.push);

    case I_POP: {
        Value discarded = { .type = instr.data.pop };
        return vm_stack_pop(vm, &discarded);
    }

    case I_BINOP:
        return vm_exec_binop(vm, instr.data.binop);

    case I_UNOP:
        return vm_exec_unop(vm, instr.data.unop);

    case I_JMP:
        return E_BAD_DATA;

    case I_JMPZ:
        return E_BAD_DATA;

    case I_MEMR:
        return E_BAD_DATA;

    case I_MEMW:
        return E_BAD_DATA;

    default:
        return E_BAD_DATA;
    }
}

error vm_execute(VMachine *vm, Program p) {
    ERR_DECL

    Instruction instr;

    vm->current = 0;
    vm->state = VMState_RUNNING;

    while (vm->current < p.length && vm->state != VMState_HALTED) {
        size_t ilen = program_read_instr(p, vm->current, &instr);
        if (ilen == 0) {
            return E_UNKNOWN;
        }

        vm->current += ilen;

        err = vm_exec_instr(vm, instr);
        if (err != E_NONE) {
            vm->state = VMState_CRASHED;
        }
    }

    vm->state = VMState_HALTED;
    return E_NONE;
}

#endif
