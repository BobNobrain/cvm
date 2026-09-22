#ifndef LANG_H
#define LANG_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum ValueType {
    V_NULL,
    V_BOOL,
    V_NUMI,
    V_NUMF,
    V_STR,
    V_CPTR,
    V_ARR,
    V_HASH,

    V_INVALID
} ValueType;

typedef char BoolValue;
typedef int NumIValue;
typedef float NumFValue;

typedef struct StrValue {
    size_t length;
} StrValue;

typedef struct ArrayValue {
    size_t length;
    ValueType value_type;
} ArrayValue;

typedef struct Value {
    ValueType type;
    union {
        BoolValue boolv;
        NumIValue numi;
        NumFValue numf;
        StrValue str;
        // ??? cptr;
        ArrayValue arr;
        // ??? hash;
    } data;
} Value;

Value value_bool(bool v) {
    Value result = { .type = V_BOOL };
    result.data.boolv = v;
    return result;
}
Value value_numi(int v) {
    Value result = { .type = V_NUMI };
    result.data.numi = v;
    return result;
}
Value value_numf(float v) {
    Value result = { .type = V_NUMF };
    result.data.numf = v;
    return result;
}

int value_to_string(Value v, char *str, size_t str_size) {
    switch (v.type) {
    case V_NULL:
        return snprintf(str, str_size, "<%s>", "null");

    case V_BOOL:
        if (v.data.boolv == 0) {
            return snprintf(str, str_size, "<b:%s>", "false");
        }
        return snprintf(str, str_size, "<b:%s>", "true");

    case V_NUMI:
        return snprintf(str, str_size, "<i:%d>", v.data.numi);

    case V_NUMF:
        return snprintf(str, str_size, "<f:%f>", v.data.numf);

    case V_STR:
        return snprintf(str, str_size, "<s:%s>", "??");

    case V_CPTR:
        return snprintf(str, str_size, "<*%s>", "??");

    case V_ARR:
        return snprintf(str, str_size, "<[]%s>", "??");

    case V_HASH:
        return snprintf(str, str_size, "<{}%s>", "??");

    default:
        return -1;
    }
}

size_t value_get_data_size(ValueType type) {
    switch (type) {
    case V_NULL:
        return 0;
    case V_BOOL:
        return sizeof(BoolValue);
    case V_NUMI:
        return sizeof(NumIValue);
    case V_NUMF:
        return sizeof(NumFValue);
    case V_STR:
        return sizeof(StrValue);
    case V_CPTR:
        return 0;
    case V_ARR:
        return sizeof(ArrayValue);
    case V_HASH:
        return 0;

    default:
        return 0;
    }
}

#endif
