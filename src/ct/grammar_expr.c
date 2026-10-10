#include <stdio.h>
#include "util.h"
#include "ct_int.h"
#include "grammar_int.h"

#define EXPR_DEBUG 0

typedef enum {
    ExprPartType_SUBEXPR,
    ExprPartType_OP
} ExprPartType;
typedef struct {
    union {
        ASTNode *subexpr;
        Token *op;
    } data;
    ExprPartType type;
} ExprPart;

ARRAY_DECL(ExprPart)
ARRAY_METHODS_IMPL(_ct_exprpart_array, ExprPart)

#if EXPR_DEBUG
void _ct_print_exprpart(ExprPart *part) {
    switch (part->type) {
        case ExprPartType_OP:
            printf("<" STR_FMT ">", STR_FMT_VAL(part->data.op->data.op));
            break;

        case ExprPartType_SUBEXPR:
            printf("<subexpr>");
            break;
    }
}
void _ct_print_exprpart_array(ExprPartArray *arr, size_t idx) {
    for (size_t i = 0; i < arr->size; i++) {
        if (i == idx) {
            printf(" >");
        } else {
            printf("  ");
        }
        _ct_print_exprpart(&arr->content[i]);
        printf(" ");
    }
    printf("\n");
}

#define IFDEBUG(S) S
#else
#define IFDEBUG(S)
#endif

ASTNode* ct_grammar_parens(Parser *p);
ASTNode* ct_grammar_lambda(Parser *p);

DocumentRange _ct_exprpart_get_range(ExprPart *part) {
    switch (part->type) {
        case ExprPartType_SUBEXPR:
            return part->data.subexpr->range;

        case ExprPartType_OP:
            return part->data.op->range;
    }

    die("bad ExprPart::type");
    return ct_document_range(ct_document_pos_zero(), 0);
}

ASTNode* _ct_token_wrap_literal(Parser *p, Token *token) {
    ASTNodeType lit_type;

    switch (token->type) {
    case TokenType_INT_LITERAL:
        lit_type = ASTNodeType_LINT;
        break;
    case TokenType_FLOAT_LITERAL:
        lit_type = ASTNodeType_LFLOAT;
        break;
    case TokenType_BOOL_LITERAL:
        lit_type = ASTNodeType_LBOOL;
        break;
    case TokenType_IDENT:
        lit_type = ASTNodeType_IDENT;
        break;

    default:
        return 0;
    }

    ASTNode* lit = ct_astnode_new(p, lit_type);
    lit->base = token;
    lit->range = token->range;

    switch (token->type) {
    case TokenType_INT_LITERAL:
        lit->data.lint = token->data.intl;
        break;
    case TokenType_FLOAT_LITERAL:
        lit->data.lfloat = token->data.floatl;
        break;
    case TokenType_BOOL_LITERAL:
        lit->data.lbool = token->data.booll;
        break;
    case TokenType_IDENT:
        lit->data.ident = (ASTIdentData) { .name = token->data.ident, .bound_index = 0 };
        break;

    default: break;
    }

    return lit;
}

