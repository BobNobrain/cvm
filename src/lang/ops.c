#include "util.h"
#include "lang_int.h"

const char DEBUG_STR_BINOPS[] = \
    "==\0<>\0" \
    "+i\0*i\0-i\0/i\0%i\0" \
    ">i\0<i\0>=\0<=\0" \
    "+f\0*f\0-f\0/f\0" \
    ">f\0<f\0>=\0<=\0" \
    "&&\0||\0" \
;

error lang_binop_to_string(BinopType t, StringBuilder *sw) {
    const size_t binop_str_len = 3;
    size_t offset = (size_t)t * binop_str_len * sizeof(char);
    return strb_appendc(sw, &DEBUG_STR_BINOPS[offset]);
}


const char DEBUG_STR_UNOPS[] = "!\0\0-i\0-f\0if\0ib\0fb\0";

error lang_unop_to_string(UnopType t, StringBuilder *sw) {
    const size_t unop_str_len = 3;
    size_t offset = (size_t)t * unop_str_len * sizeof(char);
    return strb_appendc(sw, &DEBUG_STR_UNOPS[offset]);
}
