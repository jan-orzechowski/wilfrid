char mangle_buf[kilobytes(2)];
int64_t mangle_buf_cap = kilobytes(2);
int64_t mangle_buf_len = 0;

#define mangle_clear() { memset(mangle_buf, 0, mangle_buf_cap); mangle_buf_len = 0; }

#define mangle_print(...) { \
    if (mangle_buf_len > mangle_buf_cap - 1) \
    { \
        fatal("Mangled name is over 2kb"); \
    } \
    else \
    { \
        int32_t count = stbsp_snprintf(mangle_buf + mangle_buf_len, mangle_buf_cap - mangle_buf_len, __VA_ARGS__);  \
        mangle_buf_len += count; \
        assert(mangle_buf_len < mangle_buf_cap); \
    } \
}

void print_mangled_typespec_to_buffer(typespec *typ)
{
    assert(typ);    
    mangle_print("___");
    switch (typ->kind)
    {
        case TYPESPEC_NAME:
        {
            if (0 == strcmp(typ->name, "int"))
            {
                mangle_print("0i");
            }
            else if (0 == strcmp(typ->name, "uint"))
            {
                mangle_print("0ui");
            }
            else if (0 == strcmp(typ->name, "long"))
            {
                mangle_print("0lo");
            }
            else if (0 == strcmp(typ->name, "ulong"))
            {
                mangle_print("0ul");
            }
            else if (0 == strcmp(typ->name, "char"))
            {
                mangle_print("0c");
            }
            else if (0 == strcmp(typ->name, "string"))
            {
                mangle_print("0s");
            }
            else if (0 == strcmp(typ->name, "float"))
            {
                mangle_print("0f");
            }
            else if (0 == strcmp(typ->name, "bool"))
            {
                mangle_print("0b");
            }
            else if (0 == strcmp(typ->name, "null"))
            {
                mangle_print("0n");
            }
            else if (0 == strcmp(typ->name, "void"))
            {
                mangle_print("0v");
            }
            else
            {
                mangle_print(typ->name);
            }
        }
        break;
        case TYPESPEC_ARRAY:
        {
            // ___0a_16___type
            mangle_print("0a");

            resolved_expr *e = resolve_expr(typ->array.size_expr);
            size_t arr_count = e->val;

            mangle_print(xprintf("_%zu", arr_count));
            print_mangled_typespec_to_buffer(typ->array.base_type);
        }
        break;
        case TYPESPEC_LIST:
        {
            // ___0l___type
            mangle_print("0l");
            print_mangled_typespec_to_buffer(typ->list.base_type);
        }
        break;
        case TYPESPEC_POINTER:
        {
            mangle_print("0p");
            print_mangled_typespec_to_buffer(typ->pointer.base_type);
        }
        break;
        case TYPESPEC_NONE:
        invalid_default_case;
    }
}

const char *get_function_mangled_name(decl *dec)
{
    // ___function_name___arg_type1___arg_type2
    assert(dec);
    assert(dec->kind == DECL_FUNCTION);

    if (dec->function.is_extern)
    {
        return dec->name;
    }

    mangle_clear();

    if (dec->function.method_receiver)
    {
        typespec *t = dec->function.method_receiver->type;
        print_mangled_typespec_to_buffer(t);
    }

    mangle_print("___");
    mangle_print(dec->name);
    for (size_t i = 0; i < dec->function.params.param_count; i++)
    {
        typespec *t = dec->function.params.params[i].type;
        if (t == null)
        {
            // w tym wypadku mamy błąd w resolve - kod nie zostanie wygenerowany
            // więc możemy nie generować mangled name
            return null;
        }
        print_mangled_typespec_to_buffer(t);
    }
   
    // return type odróżnia tylko constructory
    if (dec->name == str_intern("constructor") && dec->function.return_type)
    {
        print_mangled_typespec_to_buffer(dec->function.return_type);
    }
    
    const char *result = str_intern(mangle_buf);
    return result;
}

const char *pretty_print_type_name(type *ty, bool plural)
{
    const char *result = null;
    assert(ty);
    switch (ty->kind)
    {
        case TYPE_NONE:
        {
            result = "unresolved";
        }
        break;
        case TYPE_VOID:
        {
            result = "void";
        }
        break;
        case TYPE_INT:
        {
            result = plural ? "integers" : "integer";
        }
        break;
        case TYPE_UINT:
        {
            result = plural ? "unsigned integers" : "unsigned integer";
        }
        break;
        case TYPE_LONG:
        {
            result = plural ? "long integers" : "long integer";
        }
        break;
        case TYPE_ULONG:
        {
            result = plural ? "unsigned long integers" : "unsigned long integer";
        }
        break;
        case TYPE_CHAR:
        {
            result = plural ? "characters" : "character";
        }
        break;
        case TYPE_FLOAT:
        {
            result = plural ? "floating point numbers" : "floating point number";
        }
        break;
        case TYPE_BOOL:
        {
            result = plural ? "booleans" : "boolean";
        }
        break;;
        case TYPE_NULL:
        {
            result = plural ? "null values" : "null value";
        }
        break;
        case TYPE_STRUCT:
        {
            assert(ty->symbol);
            result = plural
                ? xprintf("'%s' structs", ty->symbol->name)
                : xprintf("'%s' struct", ty->symbol->name);
        }
        break;
        case TYPE_UNION:
        {
            assert(ty->symbol);
            result = plural
                ? xprintf("'%s' unions", ty->symbol->name)
                : xprintf("'%s' union", ty->symbol->name);
        }
        break;
        case TYPE_ARRAY:
        {
            assert(ty->array.base_type);
            result = plural
                ? xprintf("arrays of %s", pretty_print_type_name(ty->array.base_type, true))
                : xprintf("array of %s", pretty_print_type_name(ty->array.base_type, true));
        }
        break;
        case TYPE_LIST:
        {
            assert(ty->list.base_type);
            result = plural
                ? xprintf("lists of %s", pretty_print_type_name(ty->list.base_type, true))
                : xprintf("list of %s", pretty_print_type_name(ty->list.base_type, true));
        }
        break;
        case TYPE_POINTER:
        {
            assert(ty->pointer.base_type);
            result = plural
                ? xprintf("pointers to %s", pretty_print_type_name(ty->pointer.base_type, false))
                : xprintf("pointer to %s", pretty_print_type_name(ty->pointer.base_type, false));
        }
        break;
        case TYPE_ENUM:
        {
            assert(ty->symbol);
            result = xprintf("enumeration %s", ty->symbol->name);
        }
        break;
        case TYPE_FUNCTION:
        {
            assert(ty->symbol);
            result = xprintf("function %s", ty->symbol->name);
        }
        break;
        case TYPE_INCOMPLETE:
        default:
        {
            fatal("this shouldn't happen");
        }
        break;
    }
    return result;
}