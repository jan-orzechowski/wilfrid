void _vm_debug_printf(source_pos pos, char *format, ...)
{
    char *temp_buffer = null;
    print_source_pos(&temp_buffer, pos);
    printf("%s", temp_buffer);
    buf_free(temp_buffer);
    printf(": ");    
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

#if DEBUG_BUILD
hashmap debug_names_dict;
#define save_debug_string(str) map_put(&debug_strings, str, str);
#define debug_vm_print(...) _vm_debug_printf(__VA_ARGS__)
#define debug_print_vm_value(val) _debug_print_vm_value(val)
#elif
#define save_debug_string
#define debug_vm_print
#define debug_print_vm_value
#endif

typedef struct vm_value vm_value;
struct vm_value
{
    const char *name;
    type *type;
    union value
    {
        int32_t int_value;
        uint32_t uint_value;
        int64_t long_value;
        uint64_t ulong_value;
        float float_value;
        void *ptr_value;
    };
};

void copy_vm_val(vm_value *dest, vm_value *val)
{
    assert(compare_types(dest->type, val->type));
    dest->ulong_value = val->ulong_value;
}

char *_debug_print_vm_value(vm_value *val)
{
    // tylko w celach debugowych - nigdy nie dealokujemy
    // można ew. wrzucić na listę dynamiczną i posprzątać po jakimś czasie
    size_t buffer_size = 20;
    char *result = xcalloc(buffer_size * sizeof(char));
    switch (val->type->kind)
    {
        case TYPE_INT:
        {
            snprintf(result, buffer_size, "%i", val->int_value);
        }
        break;
        case TYPE_LONG:
        {
            snprintf(result, buffer_size, "%lld", val->long_value);
        }
        break;
        case TYPE_BOOL:
        case TYPE_NULL:
        case TYPE_CHAR:
        case TYPE_UINT:
        {
            snprintf(result, buffer_size, "%d", val->uint_value);
        }
        break;
        case TYPE_ULONG:
        {
            snprintf(result, buffer_size, "%lld", val->ulong_value);
        }
        break;
        case TYPE_POINTER:
        {
            snprintf(result, buffer_size, "%p", val->ptr_value);
        }
        break;
        case TYPE_FLOAT:
        {
            snprintf(result, buffer_size, "%ff", val->float_value);
        }
        break;
        
        case TYPE_VOID:
        {
            snprintf(result, buffer_size, "VOID");
        }
        break;

        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ARRAY:
        case TYPE_LIST:
        case TYPE_FUNCTION:

        default:
        {
            fatal("unimplemented");
        }
        break;
    }
    return result;
}

void eval_function(symbol *function_sym, vm_value *ret_value);

memory_arena *vm_global_memory;
hashmap global_identifiers;

/*
    do przemyślenia... jak to przechowywać? czy alokować tu jeszcze vm_value osobno?
*/
vm_value *define_global_identifier(const char *name, uintptr_t init_value) // czy podawać typ?
{
    // to będzie trafione jeśli dwa razy występuje taki sam const string w kodzie źródłowym...
    assert(0 == map_get(&global_identifiers, (void *)name));

    vm_value *result = (vm_value *)push_size(vm_global_memory, sizeof(vm_value));
    result->ulong_value = init_value;
    map_put(&global_identifiers, (void *)name, (void *)result);

#if DEBUG_BUILD
    map_put(&debug_names_dict, result, (void *)name);
#endif

    return result;
}

enum
{
    MAX_VM_STACK_LENGTH = 2048
};

vm_value vm_stack[MAX_VM_STACK_LENGTH];
vm_value *last_vm_stack_value = vm_stack;

vm_value *enter_vm_stack_scope(void)
{
    vm_value *marker = last_vm_stack_value;
    return marker;
}

void leave_vm_stack_scope(vm_value *marker)
{
    assert(marker >= vm_stack);
    assert(marker <= last_vm_stack_value);
    for (vm_value *val_to_clean = marker + 1; // samego markera nie czyścimy
        val_to_clean <= last_vm_stack_value;
        val_to_clean++)
    {
        *val_to_clean = (vm_value){0};
    }
    last_vm_stack_value = marker;
}

vm_value *push_identifier_on_stack(const char *name, type *type)
{
    if (name) 
    { 
        assert_is_interned(name); 
    };

    vm_value *result = null;
    int64_t stack_size = last_vm_stack_value - vm_stack;
    if (stack_size + 1 < MAX_VM_STACK_LENGTH)
    {        
        last_vm_stack_value++;
        last_vm_stack_value->name = name;
        last_vm_stack_value->type = type;
        result = last_vm_stack_value;
    }
    else
    {
        // tutaj powinniśmy rzucić błędem i przerwać program
        fatal("stack overflow");
    }
    return result;
}

// tutaj trzeba będzie jeszcze uwzględnić zmienne globalne
vm_value *get_vm_variable(const char *name)
{
    assert(name);
    assert_is_interned(name); 

    if (vm_stack != last_vm_stack_value)
    {
        // pierwsza wartość to zawsze null ze względu na to jak działa define_identifier_on_stack
        // ze względu na to, że w funkcji nazwa parametru może być taka sama 
        // jak w zewnętrznym scope w którym została wezana
        // - trzeba iść od tyłu
        for (vm_value *var = last_vm_stack_value;
            var > vm_stack;
            var--)
        {
            assert(var->type);
            if (var->name == name)
            {
                return var;
            }
        }
    }

    fatal("no variable with name '%s' on the stack", name);
    return null;
}

void eval_unary_op(vm_value *dest, token_kind operation, vm_value *operand)
{
    if (operand->type == type_int)
    {
        dest->int_value = (int32_t)eval_long_unary_op(operation, operand->int_value);
    }
    else if (operand->type == type_uint
        || operand->type == type_bool
        || operand->type == type_char)
    {
        dest->uint_value = (uint32_t)eval_ulong_unary_op(operation, operand->uint_value);
    }
    else if (operand->type == type_long)
    {
        dest->long_value = eval_long_unary_op(operation, operand->long_value);
    }
    else if (operand->type == type_ulong)
    {
        dest->ulong_value = eval_ulong_unary_op(operation, operand->ulong_value);
    }
    else if (operand->type == type_float)
    {
        dest->float_value = eval_float_unary_op(operation, operand->float_value);
    }
    else
    {
        fatal("unsupported type");
    }
}

void eval_binary_op(vm_value *dest, token_kind op, vm_value *left, vm_value *right)
{
    assert(compare_types(left->type, right->type));
    if (left->type == type_int)
    {
        dest->int_value = (int32_t)eval_long_binary_op(op, left->int_value, right->int_value);
    }
    else if (left->type == type_uint 
        || left->type == type_bool 
        || left->type == type_char)
    {
        dest->uint_value = (uint32_t)eval_ulong_binary_op(op, left->uint_value, right->uint_value);
    }
    else if (left->type == type_long)
    {
        dest->long_value = eval_long_binary_op(op, left->long_value, right->long_value);
    }
    else if (left->type == type_ulong)
    {
        dest->ulong_value = eval_ulong_binary_op(op, left->ulong_value, right->ulong_value);
    }
    else if (left->type == type_float)
    {
        dest->float_value = eval_float_binary_op(op, left->float_value, right->float_value);
    }
    else
    {
        fatal("unsupported type");
    }
}

void perform_cast(vm_value *new_val, type_kind new_type, vm_value *old_val)
{
    assert(new_val);
    assert(new_type);
    assert(old_val);

    switch (new_type)
    {
        case TYPE_INCOMPLETE:
        case TYPE_COMPLETING:
        case TYPE_NONE:
        {
            fatal("error");
        }
        break;

        case TYPE_INT:
        {
            if (old_val->type == type_int)
            {
                new_val->int_value = (int32_t)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->int_value = (int32_t)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->int_value = (int32_t)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->int_value = (int32_t)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->int_value = (int32_t)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->int_value = (int32_t)old_val->ptr_value;
            }
        } 
        break;
        case TYPE_LONG:
        {
            if (old_val->type == type_int)
            {
                new_val->long_value = (int64_t)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->long_value = (int64_t)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->long_value = (int64_t)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->long_value = (int64_t)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->long_value = (int64_t)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->long_value = (int64_t)old_val->ptr_value;
            }
        }
        break;
        case TYPE_CHAR:
        case TYPE_BOOL:
        case TYPE_UINT:
        {
            if (old_val->type == type_int)
            {
                new_val->uint_value = (uint32_t)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->uint_value = (uint32_t)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->uint_value = (uint32_t)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->uint_value = (uint32_t)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->uint_value = (uint32_t)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->uint_value = (uint32_t)old_val->ptr_value;
            }
        }
        break;
        case TYPE_ULONG:
        {
            if (old_val->type == type_int)
            {
                new_val->ulong_value = (uint64_t)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->ulong_value = (uint64_t)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->ulong_value = (uint64_t)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->ulong_value = (uint64_t)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->ulong_value = (uint64_t)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->ulong_value = (uint64_t)old_val->ptr_value;
            }
        }
        break;
        case TYPE_FLOAT:
        {
            if (old_val->type == type_int)
            {
                new_val->float_value = (float)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->float_value = (float)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->float_value = (float)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->float_value = (float)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->float_value = (float)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->float_value = (float)(uintptr_t)old_val->ptr_value;
            }
        }
        break;
        case TYPE_POINTER:
        {
            if (old_val->type == type_int)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->int_value;
            }
            else if (old_val->type == type_uint)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->uint_value;
            }
            else if (old_val->type == type_long)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->long_value;
            }
            else if (old_val->type == type_ulong)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->ulong_value;
            }
            else if (old_val->type == type_float)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->float_value;
            }
            else if (old_val->type->kind == TYPE_POINTER)
            {
                new_val->ptr_value = (void *)(uintptr_t)old_val->ptr_value;
            }
        } 
        break; 

        case TYPE_NULL: 
        case TYPE_STRUCT:
        case TYPE_UNION: 
        case TYPE_ARRAY: 
        case TYPE_LIST: 
        case TYPE_FUNCTION: 
        
        default:
        {
            fatal("unimplemented");
        }
        break;
    }
}

