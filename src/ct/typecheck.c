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
    size_t n_types_assigned;
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

TypecheckerCtx _ct_typechecker_create_ctx(TypecheckerCtx *outer) {
    TypecheckerCtx inner = {
        .typechecker = outer->typechecker,
        .table = { .symbols = { 0 }, .parent = &outer->table },
    };
    _ct_symarray_init(&inner.table.symbols, 64, outer->typechecker->arena);
    return inner;
}

void _ct_typechecker_set_node_type(Typechecker *typechecker, ASTNode *node, LmbProgramType *type) {
    if (type == 0) { return; }

    if (node->value_type != 0) {
        if (node->value_type == LmbProgramTypeKind_INVALID) { return; }
        if (_ct_types_are_equal(node->value_type, type)) { return; }

        node->value_type = _ct_type_get_invalid();
        _ct_typechecker_add_diagnostic(typechecker, (Diagnostic) {
            .location = node->range,
            .message = STR_CONST("conflicting node types"),
            .severity = DiagnosticSeverity_ERROR,
        });
        return;
    }

    typechecker->n_types_assigned += 1;
    node->value_type = type;
}


void _ct_assign_types(ASTNode *node, TypecheckerCtx *ctx);

void _ct_assign_children_types(ASTNode *node, TypecheckerCtx *ctx) {
    ASTNode *it = node->first_child;
    while (it != 0) {
        _ct_assign_types(it, ctx);
        it = it->next_sibling;
    }
}

void _ct_assign_ident_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }

    LmbProgramType *found_type = _ct_symtable_find_type(&ctx->table, node->data.ident.name);
    if (found_type != 0) {
        _ct_typechecker_set_node_type(ctx->typechecker, node, found_type);
    }
}
void _ct_assign_literal_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }

    switch (node->type) {
    // TODO: bools
    case ASTNodeType_LBOOL:
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
    break;
    case ASTNodeType_LINT:
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
    break;
    case ASTNodeType_LFLOAT:
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_F32));
    break;

    default: break;
    }
}

void _ct_assign_binop_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }

    _ct_assign_children_types(node, ctx);
    if (ct_astnode_is_error(node->first_child)) { return; }
    if (ct_astnode_is_error(node->first_child->next_sibling)) { return; }

    LmbProgramType *ltype = node->first_child->value_type;
    LmbProgramType *rtype = node->first_child->next_sibling->value_type;

    // non-primitives are not supported for operators yet
    if (ltype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return;
    }
    if (rtype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return;
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
            _ct_typechecker_set_node_type(ctx->typechecker, node, max_fitting);
            break;

        case OperatorVariant_BINARY_REM:
            if (max_fitting->data.primitive == PrimitiveType_F32) {
                _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_invalid());
                _ct_typechecker_add_diagnostic(ctx->typechecker, (Diagnostic) {
                    .location = node->base->range,
                    .message = STR_CONST("modulo remainder is not supported for floating point values"),
                    .severity= DiagnosticSeverity_ERROR,
                });
                break;
            }
            _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
            break;

        case OperatorVariant_BINARY_LT:
        case OperatorVariant_BINARY_GT:
        case OperatorVariant_BINARY_LTE:
        case OperatorVariant_BINARY_GTE:
        case OperatorVariant_BINARY_EQ:
        case OperatorVariant_BINARY_NEQ:
            // no bools yet
            _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
            break;

        case OperatorVariant_BINARY_AND:
        case OperatorVariant_BINARY_OR:
            // no bools yet
            _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
            break;
    }
}

void _ct_assign_fncall_types(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }

    _ct_assign_children_types(node, ctx);
    ASTNode *fn = node->first_child;
    if (fn == 0 || fn->value_type == 0) {
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_invalid());
        return;
    }

    switch (fn->value_type->kind) {
    case LmbProgramTypeKind_IO:
        // pretending like io always returns i32
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
    break;

    case LmbProgramTypeKind_ARROW:
        // TODO: typecheck arguments
        _ct_typechecker_set_node_type(ctx->typechecker, node, fn->value_type->data.arrow.ret_type);
    break;

    default:
        _ct_typechecker_add_diagnostic(ctx->typechecker, (Diagnostic) {
            .location = fn->range,
            .message = STR_CONST("must be a function to be called"),
            .severity = DiagnosticSeverity_ERROR,
        });
    break;
    }
}

void _ct_assign_lambda_arg_type(ASTNode *node, TypecheckerCtx *ctx) {
    // TODO: data.lambda_arg.type should instead be a subtree
    if (str_eq(node->data.lambda_arg.type, STR_CONST("i32"))) {
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_I32));
        return;
    }

    if (str_eq(node->data.lambda_arg.type, STR_CONST("f32"))) {
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_primitive(PrimitiveType_F32));
        return;
    }

    if (str_eq(node->data.lambda_arg.type, STR_CONST("io"))) {
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_io());
        return;
    }

    _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_invalid());
    _ct_typechecker_add_diagnostic(ctx->typechecker, (Diagnostic) {
        .location = node->range,
        .message = STR_CONST("argument type must be set"),
        .severity = DiagnosticSeverity_ERROR,
    });
}