void _ct_collapse_expr_parts(ExprPartArray *parts_array, Parser *p) {
    if (parts_array->size < 2) {
        return;
    }

    // - loop though the optable, from highest to lowest
    // - loop through the parts array to the first occurence of this operator
    //   - loop with respect to associativity
    //   - if no occurence found, just go to the next operator
    // - for each declaration, find the matching operand(s)
    //   - if not found (respective parts are operators, not expressions), throw (maybe need to rethink this)
    //   - collapse with the operand(s) into a single expression part
    //     - if associativity is forbidden, SOMEHOW check for the operands to not be of the same operator (TBD)
    //   - continue looping until the end
    // - at the end, if the parts array contains the only expression, it is the result
    // - otherwise, the expression is malformed

    for (size_t odi = 0; odi < p->config.optable.size; odi++) {
        // direct order, by descending priority
        OperatorDecl decl = p->config.optable.content[odi];
        bool is_right_assoc = false;
        ASTNodeType node_type = ASTNodeType_UNOP;

        switch (decl.type) {
            case OperatorType_BINARY_RIGHT:
                node_type = ASTNodeType_BINOP;
                is_right_assoc = true;
                break;

            case OperatorType_UNARY_RIGHT:
                is_right_assoc = true;
                break;

            case OperatorType_BINARY_LEFT:
            case OperatorType_BINARY_NOASSOC:
                node_type = ASTNodeType_BINOP;
                break;

            default: break;
        }

        IFDEBUG( printf("trying operator '" STR_FMT "':\n", STR_FMT_VAL(decl.op)); )

        for (size_t pi = 1; pi < parts_array->size; pi++) {
            size_t part_idx = pi;
            if (is_right_assoc) {
                // for right-associative, iterating from the right
                part_idx = parts_array->size - pi - 1;
            }

            IFDEBUG( _ct_print_exprpart_array(parts_array, part_idx); )

            ExprPart *part = _ct_exprpart_array_at(parts_array, part_idx);
            if (part->type != ExprPartType_OP) { continue; }
            if (!str_eq(part->data.op->data.op, decl.op)) { continue; }

            // found a match
            ASTNode *opnode = ct_astnode_new(p, node_type);
            opnode->base = part->data.op;
            ExprPart opnode_part = { .type = ExprPartType_SUBEXPR, .data.subexpr = opnode };

            if (node_type == ASTNodeType_UNOP) {
                size_t operand_idx = part_idx;
                size_t opposite_idx = part_idx;

                if (is_right_assoc) {
                    operand_idx = part_idx + 1;
                    opposite_idx = part_idx == 0 ? part_idx : part_idx - 1;
                } else {
                    operand_idx = part_idx - 1;
                    opposite_idx = part_idx == parts_array->size - 1 ? part_idx : part_idx + 1;
                }

                if (opposite_idx != part_idx) {
                    // the opposite part must be an operator – otherwise we'll get (expr expr)
                    ExprPart *opposite_part = _ct_exprpart_array_at(parts_array, opposite_idx);
                    if (opposite_part->type != ExprPartType_OP) {
                        IFDEBUG( printf("opposite not operator, skipping\n"); )
                        continue;
                    }
                }

                ExprPart *operand_part = _ct_exprpart_array_at(parts_array, operand_idx);
                if (operand_part->type != ExprPartType_SUBEXPR) {
                    IFDEBUG( printf("operand not expr, skipping\n"); )
                    continue;
                }

                DocumentRange node_range = ct_document_range_span(
                    _ct_exprpart_get_range(part),
                    _ct_exprpart_get_range(operand_part)
                );
                opnode->range = node_range;
                opnode->data.unop = decl;

                ASTNode *operand = operand_part->data.subexpr;
                ct_astnode_append_child(opnode, operand);

                _ct_exprpart_array_cut(parts_array, part_idx, 1);
                size_t rewrite_idx = is_right_assoc ? part_idx : part_idx - 1;
                *_ct_exprpart_array_at(parts_array, rewrite_idx) = opnode_part;
                --pi;
            } else { // AST_TYPE_BINOP
                if (part_idx == 0) {
                    IFDEBUG( printf("binary at 0, skipping\n"); )
                    continue;
                }
                if (part_idx == parts_array->size) {
                    IFDEBUG( printf("binary at the end, skipping\n"); )
                    continue;
                }

                size_t left_operand_idx = part_idx - 1;
                size_t right_operand_idx = part_idx + 1;

                ExprPart *left_operand_part = _ct_exprpart_array_at(parts_array, left_operand_idx);
                ExprPart *right_operand_part = _ct_exprpart_array_at(parts_array, right_operand_idx);

                if (left_operand_part->type != ExprPartType_SUBEXPR) {
                    IFDEBUG( printf("left operand not expr, skipping\n"); )
                    continue;
                }

                if (right_operand_part->type != ExprPartType_SUBEXPR) {
                    IFDEBUG( printf("right operand not expr, skipping\n"); )
                    continue;
                }

                opnode->range = ct_document_range_span(
                    _ct_exprpart_get_range(left_operand_part),
                    _ct_exprpart_get_range(right_operand_part)
                );
                opnode->data.binop = decl;

                ASTNode *left_operand = left_operand_part->data.subexpr;
                ASTNode *right_operand = right_operand_part->data.subexpr;
                ct_astnode_append_child(opnode, left_operand);
                ct_astnode_append_child(opnode, right_operand);

                _ct_exprpart_array_cut(parts_array, part_idx - 1, 2);
                *_ct_exprpart_array_at(parts_array, part_idx - 1) = opnode_part;
                --pi;
            }
        }
    }

    IFDEBUG( _ct_print_exprpart_array(parts_array, parts_array->size); )
}

