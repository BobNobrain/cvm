#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "ct_int.h"

ARRAY_METHODS_IMPL(ct_tokenarray, Token)
SLICE_METHODS_IMPL(ct_tokenslice, Token)
SLICE_ARRAY_METHODS_IMPL(ct_tokenarray, Token)

void ct_tokenizer_init(Tokenizer *t, Arena *arena) {
    ct_tokenarray_init(&t->tokens, 128, arena);
}

void ct_tokenizer_push_token(Tokenizer *t, Token token) {
    if (token.type == TokenType_INVALID &&
        t->tokens.size > 0 &&
        t->tokens.content[t->tokens.size - 1].type == TokenType_INVALID
    ) {
        t->tokens.content[t->tokens.size - 1].range.end = token.range.end;
        return;
    }

    ct_tokenarray_append(&t->tokens, token);
}

size_t ct_tokenizer_skip_ws(String source, DocumentPos *cursor) {
    for (size_t i = 0; i < source.size; i++) {
        char next = source.content[i];
        switch (next) {
        case '\n':
            ct_document_pos_line_break(cursor);
            break;

        case ' ':
        case '\t':
        case '\r':
            ct_document_pos_track(cursor, 1);
            break;

        default:
            return i;
        }
    }

    return source.size;
}

size_t ct_tokenizer_read_number(String source, Token *into, LangConfig config) {
    (void)config;

    bool point_met = false;
    size_t i = 0;

    for (; i < source.size; i++) {
        char next = source.content[i];

        if ('0' <= next && next <= '9') {
            continue;
        }
        if (next == '.' && !point_met) {
            point_met = true;
            continue;
        }

        break;
    }

    if (i == 0) {
        return 0;
    }

    if (point_met && i == 1) {
        // a single point is not a valid number literal
        into->type = TokenType_INVALID;
        return i;
    }

    if (point_met) {
        into->type = TokenType_FLOAT_LITERAL;
    } else {
        into->type = TokenType_INT_LITERAL;
    }

    return i;
}

size_t ct_tokenizer_read_ident(String source, Token *into, LangConfig config) {
    size_t i = 0;

    for (; i < source.size; i++) {
        char next = source.content[i];

        if ('A' <= next && next <= 'Z') {
            continue;
        }
        if ('a' <= next && next <= 'z') {
            continue;
        }
        if ('0' <= next && next <= '9') {
            continue;
        }
        if (str_index_of(config.allowed_ident_chars, next) != -1) {
            continue;
        }

        break;
    }

    if (i == 0) {
        return 0;
    }

    into->type = TokenType_IDENT;
    return i;
}

size_t ct_tokenizer_read_op(String source, Token *into, LangConfig config) {
    size_t i = 0;
    for (; i < source.size; i++) {
        char next = source.content[i];
        if (str_index_of(config.allowed_operator_chars, next) == -1) {
            break;
        }
    }

    if (i == 0) {
        return 0;
    }

    into->type = TokenType_OPERATOR;
    return i;
}

size_t ct_tokenizer_read_paren(String source, Token *into, LangConfig config) {
    (void)config;

    if (source.size == 0) {
        return 0;
    }

    char next = source.content[0];
    switch (next) {
    case '(':
        into->type = TokenType_OPEN_PAREN;
        break;
    case ')':
        into->type = TokenType_CLOSE_PAREN;
        break;

    default:
        return 0;
    }

    return 1;
}

void ct_tokenizer_parse_token(Token *token, String source, DocumentErrorArray *errors) {
    String token_content = ct_document_substring(source, token->range);

    switch (token->type) {
    case TokenType_IDENT:
        if (str_eqc(token_content, "true")) {
            token->type = TokenType_BOOL_LITERAL;
            token->data.booll = true;
        } else if (str_eqc(token_content, "false")) {
            token->type = TokenType_BOOL_LITERAL;
            token->data.booll = false;
        } else {
            token->data.ident = token_content;
        }
        break;

    case TokenType_INT_LITERAL: {
        unsigned int parsed;
        size_t n_chars = str_parse_uint_dec(token_content, &parsed);

        if (n_chars != token_content.size) {
            ct_err_array_append(errors, (DocumentError) {
                .source = source,
                .message = STR_CONST("failed to parse an integer"),
                .location = token->range
            });
            return;
        }

        token->data.intl = (int) parsed;
        break;
    }

    case TokenType_FLOAT_LITERAL: {
        unsigned int whole, frac;
        size_t n_chars_whole = str_parse_uint_dec(token_content, &whole);
        String frac_str = str_substring(token_content, n_chars_whole + 1, token_content.size);
        size_t n_chars_frac = str_parse_uint_dec(frac_str, &frac);

        if (n_chars_whole + n_chars_frac + 1 != token_content.size) {
            ct_err_array_append(errors, (DocumentError) {
                .source = source,
                .message = STR_CONST("failed to parse a float"),
                .location = token->range
            });
            return;
        }

        unsigned int frac_size = 1;
        for (size_t i = 0; i < n_chars_frac; i++) {
            frac_size *= 10;
        }
        token->data.floatl = (float) whole + ((float) frac) / ((float) frac_size);
        break;
    }

    case TokenType_OPERATOR:
        token->data.op = ct_document_substring(source, token->range);
        break;

    case TokenType_INVALID:
        ct_err_array_append(errors, (DocumentError) {
            .source = source,
            .message = STR_CONST("invalid token"),
            .location = token->range
        });
        break;

    default:
        return;
    }

    return;
}

