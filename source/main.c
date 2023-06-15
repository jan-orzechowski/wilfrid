#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#define SRC_FILE_EXT "wil"

#ifdef _MSC_VER
// wyłącz ostrzeżenie o użyciu strncpy
#pragma warning(disable:4996)
#endif

#include "utils\utils.c"

memory_arena *arena;

#include "tokens.c"
#include "keywords.c"
#include "ast.c"
#include "types.c"
#include "lexing.c"

#include "ast_print.c"
#include "parsing.c"
#include "arithmetic.c"

#include "resolving.c"
#include "cgen.c"
#include "mangling.c"

#include "../include/common.c"
#include "test_runner.c"

#include "setup.c"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

typedef struct compiler_options
{
    char **sources;
    bool run;
    bool print_c;
    bool print_ast;
    bool test_mode;
    bool help;
} compiler_options;

void parse_file(char *filename, decl ***declarations_list)
{
    string_ref source = read_file_for_parsing(filename);
    if (source.str == null || source.length == 0)
    {
        error_without_pos(xprintf("Source file '%s' doesn't exist.", filename));
        return;
    }

    lex_and_parse(source.str, filename, declarations_list);

    free(source.str);
}

#include "treewalk.c"

void parse_directory(char *path, decl ***declarations_list)
{
    char **source_files = get_source_files_in_dir_and_subdirs(path);
    for (size_t i = 0; i < buf_len(source_files); i++)
    {
        char *filename = source_files[i];
        parse_file(filename, declarations_list);       
    }

    for (size_t i = 0; i < buf_len(source_files); i++)
    {
        free(source_files[i]);
    }
    buf_free(source_files);
}

void report_errors(void)
{
    if (buf_len(errors) == 0)
    {
        return;
    }

    // zmienić na parametr
    if (true)
    {
        print_errors_to_console();
    }

#ifndef __EMSCRIPTEN__
    // zmienić na parametr
    if (true)
    {
        char *printed_errors = print_errors();
        write_file("output/errors.log", printed_errors, buf_len(printed_errors));
        buf_free(printed_errors);
    }
#endif
}

void compile_sources(compiler_options options)
{
    decl **all_declarations = null;
    symbol **resolved = null;

    parse_file("include/common." SRC_FILE_EXT, &all_declarations);
    assert(buf_len(all_declarations) > 0);

    for (size_t i = 0; i < buf_len(options.sources); i++)
    {
        char *filename = options.sources[i];
        if (path_has_extension(filename, SRC_FILE_EXT))
        {
            parse_file(filename, &all_declarations);
        }
        else
        {
            parse_directory(filename, &all_declarations);
        }
    }
    
    if (options.print_ast)
    {
        decl **decls_to_print = null;
        for (size_t i = 0; i < buf_len(all_declarations); i++)
        {
            if (0 != strcmp(all_declarations[i]->pos.filename, "include/common." SRC_FILE_EXT))
            {
                buf_push(decls_to_print, all_declarations[i]);
            }
        }

        if (buf_len(decls_to_print) > 0)
        {
            printf("\nGENERATED AST:\n");
            for (size_t i = 0; i < buf_len(decls_to_print); i++)
            {
                printf_decl_ast(decls_to_print[i]);
            }
        }
        else
        {
            printf("\nNO AST GENERATED\n");
        } 

        buf_free(decls_to_print);
    }

    if (buf_len(errors) > 0)
    {
        report_errors();
    }
    else
    {
        resolved = resolve(all_declarations, true);
        
        if (buf_len(errors) > 0)
        {
            report_errors();
        }
        else
        {
            if (options.run)
            {
                run_interpreter(resolved);
            }
            else
            {
                c_gen(resolved, "output/output.c", options.print_c);
            }
        }
    }

    buf_free(all_declarations);
}

