#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct_int.h"

ARRAY_METHODS_IMPL(ct_diagnostic_array, Diagnostic)

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

void ct_diagnostic_print(Diagnostic d, String source_doc, Arena *arena) {
    String near = ct_document_substring(source_doc, d.location);
    String escaped = str_replace_all(near, STR_CONST("\n"), STR_CONST("\\n"), arena);

    switch (d.severity) {
    case DiagnosticSeverity_ERROR: printf("[ERROR] "); break;
    case DiagnosticSeverity_WARN: printf("[WARN] "); break;
    case DiagnosticSeverity_INFO: printf("[INFO] "); break;
    }

    switch (d.source) {
    case DiagnosticSource_SYNTAX: printf("[SYNTAX] "); break;
    case DiagnosticSource_TYPECHECK: printf("[TYPECHECK] "); break;
    default: break;
    }

    printf(
        STR_FMT "\n      at %zu:%zu-%zu:%zu (near '" STR_FMT "')\n",
        STR_FMT_VAL(d.message),
        d.location.start.line,
        d.location.start.column,
        d.location.end.line,
        d.location.end.column,
        STR_FMT_VAL(escaped)
    );
}
