#include "ct_int.h"

typedef struct ProgramSymbol {
    String name;
    ASTNode *source;
} ProgramSymbol;

ARRAY_DECL(ProgramSymbol)
ARRAY_METHODS_IMPL(_ct_symarray, ProgramSymbol)

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
    ProgramSymbolArray symbols;
    SymbolTable *parent;
};

typedef struct Typechecker {
    SymbolTable *root_table;
    Arena *arena;
    DiagnosticArray *diagnostics;
} Typechecker;

typedef struct TypecheckerCtx {
    Typechecker *typechecker;
    SymbolTable table;
} TypecheckerCtx;

void _ct_typechecker_add_diagnostic(Typechecker *typechecker, Diagnostic d) {
    d.source = DiagnosticSource_TYPECHECK;
    ct_diagnostic_array_append(typechecker->diagnostics, d);
}

LmbProgramType* _ct_symtable_find_type(SymbolTable *table, String target) {
    SymbolTable *current = table;
    while (current != 0) {
        for (size_t i = 0; i < current->symbols.size; i++) {
            if (str_eq(current->symbols.content[i].name, target)) {
                // found a match
                ASTNode *node = current->symbols.content[i].source;
                if (node != 0) { return node->value_type; }
                return 0;
            }
        }

        // no match in current table, must jump up a level
        current = current->parent;
    }

    return 0;
}

bool _ct_symtable_register(SymbolTable *table, String name, ASTNode *source) {
    for (size_t i = 0; i < table->symbols.size; i++) {
        if (str_eq(table->symbols.content[i].name, name)) {
            // already registered
            return false;
        }
    }
    _ct_symarray_append(&table->symbols, (ProgramSymbol) { .name = name, .source = source });
    return true;
}

TypecheckerCtx _ct_typecheck_ctx_create(TypecheckerCtx *outer) {
    TypecheckerCtx inner = {
        .typechecker = outer->typechecker,
        .table = { .symbols = { 0 }, .parent = &outer->table },
    };
    _ct_symarray_init(&inner.table.symbols, 64, outer->typechecker->arena);
    return inner;
}

size_t _ct_assign_types(ASTNode *node, TypecheckerCtx *ctx);

size_t _ct_assign_children_types(ASTNode *node, TypecheckerCtx *ctx) {
    size_t n_total = 0;
    ASTNode *it = node->first_child;
    while (it != 0) {
        n_total += _ct_assign_types(it, ctx);
        it = it->next_sibling;
    }
    return n_total;
}

size_t _ct_assign_ident_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return 0; }

    LmbProgramType *found_type = _ct_symtable_find_type(&ctx->table, node->data.ident.name);
    if (found_type != 0) {
        node->value_type = found_type;
        return 1;
    }
    return 0;
}
size_t _ct_assign_literal_type(ASTNode *node, TypecheckerCtx *ctx) {
    (void) ctx;
    if (node->value_type != 0) { return 0; }

    switch (node->type) {
    // TODO: bools
    case ASTNodeType_LBOOL:  node->value_type = _ct_type_get_primitive(PrimitiveType_I32); return 1;
    case ASTNodeType_LINT:   node->value_type = _ct_type_get_primitive(PrimitiveType_I32); return 1;
    case ASTNodeType_LFLOAT: node->value_type = _ct_type_get_primitive(PrimitiveType_F32); return 1;
        default: return 0;
    }
}

