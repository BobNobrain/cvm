#ifndef UTIL_H
#define UTIL_H
/**
 * Utilities and helpers
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
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
#define ERR_ISSET (err != E_NONE)


/** Basic math */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max) ( ((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)) )

#define USING(INIT, RELEASE) \
    for (int i = 0, INIT; i < 1; ++i, (RELEASE))


/** Just dies with a message. */
#define die(MSG) _util_die(__FILE__, __LINE__, MSG)
extern void _util_die(const char* file, int line, const char* msg);
#define die_out_of_bounds(method, index, size) _util_die_out_of_bounds(__FILE__, __LINE__, method, index, size)
extern void _util_die_out_of_bounds(const char* file, int line, const char* method, size_t index, size_t size);


/** Same as malloc, but crashes the program if memory was not allocated */
extern void *malloc_or_die(size_t size);
/** Same as realloc, but crashes the program if memory was not reallocated */
extern void *realloc_or_die(void *old, size_t size);


/** String is a simple wrapper around C strings that allows seamless slicing. It is not zero-terminated. */
typedef struct {
    char *content;
    size_t size;
} String;

/** Wraps a zero-terminated C string into String struct (points to the same underlying data) */
extern String str_wrap(char *c_str);
extern String str_substring(String source, size_t start, size_t end);
extern void str_assign(String *into, String value);
extern size_t str_compc(String str, const char* c_str, size_t n);
extern bool str_eqc(String str, const char* c_str);
extern bool str_eq(String s1, String s2);
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
extern void strb_append(StringBuilder *sb, const String str);
extern void strb_appendc(StringBuilder *sb, const char *c_str);
extern String strb_render(StringBuilder *sb);
#endif


/** Slices */
#define SLICE_DECL(T_ELEM) \
    typedef struct {        \
        T_ELEM *content;    \
        size_t size;        \
    } T_ELEM##Slice;

#define SLICE_METHODS_DECL(PREFIX, T_ELEM) \
    extern void PREFIX##_destroy(T_ELEM##Slice *slice);                                 \
    extern T_ELEM##Slice PREFIX##_slice(T_ELEM##Slice slice, size_t start, size_t end); \
    extern T_ELEM *PREFIX##_at(T_ELEM##Slice slice, size_t at);                         \
    extern T_ELEM##Slice PREFIX##_of_const(T_ELEM array[], size_t length);

#define SLICE_METHODS_IMPL(PREFIX, T_ELEM) \
    void PREFIX##_destroy(T_ELEM##Slice *slice) {                                   \
        if (slice->content != 0) { free(slice->content); }                          \
        slice->content = 0; slice->size = 0;                                        \
    }                                                                               \
    T_ELEM##Slice PREFIX##_slice(T_ELEM##Slice slice, size_t start, size_t end) {   \
        T_ELEM##Slice result = { .content = 0, .size = 0 };                         \
        if (start >= slice.size || end <= start) { return result; }                  \
        result.content = &slice.content[start];                                     \
        result.size = end - start;                                                  \
        return result;                                                              \
    }                                                                               \
    T_ELEM *PREFIX##_at(T_ELEM##Slice slice, size_t at) {                           \
        if (at >= slice.size) {                                                     \
            die_out_of_bounds(#PREFIX "_at", at, slice.size);                       \
        }                                                                           \
        return &slice.content[at];                                                  \
    }                                                                               \
    T_ELEM##Slice PREFIX##_of_const(T_ELEM array[], size_t size) {            \
        T_ELEM##Slice result = { .content = &array[0], .size = size };              \
        return result;                                                              \
    }

/** Dynamic arrays */
#define ARRAY_DECL(T_ELEM) \
    typedef struct {        \
        T_ELEM *content;    \
        size_t size;        \
        size_t capacity;    \
    } T_ELEM##Array;

#define ARRAY_METHODS_DECL(PREFIX, T_ELEM) \
    extern void PREFIX##_init(T_ELEM##Array *arr, size_t initial_capacity); \
    extern void PREFIX##_append(T_ELEM##Array *arr, T_ELEM element);        \
    extern size_t PREFIX##_cut(T_ELEM##Array *arr, size_t start, size_t n); \
    extern T_ELEM *PREFIX##_at(T_ELEM##Array *arr, size_t at);              \
    extern void PREFIX##_destroy(T_ELEM##Array *arr);

#define ARRAY_METHODS_IMPL(PREFIX, T_ELEM) \
    void PREFIX##_init(T_ELEM##Array *arr, size_t initial_capacity) {       \
        if (initial_capacity == 0) { initial_capacity = 8; }                \
        arr->content = malloc_or_die(initial_capacity * sizeof(T_ELEM));    \
        arr->size = 0; arr->capacity = initial_capacity;                    \
    }                                                                       \
    void PREFIX##_append(T_ELEM##Array *arr, T_ELEM element) {              \
        if (arr->size >= arr->capacity) {                                   \
            size_t new_cap = arr->capacity + MIN(arr->capacity * 2, 4096);  \
            arr->content = realloc_or_die(arr->content, new_cap);           \
        }                                                                   \
        arr->content[arr->size] = element; arr->size += 1;                  \
    }                                                                       \
    size_t PREFIX##_cut(T_ELEM##Array *arr, size_t start, size_t n) {       \
        if (start >= arr->size || n == 0) { return 0; }                     \
        if (start + n >= arr->size) { n = arr->size - start; }              \
        for (size_t i = start; i < arr->size - n; i++) {                    \
            arr->content[i] = arr->content[i + n];                          \
        }                                                                   \
        arr->size -= n; return n;                                           \
    }                                                                       \
    T_ELEM *PREFIX##_at(T_ELEM##Array *arr, size_t at) {                    \
        if (at >= arr->size) {                                              \
            die_out_of_bounds(#PREFIX "_at", at, arr->size);                \
        }                                                                   \
        return &arr->content[at];                                           \
    }                                                                       \
    void PREFIX##_destroy(T_ELEM##Array *arr) {                             \
        if (arr->content != 0) { free(arr->content); }                      \
        arr->content = 0; arr->size = 0; arr->capacity = 0;                 \
    }

/** Methods for when you have both XArray and XSlice */
#define SLICE_ARRAY_METHODS_DECL(PREFIX, T_ELEM) \
    extern T_ELEM##Slice PREFIX##_seal(T_ELEM##Array *arr);

#define SLICE_ARRAY_METHODS_IMPL(PREFIX, T_ELEM) \
    T_ELEM##Slice PREFIX##_seal(T_ELEM##Array *arr) {           \
        T_ELEM##Slice result = {                                \
            .content = realloc_or_die(arr->content, arr->size), \
            .size = arr->size                                   \
        };                                                      \
        PREFIX##_destroy(arr); return result;                   \
    }

#endif
