#include "util.h"
#include "lang.h"
#include "ct_int.h"

SLICE_METHODS_IMPL(ct_opdeclslice, OperatorDecl)

BinopType _ct_find_binop_type(OperatorVariant variant, PrimitiveType operand_type) {
    switch (variant) {
    case OperatorVariant_BINARY_ADD:
        return operand_type == PrimitiveType_I32 ? BINOP_IADD : BINOP_FADD;

    case OperatorVariant_BINARY_SUB:
        return operand_type == PrimitiveType_I32 ? BINOP_ISUB : BINOP_FSUB;

    case OperatorVariant_BINARY_MUL:
        return operand_type == PrimitiveType_I32 ? BINOP_IMUL : BINOP_FMUL;

    case OperatorVariant_BINARY_DIV:
        return operand_type == PrimitiveType_I32 ? BINOP_IDIV : BINOP_FDIV;

    case OperatorVariant_BINARY_LT:
        return operand_type == PrimitiveType_I32 ? BINOP_ILT : BINOP_FLT;

    case OperatorVariant_BINARY_LTE:
        return operand_type == PrimitiveType_I32 ? BINOP_ILTE : BINOP_FLTE;

    case OperatorVariant_BINARY_GT:
        return operand_type == PrimitiveType_I32 ? BINOP_IGT : BINOP_FGT;

    case OperatorVariant_BINARY_GTE:
        return operand_type == PrimitiveType_I32 ? BINOP_IGTE : BINOP_FGTE;

    case OperatorVariant_BINARY_POWER: return BINOP_INVALID;

    case OperatorVariant_BINARY_EQ:  return BINOP_EQ;
    case OperatorVariant_BINARY_NEQ: return BINOP_NEQ;

    case OperatorVariant_BINARY_AND: return BINOP_BAND;
    case OperatorVariant_BINARY_OR: return BINOP_BOR;

    default: return BINOP_INVALID;
    }
}

UnopType _ct_find_unop_type(OperatorVariant variant, PrimitiveType operand_type, PrimitiveType result_type) {
    switch (variant) {
    case OperatorVariant_UNARY_MINUS:
        return operand_type == PrimitiveType_I32 ? UNOP_INEG : UNOP_FNEG;

    case OperatorVariant_UNARY_NOT:
        return UNOP_BNOT;

    case OperatorVariant_UNARY_CAST:
        if (operand_type == PrimitiveType_I32 && result_type == PrimitiveType_F32) {
            return UNOP_ITOF;
        }
        if (operand_type == PrimitiveType_F32 && result_type == PrimitiveType_I32) {
            return UNOP_FTOI;
        }

    default: return UNOP_INVALID;
    }
}
