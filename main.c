#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "lexing.c"
#include "print.c"
#include "parsing.c"
#include "resolve.c"
#include "cgen.c"

#include "vm.h"
#include "vm.c"

#include "main.h"
#include "gc_test.c"

void compile_and_run(void)
{
    char* test_file = "test/testcode.txt";
  
    string_ref file_buf = read_file(test_file);
    if (file_buf.str)
    {
        if (file_buf.length > 3)
        {
            // pomijanie BOM
            file_buf.str[0] = 0x20; // 0xef;
            file_buf.str[1] = 0x20; // 0xbb;
            file_buf.str[2] = 0x20; // 0xbf;
        }
       
        symbol** resolved = resolve(test_file, file_buf.str, false);

        size_t debug_count = buf_len(resolved);

        gen_common_includes();

        gen_printf_newline("\n// FORWARD DECLARATIONS\n");

        gen_forward_decls(resolved);

        gen_printf_newline("\n// DECLARATIONS\n");

        for (size_t i = 0; i < buf_len(resolved); i++)
        {
            gen_symbol_decl(resolved[i]);
        }

        debug_breakpoint;

        //printf("/// C OUTPUT:\n\n%s\n", gen_buf);

        write_file("test/testcode.c", gen_buf, buf_len(gen_buf));

        free(file_buf.str);
    }
}

#if 1
int main(int argc, char** argv)
{
    string_arena = allocate_memory_arena(megabytes(10));

    stretchy_buffers_test();
    intern_str_test();
   
    copy_test();
    map_test();
    stack_vm_test(code);

#if 0
    parse_test();
#elif 0
    resolve_test();
#elif 0
    cgen_test();
#else
    compile_and_run();
#endif

    //for (size_t i = 1; i < argc; i++)
    //{
    //    char* arg = argv[i];
    //    compile_and_run(arg);
    //}

    //gc_init();
    //gc_test();

    return 1;
}
#endif