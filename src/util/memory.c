#include <stdio.h>
#include "util_int.h"

void *malloc_or_die(size_t size) {
    void *ptr = malloc(size);
    if (ptr == 0) {
        fprintf(stderr, "Dead due to malloc failure\n");
        abort();
    }
    return ptr;
}

void *realloc_or_die(void *old, size_t size) {
    void *ptr = realloc(old, size);
    if (ptr == 0) {
        fprintf(stderr, "Dead due to realloc failure\n");
        abort();
    }
    return ptr;
}
