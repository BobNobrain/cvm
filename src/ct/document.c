#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct_int.h"

ARRAY_METHODS_IMPL(ct_err_array, DocumentError)

DocumentPos ct_document_pos_zero() {
    DocumentPos zero = {
        .caret = 0,
        .line = 1,
        .column = 1,
    };
    return zero;
}

void ct_document_pos_track(DocumentPos *pos, size_t chars) {
    pos->caret += chars;
    pos->column += chars;
}
void ct_document_pos_line_break(DocumentPos *pos) {
    pos->caret += 1;
    pos->column = 1;
    pos->line += 1;
}

DocumentRange ct_document_range(DocumentPos start, size_t length) {
    DocumentRange result = {
        .start = start,
        .end = start
    };
    ct_document_pos_track(&result.end, length);
    return result;
}

DocumentRange ct_document_range_span(DocumentRange from, DocumentRange to) {
    DocumentRange result = {
        .start = from.start,
        .end = to.end
    };
    if (to.start.caret < from.start.caret) {
        result.start = to.start;
    }
    if (from.end.caret > to.end.caret) {
        result.end = from.end;
    }
    return result;
}

int ct_document_range_length(DocumentRange range) {
    int start = (int) range.start.caret;
    int end = (int) range.end.caret;
    return end - start;
}

String ct_document_substring(String source, DocumentRange range) {
    return str_substring(source, range.start.caret, range.end.caret);
}

void ct_document_set_error(DocumentError *error, char *c_msg, DocumentRange location) {
    str_assign(&error->message, str_wrap(c_msg));
    error->location = location;
}

void ct_document_print_error(DocumentError error, Arena *arena) {
    String near = ct_document_substring(error.source, error.location);
    String escaped = str_replace_all(near, STR_CONST("\n"), STR_CONST("\\n"), arena);

    printf(
        STR_FMT "\n  at %zu:%zu-%zu:%zu (near '" STR_FMT "')\n",
        STR_FMT_VAL(error.message),
        error.location.start.line,
        error.location.start.column,
        error.location.end.line,
        error.location.end.column,
        STR_FMT_VAL(escaped)
    );
}