size_t _ct_assign_binop_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return 0; }

    size_t n_assigned = _ct_assign_children_types(node, ctx);
    if (ct_astnode_count_children(node) != 2) {
        return n_assigned;
    }

    LmbProgramType *ltype = node->first_child->value_type;
    LmbProgramType *rtype = node->first_child->next_sibling->value_type;

    // non-primitives are not supported for operators yet
    if (ltype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return n_assigned;
    }
    if (rtype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return n_assigned;
    }

    LmbProgramType *max_fitting = _ct_type_get_primitive(PrimitiveType_I32);
    if (ltype->data.primitive == PrimitiveType_F32 || rtype->data.primitive == PrimitiveType_F32) {
        max_fitting = _ct_type_get_primitive(PrimitiveType_F32);
    }

    switch (node->data.binop.variant) {
        case OperatorVariant_BINARY_ADD:
        case OperatorVariant_BINARY_SUB:
        case OperatorVariant_BINARY_MUL:
        case OperatorVariant_BINARY_DIV:
        case OperatorVariant_BINARY_POWER:
            node->value_type = max_fitting;
            n_assigned += 1;
            break;

        case OperatorVariant_BINARY_REM:
            if (max_fitting->data.primitive == PrimitiveType_F32) {
                // no modulo remainder for floats
                break;
            }
            node->value_type = _ct_type_get_primitive(PrimitiveType_I32);
            n_assigned += 1;
            break;

        case OperatorVariant_BINARY_LT:
        case OperatorVariant_BINARY_GT:
        case OperatorVariant_BINARY_LTE:
        case OperatorVariant_BINARY_GTE:
        case OperatorVariant_BINARY_EQ:
        case OperatorVariant_BINARY_NEQ:
            // no bools yet
            node->value_type = _ct_type_get_primitive(PrimitiveType_I32);
            n_assigned += 1;
            break;

        case OperatorVariant_BINARY_AND:
        case OperatorVariant_BINARY_OR:
            // no bools yet
            node->value_type = _ct_type_get_primitive(PrimitiveType_I32);
            n_assigned += 1;
            break;
    }

    return n_assigned;
}

size_t _ct_assign_lambda_arg_type(ASTNode *node, TypecheckerCtx *ctx) {
    (void) ctx; // will most probably be needed later
    // TODO: data.lambda_arg.type should instead be a subtree
    if (str_eq(node->data.lambda_arg.type, STR_CONST("i32"))) {
        node->value_type = _ct_type_get_primitive(PrimitiveType_I32);
        return 1;
    }

    if (str_eq(node->data.lambda_arg.type, STR_CONST("f32"))) {
        node->value_type = _ct_type_get_primitive(PrimitiveType_F32);
        return 1;
    }

    return 0;
}

size_t _ct_assign_lambda_types(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return 0; }
    if (node->first_child == 0) { return 0; }

    size_t n_assigned = 0;
    LmbProgramType *lambda_type = arena_alloc(ctx->typechecker->arena, sizeof(LmbProgramType));
    LmbProgramType **arg_types = arena_alloc(
        ctx->typechecker->arena,
        sizeof(LmbProgramType*) * node->data.lambda.n_args
    );

    *lambda_type = (LmbProgramType) {
        .kind = LmbProgramTypeKind_ARROW,
        .data = { .arrow = { .n_args = node->data.lambda.n_args, .arg_types = arg_types, .ret_type = 0 } },
    };

    size_t i = 0;
    TypecheckerCtx inner_ctx = _ct_typecheck_ctx_create(ctx);
    for (ASTNode *child = node->first_child; child != 0; child = child->next_sibling, ++i) {
        ASTNode *arg = child;

        if (arg->type != ASTNodeType_LAMBDA_ARG) {
            continue;
        }

        if (!arg->value_type) {
            if (_ct_assign_lambda_arg_type(arg, ctx) == 0) {
                arg_types[i] = 0;
                // could not set arg type
                continue;
            }

            n_assigned += 1;
        }

        _ct_symtable_register(&inner_ctx.table, arg->data.lambda_arg.name, arg);
        arg_types[i] = arg->value_type;
    }

    n_assigned += _ct_assign_children_types(node, &inner_ctx);

    ASTNode *lambda_body = node->data.lambda.body;
    if (!ct_astnode_is_error(lambda_body) && lambda_body->type != ASTNodeType_LAMBDA_ARG) {
        LmbProgramType *ret_type = lambda_body->value_type;

        if (ret_type != 0) {
            lambda_type->data.arrow.ret_type = ret_type;
        }
    }

    node->value_type = lambda_type;
    n_assigned += 1;
    return n_assigned;
}

