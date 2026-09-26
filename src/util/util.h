#ifndef UTIL_H
#define UTIL_H
/**
 * Utilities and helpers
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    E_NONE,
    E_UNKNOWN,
    E_MEMORY,
    E_OUT_OF_RANGE,
    E_BAD_DATA,
} error;

#define ERR_DECL error err;
#define ERR_PASS(EXPR) err = (EXPR); if (err != 0) { return err; }
#define ERR_RET(RET, EXPR) err = (EXPR); if (err != 0) { return E; }
#define ERR_CHECK_NOT_NULL(PTR) = (PTR == 0 ? E_MEMORY : E_NONE)


/** Same as malloc, but crashes the program if memory was not allocated */
extern void *malloc_or_die(size_t size);
/** Same as realloc, but crashes the program if memory was not reallocated */
extern void *realloc_or_die(void *old, size_t size);


/** String is a simple wrapper around C strings that allows seamless slicing. It is not zero-terminated. */
typedef struct {
    const char *content;
    const size_t size;
} String;

/** Wraps a zero-terminated C string into String struct (points to the same underlying data) */
extern String str_wrap(const char *c_str);
extern String str_substring(String source, size_t start, size_t end);
extern void str_assign(String *into, String value);
extern size_t str_compc(String str, const char* c_str, size_t n);
extern bool str_eqc(String str, const char* c_str);
size_t str_parse_uint_dec(String str, unsigned int *into); // TODO: shouldn't be here?

/** to use in printf and alike:
    printf("the string is '" STR_FMT "'!", STR_FMT_VAL(my_string));
 */
#define STR_FMT "%.*s"
#define STR_FMT_VAL(STR) (int) ((STR).size), (STR).content
#define STR_FMT_DEBUG "'%.*s'[%zu]"
#define STR_FMT_DEBUG_VAL(STR) (int) ((STR).size), (STR).content, (STR).size


#ifndef UTIL_INTERNAL
/** StringBuilder utility to assemble large strings from multiple parts */
typedef void StringBuilder;

extern StringBuilder *strb_new(size_t cap);
extern error strb_append(StringBuilder *sb, const String str);
extern error strb_appendc(StringBuilder *sb, const char *c_str);
extern String strb_render(StringBuilder *sb);
#endif

#endif
