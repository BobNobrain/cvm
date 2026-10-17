#include "util.h"
#include "lang.h"
#include "ct_int.h"

#define COMPILER_DEBUG 1
#if COMPILER_DEBUG
#define IFDEBUG(S) S
#else
#define IFDEBUG(S)
#endif

void ct_compiler_init(Compiler *c, LangConfig cfg, ParserGrammar grammar, Arena *arena) {
    c->config = cfg;
    c->arena = arena;
    c->grammar = grammar;
    ct_diagnostic_array_init(&c->diagnostics, 64, arena);

    String errmsg = ct_langconfig_validate(cfg);
    if (!str_is_empty(errmsg)) {
        ct_diagnostic_array_append(&c->diagnostics, (Diagnostic) {
            .location = { 0 },
            .message = errmsg,
            .severity = DiagnosticSeverity_ERROR,
            .source = DiagnosticSource_OTHER,
        });
    }
}

void _ct_compiler_start_stage(const char* name) {
    printf("Stage '%s'... ", name);
}
error _ct_compiler_end_stage(Compiler *c) {
    if (ct_compiler_has_errors(c)) {
        printf("failed\n");
        ct_compiler_print_diagnostics(c);
        return E_BAD_DATA;
    }

    printf("ok\n");
    return E_NONE;
}

Program ct_compiler_compile(Compiler *c, String program_source) {
    Program program = { 0 };

    IFDEBUG( printf("SOURCE: " STR_FMT_DEBUG "\n", STR_FMT_DEBUG_VAL(program_source)); )
    c->source = program_source;

    _ct_compiler_start_stage("Tokenization");
    TokenSlice tokens = ct_run_tokenization(c);
    if (_ct_compiler_end_stage(c) != E_NONE) { return program; }

    IFDEBUG( printf("TOKENS:\n"); )
    IFDEBUG( ct_tokenslice_print(tokens); )
    IFDEBUG( printf("\n"); )

    _ct_compiler_start_stage("Parsing");
    ASTNode *ast_root = ct_run_parsing(c, tokens);
    if (_ct_compiler_end_stage(c) != E_NONE) { return program; }

    _ct_compiler_start_stage("Typecheck");
    ct_run_typecheck(c, ast_root);
    if (_ct_compiler_end_stage(c) != E_NONE) { return program; }

    IFDEBUG( printf("AST:\n"); )
    IFDEBUG( ct_astnode_print(ast_root, 0); )

    _ct_compiler_start_stage("Bytecode generation");
    ct_run_bytecoding(c, ast_root, &program);
    _ct_compiler_end_stage(c);

    IFDEBUG( printf("Program:\n"); )
    IFDEBUG( lang_program_print(program); )

    return program;
}

bool ct_compiler_has_errors(Compiler *c) {
    for (size_t i = 0; i < c->diagnostics.size; i++) {
        if (c->diagnostics.content[i].severity == DiagnosticSeverity_ERROR) {
            return true;
        }
    }

    return false;
}

void ct_compiler_print_diagnostics(Compiler *c) {
    for (size_t i = 0; i < c->diagnostics.size; i++) {
        ct_diagnostic_print(c->diagnostics.content[i], c->source, c->arena);
    }
}

#undef COMPILER_DEBUG
#undef IFDEBUG
