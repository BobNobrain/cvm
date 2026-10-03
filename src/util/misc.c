#include <stdio.h>
#include "util_int.h"

void _util_die(const char* file, int line, const char* msg) {
    fprintf(stderr, "%s:%d: error: %s\n", file, line, msg);
    abort();
}

void _util_die_out_of_bounds(
    const char* file,
    int line,
    const char* method,
    size_t index,
    size_t size
) {
    char msg[128];
    snprintf(msg, 128, "out of bounds: %s(%zu >= %zu)", method, index, size);
    _util_die(file, line, msg);
}
