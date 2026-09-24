#ifndef TOKENS_H
#define TOKENS_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "str.h"

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
    String source;
    union {
        TOKEN_LIST(TOKEN_LIST_X)
    } data;
} Token;
#undef TOKEN_LIST_X

typedef struct Tokenizer {
    size_t size;
    size_t capacity;
    Token *tokens;
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

size_t tokenizer_skip_ws(String source) {
    for (size_t i = 0; i < source.size; i++) {
        char next = source.content[i];
        switch (next) {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
            continue;

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

typedef struct TokenParseError {
    String message;
    String source;
    size_t start;
} TokenParseError;

void tokenizer_set_error_message(TokenParseError *e, String msg) {
    str_assign(&e->message, msg);
}
void tokenizer_set_error_source(TokenParseError *e, String src) {
    memcpy(&e->source, &src, sizeof(src));
}

bool tokenizer_parse_token(Token *token, String source, TokenParseError *error) {
    switch (token->type) {
    case TOKEN_IDENT:
        if (str_eqc(token->source, "true")) {
            token->type = TOKEN_BOOL_LITERAL;
            token->data.booll = true;
        } else if (str_eqc(token->source, "false")) {
            token->type = TOKEN_BOOL_LITERAL;
            token->data.booll = false;
        }
        break;

    case TOKEN_INT_LITERAL: {
        unsigned int parsed;
        size_t n_chars = str_parse_uint_dec(token->source, &parsed);

        if (n_chars != token->source.size) {
            str_assign(&error->message, str_wrap("failed to parse an integer"));
            str_assign(&error->source, token->source);
            error->start = token->source.content - source.content;
            return false;
        }

        token->data.intl = (int) parsed;
        break;
    }

    case TOKEN_FLOAT_LITERAL: {
        unsigned int whole, frac;
        size_t n_chars_whole = str_parse_uint_dec(token->source, &whole);
        String frac_str = str_substring(token->source, n_chars_whole + 1, token->source.size);
        size_t n_chars_frac = str_parse_uint_dec(frac_str, &frac);

        if (n_chars_whole + n_chars_frac + 1 != token->source.size) {
            printf("%zu %zu\n", n_chars_whole, n_chars_frac);
            str_assign(&error->message, str_wrap("failed to parse a float"));
            str_assign(&error->source, token->source);
            error->start = token->source.content - source.content;
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
        str_assign(&current.source, str_substring(rest, 0, consumed)); \
        ERR_PASS( tokenizer_push_token(t, current) ) \
        str_assign(&rest, str_substring(rest, consumed, rest.size)); \
        str_debug_print(rest); \
        printf("\n  - %zu consumed by " #READER "\n", consumed); \
        continue; \
    }

error tokenizer_run(Tokenizer *t, String source) {
    ERR_DECL

    String rest = source;
    size_t consumed;
    Token current;

    consumed = tokenizer_skip_ws(rest);
    str_assign(&rest, str_substring(rest, consumed, rest.size));
    str_debug_print(rest);
    printf("\n  - %zu consumed by initial WS, %zu remains\n", consumed, rest.size);

    while (rest.size > 0) {
        TRY_TOKEN_READER(tokenizer_read_number)
        TRY_TOKEN_READER(tokenizer_read_op)
        TRY_TOKEN_READER(tokenizer_read_paren)

        consumed = tokenizer_skip_ws(rest);
        if (consumed > 0) {
            str_assign(&rest, str_substring(rest, consumed, rest.size));
            str_debug_print(rest);
            printf("\n  - %zu consumed by WS\n", consumed);
            continue;
        }

        return E_BAD_DATA;
    }

    TokenParseError tperr;
    for (size_t i = 0; i < t->size; i++) {
        if (!tokenizer_parse_token(&t->tokens[i], source, &tperr)) {
            printf("Parsing failed: ");
            str_print(tperr.message);
            printf("\n  near '");
            str_print(tperr.source);
            printf("', pos %zu\n", tperr.start);
            return E_BAD_DATA;
        }
    }

    return E_NONE;
}

#undef TRY_TOKEN_READER

#endif
