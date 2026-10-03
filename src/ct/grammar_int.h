#define GRAMMAR_ONEOF_START ParserRewindPoint saved_pos = { .input = p->input }; ASTNode *result = 0;
#define GRAMMAR_ONEOF_TRY(GRAMMAR) \
    result = GRAMMAR(p, docerr);        \
    if (!ct_astnode_is_error(result)) { \
        return result;                  \
    } else {                            \
        ct_parser_rewind(p, saved_pos); \
    }
