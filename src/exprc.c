#include <stdio.h>
#include <stddef.h>
#include "util.h"
#include "ct.h"

int main() {
    ERR_DECL
    const char TEST_EXPR[] = "(3.2 + 5 * 2 ) asd true ";

    Tokenizer t;
    err = ct_tokenizer_init(&t);
    if (err != E_NONE) {
        printf("tokenizer init failed\n");
        return -1;
    }

    err = ct_tokenizer_run(&t, str_wrap(TEST_EXPR));
    if (err != E_NONE) {
        printf("tokenizer run failed: %d\n", err);
        return -1;
    }

    return 0;
}
