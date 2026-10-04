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
    E_STREAM
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

#ifndef UTIL_ARENA_IMPL
/** StringBuilder utility to assemble large strings from multiple parts */
typedef void Arena;
#else
typedef struct Arena Arena;
#endif

extern Arena* arena_new(size_t region_size);
extern void arena_destroy(Arena *arena);
extern void* arena_alloc(Arena *arena, size_t bytes);
extern void* arena_realloc(Arena *arena, void *ptr, size_t old_size, size_t new_size);
extern Arena* arena_global();
extern Arena* arena_TODO();
extern void arena_set_max_regions(Arena *arena, size_t max_regions);


/** String is a simple wrapper around C strings that allows seamless slicing. It is not zero-terminated. */
typedef struct String {
    char *content;
    size_t size;
} String;

#define STR_EMPTY (String) { 0 }
#define STR_CONST(C_STR) (String) { .content = C_STR, .size = sizeof(C_STR) - 1 }

/** Wraps a zero-terminated C string into String struct (points to the same underlying data) */
extern String str_wrap(char *c_str);
extern String str_substring(String source, size_t start, size_t end);
extern bool str_is_empty(String s);
extern void str_assign(String *into, String value);
extern size_t str_compc(String str, const char* c_str, size_t n);
extern bool str_eqc(String str, const char* c_str);
extern bool str_eq(String s1, String s2);
extern size_t str_parse_uint_dec(String str, unsigned int *into); // TODO: shouldn't be here?
extern int str_index_of(String str, char needle);

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

extern StringBuilder *strb_new(Arena *arena, size_t cap);
extern void strb_append(StringBuilder *sb, const String str);
extern void strb_appendc(StringBuilder *sb, const char *c_str);
extern error strb_read_from_stream(StringBuilder *sb, FILE *from);
extern String strb_render(StringBuilder *sb);
#endif


/** Slices */
#define SLICE_DECL_NAMED(T_ELEM, NAME) \
    typedef struct NAME {        \
        T_ELEM *content;    \
        size_t size;        \
    } NAME;

#define SLICE_METHODS_DECL_NAMED(PREFIX, T_ELEM, NAME) \
    extern void PREFIX##_destroy(NAME *slice);                          \
    extern NAME PREFIX##_slice(NAME slice, size_t start, size_t end);   \
    extern T_ELEM *PREFIX##_at(NAME slice, size_t at);                  \
    extern NAME PREFIX##_of_const(T_ELEM array[], size_t length);

#define SLICE_METHODS_IMPL_NAMED(PREFIX, T_ELEM, NAME) \
    void PREFIX##_destroy(NAME *slice) {                            \
        if (slice->content != 0) { free(slice->content); }          \
        slice->content = 0; slice->size = 0;                        \
    }                                                               \
    NAME PREFIX##_slice(NAME slice, size_t start, size_t end) {     \
        NAME result = { .content = 0, .size = 0 };                  \
        if (start > slice.size || end < start) { return result; }   \
        result.content = &slice.content[start];                     \
        result.size = end - start;                                  \
        return result;                                              \
    }                                                               \
    T_ELEM *PREFIX##_at(NAME slice, size_t at) {                    \
        if (at >= slice.size) {                                     \
            die_out_of_bounds(#PREFIX "_at", at, slice.size);       \
        }                                                           \
        return &slice.content[at];                                  \
    }                                                               \
    NAME PREFIX##_of_const(T_ELEM array[], size_t size) {           \
        NAME result = { .content = &array[0], .size = size };       \
        return result;                                              \
    }

#define SLICE_DECL(T_ELEM) SLICE_DECL_NAMED(T_ELEM, T_ELEM##Slice)
#define SLICE_METHODS_DECL(PREFIX, T_ELEM) SLICE_METHODS_DECL_NAMED(PREFIX, T_ELEM, T_ELEM##Slice)
#define SLICE_METHODS_IMPL(PREFIX, T_ELEM) SLICE_METHODS_IMPL_NAMED(PREFIX, T_ELEM, T_ELEM##Slice)

/** Dynamic arrays */
#define ARRAY_DECL_NAMED(T_ELEM, NAME) \
    typedef struct NAME {   \
        Arena *arena;       \
        T_ELEM *content;    \
        size_t size;        \
        size_t capacity;    \
    } NAME;

#define ARRAY_METHODS_DECL_NAMED(PREFIX, T_ELEM, NAME) \
    extern void PREFIX##_init(NAME *arr, size_t initial_capacity, Arena *arena);   \
    extern void PREFIX##_append(NAME *arr, T_ELEM element);                        \
    extern size_t PREFIX##_cut(NAME *arr, size_t start, size_t n);                 \
    extern T_ELEM *PREFIX##_at(NAME *arr, size_t at);                              \
    extern void PREFIX##_destroy(NAME *arr);

