#ifndef LANG_H
#define LANG_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
/**
 * Common language features
 */

/** Memory-related types */
typedef size_t MemPtr;


/** Value types */
#define VALUE_TYPE_LIST(X) \
    X(V_BOOL, BoolValue, boolv) \
    X(V_NUMI, NumIValue, numi) \
    X(V_NUMF, NumFValue, numf)

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum ValueType {
    V_NULL,
    VALUE_TYPE_LIST(VALUE_TYPE_X)

    V_INVALID
} ValueType;
#undef VALUE_TYPE_X

typedef bool BoolValue;
typedef int NumIValue;
typedef float NumFValue;

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
typedef struct {
    ValueType type;
    union {
        VALUE_TYPE_LIST(VALUE_TYPE_X)
    } data;
} Value;
#undef VALUE_TYPE_X

extern Value lang_value_make_bool(bool v);
extern Value lang_value_make_numi(int v);
extern Value lang_value_make_numf(float v);

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) \
    extern void lang_value_set_##FIELD_NAME(Value *into, DATA_TYPE v);

VALUE_TYPE_LIST(VALUE_TYPE_X)
#undef VALUE_TYPE_X

extern void lang_value_to_string(Value v, StringBuilder *sw);
extern size_t lang_value_get_data_size(ValueType type);
extern void lang_value_print(Value v);


/** Operators */
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
    BINOP_FGT,
    BINOP_FLT,
    BINOP_FGTE,
    BINOP_FLTE,

    // bool ops
    BINOP_BAND,
    BINOP_BOR,

    BINOP_INVALID
} BinopType;

typedef enum UnopType {
    UNOP_BNOT,
    UNOP_INEG,
    UNOP_FNEG,

    // coercion operators
    UNOP_ITOF,
    UNOP_ITOB,
    UNOP_FTOB,

    UNOP_INVALID
} UnopType;

extern void lang_binop_to_string(BinopType t, StringBuilder *sw);
extern void lang_unop_to_string(UnopType t, StringBuilder *sw);


/** Bytecode instructions */
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

typedef size_t InstructionPtr;

typedef Value IPushData;
typedef ValueType IPopData;
typedef BinopType IBinopData;
typedef UnopType IUnopData;
typedef InstructionPtr IJmpData;
typedef InstructionPtr IJmpzData;
typedef MemPtr IMemRData;
typedef MemPtr IMemWData;

typedef struct {
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

extern Instruction lang_instr_make_push(Value v);
extern Instruction lang_instr_make_pop(uint8_t count);
extern Instruction lang_instr_make_binop(BinopType op);
extern Instruction lang_instr_make_unop(UnopType op);
extern Instruction lang_instr_make_jmp(InstructionPtr to);
extern Instruction lang_instr_make_jmpz(InstructionPtr to);
extern Instruction lang_instr_make_memr(MemPtr to);
extern Instruction lang_instr_make_memw(MemPtr to);
extern size_t lang_instr_get_size(InstructionType i);
extern void lang_instr_to_string(Instruction instr, StringBuilder *sb);


/** Bytecode program and related things */
typedef struct {
    uint8_t *code;
    size_t length;
} Program;

extern size_t lang_program_read_instr(Program p, InstructionPtr at, Instruction *into);
extern void lang_program_print(Program p);

typedef struct {
    uint8_t *code;
    size_t length;
    size_t capacity;
} ProgramWriter;

extern void lang_program_init_writer(ProgramWriter *w);
extern size_t lang_program_write_instr(ProgramWriter *w, Instruction next);
extern void lang_program_finish(ProgramWriter *from, Program *into);


/** Hiding all internal macros */
#ifndef LANG_INTERNAL
#undef VALUE_TYPE_LIST
#endif

#endif // LANG_H
