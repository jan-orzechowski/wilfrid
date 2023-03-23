#include "utils.h"

char *parse_case(char **source, char *source_end, char *case_label, char *end_label)
{
    int case_label_length = strlen(case_label);
    int end_label_length = strlen(end_label);

    char *case_str = null;
    bool end_found = false;

    char *stream = *source;
    while (*stream)
    {
        if ((source_end - stream > case_label_length)
            && 0 == strncmp(stream, case_label, case_label_length))
        {
            stream += case_label_length;
            case_str = stream;
            while (*stream)
            {
                if ((source_end - stream > end_label_length)
                    && 0 == strncmp(stream, end_label, end_label_length))
                {
                    end_found = true;
                    case_str = make_str_range_copy(case_str, stream);
                    stream += end_label_length;
                    goto parse_case_exit_loop_label;
                }
                stream++;
            }
        }
        stream++;
    }

parse_case_exit_loop_label:
    
    *source = stream;

    if (end_found)
    {
        return case_str;
    }
    else
    {
        return null;
    }
}

void test_file_parsing_test(string_ref source, bool print_results)
{
    int case_counter = 0;
    int error_counter = 0;
    buf_free(errors);
    
    char *stream_end = source.str + source.length;
    char *stream = source.str;
    char *output = null;

    while (*stream)
    {
        char *case_str = parse_case(&stream, stream_end, "CASE:", ":END");
        char *test_str = parse_case(&stream, stream_end, "TEST:", ":END");
        if (case_str)
        {
            case_counter++;
            buf_printf(output, "\nParsing test case %d:\n", case_counter);
            if (test_str)
            {                
                bool passed = true;
                decl** decls = lex_and_parse(null, case_str);                    
                for (size_t i = 0; i < buf_len(decls); i++)
                {
                    char* ast = get_decl_ast(decls[i]);                    
                    if (0 != strcmp(test_str, ast))
                    {
                        passed = false;
                        buf_printf(output, "Generated AST doesn't match the test case:\n");
                        buf_printf(output, case_str);
                        buf_printf(output, "\nExpected:\n'%s'\n", test_str);
                        buf_printf(output, "Got:\n'%s'\n", ast);
                    }
                }

                if (buf_len(errors) > 0)
                {
                    passed = false;
                    buf_printf(output, print_errors());
                }

                if (passed == false)
                {
                    error_counter++;
                }
                else
                {
                    buf_printf(output, "Passed!\n");
                }
            }
            else
            {
                error_counter++;
                buf_printf(output, "CASE without matching TEST");
            }
        }
        else
        {
            break;
        }

        buf_free(errors);
    }

    if (print_results)
    {
        printf(output);
    }

    buf_free(output);

    if (error_counter > 0)
    {
        printf("\n\n");
        fatal("Parsing tests failed! Number of failed cases: %d", error_counter);
    }
    else
    {
        printf("\nAll parsing tests passed!\n", error_counter);
    }
}

void single_case_parsing_test(void)
{
    char *test = "fn f() { if (sizeof(x) == 4) { return x } }";

    printf("\nParsing test:\n%s\n", test);
    
    decl **decls = lex_and_parse(null, test);
    if (decls != 0)
    {
        char *ast = get_decl_ast(decls[0]);
        printf("\nAST:\n%s\n", ast);
    }   

    if (buf_len(errors) > 0)
    {
        print_errors_to_console();
        debug_breakpoint;
    }
}

void parsing_test(void)
{
    single_case_parsing_test();

    string_ref test_file = read_file_for_parsing("test/parsing_tests.txt");
    test_file_parsing_test(test_file, true);
}