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
        printf("\nAll parsing tests passed!\n");
    }
}

void single_case_parsing_test(void)
{
    char *test = "let x := xcalloc(new_capacity * sizeof(void*))";

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

symbol **resolve_test_decls(char **decl_arr, size_t decl_arr_count, bool print)
{
    arena = allocate_memory_arena(megabytes(50));
    init_before_resolve();

    buf_free(errors);

    if (print)
    {
        printf("original:\n\n");
    }

    for (size_t i = 0; i < decl_arr_count; i++)
    {
        char *str = decl_arr[i];
        decl **d = lex_and_parse(null, str);
        if (print)
        {
            printf("%s", get_decl_ast(*d));
            printf("\n");
        }

        push_symbol_from_decl(*d);
    }

    for (symbol **it = global_symbols_list;
        it != buf_end(global_symbols_list);
        it++)
    {
        symbol *sym = *it;
        complete_symbol(sym);
    }

    if (print)
    {
        printf("\nordered:\n\n");
        for (symbol **it = ordered_global_symbols; it != buf_end(ordered_global_symbols); it++)
        {
            symbol *sym = *it;
            if (sym->decl)
            {
                printf("%s", get_decl_ast(sym->decl));
            }
            else
            {
                printf("%s", sym->name);
            }
            printf("\n");
        }
    }

    print_errors_to_console();

    return ordered_global_symbols;
}

void resolve_test(void)
{
    char *test_strs[] = {
#if 1
        "let f := g[1 + 3]",
        "let g: int[10]",
        "let e = *b",
        "let d = b[0]",
        "let c: int[4]",
        "let b = &a[0]",
        "let x: int = y",
        "let y: int = 1",
        "let i = n+m",
        "let z: Z",
        "const m = sizeof(z.t)",
        "const n = sizeof(&a[0])",
        "struct Z { s: S, t: T *}",
        "struct T { i: int }",
        "let a: int[3] = {1, 2, 3}",
        "struct S { t: int, c: char }",
        "let x2 = fun(1, 2)",
        "let i: int = 1",
        "fn fun2(vec: v2): int {\
            let local = fun(i, 2)\
                { let scoped = 1 }\
            vec.x = local\
            return vec.x } ",
        "fn fun(i: int, j: int): int { j++ i++ return i + j }",
        "let x := (v2){1,2}",
        "let y = fuu(fuu(7, x), {3, 4})",
        "fn fzz(x: int, y: int) : v2 { return {x + 1, y - 1} }",
        "fn fuu(x: int, v: v2) : int { return v.x + x } ",
        "fn ftest1(x: int): int { if (x) { return -x } else if (x % 2 == 0) { return x } else { return 0 } }",
        "fn ftest2(x: int): int { let p := 1 while (x) { p *= 2 x-- } return p }",
        "fn ftest3(x: int): int { let p := 1 do { p *= 2 x-- } while (x) return p }",
        "fn ftest4(x: int): int { for (let i := 0, i < x, i++) { if (i % 3 == 0) { return x } } return 0 }",
        "fn ftest5(x: int): int { switch(x) { case 0: case 1: { return 5 } case 3: default: { return -1 } } }",
        "fn return_value_test(arg: int):int{\
            if(arg>0){let i := return_value_test(arg - 2) return i } else { return 0 } }}",
        "let ooo := (bool)o && (bool)oo || ((bool)o & (bool)oo)",
        "let oo := (int)o ",
        "let o := (float)1",
        "let x3 = new v2",
        "let y = auto v2()",
        "fn deletetest1() { let x = new v2 x.x = 2 delete x }",
        "fn deletetest2() { let x = auto v2 x.x = 2 delete x }",
        "let x4 : node *= null",
        "fn ft(x: node*): bool { if (x == null) { return true } else { return false } }",
        "struct node { value: int, next: node *}",
        "let b1 : bool = 1",
        "let b2 : bool = (b1 == false)",
        "let list1 := new int[]",
        "let list2 := new v2[]",
        "struct v2 { x: int, y: int }",
        "let printed_chars1 := printf(\"numbers: %d\", 1)",
        "let printed_chars2 := printf(\"numbers: %d, %d\", 1, 2)",
        "let printed_chars3 := printf(\"numbers: %d, %d, %d\", 1, 2, 3)",
        "extern fn printf(str: char*, variadic) : int",
#endif
        "fn test () { let s := new some_struct(1) }",
        "struct some_struct { member: int } ",
        "fn constructor () : some_struct { / *empty constructor */ }",
        "fn constructor (val: int) : some_struct *{ let s := new some_struct s.member = val return s }",
    };
    size_t str_count = sizeof(test_strs) / sizeof(test_strs[0]);

    debug_breakpoint;

    symbol **result = resolve_test_decls(test_strs, str_count, true);

    debug_breakpoint;

    buf_free(result);
}

void cgen_test(void)
{
    generate_line_hints = false;

    char *test_strs[] = {
#if 0
        "const x = 10",
        "let y := (vec3){1, 2, 3}",
        "fn power_2(n:int):int{return n*n}",
        "fn _main(){\
            let i : int = 5\
            let j = power_2(i)\
        }",
        "struct vec3 { x: int, y: int, z: int }",
        "const y = (float)12",
        "let u := (bool)(y > 11) && (y < 30)",
        "let i := (int)12 + 1",
#endif
        "fn f(){\
            let list := new int[]\
            list[10] = 12\
            delete list\
        }",

    };

    symbol **resolved = resolve_test_decls(test_strs, sizeof(test_strs) / sizeof(test_strs[0]), false);

    gen_printf_newline("// FORWARD DECLARATIONS\n");

    gen_forward_decls(resolved);

    gen_printf_newline("\n// DECLARATIONS\n");

    for (size_t i = 0; i < buf_len(resolved); i++)
    {
        gen_symbol_decl(resolved[i]);
    }

    debug_breakpoint;

    printf("%s\n", gen_buf);

    debug_breakpoint;
}

void mangled_names_test()
{
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

    assert((sizeof(test_strs) / sizeof(test_strs[0])) == sizeof(cmp_strs) / sizeof(cmp_strs[0]))

        symbol **resolved = resolve_test_decls(test_strs, sizeof(test_strs) / sizeof(test_strs[0]), false);
    for (size_t i = 2 /* dwa pierwsze pomijamy!*/; i < buf_len(resolved); i++)
    {
        symbol *sym = resolved[i];
        const char *mangled_name = get_function_mangled_name(sym->decl);
        if (0 != strcmp(mangled_name, cmp_strs[i]))
        {
            // błąd!
            debug_breakpoint;
        }
    }
    debug_breakpoint;
}