size_t _ct_assign_assignment_types(ASTNode *node, TypecheckerCtx *ctx) {
    size_t n_assigned = _ct_assign_children_types(node, ctx);
    if (n_assigned > 0) {
        if (node->value_type == 0) {
            n_assigned += 1;
        }

        node->value_type = node->first_child->value_type;

        if (node->value_type != 0) {
            _ct_symtable_register(&ctx->table, node->data.assignment.identifier, node);
        }
    }

    return n_assigned;
}

size_t _ct_assign_entry_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return 0; }
    if (node->first_child == 0) { return 0; }

    ASTNode *entry_lambda = node->first_child;
    if (entry_lambda->type != ASTNodeType_LAMBDA || entry_lambda->data.lambda.n_args != 1) {
        _ct_typechecker_add_diagnostic(ctx->typechecker, (Diagnostic) {
            .location = node->base->range,
            .message = STR_CONST("entry must be a function with a single argument"),
            .severity = DiagnosticSeverity_ERROR,
        });
        return 0;
    }

    ASTNode *only_arg = entry_lambda->first_child;
    while (only_arg != 0) {
        if (only_arg->type == ASTNodeType_LAMBDA_ARG) { break; }
        only_arg = only_arg->next_sibling;
    }

    size_t n_assigned = 0;

    if (ct_astnode_is_error(only_arg)) { return n_assigned; }
    if (only_arg->value_type == 0) {
        only_arg->value_type = _ct_type_get_io();
        n_assigned += 1;
    }

    n_assigned += _ct_assign_lambda_types(entry_lambda, ctx);
    node->value_type = entry_lambda->value_type;

    return n_assigned;
}

size_t _ct_assign_types(ASTNode *node, TypecheckerCtx *ctx) {
    if (node == 0) { return 0; }

    size_t n_assigned = 0;

    switch (node->type) {
    case ASTNodeType_LMB_FILE: return _ct_assign_children_types(node, ctx);
    case ASTNodeType_ENTRY: return _ct_assign_entry_type(node, ctx);
    case ASTNodeType_IDENT: return _ct_assign_ident_type(node, ctx);
    case ASTNodeType_BINOP: return _ct_assign_binop_type(node, ctx);

    case ASTNodeType_ASSIGNMENT: // must also write into the symtable
        n_assigned += _ct_assign_assignment_types(node, ctx);
    break;

    case ASTNodeType_UNOP:
        n_assigned += _ct_assign_children_types(node, ctx);
        if (node->first_child != 0 && node->value_type == 0) {
            // TODO: this is not always the case
            node->value_type = node->first_child->value_type;
            n_assigned += 1;
        }
    break;

    case ASTNodeType_LAMBDA:
        n_assigned += _ct_assign_lambda_types(node, ctx);
    break;

    case ASTNodeType_LBOOL:
    case ASTNodeType_LINT:
    case ASTNodeType_LFLOAT:
        n_assigned += _ct_assign_literal_type(node, ctx);
    break;

    default: break;
    }

    return n_assigned;
}

void ct_parser_assign_types(Parser *p) {
    TypecheckerCtx root_ctx = {
        .typechecker = 0,
        .table = { .parent = 0, .symbols = { 0 } },
    };
    _ct_symarray_init(&root_ctx.table.symbols, 128, p->arena);

    Typechecker typechecker = {
        .arena = p->arena,
        .root_table = &root_ctx.table,
        .diagnostics = &p->diagnostics,
    };
    root_ctx.typechecker = &typechecker;

    // TODO: better algorithm – build a dependency tree first
    size_t n_assigned = 0;
    while ((n_assigned = _ct_assign_types(p->root, &root_ctx)) > 0) {
        printf("next type inference iteration: %zu new types assigned\n", n_assigned);
        // break;
        // loop until cannot resolve anymore types
    }
}
