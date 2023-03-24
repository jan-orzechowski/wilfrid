#pragma once

const char *struct_keyword;
const char *enum_keyword;
const char *union_keyword;
const char *let_keyword;
const char *fn_keyword;
const char *const_keyword;
const char *new_keyword;
const char *auto_keyword;
const char *delete_keyword;
const char *sizeof_keyword;
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

void init_keywords(void)
{
    static bool initialized = false;
    if (false == initialized)
    {
        struct_keyword = intern_keyword("struct");
        enum_keyword = intern_keyword("enum");
        union_keyword = intern_keyword("union");
        let_keyword = intern_keyword("let");
        fn_keyword = intern_keyword("fn");
        sizeof_keyword = intern_keyword("sizeof");
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
    }
    first_keyword = struct_keyword;
    last_keyword = extern_keyword;
    initialized = true;
}

bool is_name_keyword(const char *name)
{
    bool result = (name >= first_keyword && name <= last_keyword);
    return result;
}