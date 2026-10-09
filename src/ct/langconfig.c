#include "ct_int.h"

LangConfig ct_langconfig_create() {
    static OperatorDecl optable[] = {
        { .op = STR_CONST("!"),  .type = OperatorType_UNARY_RIGHT,    .priority = 150, .variant = OperatorVariant_UNARY_NOT    },
        { .op = STR_CONST("-"),  .type = OperatorType_UNARY_RIGHT,    .priority = 150, .variant = OperatorVariant_UNARY_MINUS  },
        { .op = STR_CONST("+"),  .type = OperatorType_UNARY_RIGHT,    .priority = 150, .variant = OperatorVariant_UNARY_PLUS   },

        { .op = STR_CONST("^"),  .type = OperatorType_BINARY_RIGHT,   .priority =  90, .variant = OperatorVariant_BINARY_POWER },
        { .op = STR_CONST("*"),  .type = OperatorType_BINARY_LEFT,    .priority =  80, .variant = OperatorVariant_BINARY_MUL   },
        { .op = STR_CONST("/"),  .type = OperatorType_BINARY_LEFT,    .priority =  80, .variant = OperatorVariant_BINARY_DIV   },
        { .op = STR_CONST("%"),  .type = OperatorType_BINARY_NOASSOC, .priority =  80, .variant = OperatorVariant_BINARY_REM   },
        { .op = STR_CONST("+"),  .type = OperatorType_BINARY_LEFT,    .priority =  70, .variant = OperatorVariant_BINARY_ADD   },
        { .op = STR_CONST("-"),  .type = OperatorType_BINARY_LEFT,    .priority =  70, .variant = OperatorVariant_BINARY_SUB   },
        { .op = STR_CONST("<"),  .type = OperatorType_BINARY_NOASSOC, .priority =  60, .variant = OperatorVariant_BINARY_LT    },
        { .op = STR_CONST(">"),  .type = OperatorType_BINARY_NOASSOC, .priority =  60, .variant = OperatorVariant_BINARY_GT    },
        { .op = STR_CONST("<="), .type = OperatorType_BINARY_NOASSOC, .priority =  60, .variant = OperatorVariant_BINARY_LTE   },
        { .op = STR_CONST(">="), .type = OperatorType_BINARY_NOASSOC, .priority =  60, .variant = OperatorVariant_BINARY_GTE   },
        { .op = STR_CONST("=="), .type = OperatorType_BINARY_LEFT,    .priority =  50, .variant = OperatorVariant_BINARY_EQ    },
        { .op = STR_CONST("<>"), .type = OperatorType_BINARY_LEFT,    .priority =  50, .variant = OperatorVariant_BINARY_NEQ   },
        { .op = STR_CONST("&&"), .type = OperatorType_BINARY_LEFT,    .priority =  40, .variant = OperatorVariant_BINARY_AND   },
        { .op = STR_CONST("||"), .type = OperatorType_BINARY_LEFT,    .priority =  40, .variant = OperatorVariant_BINARY_OR    }
    };

    return (LangConfig) {
        .allowed_ident_chars = STR_CONST("_"),
        .allowed_operator_chars = STR_CONST("!@$%^&*-+=<>/:|~"),
        .line_comment_start = STR_CONST("#"),
        .optable = ct_opdeclslice_of_const(optable, sizeof(optable) / sizeof(OperatorDecl))
    };
}

bool _ct_is_whitespace(char c) {
    switch (c) {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
        return true;

    default:
        return false;
    }
}

String _ct_langconfig_validate_optable(const LangConfig cfg) {
    if (cfg.optable.size == 0) {
        return STR_EMPTY;
    }

    OperatorPriority last = OPERATOR_PRIORITY_MAX;

    for (size_t i = 0; i < cfg.optable.size; i++) {
        OperatorDecl next = cfg.optable.content[i];
        if (str_is_empty(next.op)) {
            return str_wrap("optable contains an empty operator");
        }
        if (next.priority > last) {
            return str_wrap("optable must be sorted by priority, descending");
        }

        last = next.priority;
    }

    return STR_EMPTY;
}

String _ct_langconfig_validate_allowed_chars(const LangConfig cfg) {
    for (size_t i = 0; i < cfg.allowed_ident_chars.size; i++) {
        char next = cfg.allowed_ident_chars.content[i];
        if (_ct_is_whitespace(next)) {
            return str_wrap("indent chars cannot contain whitespace");
        }

        if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z') || (next >= '0' && next <= '9')) {
            return str_wrap("alphanum characters are always valid identifier characters");
        }
    }

    for (size_t i = 0; i < cfg.allowed_operator_chars.size; i++) {
        char next = cfg.allowed_operator_chars.content[i];
        if (_ct_is_whitespace(next)) {
            return str_wrap("operator chars cannot contain whitespace");
        }
    }

    return STR_EMPTY;
}

String ct_langconfig_validate(const LangConfig cfg) {
    String result;

    result = _ct_langconfig_validate_allowed_chars(cfg);
    if (!str_is_empty(result)) { return result; }

    result = _ct_langconfig_validate_optable(cfg);
    if (!str_is_empty(result)) { return result; }

    return STR_EMPTY;
}