void _ct_assign_lambda_types(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }
    if (node->first_child == 0) { return; }

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
    TypecheckerCtx inner_ctx = _ct_typechecker_create_ctx(ctx);
    for (ASTNode *child = node->first_child; child != 0; child = child->next_sibling, ++i) {
        ASTNode *arg = child;

        if (arg->type != ASTNodeType_LAMBDA_ARG) {
            continue;
        }

        if (arg->value_type == 0) {
            _ct_assign_lambda_arg_type(arg, ctx);
        }

        _ct_symtable_register(&inner_ctx.table, arg->data.lambda_arg.name, arg);
        arg_types[i] = arg->value_type;
    }

    ASTNode *lambda_body = node->data.lambda.body;
    _ct_assign_types(lambda_body, &inner_ctx);

    if (!ct_astnode_is_error(lambda_body) && lambda_body->type != ASTNodeType_LAMBDA_ARG) {
        LmbProgramType *ret_type = lambda_body->value_type;

        if (ret_type != 0) {
            lambda_type->data.arrow.ret_type = ret_type;
        }
    }

    _ct_typechecker_set_node_type(ctx->typechecker, node, lambda_type);
    return;
}

void _ct_assign_assignment_types(ASTNode *node, TypecheckerCtx *ctx) {
    _ct_assign_children_types(node, ctx);
    _ct_typechecker_set_node_type(ctx->typechecker, node, node->first_child->value_type);
    if (node->value_type != 0) {
        _ct_symtable_register(&ctx->table, node->data.assignment.identifier, node);
    }
}

void _ct_assign_entry_type(ASTNode *node, TypecheckerCtx *ctx) {
    if (node->value_type != 0) { return; }
    if (node->first_child == 0) { return; }

    ASTNode *entry_lambda = node->first_child;
    if (entry_lambda->type != ASTNodeType_LAMBDA || entry_lambda->data.lambda.n_args != 1) {
        _ct_typechecker_add_diagnostic(ctx->typechecker, (Diagnostic) {
            .location = node->base->range,
            .message = STR_CONST("entry must be a function with a single argument"),
            .severity = DiagnosticSeverity_ERROR,
        });
        _ct_typechecker_set_node_type(ctx->typechecker, node, _ct_type_get_invalid());
        return;
    }

    ASTNode *only_arg = entry_lambda->first_child;
    while (only_arg != 0) {
        if (only_arg->type == ASTNodeType_LAMBDA_ARG) { break; }
        only_arg = only_arg->next_sibling;
    }

    if (ct_astnode_is_error(only_arg)) { return; }
    if (only_arg->value_type == 0) {
        _ct_typechecker_set_node_type(ctx->typechecker, only_arg, _ct_type_get_io());
    }

    _ct_assign_lambda_types(entry_lambda, ctx);
    _ct_typechecker_set_node_type(ctx->typechecker, node, entry_lambda->value_type);
}

void _ct_assign_types(ASTNode *node, TypecheckerCtx *ctx) {
    if (ct_astnode_is_error(node)) { return; }

    switch (node->type) {
    case ASTNodeType_LMB_FILE:   _ct_assign_children_types(node, ctx); break;
    case ASTNodeType_ENTRY:      _ct_assign_entry_type(node, ctx); break;
    case ASTNodeType_IDENT:      _ct_assign_ident_type(node, ctx); break;
    case ASTNodeType_BINOP:      _ct_assign_binop_type(node, ctx); break;
    case ASTNodeType_FNCALL:     _ct_assign_fncall_types(node, ctx); break;
    case ASTNodeType_ASSIGNMENT: _ct_assign_assignment_types(node, ctx); break;
    case ASTNodeType_LAMBDA:     _ct_assign_lambda_types(node, ctx); break;

    case ASTNodeType_UNOP:
        _ct_assign_children_types(node, ctx);
        if (node->first_child != 0 && node->value_type == 0) {
            // TODO: this is not always the case
            _ct_typechecker_set_node_type(ctx->typechecker, node, node->first_child->value_type);
        }
    break;

    case ASTNodeType_LBOOL:
    case ASTNodeType_LINT:
    case ASTNodeType_LFLOAT:
        _ct_assign_literal_type(node, ctx);
    break;

    default: break;
    }
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
        .n_types_assigned = 0,
    };
    root_ctx.typechecker = &typechecker;

    // TODO: better algorithm – build a dependency tree first
    do {
        typechecker.n_types_assigned = 0;
        _ct_assign_types(p->root, &root_ctx);
        // loop until cannot resolve anymore types
        printf("next type inference iteration: %zu new types assigned\n", typechecker.n_types_assigned);
    } while (typechecker.n_types_assigned > 0);
}
