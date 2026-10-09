#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "ct_int.h"

ASTNode* ct_astnode_new(Parser *parser, ASTNodeType type) {
    ASTNode *new_node = arena_alloc(parser->arena, sizeof(*new_node));

    *new_node = (ASTNode) { 0 };
    new_node->type = type;

    new_node->n_children = 0;
    new_node->children_cap = 0;
    new_node->children = 0;

    switch (type) {
    case ASTNodeType_LAMBDA:
        ct_astnode_alloc_children(parser, new_node, 4);
        break;

    case ASTNodeType_BINOP:
        ct_astnode_alloc_children(parser, new_node, 2);
        break;

    case ASTNodeType_UNOP:
    case ASTNodeType_ASSIGNMENT:
    case ASTNodeType_ENTRY:
        ct_astnode_alloc_children(parser, new_node, 1);
        break;

    default:
        break;
    }

    return new_node;
}

ASTNode* ct_astnode_new_error(Parser *parser, String msg) {
    ASTNode *e = ct_astnode_new(parser, ASTNodeType_SYNTAX_ERROR);
    e->data.error = msg;
    e->range = ct_parser_current_range(parser);
    return e;
}
ASTNode* ct_astnode_new_error_from(Parser *parser, DocumentError docerr) {
    ASTNode *e = ct_astnode_new(parser, ASTNodeType_SYNTAX_ERROR);
    e->data.error = docerr.message;
    e->range = docerr.location;
    return e;
}
ASTNode* ct_astnode_new_error_ranged(Parser *parser, String msg, DocumentRange range) {
    ASTNode *e = ct_astnode_new(parser, ASTNodeType_SYNTAX_ERROR);
    e->data.error = msg;
    e->range = range;
    return e;
}

void ct_astnode_alloc_children(Parser *p, ASTNode *parent, size_t n) {
    if (parent->children_cap == 0) {
        parent->children = arena_alloc(p->arena, sizeof(ASTNode*) * n);
    } else {
        parent->children = arena_realloc(
            p->arena,
            parent->children,
            sizeof(ASTNode*) * parent->children_cap,
            sizeof(ASTNode*) * (parent->children_cap + n)
        );
    }
    parent->children_cap += n;
}
void ct_astnode_append_child(Parser *p, ASTNode *parent, ASTNode *child) {
    if (parent->children_cap == 0) {
        ct_astnode_alloc_children(p, parent, 16);
    } else if (parent->n_children >= parent->children_cap) {
        size_t cap_increase = parent->children_cap;
        if (cap_increase < 16) { cap_increase = 16; }
        if (cap_increase >= 128) { cap_increase = 128; }

        ct_astnode_alloc_children(p, parent, cap_increase);
    }

    parent->children[parent->n_children] = child;
    parent->n_children += 1;
}

bool ct_astnode_is_error(ASTNode *node) {
    return node == 0 || node->type == ASTNodeType_SYNTAX_ERROR;
}

void ct_astnode_print(ASTNode *node, size_t indent) {
    if (node == 0) {
        printf("<null>\n");
        return;
    }

    for (size_t i = 0; i < indent; i++) {
        printf("  ");
    }

    switch (node->type) {
    case ASTNodeType_SYNTAX_ERROR:
        printf(
            "ERROR (%zu:%zu-%zu:%zu): " STR_FMT,
            node->range.start.line,
            node->range.start.column,
            node->range.end.line,
            node->range.end.column,
            STR_FMT_VAL(node->data.error)
        );
        break;
    case ASTNodeType_LINT:
        printf("INT %d", node->data.lint);
        break;
    case ASTNodeType_LFLOAT:
        printf("FLOAT %f", node->data.lfloat);
        break;
    case ASTNodeType_LBOOL:
        printf("BOOL %d", node->data.lbool);
        break;
    case ASTNodeType_IDENT:
        printf("IDENT " STR_FMT, STR_FMT_VAL(node->data.ident.name));
        break;
    case ASTNodeType_LAMBDA:
        printf("LAMBDA (%zu) ", node->data.lambda.n_args);
        break;
    case ASTNodeType_LAMBDA_ARG:
        printf(
            "ARG " STR_FMT ":" STR_FMT,
            STR_FMT_VAL(node->data.lambda_arg.name),
            STR_FMT_VAL(node->data.lambda_arg.type)
        );
        break;
    case ASTNodeType_BINOP:
        printf("BINOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        break;
    case ASTNodeType_UNOP:
        printf("UNOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        break;
    case ASTNodeType_FNCALL:
        printf("FNCALL");
        break;
    case ASTNodeType_ASSIGNMENT:
        printf("ASSIGNMENT " STR_FMT " =", STR_FMT_VAL(node->data.assignment.identifier));
        break;
    case ASTNodeType_ENTRY:
        printf("ENTRY");
        break;
    case ASTNodeType_LMB_FILE:
        printf(".LMB FILE");
        break;

    default:
        printf("UNKNOWN");
        break;
    }

    if (node->value_type != 0) {
        printf(" :");
        ct_type_print(node->value_type);
    }

    if (node->n_children > 0) {
        printf(" (%zu children)", node->n_children);
    }
    printf("\n");

    for (size_t i = 0; i < node->n_children; i++) {
        ct_astnode_print(node->children[i], indent + 1);
    }
}
