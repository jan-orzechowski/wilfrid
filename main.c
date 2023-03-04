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
    char* test_file = "test/dynamic_lists.txt";
  
    string_ref file_buf = read_file(test_file);
    if (file_buf.str)
    {
        if (file_buf.length > 3)
        {
            // pomijanie BOM
            if (   file_buf.str[0] == (char)0xef
                && file_buf.str[1] == (char)0xbb 
                && file_buf.str[2] == (char)0xbf)
            {
                file_buf.str[0] = 0x20;
                file_buf.str[1] = 0x20;
                file_buf.str[2] = 0x20;
            }
        }
       
        symbol** resolved = resolve(test_file, file_buf.str, true);

        size_t debug_count = buf_len(resolved);

        gen_common_includes();

        gen_printf_newline("\n// FORWARD DECLARATIONS\n");

        gen_forward_decls(resolved);

        gen_printf_newline("\n// ENTRY POINT \n");

        gen_entry_point();
        
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

void common_includes_test(void);

int main(int argc, char** argv)
{
    string_arena = allocate_memory_arena(megabytes(10));

    stretchy_buffers_test();
    intern_str_test();
   
    copy_test();
    map_test();
    stack_vm_test(code);

    common_includes_test();

#if 0
    parse_test();
#elif 0
    resolve_test();
    mangled_names_test();
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

#include "common_include.c"

void common_includes_test(void)
{
    int* int_list = 0;

    assert(___get_list_length___(int_list) == 0);
    assert(___get_list_capacity___(int_list) == 0);

    {
        int ___temp = 12;
        int_list = ___list_add___(int_list, sizeof(int), &___temp);
    }

    {
        int ___temp = 16;
        int_list = ___list_add___(int_list, sizeof(int), &___temp);
    }

    {
        int ___temp = 20;
        int_list = ___list_add___(int_list, sizeof(int), &___temp);
    }

    assert(int_list[0] == 12);
    assert(int_list[1] == 16);
    assert(int_list[2] == 20);
        
    assert(___get_list_length___(int_list) == 3);
    assert(___get_list_capacity___(int_list) == 7);

    ___list_free___(int_list);
    int_list = 0;

    assert(___get_list_length___(int_list) == 0);
    assert(___get_list_capacity___(int_list) == 0);

    // alternatywna inicjalizacja
    tok* token_list = ___list_fit___(0, 16, sizeof(tok));

    assert(___get_list_length___(token_list) == 0);

    {
        tok ___temp = { .kind = TOKEN_NAME, .val = 666 };
        token_list = ___list_add___(token_list, sizeof(tok), &___temp);
    }

    assert(___get_list_length___(token_list) == 1);

    assert(token_list[0].kind == TOKEN_NAME);
    assert(token_list[0].val == 666);

    token_list[0].val = 667;

    assert(token_list[0].val == 667);

    token_list[0] = (tok){.kind = TOKEN_ASSIGN, .val = 668};

    assert(token_list[0].kind == TOKEN_ASSIGN);
    assert(token_list[0].val == 668);

    {
        tok ___temp = { .kind = TOKEN_MUL, .val = 669 };
        token_list = ___list_add_at_index___(token_list, sizeof(tok), &___temp, 2);
    }

    assert(token_list[1].kind == 0);
    assert(token_list[1].val == 0);

    assert(token_list[2].kind == TOKEN_MUL);
    assert(token_list[2].val == 669);

    ___list_remove___(token_list, sizeof(tok), 1);

    assert(token_list[1].kind == TOKEN_MUL);
    assert(token_list[1].val == 669);

    assert(token_list[2].kind == 0);
    assert(token_list[2].val == 0);

    ___list_free___(token_list);
    token_list = 0;
}