#include <stdio.h>
#include "util.h"
#include "ct_int.h"

Parser *ct_parser_new() {
    Parser *parser = malloc_or_die(sizeof(Parser));

    parser->input_start = 0;
    parser->input_next = 0;
    parser->input_size = 0;
    parser->current = 0;
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

    ERR_PASS( grammar(p, docerr) )
    p->root = p->current;

    printf("AST:\n");
    ct_astnode_print(p->root);

    return E_NONE;
}
