char *get_typespec_mangled_name(typespec *typ)
{
    assert(typ);
    char *result = null;
    buf_printf(result, "___");
    switch (typ->kind)
    {
        case TYPESPEC_NAME:
        {
            if (0 == strcmp(typ->name, "int"))
            {
                buf_printf(result, "0i");
            }
            else if (0 == strcmp(typ->name, "uint"))
            {
                buf_printf(result, "0ui");
            }
            else if (0 == strcmp(typ->name, "long"))
            {
                buf_printf(result, "0lo");
            }
            else if (0 == strcmp(typ->name, "ulong"))
            {
                buf_printf(result, "0ul");
            }
            else if (0 == strcmp(typ->name, "char"))
            {
                buf_printf(result, "0c");
            }
            else if (0 == strcmp(typ->name, "string"))
            {
                buf_printf(result, "0s");
            }
            else if (0 == strcmp(typ->name, "float"))
            {
                buf_printf(result, "0f");
            }
            else if (0 == strcmp(typ->name, "bool"))
            {
                buf_printf(result, "0b");
            }
            else if (0 == strcmp(typ->name, "null"))
            {
                buf_printf(result, "0n");
            }
            else if (0 == strcmp(typ->name, "void"))
            {
                buf_printf(result, "0v");
            }
            else
            {
                buf_printf(result, typ->name);
            }
        }
        break;
        case TYPESPEC_ARRAY:
        {
            // ___0a_16___type
            buf_printf(result, "0a");

            resolved_expr *e = resolve_expr(typ->array.size_expr);
            size_t arr_count = e->val;

            buf_printf(result, xprintf("_%zu", arr_count));
            char *mangled = get_typespec_mangled_name(typ->array.base_type);
            buf_printf(result, mangled);
            buf_free(mangled);
        }
        break;
        case TYPESPEC_LIST:
        {
            // ___0l___type
            buf_printf(result, "0l");
            char *mangled = get_typespec_mangled_name(typ->list.base_type);
            buf_printf(result, mangled);
            buf_free(mangled);
        }
        break;
        case TYPESPEC_POINTER:
        {
            buf_printf(result, "0p");
            char *mangled = get_typespec_mangled_name(typ->pointer.base_type);
            buf_printf(result, mangled);
            buf_free(mangled);
        }
        break;
        case TYPESPEC_NONE:
        invalid_default_case;
    }
    return result;
}

const char *get_function_mangled_name(decl *dec)
{
    // ___function_name___arg_type1___arg_type2___ret_type

    assert(dec);
    assert(dec->kind == DECL_FUNCTION);

    if (dec->function.is_extern)
    {
        return dec->name;
    }

    char *mangled = null;

    if (dec->function.method_receiver)
    {
        typespec *t = dec->function.method_receiver->type;
        char *mangled_rec = get_typespec_mangled_name(t);
        buf_printf(mangled, mangled_rec);
        buf_free(mangled_rec);
    }

    buf_printf(mangled, "___");
    buf_printf(mangled, dec->name);
    for (size_t i = 0; i < dec->function.params.param_count; i++)
    {
        typespec *t = dec->function.params.params[i].type;
        if (t == null)
        {
            // w tym wypadku mamy błąd w resolve - kod nie zostanie wygenerowany
            // więc możemy nie generować mangled name
            return null;
        }
        char *mangled_arg = get_typespec_mangled_name(t);
        buf_printf(mangled, mangled_arg);
        buf_free(mangled_arg);
    }

    if (dec->function.return_type)
    {
        char *mangled_ret = get_typespec_mangled_name(dec->function.return_type);
        buf_printf(mangled, mangled_ret);
        buf_free(mangled_ret);
    }
    else
    {
        buf_printf(mangled, "___0v");
    }

    const char *result = str_intern(mangled);
    buf_free(mangled);
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