ASTNode* ct_grammar_operator_expr(Parser *p) {
    ExprPartArray parts_array;
    _ct_exprpart_array_init(&parts_array, 16, p->arena);

    Token *token;
    ExprPart part;

    // wrap everything into ExprPart, parsing nested (subexpressions)
    while ((token = ct_parser_peek(p)) != 0) {
        bool ok = true;

        switch (token->type) {
        case TokenType_OPERATOR:
            part.type = ExprPartType_OP;
            part.data.op = token;
            ct_parser_consume(p);
            _ct_exprpart_array_append(&parts_array, part);
            break;

        case TokenType_BOOL_LITERAL:
        case TokenType_FLOAT_LITERAL:
        case TokenType_IDENT:
        case TokenType_INT_LITERAL:
            part.type = ExprPartType_SUBEXPR;
            part.data.subexpr = _ct_token_wrap_literal(p, token);
            ct_parser_consume(p);
            _ct_exprpart_array_append(&parts_array, part);
            break;

        case TokenType_OPEN_PAREN:
            part.type = ExprPartType_SUBEXPR;
            part.data.subexpr = ct_grammar_parens(p);
            _ct_exprpart_array_append(&parts_array, part);
            break;

        case TokenType_LAMBDA:
            part.type = ExprPartType_SUBEXPR;
            part.data.subexpr = ct_grammar_lambda(p);
            _ct_exprpart_array_append(&parts_array, part);
            break;

        default:
            ok = false;
            break;
        }

        if (!ok) { break; }
    }

    IFDEBUG( _ct_print_exprpart_array(&parts_array, parts_array.size); )
    _ct_collapse_expr_parts(&parts_array, p);
    IFDEBUG( _ct_print_exprpart_array(&parts_array, parts_array.size); )

    if (parts_array.size == 0) {
        _ct_exprpart_array_destroy(&parts_array);
        return ct_astnode_new_error(p, STR_CONST("expected an expression, found nothing"));
    }

    if (parts_array.size == 1) {
        ASTNode *result = 0;

        switch (parts_array.content[0].type) {
            case ExprPartType_SUBEXPR:
                result = parts_array.content[0].data.subexpr;
                break;

            case ExprPartType_OP:
                result = ct_astnode_new_error_ranged(
                    p, STR_CONST("expected an expression, found an operator"),
                    parts_array.content[0].data.op->range
                );
                break;

            default:
                die("[INTERNAL] bad value in parts_array.content[0].type");
                result = ct_astnode_new_error(p, STR_EMPTY);
                break;
        }

        _ct_exprpart_array_destroy(&parts_array);
        return result;
    }

    // more than one subexpression – must be function application then
    ASTNode* result = ct_astnode_new(p, ASTNodeType_FNCALL);

    for (size_t i = 0; i < parts_array.size; i++) {
        ExprPart part = parts_array.content[i];
        if (part.type == ExprPartType_OP) {
            ct_astnode_append_child(result, ct_astnode_new_error_ranged(
                p, STR_CONST("expected an expression, found an operator"),
                part.data.op->range
            ));
            continue;
        }

        if (part.type != ExprPartType_SUBEXPR) { die("unknown ExprPartType"); }
        ct_astnode_append_child(result, part.data.subexpr);
    }

    ct_astnode_set_range_span_children(result);
    return result;
}

