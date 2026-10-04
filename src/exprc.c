#include <stdio.h>
#include <stddef.h>
#include "ct.h"

#define OPERATOR(OP, TYPE, P) \
    { .op = str_wrap(OP), .type = OperatorType_##TYPE, .priority = P }

int main() {
    String source = STR_CONST("a + b0 < c + (b___ ~~ 11) == -10 && 3 / 2 <> 1 \n");

    LangConfig language = ct_langconfig_create();

    Arena *parser_arena = arena_new(32768);
    Parser *p = ct_parser_new(parser_arena);

    String config_err = ct_parser_configure(p, language);
    if (!str_is_empty(config_err)) {
        printf("bad language config: " STR_FMT "\n", STR_FMT_VAL(config_err));
        return EXIT_FAILURE;
    }

    ct_parser_parse(p, source, ct_grammar_expr);

    if (p->errors.size > 0) {
        printf("\nparse failed: \n");
        for (size_t i = 0; i < p->errors.size; i++) {
            ct_document_print_error(p->errors.content[i], p->arena);
        }

        return EXIT_FAILURE;
    }

    arena_destroy(parser_arena);
    return EXIT_SUCCESS;
}
