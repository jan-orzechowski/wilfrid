const char *struct_keyword;
const char *enum_keyword;
const char *union_keyword;
const char *let_keyword;
const char *fn_keyword;
const char *const_keyword;
const char *new_keyword;
const char *auto_keyword;
const char *delete_keyword;
const char *size_of_keyword;
const char *size_of_type_keyword;
const char *as_keyword;
const char *null_keyword;
const char *true_keyword;
const char *false_keyword;
const char *break_keyword;
const char *continue_keyword;
const char *return_keyword;
const char *if_keyword;
const char *else_keyword;
const char *while_keyword;
const char *do_keyword;
const char *for_keyword;
const char *switch_keyword;
const char *case_keyword;
const char *default_keyword;
const char *variadic_keyword;
const char *extern_keyword;

const char *first_keyword;
const char *last_keyword;
const char **keywords_list;

const char *intern_keyword(const char *keyword)
{
    const char *result = str_intern(keyword);
    buf_push(keywords_list, result);
    return result;
}

bool keywords_initialized;
void init_keywords(void)
{
    if (false == keywords_initialized)
    {
        buf_free(keywords_list);
        struct_keyword = intern_keyword("struct");
        enum_keyword = intern_keyword("enum");
        union_keyword = intern_keyword("union");
        let_keyword = intern_keyword("let");
        fn_keyword = intern_keyword("fn");
        size_of_keyword = intern_keyword("size_of");
        size_of_type_keyword = intern_keyword("size_of_type");
        const_keyword = intern_keyword("const");
        new_keyword = intern_keyword("new");
        auto_keyword = intern_keyword("auto");
        delete_keyword = intern_keyword("delete");
        as_keyword = intern_keyword("as");
        null_keyword = intern_keyword("null");
        true_keyword = intern_keyword("true");
        false_keyword = intern_keyword("false");
        break_keyword = intern_keyword("break");
        continue_keyword = intern_keyword("continue");
        return_keyword = intern_keyword("return");
        if_keyword = intern_keyword("if");
        else_keyword = intern_keyword("else");
        while_keyword = intern_keyword("while");;
        do_keyword = intern_keyword("do");
        for_keyword = intern_keyword("for");
        switch_keyword = intern_keyword("switch");
        case_keyword = intern_keyword("case");
        default_keyword = intern_keyword("default");
        variadic_keyword = intern_keyword("variadic");
        extern_keyword = intern_keyword("extern");
    
        first_keyword = struct_keyword;
        last_keyword = extern_keyword;

        keywords_initialized = true;
    }    
}

bool is_name_keyword(const char *name)
{
    bool result = (name >= first_keyword && name <= last_keyword);
    return result;
}

const char *constructor_str;
const char *main_str;
const char *mangled_main_void_str;
const char *mangled_main_args_str;
const char *add_str;
const char *length_str;
const char *capacity_str;
const char *printf_str;
const char *allocate_str;
const char *assert_str;
const char *gc_str;
const char *query_gc_total_memory_str;
const char *query_gc_total_count_str;

bool constant_strings_initialized;
void init_constant_strings(void)
{
    if (false == constant_strings_initialized)
    {
        constructor_str = str_intern("constructor");
        main_str = str_intern("main");
        mangled_main_void_str = str_intern("___main");
        mangled_main_args_str = str_intern("___main___0l___0s");
        add_str = str_intern("add");
        length_str = str_intern("length");
        capacity_str = str_intern("capacity");
        printf_str = str_intern("printf");
        allocate_str = str_intern("allocate");
        assert_str = str_intern("assert");
        gc_str = str_intern("gc");
        query_gc_total_memory_str = str_intern("query_gc_total_memory");
        query_gc_total_count_str = str_intern("query_gc_total_count");
    
        constant_strings_initialized = true;
    }
}