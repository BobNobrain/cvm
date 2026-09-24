#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "str.h"

#define VALUE_TYPE_LIST(X) \
    X(V_BOOL, BoolValue, boolv) \
    X(V_NUMI, NumIValue, numi) \
    X(V_NUMF, NumFValue, numf)

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum ValueType {
    V_NULL,
    VALUE_TYPE_LIST(VALUE_TYPE_X)

    V_INVALID
} ValueType;
#undef VALUE_TYPE_X

typedef bool BoolValue;
typedef int NumIValue;
typedef float NumFValue;

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
typedef struct Value {
    ValueType type;
    union {
        VALUE_TYPE_LIST(VALUE_TYPE_X)
    } data;
} Value;
#undef VALUE_TYPE_X

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

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) \
void value_set_##FIELD_NAME(Value *into, DATA_TYPE v) { \
    into->type = CONST_NAME; \
    into->data.FIELD_NAME = v; \
}

VALUE_TYPE_LIST(VALUE_TYPE_X)
#undef VALUE_TYPE_X

error value_to_string(Value v, StringWriter *sw) {
    const size_t buffer_size = 20;
    char buffer[20];

    switch (v.type) {
    case V_NULL:
        return strw_appendc(sw, "<null>");

    case V_BOOL:
        if (v.data.boolv == 0) {
            return strw_appendc(sw, "<false>");
        }
        return strw_appendc(sw, "<true>");

    case V_NUMI:
        snprintf(buffer, buffer_size, "<i:%d>", v.data.numi);
        return strw_appendc(sw, buffer);

    case V_NUMF:
        snprintf(buffer, buffer_size, "<f:%f>", v.data.numf);
        return strw_appendc(sw, buffer);

    default:
        strw_appendc(sw, "<?>");
        return E_BAD_DATA;
    }
}

size_t value_get_data_size(ValueType type) {
    switch (type) {
    case V_NULL:
        return 0;

    #define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) \
    case CONST_NAME: \
        return sizeof(DATA_TYPE);

    VALUE_TYPE_LIST(VALUE_TYPE_X)
    #undef VALUE_TYPE_X

    default:
        return 0;
    }
}

void value_print(Value v) {
    StringWriter sw;
    strw_init(&sw, 16);
    value_to_string(v, &sw);
    str_print(strw_render(&sw));
}

#endif
