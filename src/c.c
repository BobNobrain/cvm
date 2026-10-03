#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct.h"

int main() {
    ERR_DECL
    FILE *input = stdin;
    Arena *arena = arena_new(65536);

    StringBuilder *text_builder = strb_new(arena, 16384);
    err = strb_read_from_stream(text_builder, input);
    if (ERR_ISSET) {
        printf("Failed to read the input\n");
        return EXIT_FAILURE;
    }

    String program_text = strb_render(text_builder);

    LangConfig language = ct_langconfig_create();
    Parser *p = ct_parser_new(arena);
    String config_err = ct_parser_configure(p, language);
    if (!str_is_empty(config_err)) {
        printf("bad language config: " STR_FMT "\n", STR_FMT_VAL(config_err));
        return EXIT_FAILURE;
    }

    printf("\n\n===============================\n");
    ct_parser_parse(p, program_text, ct_grammar_expr);

    if (p->errors.size > 0) {
        printf("\nparse failed: \n");
        for (size_t i = 0; i < p->errors.size; i++) {
            ct_document_print_error(p->errors.content[i]);
        }

        return EXIT_FAILURE;
    }

    arena_destroy(arena);
    return EXIT_SUCCESS;
}
