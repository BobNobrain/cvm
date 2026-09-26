#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "util.h"
#include "lang_int.h"

Value lang_value_make_bool(bool v) {
    Value result = { .type = V_BOOL };
    result.data.boolv = v;
    return result;
}
Value lang_value_make_numi(int v) {
    Value result = { .type = V_NUMI };
    result.data.numi = v;
    return result;
}
Value lang_value_make_numf(float v) {
    Value result = { .type = V_NUMF };
    result.data.numf = v;
    return result;
}

#define VALUE_TYPE_X(CONST_NAME, DATA_TYPE, FIELD_NAME) \
    void lang_value_set_##FIELD_NAME(Value *into, DATA_TYPE v) { \
        into->type = CONST_NAME; \
        into->data.FIELD_NAME = v; \
    }

VALUE_TYPE_LIST(VALUE_TYPE_X)
#undef VALUE_TYPE_X

error lang_value_to_string(Value v, StringBuilder *sw) {
    const size_t buffer_size = 20;
    char buffer[20];

    switch (v.type) {
    case V_NULL:
        return strb_appendc(sw, "<null>");

    case V_BOOL:
        if (v.data.boolv == 0) {
            return strb_appendc(sw, "<false>");
        }
        return strb_appendc(sw, "<true>");

    case V_NUMI:
        snprintf(buffer, buffer_size, "<i:%d>", v.data.numi);
        return strb_appendc(sw, buffer);

    case V_NUMF:
        snprintf(buffer, buffer_size, "<f:%f>", v.data.numf);
        return strb_appendc(sw, buffer);

    default:
        strb_appendc(sw, "<?>");
        return E_BAD_DATA;
    }
}

size_t lang_value_get_data_size(ValueType type) {
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

void lang_value_print(Value v) {
    StringBuilder *sw = strb_new(16);
    lang_value_to_string(v, &sw);
    String rendered = strb_render(&sw);
    printf(STR_FMT, STR_FMT_VAL(rendered));
}
