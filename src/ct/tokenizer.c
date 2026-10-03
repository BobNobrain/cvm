#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "ct_int.h"

ARRAY_METHODS_IMPL(ct_tokenarray, Token)

void ct_tokenizer_init(Tokenizer *t, Arena *arena) {
    t->size = 0;
    t->capacity = 128;
    t->tokens = arena_alloc(arena, t->capacity * sizeof(Token));
    t->arena = arena;
}

void ct_tokenizer_push_token(Tokenizer *t, Token token) {
    if (t->size >= t->capacity) {
        size_t old_cap = t->capacity;
        t->capacity += 128;
        t->tokens = arena_realloc(t->arena, t->tokens, old_cap, t->capacity);
    }

    memcpy(&t->tokens[t->size], &token, sizeof(token));
    t->size += 1;
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

size_t ct_tokenizer_read_number(String source, Token *into) {
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

size_t ct_tokenizer_read_ident(String source, Token *into) {
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

size_t ct_tokenizer_read_op(String source, Token *into) {
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

size_t ct_tokenizer_read_paren(String source, Token *into) {
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

bool ct_tokenizer_parse_token(Token *token, String source, DocumentError *error) {
    String token_content = ct_document_substring(source, token->range);

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
            ct_document_set_error(error, "failed to parse an integer", token->range);
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
            ct_document_set_error(error, "failed to parse a float", token->range);
            return false;
        }

        unsigned int frac_size = 1;
        for (size_t i = 0; i < n_chars_frac; i++) {
            frac_size *= 10;
        }
        token->data.floatl = (float) whole + ((float) frac) / ((float) frac_size);
        break;
    }

    case TOKEN_OPERATOR:
        token->data.op = ct_document_substring(source, token->range);

    default:
        return true;
    }

    return true;
}

#define TRY_TOKEN_READER(READER) \
    consumed = READER(rest, &current); \
    if (consumed > 0) { \
        current.range = ct_document_range(cursor, consumed); \
        ct_tokenizer_push_token(t, current); \
        str_assign(&rest, str_substring(rest, consumed, rest.size)); \
        ct_document_pos_track(&cursor, consumed); \
        continue; \
    }

error ct_tokenizer_run(Tokenizer *t, String source, DocumentError *docerr) {
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

        return E_BAD_DATA;
    }

    for (size_t i = 0; i < t->size; i++) {
        if (!ct_tokenizer_parse_token(&t->tokens[i], source, docerr)) {
            return E_BAD_DATA;
        }
    }

    return E_NONE;
}

void ct_token_to_string(Token token, StringBuilder *sb) {
    const size_t buffer_size = 128;
    char buffer[buffer_size];

    switch (token.type) {
    case TOKEN_IDENT:
        strb_appendc(sb, "<ident>");
        break;
    case TOKEN_INT_LITERAL:
        snprintf(buffer, buffer_size, "<int:%d>", token.data.intl);
        strb_appendc(sb, buffer);
        break;
    case TOKEN_FLOAT_LITERAL:
        snprintf(buffer, buffer_size, "<float:%f>", token.data.floatl);
        strb_appendc(sb, buffer);
        break;
    case TOKEN_BOOL_LITERAL:
        if (token.data.booll) {
            strb_appendc(sb, "<true>");
        } else {
            strb_appendc(sb, "<false>");
        }
        break;
    case TOKEN_OPERATOR:
        snprintf(buffer, buffer_size, "<operator:" STR_FMT ">", STR_FMT_VAL(token.data.op));
        strb_appendc(sb, buffer);
        break;
    case TOKEN_OPEN_PAREN:
        strb_appendc(sb, "(");
        break;
    case TOKEN_CLOSE_PAREN:
        strb_appendc(sb, ")");
        break;

    default:
        strb_appendc(sb, "?");
        break;
    }
}

void ct_tokenizer_print(Tokenizer *t) {
    StringBuilder *sb = strb_new(t->arena, 128);

    for (size_t i = 0; i < t->size; i++) {
        ct_token_to_string(t->tokens[i], sb);
    }

    String result = strb_render(sb);
    printf(STR_FMT "\n", STR_FMT_VAL(result));
}