void test_directory(char *path, bool test_interpreter)
{
    compiler_options options = 
    { 
        .print_ast = true, 
        .print_c = true,
        .run = test_interpreter
    };

    char **source_files = get_source_files_in_dir_and_subdirs(path);
    assert(buf_len(source_files) > 0);
    for (size_t i = 0; i < buf_len(source_files); i++)
    {
        allocate_memory();
        char *test_file = source_files[i];
        if (0 != strcmp(test_file, "test/parsing_tests." SRC_FILE_EXT))
        {
            char **temp_buf = null;
            buf_push(temp_buf, test_file);
            options.sources = temp_buf;        
            compile_sources(options);
            buf_free(temp_buf);
            
            if (buf_len(errors) > 0)
            {
                fatal("Errors in parsing test file %s", test_file);
            }
        }
        clear_memory();
    }

    for (size_t i = 0; i < buf_len(source_files); i++)
    {
        free(source_files[i]);
    }
    buf_free(source_files);    
}

compiler_options parse_cmd_arguments(int arg_count, char **args)
{
    compiler_options result = {0};
    for (size_t i = 1; i < arg_count; i++)
    {
        char *arg = args[i];
        assert(arg != null);

        if (arg[0] == '-')
        {
            if (0 == strcmp(arg, "-run"))
            {
                result.print_ast = true;
            }
            else if (0 == strcmp(arg, "-print-c"))
            {
                result.print_c = true;
            }
            else if (0 == strcmp(arg, "-print-ast"))
            {
                result.print_ast = true;
            }
            else if (0 == strcmp(arg, "-test"))
            {
                result.test_mode = true;
            }
            else if (0 == strcmp(arg, "-help"))
            {
                result.help = true;
            }          
        }
        else
        {
            buf_push(result.sources, arg);
        }
    }
    return result;
}

#ifndef __EMSCRIPTEN__
int main(int arg_count, char **args)
{ 
    print_source_pos_mode = SOURCE_POS_PRINT_FULL;

    compiler_options options = parse_cmd_arguments(arg_count, args);

#if DEBUG_BUILD
#if 1
    options.run = true;
    buf_push(options.sources, "examples/stack_vm.wil");
#else
    options.print_ast = true;
    options.test_mode = true;
#endif
    for (size_t i = 0; i < 100; i++)
    {
#endif
        if (options.test_mode)
        {
            allocate_memory();
            run_all_tests();
            clear_memory();
            
            test_directory("test", false);
            test_directory("test", true);
        }
        else if (options.sources > 0)
        {
            allocate_memory();
            compile_sources(options);
            clear_memory();
        }       

#if DEBUG_BUILD
        debug_breakpoint;
    }
#endif

    buf_free(options.sources);

    return 0;
}

#else 

typedef enum compiler_option_flags
{
    COMPILER_OPTION_RUN = (1 << 0),
    COMPILER_OPTION_PRINT_C = (1 << 1),
    COMPILER_OPTION_PRINT_AST = (1 << 2),
    COMPILER_OPTION_TEST_MODE = (1 << 3),
    COMPILER_OPTION_HELP = (1 << 4),
} compiler_option_flags;

const char *emscripten_input_path = "input.wil";

EM_JS(void, define_compiler_constants_in_js, (void), {
    COMPILER_OPTION_RUN = (1 << 0);
    COMPILER_OPTION_PRINT_C = (1 << 1);
    COMPILER_OPTION_PRINT_AST = (1 << 2);
    COMPILER_OPTION_TEST_MODE = (1 << 3);
    COMPILER_OPTION_HELP = (1 << 4);

    COMPILER_INPUT_PATH = "input.wil";
    COMPILER_MAIN_CALLED = true;
});

extern int main(int arg_count, char **args)
{
    print_source_pos_mode = SOURCE_POS_PRINT_WITHOUT_FILE;
    define_compiler_constants_in_js();
    return 0;
}

extern void reset_memory(void)
{
    clear_memory();
}

extern void compile_input(int64_t flags)
{  
    compiler_options options = { 0 };
    buf_push(options.sources, emscripten_input_path);

    if (flags & COMPILER_OPTION_RUN)
    {
        options.run = true;
    }
    else
    {
        generate_line_hints = false;
        options.print_c = true;
    }

    allocate_memory();
   
    compile_sources(options);

    // make sure that we flush everything to the output window
    printf("\n");

    buf_free(options.sources);
    clear_memory();
}

#endif