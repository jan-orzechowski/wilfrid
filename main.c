#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define SOURCEFILE_EXTENSION "txt"

#include "lexing.c"
#include "ast_print.c"
#include "parsing.c"
#include "arithmetic.c"
#include "resolve.c"
#include "cgen.c"

#include "common_include.c"
#include "test_runner.c"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

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

    // zmienić na parametr
    if (true)
    {
        char *printed_errors = print_errors();
        write_file("output/errors.log", printed_errors, buf_len(errors));
        buf_free(printed_errors);
    }
}

void compile_sources(char **sources, bool print_ast)
{
    decl **all_declarations = null;
    symbol **resolved = null;
    for (size_t i = 0; i < buf_len(sources); i++)
    {
        char *filename = sources[i];
        if (path_has_extension(filename, SOURCEFILE_EXTENSION))
        {
            parse_file(filename, &all_declarations);
        }
        else
        {
            parse_directory(filename, &all_declarations);
        }
    }

    if (print_ast)
    {
        if (buf_len(all_declarations) > 0)
        {
            printf("\nGENERATED AST:\n");
            for (size_t i = 0; i < buf_len(all_declarations); i++)
            {
                printf_decl_ast(all_declarations[i]);
            }
        }
        else
        {
            printf("\nNO AST GENERATED\n");
        } 
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
#if __EMSCRIPTEN__
            run_interpreter(resolved);
#else
            c_gen(resolved, "output/testcode.c", false);
            run_interpreter(resolved);
#endif
        }
    }

    buf_free(all_declarations);
}

typedef struct cmd_arguments
{
    char **sources;
    bool test_mode;
    bool help;
    bool print_ast;
} cmd_arguments;

cmd_arguments parse_cmd_arguments(int arg_count, char **args)
{
    cmd_arguments result = {0};
    for (size_t i = 1; i < arg_count; i++)
    {
        char *arg = args[i];
        assert(arg != null);

        if (arg[0] == '-')
        {
            if (0 == strcmp(arg, "-test"))
            {
                result.test_mode = true;
            }
            else if (0 == strcmp(arg, "-help"))
            {
                result.help = true;
            }
            else if (0 == strcmp(arg, "-print-ast"))
            {
                result.print_ast = true;
            }
        }
        else
        {
            buf_push(result.sources, arg);
        }
    }
    return result;
}

void allocate_memory(void)
{
    arena = allocate_memory_arena(kilobytes(50));
    string_arena = allocate_memory_arena(kilobytes(500));
    interns = xcalloc(sizeof(hashmap));
    map_grow(interns, 16);

    map_grow(&global_symbols, 16);

    vm_global_memory = allocate_memory_arena(kilobytes(5));
    map_grow(&global_identifiers, 16);
}

void clear_memory()
{
    buf_free(all_tokens);

    free_memory_arena(string_arena);
    map_free(interns);
    free(interns);
    keywords_initialized = false;
    buf_free(keywords_list);

    free_memory_arena(arena);

    // tablice wskaźników - same obiekty są zaalokowane na arenie
    buf_free(global_symbols_list);
    buf_free(ordered_global_symbols);
    buf_free(cached_pointer_types);
   
    map_free(&global_symbols);

    memset(local_symbols, 0, MAX_LOCAL_SYMBOLS);
    last_local_symbol = local_symbols;

    installed_types_initialized = false;

    free_memory_arena(vm_global_memory);
    map_free(&global_identifiers);

    for (size_t i = 0; i < buf_len(global_xprintf_results); i++)
    {
        free(global_xprintf_results[i]);
    }
    buf_free(global_xprintf_results);

    buf_free(errors);
    buf_free(ast_buf);
    buf_free(gen_buf);
}

#ifndef __EMSCRIPTEN__
int main(int arg_count, char **args)
{ 
    cmd_arguments options = parse_cmd_arguments(arg_count, args);
    options.print_ast = true;
#if 1
    buf_push(options.sources, "test/dynamic_lists.txt");
#elif 1
    options.test_mode = true;
#else
    treewalk_interpreter_test();
#endif

    for (size_t i = 0; i < 1000; i++)
    {
        allocate_memory();

        if (options.help)
        {
            // po prostu wyrzucamy tekst do konsoli z predefiniowanego pliku
            printf("not implemented yet\n");
        }
        else if (options.test_mode)
        {
            run_all_tests();
        }
        else if (options.sources > 0)
        {
            compile_sources(options.sources, options.print_ast);
        }
        else
        {
            printf("No commands or source files provided.\n");
        }

        clear_memory();

        debug_breakpoint;
    }

    buf_free(options.sources);

    return 1;
}

#else 

typedef enum compiler_options
{
    COMPILER_OPTION_SHOW_AST = (1 << 0),
    COMPILER_OPTION_C = (1 << 1),
    COMPILER_OPTION_RUN = (1 << 2),
    COMPILER_OPTION_RUN_VERBOSE = (1 << 3),
    COMPILER_OPTION_HELP = (1 << 4),
    COMPILER_OPTION_TEST = (1 << 5),
} compiler_options;

EM_JS(void, define_compiler_options, (), {
    let COMPILER_OPTION_SHOW_AST = (1 << 0);
    let COMPILER_OPTION_C = (1 << 1);
    let COMPILER_OPTION_RUN = (1 << 2);
    let COMPILER_OPTION_RUN_VERBOSE = (1 << 3);
    let COMPILER_OPTION_HELP = (1 << 4);
    let COMPILER_OPTION_TEST = (1 << 5);
});

extern int main(int arg_count, char **args)
{
    // empty main
}

extern void compile_input(int64_t flags)
{
    if (flags & COMPILER_OPTION_RUN)
    {
        printf("COMPILER_OPTION_RUN\n");
    }

    if (flags & COMPILER_OPTION_SHOW_AST)
    {
        printf("COMPILER_OPTION_SHOW_AST\n");
    }

    allocate_memory();

    char **sources = null;
    buf_push(sources, "input.txt");
    compile_sources(sources, true);

    clear_memory();
}

#endif