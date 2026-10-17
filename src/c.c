#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct.h"

int main() {
    ERR_DECL
    FILE *input = fopen("./tests/lambda/2.lmb", "r"); // stdin;

    Arena *arena = arena_new(65536);
    arena_set_max_regions(arena, 8); // 512 KiB memory limit

    LangConfig language = ct_langconfig_create();

    Compiler c;
    ct_compiler_init(&c, language, ct_grammar_lmb_file, arena);

    StringBuilder *text_builder = strb_new(arena, 16384);
    err = strb_read_from_stream(text_builder, input);
    if (ERR_ISSET) {
        printf("Failed to read the input\n");
        arena_destroy(arena);
        return EXIT_FAILURE;
    }

    String program_text = strb_render(text_builder);

    ct_compiler_compile(&c, program_text);
    ct_compiler_print_diagnostics(&c);

    arena_destroy(arena);

    if (ct_compiler_has_errors(&c)) { return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
