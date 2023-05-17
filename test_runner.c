#include "utils.h"

char *test_parse_case(char **source, char *source_end, char *case_label, char *end_label)
{
    int case_label_length = (int)strlen(case_label);
    int end_label_length = (int)strlen(end_label);

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
        char *case_str = test_parse_case(&stream, stream_end, "CASE:", ":END");
        char *test_str = test_parse_case(&stream, stream_end, "TEST:", ":END");

        if (case_str)
        {
            case_counter++;
            buf_printf(output, "\nParsing test case %d:\n", case_counter);
            buf_printf(output, "%s\n", case_str);

            if (test_str)
            {                
                bool passed = true;
                decl **decls = 0;
                lex_and_parse(case_str, null, &decls);
                for (size_t i = 0; i < buf_len(decls); i++)
                {
                    char *ast = get_decl_ast(decls[i]);                    
                    if (0 != strcmp(test_str, ast))
                    {
                        passed = false;
                        buf_printf(output, "Generated AST doesn't match the test case:\n");
                        buf_printf(output, case_str);
                        buf_printf(output, "\nExpected:\n'%s'\n", test_str);
                        buf_printf(output, "Got:\n'%s'\n", ast);
                    }
                    buf_free(ast_buf);
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

                buf_free(decls);               
                free(test_str);
            }
            else
            {
                error_counter++;
                buf_printf(output, "CASE without matching TEST");
            }

            free(case_str);
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
        printf("\nAll parsing tests passed!\n");
    }
}

void test_parsing_single_case(void)
{
    char *test = "let x := xcalloc(new_capacity * size_of_type(void*))";

    printf("\nParsing test case 0:\n%s\n", test);
    
    decl **decls = 0;
    lex_and_parse(test, null, &decls);
    if (decls != 0)
    {
        printf("\nAST:");
        printf_decl_ast(decls[0]);
    }   

    if (buf_len(errors) > 0)
    {
        print_errors_to_console();
        debug_breakpoint;
    }
}

void test_parsing(void)
{
    printf("\n==== PARSING TEST ====\n");

    test_parsing_single_case();

    string_ref test_file = read_file_for_parsing("test/parsing_tests.txt");
    test_file_parsing_test(test_file, true);
}

symbol **test_resolve_decls(char **decl_arr, size_t decl_arr_count, bool print_ast)
{
    assert(arena != null);
    buf_free(global_symbols_list);
    buf_free(ordered_global_symbols);
    map_free(&global_symbols);
    map_grow(&global_symbols, 16);

    installed_types_initialized = false;
    init_installed_types();

    size_t error_counter = 0;
    decl **all_decls = 0;
    for (size_t i = 0; i < decl_arr_count; i++)
    {
        buf_free(errors);

        char *str = decl_arr[i];
        
        decl **decls = 0;
        lex_and_parse(str, null, &decls);
                
        if (buf_len(decls) > 0)
        {
            assert(buf_len(decls) == 1);
            buf_push(all_decls, decls[0]);

            if (print_ast)
            {
                printf_decl_ast(decls[0]);
            }

            error_counter += buf_len(errors);
            print_errors_to_console();
        }
        else
        {
            printf("\nResolve test error: couldn't parse test case %zu\n", i);
        }
    }

    buf_free(errors);
    // rezultaty są zapisywane do ordered_global_symbols
    resolve(all_decls, false);
    error_counter += buf_len(errors);
    
    if (print_ast)
    {
        printf("\nDeclarations in sorted order:\n");

        for (symbol **it = ordered_global_symbols; it != buf_end(ordered_global_symbols); it++)
        {
            symbol *sym = *it;
            if (sym->decl)
            {
                printf_decl_ast(sym->decl);
            }
            else
            {
                printf("\n%s\n", sym->name);
            }
        }
    }

    print_errors_to_console();

    if (error_counter > 0)
    {
        printf("\n\n");
        fatal("Resolving tests failed! Total number of errors: %d", error_counter);
    }
    else
    {
        printf("\nAll resolving tests passed!\n");
    }

    return ordered_global_symbols;
}

void resolve_test(void)
{
    printf("\n==== RESOLVING TEST ====\n");

    char *test_strs[] = {
        "let f := g[1 + 3]",
        "let g: int[10]",
        "let e = *b",
        "let d = b[0]",
        "let c: int[4]",
        "let b = &a[0]",
        "let y1: int = 1",
        "let i1 = n+m",
        "let z: Z",
        "const m = size_of_type(int)",
        "const n = size_of_type(float)",
        "struct Z { s: S, t: T *}",
        "struct T { i: int }",
        "let a: int[3] = {1, 2, 3}",
        "struct S { t: int, c: char }",
        "let x2 = fun(1, 2)",
        "let i2: int = 1",
        "fn fun2(vec: v2): int {\
            let local := fun(i2, 2)\
                { let scoped = 1 }\
            vec.x = local\
            return vec.x } ",
        "fn fun(i: int, j: int): int { j++ i++ return i + j }",
        "let x1 := {1,2} as v2",
        "let y2 = fuu(fuu(7, x1), {3, 4})",
        "fn fzz(x: int, y: int) : v2 { return {x + 1, y - 1} }",
        "fn fuu(x: int, v: v2) : int { return v.x + x } ",
        "fn ftest1(x: int): int { if (x) { return -x } else if (x % 2 == 0) { return x } else { return 0 } }",
        "fn ftest2(x: int): int { let p := 1 while (x) { p *= 2 x-- } return p }",
        "fn ftest3(x: int): int { let p := 1 do { p *= 2 x-- } while (x) return p }",
        "fn ftest4(x: int): int { for (let i := 0, i < x, i++) { if (i % 3 == 0) { return x } } return 0 }",
        "fn ftest5(x: int): int { switch(x) { case 0: case 1: { return 5 } case 3: default: { return -1 } } }",
        "fn return_value_test(arg: int):int{\
            if(arg>0){let i := return_value_test(arg - 2) return i } else { return 0 } }",
        "let ooo := o as bool && oo as bool || (o as uint & oo as uint) as bool",
        "let oo := o as int ",
        "let o := 1 as float",
        "let x3 = new v2",
        "let y3 = auto v2()",
        "fn deletetest1() { let x = new v2 x.x = 2 delete x }",
        "fn deletetest2() { let x = auto v2 x.x = 2 delete x }",
        "let x4 : node* = null",
        "fn ft(x: node*): bool { if (x == null) { return true } else { return false } }",
        "struct node { value: int, next: node *}",
        "let b1 : bool = true",
        "let b2 : bool = (b1 == false)",
        "let list1 := new int[]",
        "let list2 := new v2[]",
        "struct v2 { x: int, y: int }",
        "let printed_chars1 := printf(\"numbers: %d\", 1)",
        "let printed_chars2 := printf(\"numbers: %d, %d\", 1, 2)",
        "let printed_chars3 := printf(\"numbers: %d, %d, %d\", 1, 2, 3)",
        "extern fn printf(str: char*, variadic) : int",
        "fn test () { let s := new some_struct(1) }",
        "struct some_struct { member: int } ",
        "fn constructor () : some_struct { /* empty constructor */ }",
        "fn constructor (val: int) : some_struct* { let s := new some_struct s.member = val return s }",
    };
    size_t str_count = sizeof(test_strs) / sizeof(test_strs[0]);

    test_resolve_decls(test_strs, str_count, true);
}

void mangled_names_test(void)
{
    printf("\n==== NAME MANGLING TEST ====\n");
    buf_free(errors);

    // uwaga: reordering podczas resolve może zepsuć test
    char *test_strs[] = {
        "struct tee { i: int }",
        "union zet { s: tee, t: zet* }",
        "fn funkcja (x: int, y: int) { return } ",
        "fn funkcja ( t: tee, z: zet, i: int[16]*) { return }",
        "fn funkcja ( t: tee, z: zet*, i: int[16]*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[16]*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[16]*) : int { return 1 }",
        "fn funkcja ( t: zet, z: zet, i: int[17]*) { return }",
        "fn funkcja ( t: zet*, z: int[17]*, i: zet*) { return }",
        "fn funkcja ( t: tee*, z: zet, i: int[]) { return }",
        "fn (i: int) funkcja (o: int) { return }",
        "fn (s: int) funkcja () : int { return s }",
        "fn (x: int) funkcja () : { }",
    };

    char *cmp_strs[] = {
        "not tested",
        "not tested",
        "___funkcja___0i___0i___0v",
        "___funkcja___tee___zet___0p___0a_16___0i___0v",
        "___funkcja___tee___0p___zet___0p___0a_16___0i___0v",
        "___funkcja___0p___tee___zet___0p___0a_16___0i___0v",
        "___funkcja___0p___tee___zet___0p___0a_16___0i___0i",
        "___funkcja___zet___zet___0p___0a_17___0i___0v",
        "___funkcja___0p___zet___0p___0a_17___0i___0p___zet___0v",
        "___funkcja___0p___tee___zet___0l___0i___0v",
        "___0i___funkcja___0i___0v",
        "___0i___funkcja___0i",
        "___0i___funkcja___0v",
    };
    size_t cmp_strs_count = sizeof(test_strs) / sizeof(test_strs[0]);
    size_t test_strs_count = sizeof(cmp_strs) / sizeof(cmp_strs[0]);
    assert(cmp_strs_count == test_strs_count);

    size_t error_counter = 0;
    symbol **resolved = test_resolve_decls(test_strs, test_strs_count, false);
    for (size_t i = 2 /* dwa pierwsze pomijamy!*/; i < buf_len(resolved); i++)
    {
        symbol *sym = resolved[i];
        const char *mangled_name = get_function_mangled_name(sym->decl);
        if (0 != strcmp(mangled_name, cmp_strs[i]))
        {
            error_counter++;
            printf("\nError in name mangling, case %zu!\nExpected:\n%s\nGot:\n%s\n",
                i - 2, cmp_strs[i], mangled_name);
        }
    }

    print_errors_to_console();

    if (error_counter > 0)
    {
        printf("\n\n");
        fatal("Name mangling tests failed! Number of failed cases: %d", error_counter);
    }
    else
    {
        printf("\nAll name mangling tests passed!\n");
    }
}

#include "utils_tests.c"
//#include "gc_test.c"

void common_includes_test(void);
void fuzzy_test(void);

void run_all_tests(void)
{
    stretchy_buffers_test();
    intern_str_test();
    buf_copy_test();
    map_test();

    test_parsing();
    resolve_test();
    mangled_names_test();
    fuzzy_test();
    common_includes_test();

    //gc_init();
    //gc_test();
}

#include <time.h>

void fuzzy_test(void)
{
    char *test_file = "test/constructors.txt";
    string_ref file_buf = read_file_for_parsing(test_file);
    if (file_buf.str)
    {
        unsigned int seed = (unsigned int)time(null);
        srand(seed);

        // podmieniamy na losowe znaki
        size_t substitutions_count = file_buf.length / 30;
        for (; substitutions_count > 0; substitutions_count--)
        {
            size_t char_index = (size_t)(get_random_01() * (file_buf.length - 1));
            // chcemy kody ascii z przedziału 32-126
            char new_letter = (char)(get_random_01() * (126 - 32)) + 32;
            if (new_letter == '%')
            {
                new_letter = '_'; // żeby nie wchodziło w konflikty z printf
            }
            file_buf.str[char_index] = new_letter;
        }

        printf("Random seed: %d\n\n", seed);
        printf("Original text:\n\n");
        printf(file_buf.str);
        printf("\n\n");

        decl **decls = 0;
        lex_and_parse(file_buf.str, "fuzzy test", &decls);
        symbol **resolved = resolve(decls, false);

        // tutaj dodać printf S expressions?

        if (buf_len(errors) > 0)
        {
            print_errors_to_console();
        }
    }
}

void common_includes_test(void)
{
    ___list_hdr___ *int_list = ___list_initialize___(4, sizeof(int), 0);

    assert(int_list->length == 0);
    assert(int_list->capacity == 4);

    ___list_add___(int_list, 12, int);
    ___list_add___(int_list, 16, int);
    ___list_add___(int_list, 20, int);

    assert(((int *)int_list->buffer)[0] == 12);
    assert(((int *)int_list->buffer)[1] == 16);
    assert(((int *)int_list->buffer)[2] == 20);

    assert(___get_list_length___(int_list) == 3);
    assert(___get_list_capacity___(int_list) == 4);

    ___list_free___(int_list);

    assert(___get_list_length___(int_list) == 0);
    assert(___get_list_capacity___(int_list) == 0);

    ___list_hdr___ *token_list = ___list_initialize___(16, sizeof(token), 0);

    assert(token_list->length == 0);

    ___list_add___(token_list, ((token){.kind = TOKEN_NAME, .uint_val = 666 }), token);

    assert(token_list->length == 1);

    assert(((token *)(token_list->buffer))[0].kind == TOKEN_NAME);
    assert(((token *)(token_list->buffer))[0].uint_val == 666);

    ((token *)(token_list->buffer))[0].uint_val = 667;

    assert(((token *)(token_list->buffer))[0].uint_val == 667);

    ((token *)(token_list->buffer))[0] = (token){ .kind = TOKEN_ASSIGN, .uint_val = 668 };

    assert(((token *)(token_list->buffer))[0].kind == TOKEN_ASSIGN);
    assert(((token *)(token_list->buffer))[0].uint_val == 668);

    ___list_add___(token_list, ((token){.kind = TOKEN_MUL, .uint_val = 669 }), token);

    assert(((token *)(token_list->buffer))[1].kind == TOKEN_MUL);
    assert(((token *)(token_list->buffer))[1].uint_val == 669);

    ___list_remove_at___(token_list, sizeof(token), 1);

    assert(((token *)(token_list->buffer))[1].kind == 0);
    assert(((token *)(token_list->buffer))[1].uint_val == 0);

    ___list_free___(token_list);
}