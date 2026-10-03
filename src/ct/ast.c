#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "ct_int.h"

void ct_astree_allocate_block(ASTree *tree) {
    if (tree->n_blocks_used >= tree->capacity) {
        size_t cap_increase = tree->capacity;
        if (cap_increase > 1024) {
            cap_increase = 1024;
        }
        if (cap_increase == 0) {
            cap_increase = 32;
        }

        tree->capacity += cap_increase;
        tree->blocks = realloc_or_die(tree->blocks, sizeof(ASTNode*) * tree->capacity);
    }

    tree->blocks[tree->n_blocks_used] = malloc_or_die(sizeof(ASTNode) * tree->block_size);
    tree->n_blocks_used += 1;
    tree->current_block_size = 0;
}

void ct_astree_init(ASTree *tree) {
    tree->block_size = 64;
    tree->capacity = 32;
    tree->n_blocks_used = 0;
    tree->current_block_size = 0;
    tree->blocks = malloc_or_die(sizeof(ASTNode*) * tree->capacity);

    ct_astree_allocate_block(tree);
}
void ct_astree_destroy(ASTree *tree) {
    for (size_t i = 0; i < tree->n_blocks_used; i++) {
        free(tree->blocks[i]);
    }
    free(tree->blocks);
}

ASTNode* ct_astnode_new(ASTree *tree, ASTNodeType type) {
    if (tree->current_block_size >= tree->block_size || tree->n_blocks_used == 0) {
        ct_astree_allocate_block(tree);
    }

    ASTNode *block = tree->blocks[tree->n_blocks_used - 1];
    ASTNode *new_node = &block[tree->current_block_size];
    tree->current_block_size += 1;

    new_node->type = type;
    new_node->base = 0;
    new_node->n_children = 0;

    switch (type) {
    case AST_TYPE_BINOP:
        new_node->children_cap = 2;
        new_node->children = malloc_or_die(sizeof(ASTNode*) * new_node->children_cap);
        break;

    case AST_TYPE_UNOP:
        new_node->children_cap = 1;
        new_node->children = malloc_or_die(sizeof(ASTNode*) * new_node->children_cap);
        break;

    default:
        new_node->children = 0;
        new_node->children_cap = 0;
    }

    return new_node;
}
ASTNode* ct_astnode_new_error(ASTree *tree, DocumentError docerr) {
    ASTNode *e = ct_astnode_new(tree, AST_TYPE_SYNTAX_ERROR);
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
