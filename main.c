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
#include "cgen.c"

#include "vm.h"
#include "vm.c"

#include "main.h"

typedef struct string_ref
{
    char* str;
    size_t length;
} string_ref;

string_ref read_file(char* filename)
{
    FILE* src;
    errno_t err = fopen_s(&src, filename, "rb");
    if (err != 0)
    {
        // błąd
        return (string_ref){ 0 };
    }

    fseek(src, 0, SEEK_END);
    size_t size = ftell(src);
    fseek(src, 0, SEEK_SET);
  
    if (size > 0)
    {
        char* str_buf = xmalloc(size + 1);
        str_buf[size] = 0; // null terminator

        size_t objects_read = fread(str_buf, size, 1, src);
        if (objects_read == 1)
        {            
            fclose(src);
            return (string_ref) { .str = str_buf, .length = size };
        }
        else
        {
            fclose(src);
            free(str_buf);
            return (string_ref) { 0 };
        }
    }
    else
    {
        fclose(src);
        return (string_ref) { 0 };
    }
}

bool write_file(const char* path, const char* buf, size_t len)
{
    FILE* file;
    errno_t err = fopen_s(&file, path, "w");
    if (err != 0)
    {
        // błąd
        return false;
    }

    if (!file)
    {
        return false;
    }
    size_t elements_written = fwrite(buf, len, 1, file);
    fclose(file);
    return (elements_written == 1);
}

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
       
        symbol** resolved = resolve(file_buf.str, true);

        size_t debug_count = buf_len(resolved);

        gen_printf_newline("// FORWARD DECLARATIONS\n");

        gen_forward_decls(resolved);

        gen_printf_newline("\n// DECLARATIONS\n");

        for (size_t i = 0; i < buf_len(resolved); i++)
        {
            gen_symbol_decl(resolved[i]);
        }

        debug_breakpoint;

        printf("/// C OUTPUT:\n\n%s\n", gen_buf);

        write_file("test/testcode.c", gen_buf, buf_len(gen_buf));

        free(file_buf.str);
    }
}

int main(int argc, char** argv)
{
    string_arena = allocate_memory_arena(megabytes(10));

    stretchy_buffers_test();
    intern_str_test();

    copy_test();

    stack_vm_test(code);
    
    //parse_test();
#if 0
    resolve_test();
#else
    //cgen_test();
#endif

    compile_and_run();

    //for (size_t i = 1; i < argc; i++)
    //{
    //    char* arg = argv[i];
    //    compile_and_run(arg);
    //}

    return 1;
}