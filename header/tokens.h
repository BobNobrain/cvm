#ifndef TOKENS_H
#define TOKENS_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "str.h"
#include "document.h"

#define TOKEN_LIST(X) \
    X(TOKEN_IDENT, , ) \
    X(TOKEN_INT_LITERAL, int, intl) \
    X(TOKEN_FLOAT_LITERAL, float, floatl) \
    X(TOKEN_BOOL_LITERAL, bool, booll) \
    X(TOKEN_OPERATOR, String, op) \
    X(TOKEN_OPEN_PAREN, , ) \
    X(TOKEN_CLOSE_PAREN, , )

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum TokenType {
    TOKEN_LIST(TOKEN_LIST_X)

    TOKEN_INVALID
} TokenType;
#undef TOKEN_LIST_X

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
typedef struct Token {
    TokenType type;
    DocumentRange range;
    union {
        TOKEN_LIST(TOKEN_LIST_X)
    } data;
} Token;
#undef TOKEN_LIST_X

typedef struct Tokenizer {
    Token *tokens;
    size_t size;
    size_t capacity;
} Tokenizer;

error tokenizer_init(Tokenizer *t) {
    t->size = 0;
    t->capacity = 128;
    t->tokens = malloc(t->capacity * sizeof(Token));
    if (t->tokens == 0) {
        t->capacity = 0;
        return E_MEMORY;
    }

    return E_NONE;
}

error tokenizer_push_token(Tokenizer *t, Token token) {
    if (t->size >= t->capacity) {
        t->capacity += 128;
        t->tokens = realloc(t->tokens, t->capacity);
        if (t->tokens == 0) {
            t->capacity = 0;
            return E_MEMORY;
        }
    }

    memcpy(&t->tokens[t->size], &token, sizeof(token));
    t->size += 1;
    return E_NONE;
}

size_t tokenizer_skip_ws(String source, DocumentPos *cursor) {
    for (size_t i = 0; i < source.size; i++) {
        char next = source.content[i];
        switch (next) {
        case '\n':
            document_pos_line_break(cursor);
            break;

        case ' ':
        case '\t':
        case '\r':
            document_pos_track(cursor, 1);
            break;

        default:
            return i;
        }
    }

    return source.size;
}

size_t tokenizer_read_number(String source, Token *into) {
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

    if (point_met) {
        into->type = TOKEN_FLOAT_LITERAL;
    } else {
        into->type = TOKEN_INT_LITERAL;
    }

    return i;
}

size_t tokenizer_read_ident(String source, Token *into) {
    size_t i = 0;

    for (; i < source.size; i++) {
        char next = source.content[i];

        if ('A' <= next && next <= 'Z') {
            continue;
        }
        if ('a' <= next && next <= 'z') {
            continue;
        }
        if (next == '_') {
            continue;
        }

        break;
    }

    if (i == 0) {
        return 0;
    }

    into->type = TOKEN_IDENT;
    return i;
}

size_t tokenizer_read_op(String source, Token *into) {
    size_t i = 0;
    for (; i < source.size; i++) {
        char next = source.content[i];
        bool ok = false;
        switch (next) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '=':
        case '<':
        case '>':
        case '!':
        case '&':
        case '|':
        case '^':
        case '~':
            ok = true;
            break;
        }

        if (!ok) {
            break;
        }
    }

    if (i == 0) {
        return 0;
    }

    into->type = TOKEN_OPERATOR;
    return i;
}

size_t tokenizer_read_paren(String source, Token *into) {
    if (source.size == 0) {
        return 0;
    }

    char next = source.content[0];
    switch (next) {
    case '(':
        into->type = TOKEN_OPEN_PAREN;
        break;
    case ')':
        into->type = TOKEN_CLOSE_PAREN;
        break;

    default:
        return 0;
    }

    return 1;
}

bool tokenizer_parse_token(Token *token, String source, DocumentError *error) {
    String token_content = document_substring(source, token->range);

    switch (token->type) {
    case TOKEN_IDENT:
        if (str_eqc(token_content, "true")) {
            token->type = TOKEN_BOOL_LITERAL;
            token->data.booll = true;
        } else if (str_eqc(token_content, "false")) {
            token->type = TOKEN_BOOL_LITERAL;
            token->data.booll = false;
        }
        break;

    case TOKEN_INT_LITERAL: {
        unsigned int parsed;
        size_t n_chars = str_parse_uint_dec(token_content, &parsed);

        if (n_chars != token_content.size) {
            document_set_error(error, "failed to parse an integer", token->range);
            return false;
        }

        token->data.intl = (int) parsed;
        break;
    }

    case TOKEN_FLOAT_LITERAL: {
        unsigned int whole, frac;
        size_t n_chars_whole = str_parse_uint_dec(token_content, &whole);
        String frac_str = str_substring(token_content, n_chars_whole + 1, token_content.size);
        size_t n_chars_frac = str_parse_uint_dec(frac_str, &frac);

        if (n_chars_whole + n_chars_frac + 1 != token_content.size) {
            document_set_error(error, "failed to parse a float", token->range);
            return false;
        }

        unsigned int frac_size = 1;
        for (size_t i = 0; i < n_chars_frac; i++) {
            frac_size *= 10;
        }
        token->data.floatl = (float) whole + ((float) frac) / ((float) frac_size);
        break;
    }

    default:
        return true;
    }

    return true;
}

#define TRY_TOKEN_READER(READER) \
    consumed = READER(rest, &current); \
    if (consumed > 0) { \
        current.range = document_range(cursor, consumed); \
        ERR_PASS( tokenizer_push_token(t, current) ) \
        str_assign(&rest, str_substring(rest, consumed, rest.size)); \
        document_pos_track(&cursor, consumed); \
        continue; \
    }

error tokenizer_run(Tokenizer *t, String source) {
    ERR_DECL

    String rest = source;
    size_t consumed;
    Token current;
    DocumentPos cursor = document_pos_zero();

    consumed = tokenizer_skip_ws(rest, &cursor);
    str_assign(&rest, str_substring(rest, consumed, rest.size));

    while (rest.size > 0) {
        TRY_TOKEN_READER(tokenizer_read_number)
        TRY_TOKEN_READER(tokenizer_read_op)
        TRY_TOKEN_READER(tokenizer_read_paren)
        TRY_TOKEN_READER(tokenizer_read_ident)

        consumed = tokenizer_skip_ws(rest, &cursor);
        if (consumed > 0) {
            str_assign(&rest, str_substring(rest, consumed, rest.size));
            continue;
        }

        return E_BAD_DATA;
    }

    DocumentError docerr = { .source = source };
    for (size_t i = 0; i < t->size; i++) {
        if (!tokenizer_parse_token(&t->tokens[i], source, &docerr)) {
            printf("Parsing failed: ");
            document_print_error(docerr);
            return E_BAD_DATA;
        }
    }

    return E_NONE;
}

#undef TRY_TOKEN_READER

#endif
