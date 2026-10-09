#ifndef CT_H
#define CT_H
/**
 * Compile-time library
 */

#include <stddef.h>
#include <stdbool.h>
#include "util.h"
#include "lang.h"

typedef struct Parser Parser;
typedef struct ASTNode ASTNode;
typedef struct LmbProgramType LmbProgramType;


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
extern void ct_document_print_error(DocumentError error, Arena *arena);

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

typedef int OperatorVariant;
enum OperatorVariant {
    OperatorVariant_UNKNOWN,
    OperatorVariant_UNARY_NOT,
    OperatorVariant_UNARY_MINUS,
    OperatorVariant_UNARY_PLUS,
    OperatorVariant_BINARY_POWER,
    OperatorVariant_BINARY_MUL,
    OperatorVariant_BINARY_DIV,
    OperatorVariant_BINARY_REM,
    OperatorVariant_BINARY_ADD,
    OperatorVariant_BINARY_SUB,
    OperatorVariant_BINARY_LT,
    OperatorVariant_BINARY_GT,
    OperatorVariant_BINARY_LTE,
    OperatorVariant_BINARY_GTE,
    OperatorVariant_BINARY_EQ,
    OperatorVariant_BINARY_NEQ,
    OperatorVariant_BINARY_AND,
    OperatorVariant_BINARY_OR,
};

typedef struct {
    String op;
    OperatorType type;
    OperatorPriority priority;
    OperatorVariant variant;
} OperatorDecl;

SLICE_DECL(OperatorDecl)
SLICE_METHODS_DECL(ct_opdeclslice, OperatorDecl)


/** Language configuration */
typedef struct LangConfig {
    OperatorDeclSlice optable;
    String allowed_ident_chars;
    String allowed_operator_chars;
    String line_comment_start;
} LangConfig;

extern LangConfig ct_langconfig_create();
extern String ct_langconfig_validate(const LangConfig cfg);


/** Tokenizer */
// Language keywords
typedef enum Keyword {
    Keyword_LET,
    Keyword_IF,
    Keyword_ELSE,
    Keyword_ENTRY,
} Keyword;

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
    X(TokenType_COLON, , ) \
    X(TokenType_ASSIGNMENT, ,)

#define TOKEN_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) CONST_NAME,
// All language token types, + INVALID
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
    X(ASTNodeType_IDENT, ASTIdentData, ident) \
    X(ASTNodeType_LAMBDA, ASTLambdaData, lambda) \
    X(ASTNodeType_LAMBDA_ARG, ASTLambdaArgData, lambda_arg) \
    X(ASTNodeType_BINOP, OperatorDecl, binop) \
    X(ASTNodeType_UNOP, OperatorDecl, unop) \
    X(ASTNodeType_ASSIGNMENT, ASTAssignmentData, assignment) \
    X(ASTNodeType_ENTRY, , ) \
    X(ASTNodeType_LMB_FILE, , ) \
    X(ASTNodeType_FNCALL, , ) \

typedef enum ASTNodeType {
    AST_TYPES_LIST(ENUM_MEMBERS)
} ASTNodeType;

// AST node data for assignments (let x = ?)
typedef struct ASTAssignmentData {
    String identifier;
    Token *ident_token;
} ASTAssignmentData;

// AST node data for identifiers
typedef struct ASTIdentData {
    String name;
    // 0 for unbound, 1+ for closest lambda arguments
    size_t bound_index;
} ASTIdentData;

// AST node data for lambdas (\x y.?)
typedef struct ASTLambdaData {
    size_t n_args;
} ASTLambdaData;

// AST node data for lambda args
typedef struct ASTLambdaArgData {
    String name;
    String type;
} ASTLambdaArgData;

