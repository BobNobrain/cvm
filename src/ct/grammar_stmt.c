#include "ct_int.h"
#include "grammar_int.h"

/*
TODO: statement parsing

x = 10 * 5
y = 11.0 / 2

function calls??
f = \x y. { x + y }
f2 = \x y. x - y
f3 = \x y. z { z = x - y }
apply = \f x. f x
z = { f (f 2 10) x }
*/

void _ct_skip_newlines(Parser *p) {
    Token *t = 0;
    while ((t = ct_parser_consume_if(p, TokenType_NEWLINE)) != 0) {}
}

void _ct_skip_until_assignment_again(Parser *p) {
    Token *next = 0;

    while ((next = ct_parser_consume(p)) != 0) {
        switch (next->type) {
            case TokenType_IDENT:
                ct_parser_rewind_n(p, -1);
                return;

            default:
                break;
        }
    }
}

ASTNode* _ct_grammar_assignment(Parser *p) {
    Token *var = ct_parser_consume_if(p, TokenType_IDENT);
    if (var == 0) {
        return ct_astnode_new_error2(p, STR_CONST("expected a variable name"), ct_parser_current_range(p));
    }

    Token *assignment = ct_parser_consume_if(p, TokenType_ASSIGNMENT);
    if (assignment == 0) {
        return ct_astnode_new_error2(p, STR_CONST("expected an assignment"), ct_parser_current_range(p));
    }

    ASTNode *value = ct_grammar_expr(p);

    Token *newline = ct_parser_consume_if(p, TokenType_NEWLINE);
    if (newline == 0 && p->input.size > 0) {
        return ct_astnode_new_error2(p, STR_CONST("expected a newline after an assignment"), ct_parser_current_range(p));
    }

    ASTNode *result = ct_astnode_new(p, ASTNodeType_ASSIGNMENT);
    result->data.assignment.ident_token = var;
    result->data.assignment.identifier = var->data.ident;
    result->children[0] = value;
    result->n_children = 1;
    return result;
}

ASTNode* ct_grammar_lmb_file(Parser *p) {
    _ct_skip_newlines(p);

    ASTNode *file_content = ct_astnode_new(p, ASTNodeType_LMB_FILE);

    while (p->input.size > 0) {
        Token *start = &p->input.content[0];
        ASTNode *next =_ct_grammar_assignment(p);
        ct_astnode_append_child(p, file_content, next);

        Token *current = &p->input.content[0];
        if (start == current) {
            // nothing was consumed, and all options were exhausted;
            // let's give up and generate a big syntax error,
            // and skip tokens until it's parseable again
            _ct_skip_until_assignment_again(p);

            if (!ct_astnode_is_error(next)) {
                next = ct_astnode_new_error2(p, STR_CONST("expected an assignment"), ct_document_range_span(
                    start->range,
                    ct_parser_current_range(p)
                ));
                ct_astnode_append_child(p, file_content, next);
            } else {
                next->range = ct_document_range_span(
                    start->range,
                    ct_parser_current_range(p)
                );
            }
        }

        _ct_skip_newlines(p);
    }

    return file_content;
}
