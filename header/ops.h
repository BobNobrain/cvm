#ifndef OPS_H
#define OPS_H

#include "str.h"

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

const char DEBUG_STR_BINOPS[] = \
    "==\0<>\0" \
    "+i\0*i\0-i\0/i\0%i\0" \
    ">i\0<i\0>=\0<=\0" \
    "+f\0*f\0-f\0/f\0" \
    ">f\0<f\0>=\0<=\0" \
    "&&\0||\0" \
;

error binop_to_string(BinopType t, StringWriter *sw) {
    const size_t binop_str_len = 3;
    size_t offset = (size_t)t * binop_str_len * sizeof(char);
    return strw_appendc(sw, &DEBUG_STR_BINOPS[offset]);
}

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

const char DEBUG_STR_UNOPS[] = "!\0\0-i\0-f\0if\0ib\0fb\0";

error unop_to_string(UnopType t, StringWriter *sw) {
    const size_t unop_str_len = 3;
    size_t offset = (size_t)t * unop_str_len * sizeof(char);
    return strw_appendc(sw, &DEBUG_STR_UNOPS[offset]);
}

#endif
