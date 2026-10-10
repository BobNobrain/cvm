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

void _ct_skip_until_top_level_decl_again(Parser *p) {
    Token *next = 0;

    while ((next = ct_parser_peek(p)) != 0) {
        switch (next->type) {
            case TokenType_KEYWORD:
                if (next->data.keyword == Keyword_LET || next->data.keyword == Keyword_ENTRY) {
                    return;
                }

            default:
                ct_parser_consume(p);
                break;
        }
    }
}

ASTNode* _ct_grammar_let_declaration(Parser *p) {
    Token *base = ct_parser_peek(p);
    REQUIRE_KEYWORD(Keyword_LET, "expected a 'let' keyword")

    Token *var, *assignment;
    REQUIRE_AND_SET_TOKEN(var, TokenType_IDENT, "expected a variable name")
    REQUIRE_AND_SET_TOKEN(assignment, TokenType_ASSIGNMENT, "expected an assignment")

    ASTNode *value = ct_grammar_expr(p);

    Token *newline = ct_parser_consume_if(p, TokenType_NEWLINE);
    if (newline == 0 && p->input.size > 0) {
        return ct_astnode_new_error(p, STR_CONST("expected a newline after an assignment"));
    }

    ASTNode *result = ct_astnode_new(p, ASTNodeType_ASSIGNMENT);
    result->data.assignment.ident_token = var;
    result->data.assignment.identifier = var->data.ident;
    result->base = base;
    result->range = ct_document_range_span(base->range, value->range);
    ct_astnode_append_child(result, value);
    return result;
}

ASTNode* _ct_grammar_entry_declaration(Parser *p) {
    Token *entry_kw = ct_parser_peek(p);
    REQUIRE_KEYWORD(Keyword_ENTRY, "expected an 'entry' keyword")

    ASTNode *entry_fn = ct_grammar_lambda(p);
    ASTNode *entry = ct_astnode_new(p, ASTNodeType_ENTRY);
    entry->base = entry_kw;
    entry->range = ct_document_range_span(entry_kw->range, entry_fn->range);
    ct_astnode_append_child(entry, entry_fn);

    if (ct_astnode_is_error(entry_fn)) {
        _ct_skip_until_top_level_decl_again(p);

        entry_fn->range = ct_document_range_span(entry_fn->range, ct_parser_current_range(p));
    }

    return entry;
}

ASTNode* _ct_grammar_lmb_toplevel(Parser *p) {
    GRAMMAR_ONEOF_START

    GRAMMAR_ONEOF_TRY(_ct_grammar_let_declaration);
    GRAMMAR_ONEOF_TRY(_ct_grammar_entry_declaration);

    return ct_astnode_new_error(p, STR_CONST("expected either 'let' or 'entry' declaration at top level"));
}

ASTNode* ct_grammar_lmb_file(Parser *p) {
    _ct_skip_newlines(p);

    ASTNode *file_content = ct_astnode_new(p, ASTNodeType_LMB_FILE);

    while (p->input.size > 0) {
        Token *start = &p->input.content[0];
        ASTNode *next = _ct_grammar_lmb_toplevel(p);

        ct_astnode_append_child(file_content, next);

        if (ct_astnode_is_error(next)) {
            // nothing was consumed, and all options were exhausted;
            // let's give up and generate a big syntax error,
            // and skip tokens until it's parseable again
            _ct_skip_until_top_level_decl_again(p);

            next->range = ct_document_range_span(
                start->range,
                ct_parser_current_range(p)
            );
        }

        _ct_skip_newlines(p);
    }

    return file_content;
}
