#ifndef RT_H
#define RT_H
/**
 * Runtime library
 */

#include <stddef.h>
#include <stdint.h>
#include "util.h"
#include "lang.h"


/** Virtual machine memory */
#define MEMORY_TYPES_LIST(X) \
    X(bool, BoolValue) \
    X(numi, NumIValue) \
    X(numf, NumFValue)

typedef struct Memory {
    uint8_t *content;
    size_t length;
} Memory;

extern void rt_memory_init(Memory *mem);
extern size_t rt_memory_read(Memory mem, MemPtr at, Value *into);
extern size_t rt_memory_write(Memory mem, MemPtr at, Value value);
extern void rt_memory_print(Memory mem, size_t max);

#define DECLARE_MEMORY_READ_FN(SUFFIX, VALUE_TYPE) \
    extern size_t rt_memory_read_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE *into);

MEMORY_TYPES_LIST(DECLARE_MEMORY_READ_FN)
#undef DECLARE_MEMORY_READ_FN

#define DECLARE_MEMORY_WRITE_FN(SUFFIX, VALUE_TYPE) \
    extern size_t rt_memory_write_##SUFFIX (Memory mem, MemPtr at, VALUE_TYPE value);

MEMORY_TYPES_LIST(DECLARE_MEMORY_WRITE_FN)
#undef DECLARE_MEMORY_WRITE_FN

/** Virtual machine itself */
typedef enum {
    VMState_READY,
    VMState_RUNNING,
    VMState_HALTED,
    VMState_CRASHED,
} VMState;

typedef struct {
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

extern void rt_vm_init(VMachine *vm, VMConfig cfg);
extern error rt_vm_stack_push(VMachine *vm, Value value);
extern error rt_vm_stack_pop(VMachine *vm, Value *into);
extern error rt_vm_exec_instr(VMachine *vm, Instruction instr);
extern error rt_vm_execute(VMachine *vm, Program p);


/** Hiding all internal macros */
#ifndef RT_INTERNAL
#undef MEMORY_TYPES_LIST
#endif

#endif