ASTNode* ct_grammar_parens(Parser *p) {
    Token *opening = ct_parser_consume_if(p, TokenType_OPEN_PAREN);
    if (opening == 0) {
        return ct_astnode_new_error(p, STR_CONST("expected '('"));
    }

    ASTNode *expr = ct_grammar_expr(p);
    if (expr == 0) {
        return ct_astnode_new_error_ranged(p, STR_CONST("expected expression after '('"), opening->range);
    }

    Token *closing = ct_parser_consume_if(p, TokenType_CLOSE_PAREN);
    if (closing == 0) {
        return ct_astnode_new_error_ranged(p, STR_CONST("expected ')'"), expr->range);
    }

    return expr;
}

ASTNode* _ct_grammar_lambda_arg(Parser *p) {
    Token *arg_name = ct_parser_consume_if(p, TokenType_IDENT);
    if (arg_name == 0) {
        return ct_astnode_new_error(p, STR_CONST("expected an argument name"));
    }

    ASTNode *arg = ct_astnode_new(p, ASTNodeType_LAMBDA_ARG);
    arg->base = arg_name;
    arg->range = arg_name->range;
    arg->data.lambda_arg = (ASTLambdaArgData) {
        .name = arg_name->data.ident,
        .type = STR_EMPTY,
    };

    Token *colon = ct_parser_consume_if(p, TokenType_COLON);
    if (colon == 0) { return arg; }

    // TODO: this should become its own grammar, _ct_grammar_lambda_arg_spec
    Token *arg_type = ct_parser_consume_if(p, TokenType_IDENT);
    if (arg_type == 0) {
        return ct_astnode_new_error_ranged(
            p, STR_CONST("expected a specifier after ':'"),
            ct_document_range_span(arg_name->range, ct_parser_current_range(p))
        );
    }

    arg->data.lambda_arg.type = arg_type->data.ident;
    arg->range = ct_document_range_span(arg_name->range, arg_type->range);
    return arg;
}

void _ct_skip_until_ident_or_dot(Parser *p) {
    Token *next = 0;
    while ((next = ct_parser_peek(p)) != 0) {
        switch (next->type) {
            case TokenType_IDENT:
            case TokenType_DOT:
                return;

            default:
                ct_parser_consume(p);
                break;
        }
    }
}

ASTNode* ct_grammar_lambda(Parser *p) {
    Token *lambda_symbol = ct_parser_peek(p);
    REQUIRE_TOKEN(TokenType_LAMBDA, "expected a '\\'")

    ASTNode *result = ct_astnode_new(p, ASTNodeType_LAMBDA);
    result->data.lambda = (ASTLambdaData) { 0 };

    Token *next = 0;
    while ((next = ct_parser_peek(p)) != 0) {
        if (next->type == TokenType_DOT) { break; }

        ASTNode *arg = _ct_grammar_lambda_arg(p);
        ct_astnode_append_child(result, arg);

        if (!ct_astnode_is_error(arg)) {
            result->data.lambda.n_args += 1;
            continue;
        }

        // in case of error, let's try to fast forward until next potentially parseable position
        _ct_skip_until_ident_or_dot(p);
    }

    REQUIRE_TOKEN(TokenType_DOT, "expected a '.'")

    ASTNode *body = ct_grammar_expr(p);
    ct_astnode_append_child(result, body);
    result->data.lambda.body = body;
    result->range = ct_document_range_span(lambda_symbol->range, body->range);
    return result;
}

ASTNode* ct_grammar_expr(Parser *p) {
    Token *next = ct_parser_peek(p);
    if (next == 0) {
        return ct_astnode_new_error(p, STR_CONST("expected an expression"));
    }

    switch (next->type) {
        // DO NOT DO THIS:
        // case TokenType_OPEN_PAREN:
        //     return ct_grammar_parens(p);
        // it will stop at the first closing paren, when the expression might be longer!
        // e.g. "(1 + 2) + 3" will yield a node for "(1 + 2)", and leave "+ 3" outside the expression

        case TokenType_LAMBDA:
            return ct_grammar_lambda(p);

        default:
            return ct_grammar_operator_expr(p);
    }
}