#define TRY_TOKEN_READER(READER) \
    consumed = READER(rest, &current, config); \
    if (consumed > 0) { \
        current.range = ct_document_range(cursor, consumed); \
        ct_tokenizer_push_token(t, current); \
        str_assign(&rest, str_substring(rest, consumed, rest.size)); \
        ct_document_pos_track(&cursor, consumed); \
        continue; \
    }

void ct_tokenizer_run(Tokenizer *t, String source, LangConfig config, DocumentErrorArray *errors) {
    String rest = source;
    size_t consumed;
    Token current;
    DocumentPos cursor = ct_document_pos_zero();

    consumed = ct_tokenizer_skip_ws(rest, &cursor);
    str_assign(&rest, str_substring(rest, consumed, rest.size));

    while (rest.size > 0) {
        TRY_TOKEN_READER(ct_tokenizer_read_number)
        TRY_TOKEN_READER(ct_tokenizer_read_op)
        TRY_TOKEN_READER(ct_tokenizer_read_paren)
        TRY_TOKEN_READER(ct_tokenizer_read_ident)

        consumed = ct_tokenizer_skip_ws(rest, &cursor);
        if (consumed > 0) {
            str_assign(&rest, str_substring(rest, consumed, rest.size));
            continue;
        }

        // cannot consume the rest, must be invalid input
        current.type = TokenType_INVALID;
        current.range = ct_document_range(cursor, 1);
        ct_tokenizer_push_token(t, current);
        str_assign(&rest, str_substring(rest, 1, rest.size));
        ct_document_pos_track(&cursor, 1);
    }

    for (size_t i = 0; i < t->tokens.size; i++) {
        ct_tokenizer_parse_token(&t->tokens.content[i], source, errors);
    }
}

void ct_token_to_string(Token token, StringBuilder *sb) {
    const size_t buffer_size = 128;
    char buffer[buffer_size];

    switch (token.type) {
    case TokenType_IDENT:
        strb_appendc(sb, "<ident> ");
        break;
    case TokenType_INT_LITERAL:
        snprintf(buffer, buffer_size, "<int:%d> ", token.data.intl);
        strb_appendc(sb, buffer);
        break;
    case TokenType_FLOAT_LITERAL:
        snprintf(buffer, buffer_size, "<float:%f> ", token.data.floatl);
        strb_appendc(sb, buffer);
        break;
    case TokenType_BOOL_LITERAL:
        if (token.data.booll) {
            strb_appendc(sb, "<true> ");
        } else {
            strb_appendc(sb, "<false> ");
        }
        break;
    case TokenType_OPERATOR:
        snprintf(buffer, buffer_size, "<operator:" STR_FMT "> ", STR_FMT_VAL(token.data.op));
        strb_appendc(sb, buffer);
        break;
    case TokenType_OPEN_PAREN:
        strb_appendc(sb, "( ");
        break;
    case TokenType_CLOSE_PAREN:
        strb_appendc(sb, ") ");
        break;

    case TokenType_INVALID:
        strb_appendc(sb, "<?>");
        break;

    default:
        strb_appendc(sb, "?");
        break;
    }
}

void ct_tokenizer_print(Tokenizer *t) {
    StringBuilder *sb = strb_new(t->tokens.arena, 128);

    for (size_t i = 0; i < t->tokens.size; i++) {
        ct_token_to_string(t->tokens.content[i], sb);
    }

    String result = strb_render(sb);
    printf(STR_FMT "\n", STR_FMT_VAL(result));
}
