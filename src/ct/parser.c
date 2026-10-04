#include <stdio.h>
#include "util.h"
#include "ct_int.h"

Parser* ct_parser_new(Arena *arena) {
    Parser *parser = arena_alloc(arena, sizeof(Parser));
    parser->arena = arena;

    parser->input = (TokenSlice) { 0 };
    parser->root = 0;
    ct_err_array_init(&parser->errors, 8, parser->arena);

    return parser;
}

Token* ct_parser_consume(Parser *p) {
    if (p->input.size == 0) { return 0; }

    Token *result = &p->input.content[0];
    p->input = ct_tokenslice_slice(p->input, 1, p->input.size);
    return result;
}
Token* ct_parser_peek(Parser *p) {
    if (p->input.size == 0) { return 0; }
    return &p->input.content[0];
}
Token* ct_parser_consume_if(Parser *p, TokenType type) {
    if (p->input.size == 0) { return 0; }

    Token *result = &p->input.content[0];
    if (result->type != type) { return 0; }

    p->input = ct_tokenslice_slice(p->input, 1, p->input.size);
    return result;
}
Token* ct_parser_consume_keyword(Parser *p, Keyword kw) {
    if (p->input.size == 0) { return 0; }

    Token *result = &p->input.content[0];
    if (result->type != TokenType_KEYWORD) { return 0; }
    if (result->data.keyword != kw) { return 0; }

    p->input = ct_tokenslice_slice(p->input, 1, p->input.size);
    return result;
}

void ct_parser_rewind(Parser *p, ParserRewindPoint to) {
    p->input = to.input;
}
void ct_parser_rewind_n(Parser *p, int n) {
    while (n < 0 && p->input_original.content != p->input.content) {
        p->input.content -= 1;
        p->input.size += 1;
        n += 1;
    }
}

DocumentRange ct_parser_current_range(Parser *p) {
    if (p->input.size == 0) {
        if (p->input_original.size == 0) {
            return (DocumentRange) { 0 };
        }

        return ct_tokenslice_at(p->input_original, p->input_original.size - 1)->range;
    }

    return p->input.content[0].range;
}

void _ct_parser_collect_errors(Parser *p, ASTNode *node) {
    if (node == 0) {
        ct_parser_append_error(p, "null node found", (DocumentRange) { 0 });
        return;
    }

    if (node->type == ASTNodeType_SYNTAX_ERROR) {
        ct_err_array_append(&p->errors, (DocumentError) {
            .source = p->source,
            .location = node->range,
            .message = node->data.error
        });
    }

    for (size_t i = 0; i < node->n_children; i++) {
        _ct_parser_collect_errors(p, node->children[i]);
    }
}

void ct_parser_parse(Parser *p, String source, ParserGrammar grammar) {
    printf("SOURCE: " STR_FMT_DEBUG "\n", STR_FMT_DEBUG_VAL(source));
    p->source = source;

    Tokenizer t;
    ct_tokenizer_init(&t, p->arena, p->config);
    ct_tokenizer_run(&t, source, &p->errors);

    printf("TOKENS:\n");
    ct_tokenizer_print(&t);
    printf("\n");

    TokenSlice tokens = ct_tokenarray_seal(&t.tokens);
    p->input_original = tokens;
    p->input = tokens;

    p->root = grammar(p);

    printf("AST:\n");
    ct_astnode_print(p->root, 0);

    _ct_parser_collect_errors(p, p->root);

    if (p->input.size > 0) {
        ct_parser_append_error(p, "parser stopped prematurely", p->input.content[0].range);
    }
}

String ct_parser_configure(Parser *p, LangConfig config) {
    String errmsg = ct_langconfig_validate(config);
    if (!str_is_empty(errmsg)) {
        return errmsg;
    }

    p->config = config;
    return STR_EMPTY;
}

void ct_parser_append_error(Parser *p, char* msg, DocumentRange range) {
    DocumentError e = { .source = p->source, .location = range, .message = str_wrap(msg) };
    ct_err_array_append(&p->errors, e);
}

DocumentError ct_parser_make_error(Parser *p, char* msg, DocumentRange range) {
    DocumentError e = { .source = p->source, .location = range, .message = str_wrap(msg) };
    return e;
}
