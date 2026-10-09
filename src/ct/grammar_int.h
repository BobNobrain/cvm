#define GRAMMAR_ONEOF_START ParserRewindPoint saved_pos = { .input = p->input }; ASTNode *result = 0;
#define GRAMMAR_ONEOF_TRY(GRAMMAR) \
    result = GRAMMAR(p);                \
    if (!ct_astnode_is_error(result)) { \
        return result;                  \
    } else {                            \
        ct_parser_rewind(p, saved_pos); \
    }

#define REQUIRE_TOKEN(TOKEN_TYPE, ERR_MSG) \
    if (ct_parser_consume_if(p, TOKEN_TYPE) == 0) { return ct_astnode_new_error(p, STR_CONST(ERR_MSG)); }   \

#define REQUIRE_KEYWORD(KEYWORD_TYPE, ERR_MSG) \
    if (ct_parser_consume_keyword(p, KEYWORD_TYPE) == 0) {  \
        return ct_astnode_new_error(p, STR_CONST(ERR_MSG)); \
    }

#define REQUIRE_AND_SET_TOKEN(VAR, TOKEN_TYPE, ERR_MSG) \
    if ((VAR = ct_parser_consume_if(p, TOKEN_TYPE)) == 0) { \
        return ct_astnode_new_error(p, STR_CONST(ERR_MSG)); \
    }
