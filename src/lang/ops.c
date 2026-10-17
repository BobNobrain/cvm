#include "util.h"
#include "lang_int.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

const char DEBUG_STR_BINOPS[] = \
    "==\0<>\0" \
    "+i\0*i\0-i\0/i\0%i\0" \
    ">i\0<i\0>=\0<=\0" \
    "+f\0*f\0-f\0/f\0" \
    ">f\0<f\0>=\0<=\0" \
    "&&\0||\0" \
;

void lang_print_binop(BinopType t) {
    const size_t binop_str_len = 3;
    size_t offset = (size_t)t * binop_str_len * sizeof(char);
    printf(&DEBUG_STR_BINOPS[offset]);
}


const char DEBUG_STR_UNOPS[] = "!\0\0-i\0-f\0if\0fi\0fb\0";

void lang_print_unop(UnopType t) {
    if (t >= UNOP_INVALID) {
        printf("invalid");
        return;
    }
    const size_t unop_str_len = 3;
    size_t offset = (size_t)t * unop_str_len * sizeof(char);
    printf(&DEBUG_STR_UNOPS[offset]);
}

#pragma GCC diagnostic pop
