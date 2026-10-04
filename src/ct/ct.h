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

ARRAY_DECL(DocumentError)
ARRAY_METHODS_DECL(ct_err_array, DocumentError)


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


/** Language configuration */
typedef struct LangConfig {
    OperatorDeclSlice optable;
    String allowed_ident_chars;
    String allowed_operator_chars;
} LangConfig;

extern LangConfig ct_langconfig_create();
extern String ct_langconfig_validate(const LangConfig cfg);


/** Tokenizer */
#define TOKEN_LIST(X) \
    X(TokenType_NEWLINE, , ) \
    X(TokenType_IDENT, String, ident) \
    X(TokenType_KEYWORD, Keyword, keyword) \
    X(TokenType_INT_LITERAL, int, intl) \
    X(TokenType_FLOAT_LITERAL, float, floatl) \
    X(TokenType_BOOL_LITERAL, bool, booll) \
    X(TokenType_OPERATOR, String, op) \
    X(TokenType_OPEN_PAREN, , ) \
    X(TokenType_CLOSE_PAREN, , ) \
    X(TokenType_LAMBDA, , ) \
    X(TokenType_DOT, , ) \
    X(TokenType_ASSIGNMENT, ,)

typedef enum Keyword {
    Keyword_LET,
    Keyword_IF,
    Keyword_ELSE,
} Keyword;

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum TokenType {
    TOKEN_LIST(TOKEN_LIST_X)

    TokenType_INVALID
} TokenType;
#undef TOKEN_LIST_X

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
typedef struct Token {
    TokenType type;
    DocumentRange range;
    union {
        TOKEN_LIST(TOKEN_LIST_X)
    } data;
} Token;
#undef TOKEN_LIST_X

ARRAY_DECL(Token)
ARRAY_METHODS_DECL(ct_tokenarray, Token)

SLICE_DECL(Token)
SLICE_METHODS_DECL(ct_tokenslice, Token)

SLICE_ARRAY_METHODS_DECL(ct_tokenarray, Token)

typedef struct Tokenizer {
    TokenArray tokens;
    DocumentPos cursor;
    LangConfig config;
    String source;
} Tokenizer;

extern void ct_tokenizer_init(Tokenizer *t, Arena *arena, LangConfig config);
extern void ct_tokenizer_run(Tokenizer *t, String source, DocumentErrorArray *errors);
extern void ct_tokenizer_print(Tokenizer *t);


/** Language AST */
#define AST_TYPES_LIST(X) \
    X(ASTNodeType_SYNTAX_ERROR, String, error) \
    X(ASTNodeType_LINT, int, lint) \
    X(ASTNodeType_LFLOAT, float, lfloat) \
    X(ASTNodeType_LBOOL, bool, lbool) \
    X(ASTNodeType_IDENT, String, ident) \
    X(ASTNodeType_LAMBDA, ASTLambdaData, lambda) \
    X(ASTNodeType_BINOP, , ) \
    X(ASTNodeType_UNOP, , ) \
    X(ASTNodeType_ASSIGNMENT, ASTAssignmentData, assignment) \
    X(ASTNodeType_LMB_FILE, , ) \
    X(ASTNodeType_FNCALL, , ) \

#define AST_TYPES_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
typedef enum ASTNodeType {
    AST_TYPES_LIST(AST_TYPES_LIST_X)
} ASTNodeType;
#undef AST_TYPES_LIST_X

typedef struct ASTAssignmentData {
    String identifier;
    Token *ident_token;
} ASTAssignmentData;

typedef struct ASTLambdaData {
    StringArray argnames;
} ASTLambdaData;

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

extern ASTNode* ct_astnode_new(Parser *p, ASTNodeType type);
extern ASTNode* ct_astnode_new_error(Parser *p, String msg);
extern ASTNode* ct_astnode_new_error_from(Parser *p, DocumentError docerr);
extern ASTNode* ct_astnode_new_error_ranged(Parser *parser, String msg, DocumentRange range);
extern void ct_astnode_alloc_children(Parser *p, ASTNode *parent, size_t n);
extern void ct_astnode_append_child(Parser *p, ASTNode *parent, ASTNode *child);
extern void ct_astnode_print(ASTNode *node, size_t indent);
extern bool ct_astnode_is_error(ASTNode *node);


/** The parser itself */
struct Parser {
    // language parsing and tokenizing config
    LangConfig config;
    // allocator
    Arena *arena;
    // program source
    String source;
    // remaining tokens to parse
    TokenSlice input;
    // tokenizer result
    TokenSlice input_original;
    // AST root
    ASTNode *root;
    // tokenizing and parsing errors
    DocumentErrorArray errors;
};

typedef ASTNode* (*ParserGrammar)(Parser*);

typedef struct ParserRewindPoint {
    TokenSlice input;
} ParserRewindPoint;

extern Parser* ct_parser_new(Arena *arena);
extern String ct_parser_configure(Parser *p, LangConfig config);
extern void ct_parser_parse(Parser *p, String source, ParserGrammar grammar);
extern Token* ct_parser_consume(Parser *p);
extern Token* ct_parser_peek(Parser *p);
extern Token* ct_parser_consume_if(Parser *p, TokenType type);
extern Token* ct_parser_consume_keyword(Parser *p, Keyword kw);
extern void ct_parser_rewind(Parser *p, ParserRewindPoint to);
extern void ct_parser_rewind_n(Parser *p, int n);
extern DocumentRange ct_parser_current_range(Parser *p);
extern void ct_parser_append_error(Parser *p, char* msg, DocumentRange range);
extern DocumentError ct_parser_make_error(Parser *p, char* msg, DocumentRange range);


/** Grammars */
extern ASTNode* ct_grammar_expr(Parser *p);
extern ASTNode* ct_grammar_lmb_file(Parser *p);


/** Hiding all internal macros */
#ifndef CT_INTERNAL
#undef TOKEN_LIST
#undef AST_TYPES_LIST
#endif

#endif
