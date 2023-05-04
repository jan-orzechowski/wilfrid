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
        error("Failed to parse source file", (source_pos) { .filename = filename }, 0);
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

void compile_sources(char **sources, bool print_ast)
{
    decl **all_declarations = null;
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
        printf("\nOUTPUT AST:\n");
        for (size_t i = 0; i < buf_len(all_declarations); i++)
        {
            printf("\n%s\n", get_decl_ast(all_declarations[i]));
        }        
    }

    symbol **resolved = resolve(all_declarations, true);
    c_gen(resolved, "test/testcode.c", "errors.log", false);
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

int main(int arg_count, char **args)
{
    arena = allocate_memory_arena(kilobytes(50));
    string_arena = allocate_memory_arena(kilobytes(500));
    interns = xcalloc(sizeof(hashmap));
    map_grow(interns, 16);
        
    cmd_arguments options = parse_cmd_arguments(arg_count, args);

#if DEBUG_BUILD 
    options.print_ast = true;
#if 0
    buf_push(options.sources, "test/dynamic_lists.txt");
#elif 0
    options.test_mode = true;
#else
    treewalk_interpreter_test();
#endif
#endif

    if (options.help)
    {
        // po prostu wyrzucamy tekst do konsoli z predefiniowanego pliku
        printf("not implemented yet");
    }
    else if (options.test_mode)
    {
        run_all_tests();
    }
    else if (options.sources > 0)
    {        
        compile_sources(options.sources, options.print_ast);
        print_errors_to_console();
    }
    else
    {
        printf("No commands or source files provided.");
    }

    return 1;
}