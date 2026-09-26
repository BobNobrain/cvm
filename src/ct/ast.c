#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "util.h"
#include "ct_int.h"

void ct_ast_tree_allocate_block(ASTree *tree) {
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

void ct_ast_tree_init(ASTree *tree) {
    tree->block_size = 64;
    tree->capacity = 32;
    tree->n_blocks_used = 0;
    tree->current_block_size = 0;
    tree->blocks = malloc_or_die(sizeof(ASTNode*) * tree->capacity);

    ct_ast_tree_allocate_block(tree);
}

ASTNode *ct_ast_node_new(ASTree *tree, ASTNodeType type, ASTNode *parent) {
    if (tree->current_block_size >= tree->block_size || tree->n_blocks_used == 0) {
        ct_ast_tree_allocate_block(tree);
    }

    ASTNode *block = tree->blocks[tree->n_blocks_used - 1];
    ASTNode *new_node = &block[tree->current_block_size];
    tree->current_block_size += 1;

    new_node->type = type;
    new_node->parent = parent;
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

void ct_ast_parser_init(Parser *p, Tokenizer *input) {
    p->input_start = input->tokens; // TODO: copy into own memory?
    p->input_size = input->size;
    p->input_next = p->input_start;
    p->current = 0;

    ct_ast_tree_init(&p->result);
}
