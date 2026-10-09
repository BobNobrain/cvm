#include "ct_int.h"

typedef struct ProgramSymbol {
    String name;
    ASTNode *source;
} ProgramSymbol;

ARRAY_DECL_NAMED(ProgramSymbol, SymbolTable)
ARRAY_METHODS_IMPL_NAMED(_ct_symtable, ProgramSymbol, SymbolTable)

typedef struct ProgramTypeContext {
    SymbolTable symbols;
    Arena *arena;
} ProgramTypeContext;

LmbProgramType* _ct_symtable_find_type(ProgramTypeContext *ctx, String target) {
    for (size_t i = 0; i < ctx->symbols.size; i++) {
        // iterating from the end, so variable shadowing works properly
        size_t idx = ctx->symbols.size - i - 1;
        if (str_eq(ctx->symbols.content[idx].name, target)) {
            ASTNode *node = ctx->symbols.content[idx].source;
            if (node != 0) { return node->value_type; }
        }
    }

    return 0;
}

void _ct_symtable_register(ProgramTypeContext *ctx, String name, ASTNode *source) {
    _ct_symtable_append(&ctx->symbols, (ProgramSymbol) { .name = name, .source = source });
}

size_t _ct_assign_types(ASTNode *node, ProgramTypeContext *ctx);

void ct_type_print(LmbProgramType *type) {
    if (type == 0) {
        printf("?");
        return;
    }

    switch (type->kind) {
    case LmbProgramTypeKind_IO:
        printf("io");
        break;

    case LmbProgramTypeKind_PRIMITIVE:
        switch (type->data.primitive) {
            case PrimitiveType_I32: printf("i32"); break;
            case PrimitiveType_F32: printf("f32"); break;
            default: printf("??"); break;
        }
        break;

    case LmbProgramTypeKind_ARROW:
        printf("(");

        if (type->data.arrow.n_args > 0) {
            ct_type_print(type->data.arrow.arg_types[0]);
        }

        for (size_t i = 1; i < type->data.arrow.n_args; i++) {
            printf(", ");
            ct_type_print(type->data.arrow.arg_types[i]);
        }
        printf(") -> ");
        ct_type_print(type->data.arrow.ret_type);
        break;

    default:
        printf("??");
        break;
    }
}

static LmbProgramType PRIMITIVE_I32 = (LmbProgramType) {
    .kind = LmbProgramTypeKind_PRIMITIVE,
    .data = { .primitive = PrimitiveType_I32 },
};
static LmbProgramType PRIMITIVE_F32 = (LmbProgramType) {
    .kind = LmbProgramTypeKind_PRIMITIVE,
    .data = { .primitive = PrimitiveType_F32 },
};

size_t _ct_assign_children_types(ASTNode *node, ProgramTypeContext *ctx) {
    size_t n_total = 0;
    for (size_t i = 0; i < node->n_children; i++) {
        n_total += _ct_assign_types(node->children[i], ctx);
    }
    return n_total;
}

size_t _ct_assign_ident_type(ASTNode *node, ProgramTypeContext *ctx) {
    if (node->value_type != 0) { return 0; }

    LmbProgramType *found_type = _ct_symtable_find_type(ctx, node->data.ident.name);
    if (found_type != 0) {
        node->value_type = found_type;
        return 1;
    }
    return 0;
}
size_t _ct_assign_literal_type(ASTNode *node, ProgramTypeContext *ctx) {
    (void) ctx;
    if (node->value_type != 0) { return 0; }

    switch (node->type) {
        case ASTNodeType_LBOOL:     node->value_type = &PRIMITIVE_I32; return 1;
        case ASTNodeType_LINT:      node->value_type = &PRIMITIVE_I32; return 1;
        case ASTNodeType_LFLOAT:    node->value_type = &PRIMITIVE_F32; return 1;
        default: return 0;
    }
}

size_t _ct_assign_binop_type(ASTNode *node, ProgramTypeContext *ctx) {
    if (node->value_type != 0) { return 0; }

    size_t n_assigned = _ct_assign_children_types(node, ctx);
    if (node->n_children != 2) {
        return n_assigned;
    }

    LmbProgramType *ltype = node->children[0]->value_type;
    LmbProgramType *rtype = node->children[1]->value_type;

    // non-primitives are not supported for operators yet
    if (ltype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return n_assigned;
    }
    if (rtype->kind != LmbProgramTypeKind_PRIMITIVE) {
        return n_assigned;
    }

    LmbProgramType *max_fitting = &PRIMITIVE_I32;
    if (ltype->data.primitive == PrimitiveType_F32 || rtype->data.primitive == PrimitiveType_F32) {
        max_fitting = &PRIMITIVE_F32;
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
            node->value_type = &PRIMITIVE_I32;
            n_assigned += 1;
            break;

        case OperatorVariant_BINARY_LT:
        case OperatorVariant_BINARY_GT:
        case OperatorVariant_BINARY_LTE:
        case OperatorVariant_BINARY_GTE:
        case OperatorVariant_BINARY_EQ:
        case OperatorVariant_BINARY_NEQ:
            // no bools yet
            node->value_type = &PRIMITIVE_I32;
            n_assigned += 1;
            break;

        case OperatorVariant_BINARY_AND:
        case OperatorVariant_BINARY_OR:
            // no bools yet
            node->value_type = &PRIMITIVE_I32;
            n_assigned += 1;
            break;
    }

    return n_assigned;
}

