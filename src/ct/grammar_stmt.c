#include "ct_int.h"

/*
TODO: statement parsing

x = 10 * 5
y = 11.0 / 2

function calls??
f = \x y. { x + y }
f2 = \x y. x - y
f3 = \x y. z { z = x - y }
apply = \f x. f x
z = { f (f 2 10) x }
*/

ASTNode* ct_grammar_stmt_list(Parser *p) {
    (void)p;
    return 0;
}
