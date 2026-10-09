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

void ct_astnode_append_child(ASTNode *parent, ASTNode *child) {
    if (parent->first_child == 0) {
        parent->first_child = child;
        return;
    }

    ASTNode *last = ct_astnode_find_last_child(parent);
    last->next_sibling = child;
}

size_t ct_astnode_count_children(ASTNode *parent) {
    ASTNode *it = parent->first_child;
    size_t result = 0;
    while (it) {
        result += 1;
        it = it->next_sibling;
    }
    return result;
}

ASTNode* ct_astnode_find_last_child(ASTNode *parent) {
    if (parent->first_child == 0) { return 0; }

    ASTNode *it = parent->first_child;
    while (it->next_sibling != 0) {
        it = it->next_sibling;
    }

    return it;
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
    printf("\n");

    ASTNode *it = node->first_child;
    while (it != 0) {
        ct_astnode_print(it, indent + 1);
        it = it->next_sibling;
    }
}
