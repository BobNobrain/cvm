#include "ct_int.h"

void ct_type_print(LmbProgramType *type) {
    if (type == 0) {
        printf("?");
        return;
    }

    switch (type->kind) {
    case LmbProgramTypeKind_IO:
        printf("io");
        break;

    case LmbProgramTypeKind_PRIMITIVE:
        switch (type->data.primitive) {
            case PrimitiveType_I32: printf("i32"); break;
            case PrimitiveType_F32: printf("f32"); break;
            default: printf("??"); break;
        }
        break;

    case LmbProgramTypeKind_ARROW:
        printf("\\");

        if (type->data.arrow.n_args > 0) {
            ct_type_print(type->data.arrow.arg_types[0]);
        }

        for (size_t i = 1; i < type->data.arrow.n_args; i++) {
            printf(" ");
            ct_type_print(type->data.arrow.arg_types[i]);
        }
        printf(". ");
        ct_type_print(type->data.arrow.ret_type);
        break;

    default:
        printf("??");
        break;
    }
}

static LmbProgramType PRIMITIVE_I32 = (LmbProgramType) {
    .kind = LmbProgramTypeKind_PRIMITIVE,
    .data = { .primitive = PrimitiveType_I32 },
};
static LmbProgramType PRIMITIVE_F32 = (LmbProgramType) {
    .kind = LmbProgramTypeKind_PRIMITIVE,
    .data = { .primitive = PrimitiveType_F32 },
};

LmbProgramType* _ct_type_get_primitive(PrimitiveType type) {
    switch (type) {
    case PrimitiveType_I32:
        return &PRIMITIVE_I32;

    case PrimitiveType_F32:
        return &PRIMITIVE_F32;

    default:
        die("unknown type");
    }
}

static LmbProgramType IO_TYPE = (LmbProgramType) {
    .kind = LmbProgramTypeKind_IO,
    .data = { 0 },
};

LmbProgramType* _ct_type_get_io() { return &IO_TYPE; }
