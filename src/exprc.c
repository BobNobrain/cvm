#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct.h"

int main() {
    ERR_DECL
    char TEST_EXPR[] = "(30.208 + 502) ";
    String source = str_wrap(TEST_EXPR);

    Parser *p = ct_parser_new();
    DocumentError docerr;

    err = ct_parser_parse(p, source, ct_grammar_expr, &docerr);
    if (ERR_ISSET) {
        printf("parse failed: ");
        ct_document_print_error(docerr);
        return -1;
    }

    ct_parser_destroy(p);
    return 0;
}
