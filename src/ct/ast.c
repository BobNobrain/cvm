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

ASTNode *ct_astnode_new(ASTree *tree, ASTNodeType type) {
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

void ct_astnode_to_string(ASTNode *node, StringBuilder *sb, size_t indent) {
    const size_t buffer_size = 128;
    char buffer[buffer_size];

    for (size_t i = 0; i < indent; i++) {
        strb_appendc(sb, "  ");
    }

    switch (node->type) {
    case AST_TYPE_LINT:
        snprintf(buffer, buffer_size, "INT %d", node->base->data.intl);
        strb_appendc(sb, buffer);
        break;
    case AST_TYPE_LFLOAT:
        snprintf(buffer, buffer_size, "FLOAT %f", node->base->data.floatl);
        strb_appendc(sb, buffer);
        break;
    case AST_TYPE_LBOOL:
        snprintf(buffer, buffer_size, "BOOL %d", node->base->data.booll);
        strb_appendc(sb, buffer);
        break;
    case AST_TYPE_IDENT:
        strb_appendc(sb, "IDENT");
        break;
    case AST_TYPE_BINOP:
        snprintf(buffer, buffer_size, "BINOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        strb_appendc(sb, buffer);
        break;
    case AST_TYPE_UNOP:
        snprintf(buffer, buffer_size, "UNOP " STR_FMT, STR_FMT_VAL(node->base->data.op));
        strb_appendc(sb, buffer);
        break;

    default:
        strb_appendc(sb, "UNKNOWN");
        break;
    }

    strb_appendc(sb, "\n");

    for (size_t i = 0; i < node->n_children; i++) {
        ct_astnode_to_string(node->children[i], sb, indent + 1);
    }
}

void ct_astnode_print(ASTNode *node) {
    if (node == 0) {
        printf("<null>");
        return;
    }

    StringBuilder *sb = strb_new(256);
    ct_astnode_to_string(node, sb, 0);

    String result = strb_render(sb);
    printf(STR_FMT "\n", STR_FMT_VAL(result));
}
