#include <stdio.h>
#include "util.h"
#include "ct_int.h"

#define GRAMMAR_ONEOF_START ParserRewindPoint saved_pos = { .input = p->input }; ASTNode *result = 0;
#define GRAMMAR_ONEOF_TRY(GRAMMAR) \
    result = GRAMMAR(p, docerr);        \
    if (result != 0) {                  \
        return result;                  \
    } else {                            \
        ct_parser_rewind(p, saved_pos); \
    }

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
    case TOKEN_INT_LITERAL:
        lit_type = AST_TYPE_LINT;
        break;
    case TOKEN_FLOAT_LITERAL:
        lit_type = AST_TYPE_LFLOAT;
        break;
    case TOKEN_BOOL_LITERAL:
        lit_type = AST_TYPE_LBOOL;
        break;
    case TOKEN_IDENT:
        lit_type = AST_TYPE_IDENT;
        break;

    default:
        return 0;
    }

    ASTNode* lit = ct_astnode_new(p, lit_type);
    lit->base = token;
    lit->range = token->range;

    switch (token->type) {
    case TOKEN_INT_LITERAL:
        lit->data.lint = token->data.intl;
        break;
    case TOKEN_FLOAT_LITERAL:
        lit->data.lfloat = token->data.floatl;
        break;
    case TOKEN_BOOL_LITERAL:
        lit->data.lbool = token->data.booll;
        break;
    case TOKEN_IDENT:
        lit->data.ident = token->data.ident;
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
        ASTNodeType node_type = AST_TYPE_UNOP;

        switch (decl.type) {
            case OperatorType_BINARY_RIGHT:
                node_type = AST_TYPE_BINOP;
                is_right_assoc = true;
                break;

            case OperatorType_UNARY_RIGHT:
                is_right_assoc = true;
                break;

            case OperatorType_BINARY_LEFT:
            case OperatorType_BINARY_NOASSOC:
                node_type = AST_TYPE_BINOP;
                break;

            default: break;
        }

        IFDEBUG( printf("trying operator '" STR_FMT "':\n", STR_FMT_VAL(decl.op)); )

        for (size_t pi = 0; pi < parts_array->size - 1; pi++) {
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

            if (node_type == AST_TYPE_UNOP) {
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

                ASTNode *operand = operand_part->data.subexpr;
                opnode->children[0] = operand;
                opnode->n_children = 1;

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

                ASTNode *left_operand = left_operand_part->data.subexpr;
                ASTNode *right_operand = right_operand_part->data.subexpr;
                opnode->children[0] = left_operand;
                opnode->children[1] = right_operand;
                opnode->n_children = 2;

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
    while ((token = ct_parser_consume(p)) != 0) {
        bool ok = true;

        switch (token->type) {
        case TOKEN_OPERATOR:
            part.type = ExprPartType_OP;
            part.data.op = token;
            _ct_exprpart_array_append(&parts_array, part);
            break;

        case TOKEN_BOOL_LITERAL:
        case TOKEN_FLOAT_LITERAL:
        case TOKEN_IDENT:
        case TOKEN_INT_LITERAL:
            part.type = ExprPartType_SUBEXPR;
            part.data.subexpr = _ct_token_wrap_literal(p, token);
            _ct_exprpart_array_append(&parts_array, part);
            break;

        case TOKEN_OPEN_PAREN:
            ct_parser_rewind_n(p, -1);
            part.type = ExprPartType_SUBEXPR;
            part.data.subexpr = ct_grammar_parens(p);
            if (part.data.subexpr == 0) {
                return ct_astnode_new_error(p, ct_parser_make_error(
                    p, "expected a subexpression", ct_parser_current_range(p)
                ));
            }
            _ct_exprpart_array_append(&parts_array, part);
            break;

        default:
            ct_parser_rewind_n(p, -1);
            ok = false;
            break;
        }

        if (!ok) { break; }
    }

    IFDEBUG( _ct_print_exprpart_array(&parts_array, parts_array.size); )
    _ct_collapse_expr_parts(&parts_array, p);
    IFDEBUG( _ct_print_exprpart_array(&parts_array, parts_array.size); )

    if (parts_array.size != 1) {
        DocumentError docerr = ct_parser_make_error(p, "", (DocumentRange) { 0 });
        if (parts_array.size > 0) {
            ct_document_set_error(
                &docerr,
                "invalid expression",
                ct_document_range_span(
                    _ct_exprpart_get_range(&parts_array.content[0]),
                    _ct_exprpart_get_range(&parts_array.content[parts_array.size - 1])
                )
            );
        } else {
            Token *next = ct_parser_peek(p);
            if (next != 0) {
                ct_document_set_error(&docerr, "expected expression, found nothing", next->range);
            } else {
                ct_document_set_error(&docerr, "empty expression ??", ct_parser_current_range(p));
            }
        }

        _ct_exprpart_array_destroy(&parts_array);
        return ct_astnode_new_error(p, docerr);
    }

    ASTNode *result = 0;

    switch (parts_array.content[0].type) {
        case ExprPartType_SUBEXPR:
            result = parts_array.content[0].data.subexpr;
            break;

        case ExprPartType_OP:
            result = ct_astnode_new_error(p, ct_parser_make_error(
                p, "invalid expression", parts_array.content[0].data.op->range
            ));
            break;

        default:
            die("[INTERNAL] bad value in parts_array.content[0].type");
            result = ct_astnode_new_error(p, (DocumentError) { 0 });
            break;
    }

    _ct_exprpart_array_destroy(&parts_array);
    return result;
}

ASTNode* ct_grammar_parens(Parser *p) {
    Token *opening = ct_parser_consume_if(p, TOKEN_OPEN_PAREN);
    if (opening == 0) {
        return ct_astnode_new_error(p, ct_parser_make_error(p, "expected '('", ct_parser_current_range(p)));
    }

    ASTNode *expr = ct_grammar_expr(p);
    if (expr == 0) {
        return ct_astnode_new_error(p, ct_parser_make_error(p, "expected expression after '('", opening->range));
    }

    Token *closing = ct_parser_consume_if(p, TOKEN_CLOSE_PAREN);
    if (closing == 0) {
        return ct_astnode_new_error(p, ct_parser_make_error(p, "expected ')'", expr->range));
    }

    return expr;
}

ASTNode* ct_grammar_expr(Parser *p) {
    return ct_grammar_operator_expr(p);
}