#define ARRAY_METHODS_IMPL_NAMED(PREFIX, T_ELEM, NAME) \
    void PREFIX##_init(NAME *arr, size_t icap, Arena *arena) {              \
        if (icap == 0) { icap = 8; }                                        \
        arr->content = arena_alloc(arena, icap * sizeof(T_ELEM));           \
        arr->arena = arena; arr->size = 0; arr->capacity = icap;            \
    }                                                                       \
    void PREFIX##_append(NAME *arr, T_ELEM element) {                       \
        if (arr->size >= arr->capacity) {                                   \
            size_t new_cap = arr->capacity + MIN(arr->capacity * 2, 4096);  \
            arr->content = arena_realloc(                                   \
                arr->arena, arr->content,                                   \
                arr->capacity * sizeof(T_ELEM), new_cap * sizeof(T_ELEM)    \
            );                                                              \
            arr->capacity = new_cap;                                        \
        }                                                                   \
        arr->content[arr->size] = element; arr->size += 1;                  \
    }                                                                       \
    size_t PREFIX##_cut(NAME *arr, size_t start, size_t n) {                \
        if (start >= arr->size || n == 0) { return 0; }                     \
        if (start + n >= arr->size) { n = arr->size - start; }              \
        for (size_t i = start; i < arr->size - n; i++) {                    \
            arr->content[i] = arr->content[i + n];                          \
        }                                                                   \
        arr->size -= n; return n;                                           \
    }                                                                       \
    T_ELEM *PREFIX##_at(NAME *arr, size_t at) {                             \
        if (at >= arr->size) {                                              \
            die_out_of_bounds(#PREFIX "_at", at, arr->size);                \
        }                                                                   \
        return &arr->content[at];                                           \
    }                                                                       \
    void PREFIX##_destroy(NAME *arr) {                                      \
        if (arr->content != 0 && arr->arena == arena_global()) {            \
            free(arr->content);                                             \
        }                                                                   \
        *arr = (NAME) { 0 };                                                \
    }

#define ARRAY_DECL(T_ELEM) ARRAY_DECL_NAMED(T_ELEM, T_ELEM##Array)
#define ARRAY_METHODS_DECL(PREFIX, T_ELEM) ARRAY_METHODS_DECL_NAMED(PREFIX, T_ELEM, T_ELEM##Array)
#define ARRAY_METHODS_IMPL(PREFIX, T_ELEM) ARRAY_METHODS_IMPL_NAMED(PREFIX, T_ELEM, T_ELEM##Array)

/** Methods for when you have both XArray and XSlice */
#define SLICE_ARRAY_METHODS_DECL_NAMED(PREFIX, T_ELEM, ARRAY_NAME, SLICE_NAME) \
    extern SLICE_NAME PREFIX##_seal(ARRAY_NAME *arr);

#define SLICE_ARRAY_METHODS_IMPL_NAMED(PREFIX, T_ELEM, ARRAY_NAME, SLICE_NAME) \
    SLICE_NAME PREFIX##_seal(ARRAY_NAME *arr) {                                \
        SLICE_NAME result = {                                            \
            .content = arena_realloc(                                       \
                arr->arena, arr->content,                                   \
                arr->capacity * sizeof(T_ELEM), arr->size * sizeof(T_ELEM)  \
            ),                                                              \
            .size = arr->size                                               \
        };                                                                  \
        PREFIX##_destroy(arr); return result;                               \
    }

#define SLICE_ARRAY_METHODS_DECL(PREFIX, T_ELEM) \
    SLICE_ARRAY_METHODS_DECL_NAMED(PREFIX, T_ELEM, T_ELEM##Array, T_ELEM##Slice)
#define SLICE_ARRAY_METHODS_IMPL(PREFIX, T_ELEM) \
    SLICE_ARRAY_METHODS_IMPL_NAMED(PREFIX, T_ELEM, T_ELEM##Array, T_ELEM##Slice)

#define BUILTIN_ARRAYS_LIST(X) \
    X(size_t,   SizeTArray,   sizet_array,    SizeTSlice,     sizet_slice)  \
    X(String,   StringArray,  strarr,         StringSlice,    strslice)     \

#define BUILTIN_ARRAYS_DECLARE(T_ELEM, ARRAY_NAME, ARRAY_PREFIX, SLICE_NAME, SLICE_PREFIX) \
    ARRAY_DECL_NAMED(T_ELEM, ARRAY_NAME)                                            \
    ARRAY_METHODS_DECL_NAMED(ARRAY_PREFIX, T_ELEM, ARRAY_NAME)                      \
    SLICE_DECL_NAMED(T_ELEM, SLICE_NAME)                                            \
    SLICE_METHODS_DECL_NAMED(SLICE_PREFIX, T_ELEM, SLICE_NAME)                      \
    SLICE_ARRAY_METHODS_DECL_NAMED(ARRAY_PREFIX, T_ELEM, ARRAY_NAME, SLICE_NAME)    \

BUILTIN_ARRAYS_LIST(BUILTIN_ARRAYS_DECLARE)

#undef BUILTIN_ARRAYS_DECLARE


#ifndef UTIL_INTERNAL
#undef BUILTIN_ARRAYS_LIST
#endif

#endif
