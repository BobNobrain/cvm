#ifndef STR_H
#define STR_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "hoduli.h"

typedef struct String {
    const char *content;
    const size_t size;
} String;

const String EMPTY_STRING = { .content = 0, .size = 0 };

String str_wrap(const char *c_str) {
    size_t size = strlen(c_str);
    String result = { .content = c_str, .size = size };
    return result;
}

String str_substring(String source, size_t start, size_t end) {
    if (start > source.size) {
        return EMPTY_STRING;
    }
    if (end > source.size) {
        end = source.size;
    }

    String result = {
        .content = &source.content[start],
        .size = end - start
    };
    return result;
}

void str_assign(String *into, String value) {
    memcpy(into, &value, sizeof(String));
}

size_t str_compc(String str, const char* c_str, size_t n) {
    size_t len = n;
    if (str.size < len) {
        len = str.size;
    }

    for (size_t i = 0; i < len; i++) {
        if (str.content[i] != c_str[i]) {
            return i;
        }
    }

    return len;
}

bool str_eqc(String str, const char* c_str) {
    for (size_t i = 0; i < str.size; i++) {
        if (c_str[i] == '\0') {
            return false;
        }
        if (str.content[i] != c_str[i]) {
            return false;
        }
    }
    return true;
}

size_t str_parse_uint_dec(String str, unsigned int *into) {
    *into = 0;

    unsigned int place = 1;

    for (size_t i = 0; i < str.size; i++) {
        char next = str.content[str.size - i - 1];
        if (next < '0' || '9' < next) {
            return i;
        }

        *into += place * (next - '0');
        place *= 10;
    }

    return str.size;
}

void str_print(String str) {
    printf("%.*s", (int)str.size, str.content);
}

void str_debug_print(String str) {
    if (str.content == 0) {
        printf("''[empty]");
        return;
    }
    printf("'%.*s'[%zu]", (int)str.size, str.content, str.size);
}

typedef struct StringWriter {
    char *content;
    size_t size;
    size_t capacity;
} StringWriter;

error strw_init(StringWriter *sw, size_t cap) {
    if (cap == 0) {
        cap = 64;
    }

    sw->content = malloc(cap * sizeof(char));
    if (sw->content == 0) {
        return E_MEMORY;
    }

    sw->size = 0;
    sw->capacity = cap;
    return E_NONE;
}

error strw_append(StringWriter *sw, const String str) {
    if (sw->size + str.size > sw->capacity) {
        size_t cap_incr = sw->capacity;

        if (cap_incr == 0) {
            cap_incr = 64;
        } else if (cap_incr > 1024) {
            cap_incr = 1024;
        }

        if (cap_incr < str.size) {
            cap_incr = str.size;
        }

        sw->content = realloc(sw->content, sw->capacity + cap_incr);
        if (sw->content == 0) {
            return E_MEMORY;
        }

        sw->capacity += cap_incr;
    }

    for (size_t i = 0; i < str.size; i++) {
        sw->content[sw->size + i] = str.content[i];
    }

    sw->size += str.size;
    return E_NONE;
}

error strw_appendc(StringWriter *sw, const char *c_str) {
    return strw_append(sw, str_wrap(c_str));
}

String strw_render(StringWriter *sw) {
    String result = {
        .content = realloc(sw->content, sw->size),
        .size = sw->size
    };

    sw->content = 0;
    sw->size = 0;
    sw->capacity = 0;

    if (result.content == 0) {
        return EMPTY_STRING;
    }

    return result;
}

#endif
