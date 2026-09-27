#include <stdio.h>
#include "util.h"
#include "ct_int.h"

#define GRAMMAR_ONEOF_START Token *saved_pos = p->input_next;
#define GRAMMAR_ONEOF_TRY(GRAMMAR) \
    err = GRAMMAR(p, docerr); \
    if (err == E_NONE) { \
        return E_NONE; \
    } else { \
        ct_parser_rewind(p, saved_pos); \
    }

error ct_grammar_binop_operand(Parser *p, DocumentError *docerr);

error ct_grammar_binop_expr(Parser *p, DocumentError *docerr) {
    ERR_DECL

    ERR_PASS( ct_grammar_binop_operand(p, docerr) )
    ASTNode* left = p->current;

    Token *op = ct_parser_consume_if(p, TOKEN_OPERATOR);
    if (op == 0) {
        return E_BAD_DATA;
    }

    ERR_PASS( ct_grammar_binop_operand(p, docerr) )
    ASTNode* right = p->current;

    ASTNode* binop = ct_astnode_new(&p->tree, AST_TYPE_BINOP);
    binop->base = op;
    binop->children[0] = left;
    binop->children[1] = right;
    binop->n_children = 2;

    p->current = binop;
    return E_NONE;
}

error ct_grammar_unop_expr(Parser *p, DocumentError *docerr) {
    ERR_DECL

    Token *op = ct_parser_consume_if(p, TOKEN_OPERATOR);
    if (op == 0) {
        return E_BAD_DATA;
    }

    ERR_PASS( ct_grammar_expr(p, docerr) )
    ASTNode* inner = p->current;

    ASTNode* unop = ct_astnode_new(&p->tree, AST_TYPE_UNOP);
    unop->base = op;
    unop->children[0] = inner;
    unop->n_children = 1;

    p->current = unop;
    return E_NONE;
}

error ct_grammar_literal(Parser *p, DocumentError *docerr) {
    (void)docerr;

    Token *next = ct_parser_consume(p);
    if (next == 0) {
        return E_BAD_DATA;
    }

    ASTNodeType lit_type;

    switch (next->type) {
    case TOKEN_INT_LITERAL:
        lit_type = AST_TYPE_LINT;
        break;
    case TOKEN_FLOAT_LITERAL:
        lit_type = AST_TYPE_LFLOAT;
        break;
    case TOKEN_BOOL_LITERAL:
        lit_type = AST_TYPE_LBOOL;
        break;
    case TOKEN_IDENT:
        lit_type = AST_TYPE_IDENT;
        break;

    default:
        return E_BAD_DATA;
    }

    ASTNode* lit = ct_astnode_new(&p->tree, lit_type);
    lit->base = next;

    p->current = lit;
    return E_NONE;
}

error ct_grammar_parens(Parser *p, DocumentError *docerr) {
    ERR_DECL

    Token *opening = ct_parser_consume_if(p, TOKEN_OPEN_PAREN);
    if (opening == 0) {
        return E_BAD_DATA;
    }

    ERR_PASS( ct_grammar_expr(p, docerr) )

    Token *closing = ct_parser_consume_if(p, TOKEN_CLOSE_PAREN);
    if (closing == 0) {
        return E_BAD_DATA;
    }

    return E_NONE;
}

error ct_grammar_expr(Parser *p, DocumentError *docerr) {
    ERR_DECL

    GRAMMAR_ONEOF_START
    GRAMMAR_ONEOF_TRY( ct_grammar_parens )
    GRAMMAR_ONEOF_TRY( ct_grammar_unop_expr )
    GRAMMAR_ONEOF_TRY( ct_grammar_binop_expr )
    GRAMMAR_ONEOF_TRY( ct_grammar_literal )

    return E_BAD_DATA;
}

error ct_grammar_binop_operand(Parser *p, DocumentError *docerr) {
    ERR_DECL
    GRAMMAR_ONEOF_START
    GRAMMAR_ONEOF_TRY( ct_grammar_parens )
    GRAMMAR_ONEOF_TRY( ct_grammar_literal )
    GRAMMAR_ONEOF_TRY( ct_grammar_unop_expr )

    return E_BAD_DATA;
}