vm_value *eval_expression(expr *exp)
{
    assert(exp);
    assert(exp->resolved_type);
    
    vm_value *result = push_identifier_on_stack(null, exp->resolved_type);

    switch (exp->kind)
    {
        case EXPR_INT:
        {    
            // działa niezależnie od konkretnego typu
            result->ulong_value = exp->integer_value;
        }
        break;
        case EXPR_FLOAT:
        {
            // czy to jest dobrze?
            result->float_value = exp->float_value;
        }
        break;
        case EXPR_CHAR:
        case EXPR_STRING:
        {
            result->ptr_value = exp->string_value;
        }
        break;
        case EXPR_NULL:
        {
            result->ptr_value = 0;
        }
        break;
        case EXPR_BOOL:
        {
            if (exp->bool_value)
            {
                result->ulong_value = 1;
            }
            else
            {
                result->ulong_value = 0;
            }
        }
        break;

        case EXPR_NAME:
        {
            assert_is_interned(exp->name);
            result = get_vm_variable(exp->name);
            assert(result);

            debug_vm_print(exp->pos, "read var '%s' from stack, value %s", 
                result->name, debug_print_vm_value(result));
        }
        break;
        case EXPR_UNARY:
        {
            vm_value *operand = eval_expression(exp->unary.operand);

            if (exp->unary.operator == TOKEN_MUL) // pointer dereference
            {
                assert(operand->type->kind == TYPE_POINTER);          
                assert(exp->resolved_type == operand->type->pointer.base_type);
                
                result = ((vm_value *)operand->ptr_value);

                debug_breakpoint;
            }
            else if (exp->unary.operator == TOKEN_BITWISE_AND) // address of
            {
                result->ptr_value = operand; // wskaźnik do całego vm_value - trzeba to będzie poprawić
                
                debug_breakpoint;
            }
            else if (exp->unary.operator == TOKEN_INC || exp->unary.operator == TOKEN_DEC)
            {
                // te operatory zmieniają wartość, do której się odnosiły
                eval_unary_op(operand, exp->unary.operator, operand);
                copy_vm_val(result, operand);
            }
            else
            {
                eval_unary_op(result, exp->unary.operator, operand);
            }

            debug_vm_print(exp->pos, "operation %s, result %s", 
                get_token_kind_name(exp->unary.operator), debug_print_vm_value(result));
        }
        break;
        case EXPR_BINARY:
        {
            vm_value *left = eval_expression(exp->binary.left);
            vm_value *right = eval_expression(exp->binary.right);
            
            eval_binary_op(result, exp->binary.operator, left, right);

            debug_vm_print(exp->pos, "operation %s, result %s", 
                get_token_kind_name(exp->binary.operator), debug_print_vm_value(result));
        }
        break;
        case EXPR_TERNARY:
        {
            vm_value *val = eval_expression(exp->ternary.condition);
            if (val->uint_value != 0)
            {
                result = eval_expression(exp->ternary.if_true);
            }
            else
            {
                result = eval_expression(exp->ternary.if_false);
            }
        }
        break;
        case EXPR_CALL:
        {
            // specjalny przypadek na razie
            if (exp->call.resolved_function->name == str_intern("printf"))
            {
                assert(exp->call.args_num >= 2);
                char *format = exp->call.args[0]->string_value;

                vm_value *val = eval_expression(exp->call.args[1]);
                debug_vm_print(exp->pos, format, val->ulong_value);
            }
            else
            {
                assert(exp->call.resolved_function);
                assert(exp->call.resolved_function->type);

                debug_vm_print(exp->pos, "FUNCTION CALL - %s - enter", exp->call.resolved_function->name);

                vm_value **arg_vals = null;
                for (size_t i = 0; i < exp->call.args_num; i++)
                {
                    expr *arg_expr = exp->call.args[i];
                    vm_value *arg_val = eval_expression(arg_expr);                    
                    buf_push(arg_vals, arg_val);
                }

                assert(buf_len(arg_vals) == exp->call.args_num);
                assert(exp->call.args_num == exp->call.resolved_function->type->function.param_count);

                vm_value *ret_val = push_identifier_on_stack(null, exp->resolved_type);

                vm_value *marker = enter_vm_stack_scope();
                {
                    for (size_t i = 0; i < buf_len(arg_vals); i++)
                    {
                        const char *name = exp->call.resolved_function->decl->function.params.params[i].name;
                        type *t = exp->call.resolved_function->type->function.param_types[i];
                        
                        vm_value *arg_val = push_identifier_on_stack(name, t);
                        copy_vm_val(arg_val, arg_vals[i]);
                    }

                    eval_function(exp->call.resolved_function, ret_val);
                }
                leave_vm_stack_scope(marker);
                buf_free(arg_vals);

                debug_vm_print(exp->pos, "FUNCTION CALL - %s - exit", exp->call.resolved_function->name);
            }
        }
        break;
        case EXPR_FIELD:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_INDEX:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_NEW:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_AUTO:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_SIZEOF:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_CAST:
        {
            vm_value *old_val = eval_expression(exp->cast.expr);
            perform_cast(result, exp->cast.resolved_type->kind, old_val);
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            fatal("COMPOUND LITERALS NOT SUPPORTED");
        }
        break;
        case EXPR_STUB:
        {
            fatal("unimplemented");
        }
        break;

        case EXPR_NONE:
        invalid_default_case;
    }
    return result;
}