size_t _ct_assign_lambda_arg_type(ASTNode *node, ProgramTypeContext *ctx) {
    (void) ctx; // will most probably be needed later
    // TODO: data.lambda_arg.type should instead be a subtree
    if (str_eq(node->data.lambda_arg.type, STR_CONST("i32"))) {
        node->value_type = &PRIMITIVE_I32;
        return 1;
    }

    if (str_eq(node->data.lambda_arg.type, STR_CONST("f32"))) {
        node->value_type = &PRIMITIVE_F32;
        return 1;
    }

    return 0;
}

size_t _ct_assign_lambda_types(ASTNode *node, ProgramTypeContext *ctx) {
    if (node->value_type != 0) { return 0; }
    if (node->n_children == 0) { return 0; }

    size_t ctx_size = ctx->symbols.size;
    size_t n_assigned = 0;
    LmbProgramType *lambda_type = arena_alloc(ctx->arena, sizeof(LmbProgramType));
    LmbProgramType **arg_types = arena_alloc(ctx->arena, sizeof(LmbProgramType*) * (node->n_children - 1));

    *lambda_type = (LmbProgramType) {
        .kind = LmbProgramTypeKind_ARROW,
        .data = { .arrow = { .n_args = 0, .arg_types = arg_types, .ret_type = 0 } },
    };

    for (size_t i = 0; i < node->n_children; i++) {
        ASTNode *arg = node->children[i];

        if (arg->type != ASTNodeType_LAMBDA_ARG) {
            continue;
        }

        lambda_type->data.arrow.n_args += 1;

        if (!arg->value_type) {
            if (_ct_assign_lambda_arg_type(arg, ctx) == 0) {
                arg_types[i] = 0;
                // could not set arg type
                continue;
            }

            n_assigned += 1;
        }

        _ct_symtable_append(&ctx->symbols, (ProgramSymbol) {
            .name = arg->data.lambda_arg.name,
            .source = arg,
        });
        arg_types[i] = arg->value_type;
    }

    n_assigned += _ct_assign_children_types(node, ctx);
    ctx->symbols.size = ctx_size; // pop off all the inner created name-type associations

    ASTNode *lambda_body = node->children[node->n_children - 1];
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

size_t _ct_assign_assignment_types(ASTNode *node, ProgramTypeContext *ctx) {
    size_t n_assigned = _ct_assign_children_types(node, ctx);
    if (n_assigned > 0) {
        if (node->value_type == 0) {
            n_assigned += 1;
        }

        node->value_type = node->children[0]->value_type;

        if (node->value_type != 0) {
            _ct_symtable_append(&ctx->symbols, (ProgramSymbol) {
                .name = node->data.assignment.identifier,
                .source = node,
            });
        }
    }
    return n_assigned;
}

size_t _ct_assign_types(ASTNode *node, ProgramTypeContext *ctx) {
    if (node == 0) { return 0; }

    size_t n_assigned = 0;

    switch (node->type) {
    case ASTNodeType_LMB_FILE:
        return _ct_assign_children_types(node, ctx);
    break;

    case ASTNodeType_IDENT:
        return _ct_assign_ident_type(node, ctx);
    break;

    case ASTNodeType_BINOP:
        return _ct_assign_binop_type(node, ctx);
    break;

    case ASTNodeType_ASSIGNMENT: // must also write into the symtable
        n_assigned += _ct_assign_assignment_types(node, ctx);
    break;

    case ASTNodeType_UNOP:
        n_assigned += _ct_assign_children_types(node, ctx);
        if (node->n_children > 0) {
            // TODO: this is not always the case
            node->value_type = node->children[0]->value_type;
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
    ProgramTypeContext ctx = {
        .arena = p->arena,
        .symbols = { 0 }
    };
    _ct_symtable_init(&ctx.symbols, 128, p->arena);

    // TODO: better algorithm – build a dependency tree first
    while (_ct_assign_types(p->root, &ctx) > 0) {
        break;
        // loop until cannot resolve anymore types
    }
}
