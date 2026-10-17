#include "lang.h"
#include "ct_int.h"

void _ct_bytecode_add_diagnostic(Compiler *c, Diagnostic d) {
    d.source = DiagnosticSource_COMPILER;
    ct_diagnostic_array_append(&c->diagnostics, d);
}

uint32_t _ct_bytecode_encode_literal_32(ASTNode *literal) {
    switch (literal->type) {
    case ASTNodeType_LINT:      return *((uint32_t*) &literal->data.lint);
    case ASTNodeType_LFLOAT:    return *((uint32_t*) &literal->data.lfloat);

    default: return 0;
    }
}

void _ct_bytecode_write_expr(ProgramWriter *pw, ASTNode *expr, Compiler *c);

void _ct_bytecode_write_fncall(ProgramWriter *pw, ASTNode *fncall, Compiler *c) {
    ASTNode *fn = fncall->first_child;

    if (fn->value_type->kind != LmbProgramTypeKind_IO) {
        _ct_bytecode_add_diagnostic(c, (Diagnostic) {
            .message = STR_CONST("Only supporting io for now"),
            .severity = DiagnosticSeverity_ERROR,
            .location = fn->range,
        });
        return;
    }

    for (ASTNode *arg = fn->next_sibling; arg != 0; arg = arg->next_sibling) {
        _ct_bytecode_write_expr(pw, arg, c);
    }

    lang_program_write_instr(pw, (Instruction) { .type = I_IOC1, .data = { 0 } });
}

void _ct_bytecode_write_binop(ProgramWriter *pw, ASTNode *expr, Compiler *c) {
    ASTNode *left_operand = expr->first_child;
    ASTNode *right_operand = expr->first_child->next_sibling;

    _ct_bytecode_write_expr(pw, left_operand, c);
    _ct_bytecode_write_expr(pw, right_operand, c);

    BinopType binop = _ct_find_binop_type(
        expr->data.binop.variant,
        left_operand->value_type->data.primitive
    );
    lang_program_write_instr(pw, (Instruction) { .type = I_BINOP, .data = { .binop = binop } });
}

void _ct_bytecode_write_unop(ProgramWriter *pw, ASTNode *expr, Compiler *c) {
    ASTNode *operand = expr->first_child;
    _ct_bytecode_write_expr(pw, operand, c);

    UnopType unop = _ct_find_unop_type(
        expr->data.unop.variant,
        operand->value_type->data.primitive, expr->value_type->data.primitive
    );
    lang_program_write_instr(pw, (Instruction) { .type = I_UNOP, .data = { .unop = unop } });
}

void _ct_bytecode_write_expr(ProgramWriter *pw, ASTNode *expr, Compiler *c) {
    switch (expr->type) {
    case ASTNodeType_FNCALL: _ct_bytecode_write_fncall(pw, expr, c); break;
    case ASTNodeType_BINOP: _ct_bytecode_write_binop(pw, expr, c); break;
    case ASTNodeType_UNOP: _ct_bytecode_write_unop(pw, expr, c); break;

    case ASTNodeType_LINT:
    case ASTNodeType_LFLOAT:
        lang_program_write_instr(pw, (Instruction) {
            .type = I_PUSH32,
            .data = { .push32 = _ct_bytecode_encode_literal_32(expr) },
        });
    break;

    default:
        _ct_bytecode_add_diagnostic(c, (Diagnostic) {
            .message = STR_CONST("Cannot compile expression"),
            .severity = DiagnosticSeverity_ERROR,
            .location = expr->range,
        });
        return;
    }
}

void ct_run_bytecoding(Compiler *c, ASTNode *program_root, Program *into) {
    ProgramWriter pw = { 0 };
    lang_program_init_writer(&pw);

    ASTNode *entry = program_root->first_child;
    while (entry != 0 && entry->type != ASTNodeType_ENTRY) {
        entry = entry->next_sibling;
    }

    if (entry == 0 || entry->first_child == 0) {
        _ct_bytecode_add_diagnostic(c, (Diagnostic) {
            .message = STR_CONST("Could not find an entry"),
            .severity = DiagnosticSeverity_ERROR,
        });
        return;
    }

    ASTNode *entry_lambda = entry->first_child;
    ASTNode *entry_body = entry_lambda->data.lambda.body;
    if (entry_body == 0) {
        _ct_bytecode_add_diagnostic(c, (Diagnostic) {
            .message = STR_CONST("Entry does not have a valid body"),
            .severity = DiagnosticSeverity_ERROR,
            .location = entry->first_child->range,
        });
        return;
    }

    _ct_bytecode_write_expr(&pw, entry_body, c);

    lang_program_finish(&pw, into);
}
