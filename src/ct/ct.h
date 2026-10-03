#ifndef CT_H
#define CT_H
/**
 * Compile-time library
 */

#include <stddef.h>
#include <stdbool.h>
#include "util.h"

typedef struct Parser Parser;


/** Aux structures to aid with parsing */
typedef struct DocumentPos {
    size_t caret;
    size_t line;
    size_t column;
} DocumentPos;

extern DocumentPos ct_document_pos_zero();
extern void ct_document_pos_track(DocumentPos *pos, size_t chars);
extern void ct_document_pos_line_break(DocumentPos *pos);

typedef struct DocumentRange {
    DocumentPos start;
    DocumentPos end;
} DocumentRange;

extern DocumentRange ct_document_range(DocumentPos start, size_t length);
extern DocumentRange ct_document_range_span(DocumentRange from, DocumentRange to);
extern int ct_document_range_length(DocumentRange range);
extern String ct_document_substring(String source, DocumentRange range);

typedef struct DocumentError {
    String message;
    String source;
    DocumentRange location;
} DocumentError;

extern void ct_document_set_error(DocumentError *error, char *c_msg, DocumentRange location);
extern void ct_document_print_error(DocumentError error);


/** Tokenizer */
#define TOKEN_LIST(X) \
    X(TOKEN_IDENT, , ) \
    X(TOKEN_INT_LITERAL, int, intl) \
    X(TOKEN_FLOAT_LITERAL, float, floatl) \
    X(TOKEN_BOOL_LITERAL, bool, booll) \
    X(TOKEN_OPERATOR, String, op) \
    X(TOKEN_OPEN_PAREN, , ) \
    X(TOKEN_CLOSE_PAREN, , )

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum {
    TOKEN_LIST(TOKEN_LIST_X)

    TOKEN_INVALID
} TokenType;
#undef TOKEN_LIST_X

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
typedef struct {
    TokenType type;
    DocumentRange range;
    union {
        TOKEN_LIST(TOKEN_LIST_X)
    } data;
} Token;
#undef TOKEN_LIST_X

typedef struct Tokenizer {
    Token *tokens;
    size_t size;
    size_t capacity;
    Arena *arena;
} Tokenizer;

extern void ct_tokenizer_init(Tokenizer *t, Arena *arena);
extern error ct_tokenizer_run(Tokenizer *t, String source, DocumentError *err);
extern void ct_tokenizer_print(Tokenizer *t);


/** Operators */
typedef uint8_t OperatorPriority;
#define OPERATOR_PRIORITY_MIN 0
#define OPERATOR_PRIORITY_MAX 255
typedef enum {
    OperatorType_INVALID,
    OperatorType_BINARY_NOASSOC,
    OperatorType_BINARY_LEFT,
    OperatorType_BINARY_RIGHT,
    OperatorType_UNARY_NOASSOC,
    OperatorType_UNARY_LEFT,
    OperatorType_UNARY_RIGHT,
} OperatorType;

typedef struct {
    String op;
    OperatorType type;
    OperatorPriority priority;
} OperatorDecl;

SLICE_DECL(OperatorDecl)
SLICE_METHODS_DECL(ct_opdeclslice, OperatorDecl)


/** Language AST */
#define AST_TYPES_LIST(X) \
    X(AST_TYPE_SYNTAX_ERROR, String, error) \
    X(AST_TYPE_LINT, int, lint) \
    X(AST_TYPE_LFLOAT, float, lfloat) \
    X(AST_TYPE_LBOOL, bool, lbool) \
    X(AST_TYPE_IDENT, , ) \
    X(AST_TYPE_BINOP, , ) \
    X(AST_TYPE_UNOP, , )

#define AST_TYPES_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum ASTNodeType {
    AST_TYPES_LIST(AST_TYPES_LIST_X)
} ASTNodeType;
#undef AST_TYPES_LIST_X

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode **children;
    size_t n_children;
    size_t children_cap;
    DocumentRange range;
    Token *base;

    #define AST_TYPES_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
    union {
        AST_TYPES_LIST(AST_TYPES_LIST_X)
    } data;
    #undef AST_TYPES_LIST_X
} ASTNode;

extern ASTNode *ct_astnode_new(Parser *p, ASTNodeType type);
extern ASTNode *ct_astnode_new_error(Parser *p, DocumentError docerr);
extern void ct_astnode_print(ASTNode *node, size_t indent);
extern bool ct_astnode_is_error(ASTNode *node);


/** The parser itself */
struct Parser {
    Arena *arena;

    Token *input_start;
    Token *input_next;
    size_t input_size;

    ASTNode *root;

    // language settings
    OperatorDeclSlice optable;
};

typedef ASTNode* (*ParserGrammar)(Parser*, DocumentError*);

extern Parser *ct_parser_new(Arena *arena);
extern void ct_parser_configure_operators(Parser *p, OperatorDeclSlice optable);
extern error ct_parser_parse(Parser *p, String source, ParserGrammar grammar, DocumentError *error);
extern Token *ct_parser_consume(Parser *p);
extern Token *ct_parser_peek(Parser *p);
extern Token *ct_parser_consume_if(Parser *p, TokenType type);
extern void ct_parser_rewind(Parser *p, Token *to);
extern void ct_parser_rewind_n(Parser *p, int n);
extern DocumentRange ct_parser_current_range(Parser *p);


/** Grammars */
extern ASTNode* ct_grammar_expr(Parser *p, DocumentError *err);


/** Hiding all internal macros */
#ifndef CT_INTERNAL
#undef TOKEN_LIST
#undef AST_TYPES_LIST
#endif

#endif
