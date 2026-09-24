#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stdio.h>
#include <stddef.h>
#include "hoduli.h"
#include "str.h"

typedef struct DocumentPos {
    size_t caret;
    size_t line;
    size_t column;
} DocumentPos;

DocumentPos document_pos_zero() {
    DocumentPos zero = {
        .caret = 0,
        .line = 1,
        .column = 1,
    };
    return zero;
}

void document_pos_track(DocumentPos *pos, size_t chars) {
    pos->caret += chars;
    pos->column += chars;
}
void document_pos_line_break(DocumentPos *pos) {
    pos->caret += 1;
    pos->column = 1;
    pos->line += 1;
}

typedef struct DocumentRange {
    DocumentPos start;
    DocumentPos end;
} DocumentRange;

DocumentRange document_range(DocumentPos start, size_t length) {
    DocumentRange result = {
        .start = start,
        .end = start
    };
    document_pos_track(&result.end, length);
    return result;
}

int document_range_length(DocumentRange range) {
    int start = (int) range.start.caret;
    int end = (int) range.end.caret;
    return end - start;
}

String document_substring(String source, DocumentRange range) {
    return str_substring(source, range.start.caret, range.end.caret);
}

typedef struct DocumentError {
    String message;
    String source;
    DocumentRange location;
} DocumentError;

void document_set_error(DocumentError *error, char *c_msg, DocumentRange location) {
    str_assign(&error->message, str_wrap(c_msg));
    error->location = location;
}

void document_print_error(DocumentError error) {
    str_print(error.message);
    printf(
        "\n  at %zu:%zu-%zu:%zu (near '",
        error.location.start.line,
        error.location.start.column,
        error.location.end.line,
        error.location.end.column
    );
    str_print(document_substring(error.source, error.location));
    printf("')\n");
}

#endif
