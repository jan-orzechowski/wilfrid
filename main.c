#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "lexing.c"
#include "parsing.c"
#include "vm.h"
#include "vm.c"

#include "main.h"

/*
void parsing_and_vm_test(char* expr, int value)
{
    init_stream(expr);
    while (token.kind)
    {
        next_token();
    }

    get_first_lexed_token();
    //parse_expr();
    buf_push(code, POP);
    
    stack_size = 1024;
    stack = xmalloc(sizeof(int) * stack_size);

    buf_free(vm_output);

    run_vm(code);

    assert(vm_output[0] == value);

    buf_free(code);
    buf_free(vm_output);
    free(stack);
}
*/

//#define PARSING_TEST(expr) { int val = (expr); parsing_and_vm_test(#expr, val); } 

int main(int argc, char** argv)
{
    stretchy_buffers_test();
    intern_str_test();

    stack_vm_test(code);
    
    test_parsing();

    //PARSING_TEST(2 + 2);
    //PARSING_TEST((12    + 4) + 28 - 14 + (8 - 4) / 2 + (2 * 2 - 1 * 4));
    //PARSING_TEST(2 +-2 / -2);

    return 1;
}