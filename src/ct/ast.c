#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "ct_int.h"

ASTNode* ct_astnode_new(Parser *parser, ASTNodeType type) {
    ASTNode *new_node = arena_alloc(parser->arena, sizeof(*new_node));

    new_node->type = type;
    new_node->base = 0;
    new_node->n_children = 0;

    switch (type) {
    case AST_TYPE_BINOP:
        new_node->children_cap = 2;
        new_node->children = arena_alloc(parser->arena, sizeof(ASTNode*) * new_node->children_cap);
        break;

    case AST_TYPE_UNOP:
        new_node->children_cap = 1;
        new_node->children = arena_alloc(parser->arena, sizeof(ASTNode*) * new_node->children_cap);
        break;

    default:
        new_node->children = 0;
        new_node->children_cap = 0;
    }

    return new_node;
}
ASTNode* ct_astnode_new_error(Parser *parser, DocumentError docerr) {
    ASTNode *e = ct_astnode_new(parser, AST_TYPE_SYNTAX_ERROR);
    e->data.error = docerr.message;
    e->range = docerr.location;
    return e;
}

bool ct_astnode_is_error(ASTNode *node) {
    return node == 0 || node->type == AST_TYPE_SYNTAX_ERROR;
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
    case AST_TYPE_SYNTAX_ERROR:
        printf("ERROR: " STR_FMT, STR_FMT_VAL(node->data.error));
        break;
    case AST_TYPE_LINT:
        printf("INT %d", node->base->data.intl);
        break;
    case AST_TYPE_LFLOAT:
        printf("FLOAT %f", node->base->data.floatl);
        break;
    case AST_TYPE_LBOOL:
        printf("BOOL %d", node->base->data.booll);
        break;
    case AST_TYPE_IDENT:
        printf("IDENT");
        break;
    case AST_TYPE_BINOP:
        printf("BINOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        break;
    case AST_TYPE_UNOP:
        printf("UNOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        break;

    default:
        printf("UNKNOWN");
        break;
    }

    if (node->n_children > 0) {
        printf(" (%zu children)", node->n_children);
    }
    printf("\n");

    for (size_t i = 0; i < node->n_children; i++) {
        ct_astnode_print(node->children[i], indent + 1);
    }
}
