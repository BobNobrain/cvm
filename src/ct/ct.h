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


/**
 * Aux structures to aid with parsing
 */
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

typedef enum DiagnosticSeverity {
    DiagnosticSeverity_ERROR,
    DiagnosticSeverity_WARN,
    DiagnosticSeverity_INFO,
} DiagnosticSeverity;

typedef enum DiagnosticSource {
    DiagnosticSource_OTHER,
    DiagnosticSource_SYNTAX,
    DiagnosticSource_TYPECHECK,
} DiagnosticSource;

typedef struct Diagnostic {
    DocumentRange location;
    String message;
    DiagnosticSource source;
    DiagnosticSeverity severity;
} Diagnostic;

extern void ct_diagnostic_print(Diagnostic d, String source_doc, Arena *arena);

ARRAY_DECL(Diagnostic)
ARRAY_METHODS_DECL(ct_diagnostic_array, Diagnostic)


/**
 * Operators and language configuration
 */
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

typedef struct LangConfig {
    OperatorDeclSlice optable;
    String allowed_ident_chars;
    String allowed_operator_chars;
    String line_comment_start;
} LangConfig;

extern LangConfig ct_langconfig_create();
extern String ct_langconfig_validate(const LangConfig cfg);


/**
 * Tokenizer
 */
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


/**
 * Language AST
 */
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
    ASTNode *first_child;
    ASTNode *next_sibling;

    // What document range corresponds to this node
    DocumentRange range;
    // The defining token of this node (main keyword, or operator, or the single token that generates the node)
    Token *base;

    // If this node represents a value (or an expression), what type does it have (see LmbProgramType)
    LmbProgramType *value_type;
    // TODO: type context?

    // Additional data, as defined by .type
    union {
        AST_TYPES_LIST(UNION_FIELDS)
    } data;
    ASTNodeType type;
};

extern ASTNode* ct_astnode_new(Parser *p, ASTNodeType type);
extern ASTNode* ct_astnode_new_error(Parser *p, String msg);
extern ASTNode* ct_astnode_new_error_ranged(Parser *parser, String msg, DocumentRange range);

extern void ct_astnode_append_child(ASTNode *parent, ASTNode *child);
extern size_t ct_astnode_count_children(ASTNode *parent);
extern ASTNode* ct_astnode_find_last_child(ASTNode *parent);
extern void ct_astnode_print(ASTNode *node, size_t indent);
extern bool ct_astnode_is_error(ASTNode *node);


/**
 * The parser itself
 */
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
    // Diagnostic messages
    DiagnosticArray diagnostics;
};

typedef ASTNode* (*ParserGrammar)(Parser*);

extern Parser* ct_parser_new(Arena *arena);
extern String ct_parser_configure(Parser *p, LangConfig config);
extern void ct_parser_parse(Parser *p, String source, ParserGrammar grammar);


/**
 * Grammars
 */
// Expression
extern ASTNode* ct_grammar_expr(Parser *p);
// A lambda literal
extern ASTNode* ct_grammar_lambda(Parser *p);
// An .lmb file (assignments and entry declarations)
extern ASTNode* ct_grammar_lmb_file(Parser *p);


/**
 * Type checking
 */
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


/** Hiding all internal macros */
#ifndef CT_INTERNAL
#undef TOKEN_LIST
#undef AST_TYPES_LIST
#endif

#endif