bool break_loop = false;
bool continue_loop = false;
bool return_func = false;

void eval_statement(stmt *st, vm_value *opt_ret_value);

void eval_statement_block(stmt_block block, vm_value *opt_ret_value)
{
    if (block.stmts_count > 0)
    {
        debug_vm_print(block.stmts[0]->pos, "BLOCK SCOPE - start");
        
        vm_value *marker = enter_vm_stack_scope();
        for (size_t i = 0; i < block.stmts_count; i++)
        {
            eval_statement(block.stmts[i], opt_ret_value);

            if (continue_loop || break_loop || return_func)
            {
                debug_vm_print(block.stmts[i]->pos, "BLOCK SCOPE - continue/break/return");
                break;
            }
        }
        leave_vm_stack_scope(marker);
        
        debug_vm_print(block.stmts[block.stmts_count - 1]->pos, "BLOCK SCOPE - end");
    }
}

void eval_statement(stmt *st, vm_value *opt_ret_value)
{
    assert(st);
    switch (st->kind)
    {
        case STMT_NONE:
        {
            fatal("codegen: none is not supported statement kind");
        }
        break;
        case STMT_RETURN:
        {
            assert(opt_ret_value);
            vm_value *val = eval_expression(st->return_stmt.ret_expr);
            copy_vm_val(opt_ret_value, val);
            return_func = true;
            return;
        }
        break;
        case STMT_BREAK:
        {
            break_loop = true;
            return;
        }
        break;
        case STMT_CONTINUE:
        {
            continue_loop = true;
        }
        break;
        case STMT_DECL:
        {
            assert(st->decl_stmt.decl);
            assert(st->decl_stmt.decl->type);
            decl *dec = st->decl_stmt.decl;

            if (dec->kind == DECL_VARIABLE)
            {
                assert(dec->type);
                //size_t size = get_type_size(dec->type);

                vm_value *new_value = eval_expression(dec->variable.expr);
                assert(new_value);

                if (new_value->type->kind == TYPE_VOID)
                {
                    debug_breakpoint;
                    new_value = eval_expression(dec->variable.expr);
                }

                assert(compare_types(new_value->type, dec->type));
                new_value->name = dec->name;

                // na razie bez structów na stacku
                assert(new_value->type->kind != TYPE_STRUCT);
                assert(new_value->type->kind != TYPE_UNION);
                                
                debug_vm_print(dec->pos, "declaration of %s, init value %s", 
                    dec->name, debug_print_vm_value(new_value));
            }
            else
            {
                fatal("shouldn't be possible");
            }
        }
        break;
        case STMT_IF_ELSE:
        {
            vm_value *branch = eval_expression(st->if_else.cond_expr);
            debug_vm_print(st->pos, "IF - condition evaluated as: %s", debug_print_vm_value(branch));
            if (branch && branch->ulong_value)
            {
                debug_vm_print(st->pos, "IF - then block start");
                eval_statement_block(st->if_else.then_block, opt_ret_value);
                debug_vm_print(st->pos, "IF - then block end");
            }
            else
            {
                if (st->if_else.else_stmt)
                {
                    debug_vm_print(st->pos, "IF - else stmt start");
                    eval_statement(st->if_else.else_stmt, opt_ret_value);
                    debug_vm_print(st->pos, "IF - else stmt end");
                }
            }
        }
        break;
        case STMT_WHILE:
        {
            debug_vm_print(st->pos, "WHILE - start");

            vm_value *marker = enter_vm_stack_scope();
            vm_value *loop = eval_expression(st->while_stmt.cond_expr);
            debug_vm_print(st->pos, "WHILE - condition evaluated as: %s", debug_print_vm_value(loop));
            while (loop && loop->ulong_value)
            {
                eval_statement_block(st->while_stmt.stmts, opt_ret_value);

                if (return_func)
                {
                    debug_vm_print(st->pos, "WHILE - return");
                    break;
                }

                if (continue_loop)
                {
                    debug_vm_print(st->pos, "WHILE - continue");
                    continue_loop = false;
                }

                if (break_loop)
                {
                    debug_vm_print(st->pos, "WHILE - break");
                    break_loop = false;
                    break;
                }

                loop = eval_expression(st->while_stmt.cond_expr);

                debug_vm_print(st->pos, "WHILE - condition evaluated as: %s", debug_print_vm_value(loop));
            }
            leave_vm_stack_scope(marker);

            debug_vm_print(st->pos, "WHILE - end");
        }
        break;
        case STMT_DO_WHILE:
        {
            debug_vm_print(st->pos, "DO WHILE - start");

            vm_value *marker = enter_vm_stack_scope();
            vm_value *loop = null;
            do
            {                
                eval_statement_block(st->do_while_stmt.stmts, opt_ret_value);

                if (return_func)
                {
                    debug_vm_print(st->pos, "DO WHILE - return");
                    break;
                }

                if (continue_loop)
                {
                    debug_vm_print(st->pos, "DO WHILE - continue");
                    continue_loop = false;
                }

                if (break_loop)
                {
                    debug_vm_print(st->pos, "DO WHILE - break");
                    break_loop = false;
                    break;
                }

                loop = eval_expression(st->do_while_stmt.cond_expr);

                debug_vm_print(st->pos, "DO WHILE - condition evaluated as: %s", debug_print_vm_value(loop));
            }
            while (loop && loop->ulong_value);
            leave_vm_stack_scope(marker);

            debug_vm_print(st->pos, "DO WHILE - end");
        }
        break;
        case STMT_FOR:
        {
            debug_vm_print(st->pos, "FOR - start");

            eval_statement(st->for_stmt.init_stmt, null);

            vm_value *marker = enter_vm_stack_scope();
            vm_value *loop = eval_expression(st->for_stmt.cond_expr);
            debug_vm_print(st->pos, "FOR - condition evaluated as: %s", debug_print_vm_value(loop));
            while (loop && loop->ulong_value)
            {
                eval_statement_block(st->for_stmt.stmts, opt_ret_value);

                if (return_func)
                {
                    debug_vm_print(st->pos, "WHILE - return");
                    break;
                }

                if (continue_loop)
                {
                    debug_vm_print(st->pos, "FOR - continue");
                    continue_loop = false;
                }

                if (break_loop)
                {
                    debug_vm_print(st->pos, "FOR - break");
                    break_loop = false;
                    break;
                }

                eval_statement(st->for_stmt.next_stmt, null);
                loop = eval_expression(st->for_stmt.cond_expr);

                debug_vm_print(st->pos, "FOR - condition evaluated as: %s", debug_print_vm_value(loop));
            }
            leave_vm_stack_scope(marker);          

            debug_vm_print(st->pos, "FOR - end");
        }
        break;
        case STMT_ASSIGN:
        {   
            assert(is_assign_operation(st->assign.operation));
            
            vm_value *new_val = eval_expression(st->assign.value_expr);
            vm_value *old_val = eval_expression(st->assign.assigned_var_expr);
            
            if (st->assign.operation != TOKEN_ASSIGN)
            {
                token_kind op = get_assignment_operation_token(st->assign.operation);
                eval_binary_op(old_val, op, old_val, new_val);

                debug_vm_print(st->assign.assigned_var_expr->pos, "operation %s for assignment, result %s",
                    get_token_kind_name(op), debug_print_vm_value(old_val));
            }

            copy_vm_val(old_val, new_val);

            debug_vm_print(st->assign.assigned_var_expr->pos, "copied value %d to variable %s", 
                debug_print_vm_value(new_val), debug_print_vm_value(old_val));
        }
        break;
        case STMT_SWITCH:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_EXPR:
        {
            vm_value *result = eval_expression(st->expr);
            debug_vm_print(st->expr->pos, "expression as statement, result %s",
                debug_print_vm_value(result));
        }
        break;
        case STMT_BLOCK:
        {
            eval_statement_block(st->block, opt_ret_value);
        }
        break;
        case STMT_DELETE:
        {
            fatal("unimplemented");
        }
        break;
        invalid_default_case;
    }
}

