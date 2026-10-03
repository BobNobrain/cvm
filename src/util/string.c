#include <string.h>
#include "util_int.h"

String str_wrap(char *c_str) {
    size_t size = strlen(c_str);
    String result = { .content = c_str, .size = size };
    return result;
}

bool str_is_empty(String s) {
    return s.size == 0;
}

String str_substring(String source, size_t start, size_t end) {
    if (start > source.size) {
        return STR_EMPTY;
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

bool str_eq(String s1, String s2) {
    if (s1.size != s2.size) { return false; }
    for (size_t i = 0; i < s1.size; i++) {
        if (s1.content[i] != s2.content[i]) {
            return false;
        }
    }
    return true;
}

size_t str_parse_uint_dec(String str, unsigned int *into) {
    *into = 0;

    for (size_t i = 0; i < str.size; i++) {
        char next = str.content[i];
        if (next < '0' || '9' < next) {
            return i;
        }

        *into *= 10;
        *into += next - '0';
    }

    return str.size;
}

int str_index_of(String str, char needle) {
    for (size_t i = 0; i < str.size; i++) {
        if (str.content[i] == needle) {
            return (int) i;
        }
    }

    return -1;
}

typedef struct {
    char *content;
    size_t size;
    size_t capacity;
    Arena *arena;
} StringBuilder;

StringBuilder *strb_new(Arena *arena, size_t cap) {
    if (cap == 0) {
        cap = 64;
    }

    StringBuilder *sb = arena_alloc(arena, sizeof(StringBuilder));
    sb->arena = arena;
    sb->content = arena_alloc(arena, cap * sizeof(char));

    sb->size = 0;
    sb->capacity = cap;
    return sb;
}

void strb_append(StringBuilder *sb, const String str) {
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

        sb->content = arena_realloc(sb->arena, sb->content, sb->capacity, sb->capacity + cap_incr);
        sb->capacity += cap_incr;
    }

    for (size_t i = 0; i < str.size; i++) {
        sb->content[sb->size + i] = str.content[i];
    }

    sb->size += str.size;
}

void strb_appendc(StringBuilder *sb, char *c_str) {
    strb_append(sb, str_wrap(c_str));
}

String strb_render(StringBuilder *sb) {
    String result = {
        .content = arena_realloc(sb->arena, sb->content, sb->capacity, sb->size),
        .size = sb->size
    };

    sb->content = 0;
    sb->size = 0;
    sb->capacity = 0;

    return result;
}
