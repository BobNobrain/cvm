#include "ct_int.h"

void ct_type_print(LmbProgramType *type) {
    if (type == 0) {
        printf("?");
        return;
    }

    switch (type->kind) {
    case LmbProgramTypeKind_PRIMITIVE:
        switch (type->data.primitive) {
            case PrimitiveType_I32: printf("i32"); break;
            case PrimitiveType_F32: printf("f32"); break;
            default:                printf("??"); break;
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

    case LmbProgramTypeKind_INVALID: printf("(invalid)"); break;
    case LmbProgramTypeKind_IO:      printf("io"); break;
    default:                         printf("?!"); break;
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

static LmbProgramType INVALID_TYPE = (LmbProgramType) {
    .kind = LmbProgramTypeKind_INVALID,
    .data = { 0 },
};

LmbProgramType* _ct_type_get_invalid() { return &INVALID_TYPE; }

bool _ct_types_are_equal(LmbProgramType *t1, LmbProgramType *t2) {
    if (t1 == t2) { return true; }
    if (t1 == 0 || t2 == 0) { return false; }

    if (t1->kind != t2->kind) { return false; }

    switch (t1->kind) {
    case LmbProgramTypeKind_INVALID:
    case LmbProgramTypeKind_IO:
        return true;

    case LmbProgramTypeKind_PRIMITIVE:
        return t1->data.primitive == t2->data.primitive;

    case LmbProgramTypeKind_ARROW:
        if (t1->data.arrow.n_args != t2->data.arrow.n_args) { return false; }
        if (!_ct_types_are_equal(t1->data.arrow.ret_type, t2->data.arrow.ret_type)) { return false; }
        for (size_t i = 0; i < t1->data.arrow.n_args; i++) {
            if (!_ct_types_are_equal(t1->data.arrow.arg_types[i], t2->data.arrow.arg_types[i])) {
                return false;
            }
        }
        return true;
    }
}