void eval_global_declarations(symbol **syms)
{
    for (size_t i = 0; i < buf_len(syms); i++)
    {
        symbol *sym = syms[i];
        switch (sym->kind)
        {
            case SYMBOL_VARIABLE:
            case SYMBOL_CONST:
            {
                //size_t size = get_type_size(sym->type);
                vm_value *global_val = define_global_identifier(sym->name, sym->val);
                assert(sizeof(sym->val) == sizeof(uintptr_t));
            }
            break;           
        }
    }
}

void eval_function(symbol *function_sym, vm_value *ret_value)
{
    assert(function_sym);
    assert(function_sym->state == SYMBOL_RESOLVED);
    assert(function_sym->kind == SYMBOL_FUNCTION);

    eval_statement_block(function_sym->decl->function.stmts, ret_value);
}

void treewalk_interpreter_test(void)
{
    vm_global_memory = allocate_memory_arena(kilobytes(5));
    map_grow(&global_identifiers, 16);

#if DEBUG_BUILD    
    map_grow(&debug_names_dict, 16);
#endif

    decl **all_declarations = null;
    parse_file("test/loops.txt", &all_declarations);
    symbol **resolved = resolve(all_declarations, true);
    assert(all_declarations);
    assert(resolved);

    shorten_source_pos = true;

    if (buf_len(errors) > 0)
    {
        print_errors_to_console();
        return;
    }

    bool print_ast = true;
    if (print_ast)
    {
        printf("\nDeclarations in sorted order:\n");

        for (symbol **it = ordered_global_symbols; it != buf_end(ordered_global_symbols); it++)
        {
            symbol *sym = *it;
            if (sym->decl)
            {
                printf("\n%s\n", get_decl_ast(sym->decl));
            }
            else
            {
                printf("\n%s\n", sym->name);
            }
        }
    }

    printf("\n=== TREEWALK INTERPRETER RUN ===\n\n");

    eval_global_declarations(resolved);

    symbol *main = get_entry_point(resolved);
    assert(main);

    if (buf_len(errors) > 0)
    {
        print_errors_to_console();
        return;
    }

    // main teoretycznie zwraca int - można tutaj to uszanować
    eval_function(main, null);

    shorten_source_pos = false;

    printf("\n=== FINISHED INTERPRETER RUN ===\n\n");

    debug_breakpoint;
}