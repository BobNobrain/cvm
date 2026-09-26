#include <string.h>
#include "util_int.h"

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

typedef struct {
    char *content;
    size_t size;
    size_t capacity;
} StringBuilder;

StringBuilder *strb_new(size_t cap) {
    if (cap == 0) {
        cap = 64;
    }

    StringBuilder *sb = malloc(sizeof(StringBuilder));

    sb->content = malloc(cap * sizeof(char));
    if (sb->content == 0) {
        return 0;
    }

    sb->size = 0;
    sb->capacity = cap;
    return sb;
}

error strb_append(StringBuilder *sb, const String str) {
    if (sb->size + str.size > sb->capacity) {
        size_t cap_incr = sb->capacity;

        if (cap_incr == 0) {
            cap_incr = 64;
        } else if (cap_incr > 1024) {
            cap_incr = 1024;
        }

        if (cap_incr < str.size) {
            cap_incr = str.size;
        }

        sb->content = realloc(sb->content, sb->capacity + cap_incr);
        if (sb->content == 0) {
            return E_MEMORY;
        }

        sb->capacity += cap_incr;
    }

    for (size_t i = 0; i < str.size; i++) {
        sb->content[sb->size + i] = str.content[i];
    }

    sb->size += str.size;
    return E_NONE;
}

error strb_appendc(StringBuilder *sb, const char *c_str) {
    return strb_append(sb, str_wrap(c_str));
}

String strb_render(StringBuilder *sb) {
    String result = {
        .content = realloc(sb->content, sb->size),
        .size = sb->size
    };

    sb->content = 0;
    sb->size = 0;
    sb->capacity = 0;

    if (result.content == 0) {
        return EMPTY_STRING;
    }

    return result;
}
