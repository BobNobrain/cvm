#ifndef CT_INTERNAL
#define CT_INTERNAL

#include "ct.h"

typedef struct Tokenizer {
    TokenArray tokens;
    DocumentPos cursor;
    LangConfig config;
    String source;
} Tokenizer;

extern void ct_tokenizer_init(Tokenizer *t, Arena *arena, LangConfig config);
extern void ct_tokenizer_run(Tokenizer *t, String source, DiagnosticArray *errors);
extern void ct_tokenizer_print(Tokenizer *t);

typedef struct ParserRewindPoint {
    TokenSlice input;
} ParserRewindPoint;

extern Token* ct_parser_consume(Parser *p);
extern Token* ct_parser_peek(Parser *p);
extern Token* ct_parser_consume_if(Parser *p, TokenType type);
extern Token* ct_parser_consume_keyword(Parser *p, Keyword kw);
extern void ct_parser_rewind(Parser *p, ParserRewindPoint to);
extern void ct_parser_rewind_n(Parser *p, int n);
extern DocumentRange ct_parser_current_range(Parser *p);
extern void ct_parser_add_diagnostic(Parser *p, Diagnostic d);

extern LmbProgramType* _ct_type_get_primitive(PrimitiveType type);

#endif
