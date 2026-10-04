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

void ct_tokenizer_init(Tokenizer *t, Arena *arena, LangConfig config) {
    ct_tokenarray_init(&t->tokens, 128, arena);
    t->config = config;
    t->cursor = ct_document_pos_zero();
    t->source = STR_EMPTY;
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

typedef struct TokenScannerResult {
    size_t chars_scanned;
    bool cursor_updated;
    bool token_set;
    bool token_range_set;
} TokenScannerResult;

TokenScannerResult _ct_result_ok(size_t chars_scanned) {
    return (TokenScannerResult) {
        .chars_scanned = chars_scanned,
        .cursor_updated = false,
        .token_set = true,
        .token_range_set = false,
    };
}
TokenScannerResult _ct_result_fail() {
    return (TokenScannerResult) {
        .chars_scanned = 0,
        .cursor_updated = false,
        .token_set = false,
        .token_range_set = false,
    };
}

size_t _ct_tokenizer_scan_ws(Tokenizer *t) {
    for (size_t i = 0; i < t->source.size; i++) {
        char next = t->source.content[i];

        switch (next) {
        case ' ':
        case '\t':
            break;

        default:
            return i;
        }
    }

    return t->source.size;
}

TokenScannerResult _ct_tokenizer_scan_newline(Tokenizer *t, Token *into) {
    if (t->source.size == 0) { return _ct_result_fail(); }

    size_t consumed = 0;
    char next = t->source.content[0];

    if (next == '\n') {
        consumed = 1;
    } else if (next == '\r') {
        if (t->source.size > 1 && t->source.content[1] == '\n') {
            consumed = 2;
        } else {
            consumed = 1;
        }
    }

    if (consumed > 0) {
        into->type = TokenType_NEWLINE;
        into->range = ct_document_range(t->cursor, consumed);
        ct_document_pos_line_break(&t->cursor);
        return (TokenScannerResult) {
            .chars_scanned = consumed,
            .cursor_updated = true,
            .token_set = true,
            .token_range_set = true,
        };
    }

    return _ct_result_fail();
}

TokenScannerResult _ct_tokenizer_scan_number(Tokenizer *t, Token *into) {
    bool point_met = false;
    size_t i = 0;

    for (; i < t->source.size; i++) {
        char next = t->source.content[i];

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
        return _ct_result_fail();
    }

    if (point_met && i == 1) {
        // a single point is not a valid number literal
        into->type = TokenType_DOT;
        return _ct_result_ok(i);
    }

    if (point_met) {
        into->type = TokenType_FLOAT_LITERAL;
    } else {
        into->type = TokenType_INT_LITERAL;
    }

    return _ct_result_ok(i);
}

TokenScannerResult _ct_tokenizer_scan_ident(Tokenizer *t, Token *into) {
    size_t i = 0;

    for (; i < t->source.size; i++) {
        char next = t->source.content[i];

        if ('A' <= next && next <= 'Z') {
            continue;
        }
        if ('a' <= next && next <= 'z') {
            continue;
        }
        if ('0' <= next && next <= '9') {
            continue;
        }
        if (str_index_of(t->config.allowed_ident_chars, next) != -1) {
            continue;
        }

        break;
    }

    if (i == 0) {
        return _ct_result_fail();
    }

    into->type = TokenType_IDENT;
    return _ct_result_ok(i);
}

TokenScannerResult _ct_tokenizer_scan_op(Tokenizer *t, Token *into) {
    size_t i = 0;
    for (; i < t->source.size; i++) {
        char next = t->source.content[i];
        if (str_index_of(t->config.allowed_operator_chars, next) == -1) {
            break;
        }
    }

    if (i == 0) {
        return _ct_result_fail();
    }

    into->type = TokenType_OPERATOR;
    return _ct_result_ok(i);
}

TokenScannerResult _ct_tokenizer_scan_paren(Tokenizer *t, Token *into) {
    if (t->source.size == 0) {
        return _ct_result_fail();
    }

    char next = t->source.content[0];
    switch (next) {
    case '(':
        into->type = TokenType_OPEN_PAREN;
        break;
    case ')':
        into->type = TokenType_CLOSE_PAREN;
        break;

    default:
        return _ct_result_fail();
    }

    return _ct_result_ok(1);
}

TokenScannerResult _ct_tokenizer_scan_builtin_op(Tokenizer *t, Token *into) {
    if (t->source.size == 0) {
        return _ct_result_fail();
    }

    char next = t->source.content[0];
    switch (next) {
    case '\\':
        into->type = TokenType_LAMBDA;
        break;
    case '.':
        into->type = TokenType_DOT;
        break;
    case '=':
        into->type = TokenType_ASSIGNMENT;
        break;

    default:
        return _ct_result_fail();
    }

    return _ct_result_ok(1);
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
        } else if (str_eqc(token_content, "let")) {
            token->type = TokenType_KEYWORD;
            token->data.keyword = Keyword_LET;
        } else if (str_eqc(token_content, "if")) {
            token->type = TokenType_KEYWORD;
            token->data.keyword = Keyword_IF;
        } else if (str_eqc(token_content, "else")) {
            token->type = TokenType_KEYWORD;
            token->data.keyword = Keyword_ELSE;
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
        // in case these built-in operators were caught by the allowed operator characters set
        if (str_eq(token->data.op, STR_CONST("="))) {
            token->type = TokenType_ASSIGNMENT;
        } else if (str_eq(token->data.op, STR_CONST("\\"))) {
            token->type = TokenType_LAMBDA;
        }
         else if (str_eq(token->data.op, STR_CONST("."))) {
            token->type = TokenType_DOT;
        }
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

void _ct_tokenizer_consume_source(Tokenizer *t, size_t n) {
    t->source = str_substring(t->source, n, t->source.size);
}

void ct_tokenizer_run(Tokenizer *t, String source, DocumentErrorArray *errors) {
    t->source = source;
    t->cursor = ct_document_pos_zero();

    Token current = { .type = TokenType_INVALID };
    TokenScannerResult scan_result = { 0 };

    #define TRY_TOKEN_SCANNER(READER) \
    scan_result = READER(t, &current);                                                      \
    if (scan_result.chars_scanned > 0) {                                                    \
        _ct_tokenizer_consume_source(t, scan_result.chars_scanned);                         \
        if (scan_result.token_set) {                                                        \
            if (!scan_result.token_range_set) {                                             \
                current.range = ct_document_range(t->cursor, scan_result.chars_scanned);    \
            }                                                                               \
            ct_tokenizer_push_token(t, current);                                            \
        }                                                                                   \
        if (!scan_result.cursor_updated) {                                                  \
            ct_document_pos_track(&t->cursor, scan_result.chars_scanned);                   \
        }                                                                                   \
        continue;                                                                           \
    }

    while (t->source.size > 0) {
        size_t ws_consumed = _ct_tokenizer_scan_ws(t);
        if (ws_consumed > 0) {
            _ct_tokenizer_consume_source(t, ws_consumed);
            ct_document_pos_track(&t->cursor, ws_consumed);

            if (t->source.size == 0) { break; }
        }

        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_number)
        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_op)
        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_builtin_op)
        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_paren)
        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_ident)
        TRY_TOKEN_SCANNER(_ct_tokenizer_scan_newline)

        // cannot consume the rest, must be invalid input
        current.type = TokenType_INVALID;
        current.range = ct_document_range(t->cursor, 1);
        ct_tokenizer_push_token(t, current);
        _ct_tokenizer_consume_source(t, 1);
        ct_document_pos_track(&t->cursor, 1);
    }

    #undef TRY_TOKEN_SCANNER

    for (size_t i = 0; i < t->tokens.size; i++) {
        ct_tokenizer_parse_token(&t->tokens.content[i], source, errors);
    }
}

void ct_token_to_string(Token token, StringBuilder *sb) {
    const size_t buffer_size = 128;
    char buffer[buffer_size];

    switch (token.type) {
    case TokenType_IDENT:
        strb_appendc(sb, "<ident:");
        strb_append(sb, token.data.ident);
        strb_appendc(sb, "> ");
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

    case TokenType_NEWLINE:
        strb_appendc(sb, "<\\n>\n");
        break;

    case TokenType_LAMBDA:
        strb_appendc(sb, "\\ ");
        break;

    case TokenType_DOT:
        strb_appendc(sb, ". ");
        break;

    case TokenType_ASSIGNMENT:
        strb_appendc(sb, "= ");
        break;

    case TokenType_KEYWORD:
        switch (token.data.keyword) {
            case Keyword_LET: strb_appendc(sb, "LET "); break;
            case Keyword_IF: strb_appendc(sb, "IF "); break;
            case Keyword_ELSE: strb_appendc(sb, "ELSE "); break;
            default: strb_appendc(sb, "<unknown kw> "); break;
        }
        break;

    case TokenType_INVALID:
        strb_appendc(sb, "<?> ");
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
