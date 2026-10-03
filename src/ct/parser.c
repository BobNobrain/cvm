#include <stdio.h>
#include "util.h"
#include "ct_int.h"

Parser *ct_parser_new() {
    Parser *parser = malloc_or_die(sizeof(Parser));

    parser->input_start = 0;
    parser->input_next = 0;
    parser->input_size = 0;
    parser->root = 0;

    ct_astree_init(&parser->tree);
    return parser;
}

void ct_parser_destroy(Parser *p) {
    ct_astree_destroy(&p->tree);
    free(p);
}

Token *ct_parser_consume(Parser *p) {
    if (p->input_next >= &p->input_start[p->input_size]) {
        return 0;
    }

    Token *result = p->input_next;
    p->input_next += 1;
    return result;
}
Token *ct_parser_peek(Parser *p) {
    if (p->input_next >= &p->input_start[p->input_size]) {
        return 0;
    }

    Token *result = p->input_next;
    return result;
}
Token *ct_parser_consume_if(Parser *p, TokenType type) {
    if (p->input_next >= &p->input_start[p->input_size]) {
        return 0;
    }

    Token *result = p->input_next;
    if (result->type != type) {
        return 0;
    }

    p->input_next += 1;
    return result;
}
void ct_parser_rewind(Parser *p, Token *to) {
    p->input_next = to;
}
void ct_parser_rewind_n(Parser *p, int n) {
    p->input_next += n;
    if (p->input_next < p->input_start) {
        p->input_next = p->input_start;
    } else if (p->input_next >= &p->input_start[p->input_size]) {
        p->input_next = &p->input_start[p->input_size];
    }
}
DocumentRange ct_parser_current_range(Parser *p) {
    if (p->input_start == 0 || p->input_size == 0) {
        return (DocumentRange) { 0 };
    }

    Token *current = p->input_next;

    if (p->input_next >= &p->input_start[p->input_size]) {
        current = &p->input_start[p->input_size - 1];
    } else if (p->input_next == 0) {
        current = p->input_start;
    }

    return current->range;
}

error ct_parser_parse(Parser *p, String source, ParserGrammar grammar, DocumentError *docerr) {
    printf("SOURCE: " STR_FMT "\n", STR_FMT_VAL(source));

    ERR_DECL
    Tokenizer t;
    ct_tokenizer_init(&t);
    ERR_PASS( ct_tokenizer_run(&t, source, docerr) )

    printf("TOKENS:\n");
    ct_tokenizer_print(&t);
    printf("\n");

    p->input_start = t.tokens;
    p->input_size = t.size;
    p->input_next = p->input_start;

    p->root = grammar(p, docerr);

    printf("AST:\n");
    ct_astnode_print(p->root, 0);

    if (ct_astnode_is_error(p->root)) {
        return E_BAD_DATA;
    }

    return E_NONE;
}

void ct_parser_configure_operators(Parser *p, OperatorDeclSlice optable) {
    // TODO: validate the table:
    // - no mixing unary/binary and associativity on the same priority level
    // - must be sorted by priority, desc
    p->optable = optable;
}
