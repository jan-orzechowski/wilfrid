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
#include "print.c"
#include "parsing.c"
#include "resolve.c"
#include "vm.h"
#include "vm.c"

#include "main.h"

int main(int argc, char** argv)
{
    string_arena = allocate_memory_arena(megabytes(10));

    stretchy_buffers_test();
    intern_str_test();

    copy_test();

    stack_vm_test(code);
    
    //parse_test();
    resolve_test();

    return 1;
}