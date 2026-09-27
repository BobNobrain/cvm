# CVM

> CVM – a toy virtual machine, written in C

## Project Structure

```
cvm/
    header/         common includes (nothing for now)
    out/            output files
    src/            source code: each file is an executable, each dir is a module, compiled to a static lib
        ct/         compile-time functionality: tokens, parsing, AST, etc.
        lang/       common language data structures and functions to work with them
        rt/         runtime functionality: bytecode interpreter, etc.
        util/       common utilities (strings, helper defines, and such)
        c.c         language to bytecode compiler
        exprc.c     test compiler that only works with expressions
        vm.c        bytecode interpreter
    tests/          reserved for tests
```

Each `src/` subdirectory is considered a "module" and compiled into its own object file.

Each `src/*.c` file is considered an executable entry point; each has its own `make` target, of the same name.

Not every executable needs every module; for each executable, the dependency list is *manually* specified in the
`Makefile`. To create an executable, every module it needs is compiled into an object file, and then linked with the
object file for the entrypoint.

## Environment Variables

An env variable `MODE` can be used to select between `debug` and `release` modes.
