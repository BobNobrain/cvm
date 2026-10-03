#include <stdio.h>
#include <stddef.h>
#include "ct.h"

#define OPERATOR(OP, TYPE, P) \
    { .op = str_wrap(OP), .type = OperatorType_##TYPE, .priority = P }

int main() {
    ERR_DECL
    char TEST_EXPR[] = "(30.208 + -502 * 1) ";
    String source = str_wrap(TEST_EXPR);

    OperatorDecl optable[] = {
        OPERATOR("!",  UNARY_RIGHT,   150) ,
        OPERATOR("-",  UNARY_RIGHT,   150) ,
        OPERATOR("+",  UNARY_RIGHT,   150) ,

        OPERATOR("^",  BINARY_RIGHT,   90) ,
        OPERATOR("*",  BINARY_LEFT,    80) ,
        OPERATOR("/",  BINARY_LEFT,    80) ,
        OPERATOR("%",  BINARY_NOASSOC, 80) ,
        OPERATOR("+",  BINARY_LEFT,    70) ,
        OPERATOR("-",  BINARY_LEFT,    70) ,
        OPERATOR("<",  BINARY_NOASSOC, 60) ,
        OPERATOR(">",  BINARY_NOASSOC, 60) ,
        OPERATOR(">=", BINARY_NOASSOC, 60) ,
        OPERATOR("<=", BINARY_NOASSOC, 60) ,
        OPERATOR("==", BINARY_LEFT,    50) ,
        OPERATOR("<>", BINARY_LEFT,    50) ,
        OPERATOR("&&", BINARY_LEFT,    40) ,
        OPERATOR("||", BINARY_LEFT,    40)
    };
    OperatorDeclSlice optable_slice = ct_opdeclslice_of_const(optable, sizeof(optable) / sizeof(OperatorDecl));

    Parser *p = ct_parser_new();
    ct_parser_configure_operators(p, optable_slice);
    DocumentError docerr = { .source = source };

    err = ct_parser_parse(p, source, ct_grammar_expr, &docerr);
    if (ERR_ISSET) {
        printf("parse failed: ");
        ct_document_print_error(docerr);
        return -1;
    }

    ct_parser_destroy(p);
    return 0;
}