struct ASTNode {
    ASTNodeType type;
    // A dynamic array of child nodes. Use ct_astnode_append_child to add a child
    ASTNode **children;
    size_t n_children;
    size_t children_cap;
    DocumentRange range;
    // The defining token of this node (main keyword, or operator, or the single token that generates the node)
    Token *base;
    // If this node represents a value (or an expression), what type does it have (see LmbProgramType)
    LmbProgramType *value_type;

    #define AST_TYPES_LIST_X(CONST_NAME, DATA_TYPE, FIELD_NAME) DATA_TYPE FIELD_NAME;
    // Additional data, as defined by .type
    union {
        AST_TYPES_LIST(AST_TYPES_LIST_X)
    } data;
    #undef AST_TYPES_LIST_X
};

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
    // tokenizer result (not changed)
    TokenSlice input_original;
    // AST root (whatever is generated by the grammar that was passed into ct_parser_parse)
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
// Expression
extern ASTNode* ct_grammar_expr(Parser *p);
// A lambda literal
extern ASTNode* ct_grammar_lambda(Parser *p);
// An .lmb file (assignments and entry declarations)
extern ASTNode* ct_grammar_lmb_file(Parser *p);


/** program types and type checking */
// Enumeration of possible type kinds
typedef enum LmbProgramTypeKind {
    LmbProgramTypeKind_IO,
    LmbProgramTypeKind_PRIMITIVE,
    LmbProgramTypeKind_ARROW,
} LmbProgramTypeKind;

typedef struct LmbProgramArrowTypeData {
    size_t n_args;
    LmbProgramType **arg_types;
    LmbProgramType *ret_type;
} LmbProgramArrowTypeData;

struct LmbProgramType {
    LmbProgramTypeKind kind;
    union {
        LmbProgramArrowTypeData arrow;
        PrimitiveType primitive;
    } data;
};

extern void ct_type_print(LmbProgramType *type);
extern void ct_parser_assign_types(Parser *p);

// typedef struct LmbProgramExpr LmbProgramExpr;

// typedef struct LmbProgramLambdaData {
//     size_t n_args;
//     LmbProgramExpr *body;
// } LmbProgramLambdaData;

// typedef struct LmbProgramFnCallData {
//     LmbProgramExpr *fn;
//     LmbProgramExpr *args;
//     size_t args_size;
// } LmbProgramFnCallData;

// typedef struct LmbProgramPrimitiveData {
//     PrimitiveValue value;
// } LmbProgramPrimitiveData;

// typedef struct LmbProgramVariableData {
//     size_t bound_var_index;
// } LmbProgramVariableData;

// #define LMB_PROGRAM_EXPR_TYPE_LIST(X) \
//     X(LmbProgramExprType_BUILTIN, String, builtin) \
//     X(LmbProgramExprType_FNCALL, LmbProgramFnCallData, fncall) \
//     X(LmbProgramExprType_PRIMITIVE, LmbProgramPrimitiveData, primitive) \
//     X(LmbProgramExprType_LAMBDA, LmbProgramLambdaData, lambda) \
//     X(LmbProgramExprType_VARIABLE, LmbProgramVariableData, variable) \

// typedef enum LmbProgramExprType {
//     LMB_PROGRAM_EXPR_TYPE_LIST(ENUM_MEMBERS)
// } LmbProgramExprType;

// struct LmbProgramExpr {
//     TAGGED_UNION(LmbProgramExprType, LMB_PROGRAM_EXPR_TYPE_LIST)
//     LmbProgramType *expr_type;
// };

// typedef struct LmbProgramAssignment {
//     String name;
//     LmbProgramExpr *value;
// } LmbProgramAssignment;

// typedef struct LmbProgramFile {
//     LmbProgramAssignment *assignments;
//     LmbProgramLambdaData entry;
// } LmbProgramFile;


/** Hiding all internal macros */
#ifndef CT_INTERNAL
#undef TOKEN_LIST
#undef AST_TYPES_LIST
#undef LMB_PROGRAM_EXPR_TYPE_LIST
#endif

#endif
