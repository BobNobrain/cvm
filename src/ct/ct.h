#ifndef CT_H
#define CT_H
/**
 * Compile-time library
 */

#include <stddef.h>
#include <stdbool.h>
#include "util.h"


/** Aux structures to aid with parsing */
typedef struct {
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
} Tokenizer;

extern error ct_tokenizer_init(Tokenizer *t);
extern error ct_tokenizer_run(Tokenizer *t, String source);


/** Language AST */
#define AST_TYPES_LIST(X) \
    X(AST_TYPE_LINT, int, lint) \
    X(AST_TYPE_LFLOAT, float, lfloat) \
    X(AST_TYPE_LBOOL, bool, lbool) \
    X(AST_TYPE_IDENT, , ) \
    X(AST_TYPE_BINOP, , ) \
    X(AST_TYPE_UNOP, , ) \

#define AST_TYPES_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum ASTNodeType {
    AST_TYPES_LIST(AST_TYPES_LIST_X)
} ASTNodeType;
#undef AST_TYPES_LIST_X

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode *parent;
    struct ASTNode **children;
    size_t n_children;
    size_t children_cap;
    DocumentRange range;
    Token *base;
} ASTNode;

typedef struct {
    ASTNode **blocks;
    size_t block_size;
    size_t n_blocks_used;
    size_t capacity;
    size_t current_block_size;
} ASTree;

extern error ct_ast_tree_init(ASTree *tree);
extern ASTNode *ct_ast_node_new(ASTree *tree, ASTNodeType type, ASTNode *parent);


/** The parser itself */
typedef struct {
    Token *input_start;
    Token *input_next;
    size_t input_size;

    ASTree result;
    ASTNode *current;
} Parser;

extern error ct_ast_parser_init(Parser *p, Tokenizer *input);

/** Hiding all internal macros */
#ifndef CT_INTERNAL
#undef TOKEN_LIST
#undef AST_TYPES_LIST
#endif

#endif
