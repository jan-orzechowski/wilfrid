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

size_t failed_asserts;

#if DEBUG_BUILD
hashmap debug_names_dict;
char *debug_print_buffer;
size_t debug_print_buffer_size = 100;
#define save_debug_string(str) map_put(&debug_strings, str, str);
#define debug_vm_print(...) _vm_debug_printf(__VA_ARGS__)
#define debug_print_vm_value(val, typ) _debug_print_vm_value(val, typ)
#elif
#define save_debug_string
#define debug_vm_print
#define debug_print_vm_value
#endif

typedef struct vm_value_meta
{
    char *name;
    void *stack_ptr;
} vm_value_meta;

typedef char byte;

void copy_vm_val(byte *dest, byte *val, size_t size)
{
    if (val)
    {
        for (size_t offset = 0; offset < size; offset++)
        {
            *(dest + offset) = *(val + offset);
        }
    }
    else
    {
        for (size_t offset = 0; offset < size; offset++)
        {
            *(dest + offset) = 0;
        }
    }
}

bool is_non_zero(byte *val, size_t size)
{
    for (byte *b = val; b < val + size; b++)
    {
        if (*b)
        {
            return true;
        }
    }
    return false;
}

vm_value_meta *get_metadata_by_ptr(byte *ptr);

char *_debug_print_vm_value(byte *val, type *typ)
{     
    // jeśli używamy tylko jednego bufora, nie możemy użyć dwa razy debug_print_vm_value w jednej wiadomości...

    //if (debug_print_buffer == null)
    //{
        debug_print_buffer = xcalloc(debug_print_buffer_size * sizeof(char));
    //}
    
    switch (typ->kind)
    {
        case TYPE_INT:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%i", *(int32_t*)val);
        }
        break;
        case TYPE_LONG:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%lld", *(int64_t*)val);
        }
        break;
        case TYPE_BOOL:
        case TYPE_NULL:
        case TYPE_CHAR:
        case TYPE_UINT:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%d", *(uint32_t *)val);
        }
        break;
        case TYPE_ULONG:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%lld", *(uint64_t *)val);
        }
        break;
        case TYPE_POINTER:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%p", (void *)val);
        }
        break;
        case TYPE_FLOAT:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%ff", *(float *)val);
        }
        break;
        
        case TYPE_VOID:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "VOID");
        }
        break;

        case TYPE_STRUCT:
        case TYPE_UNION:
        {            
            char *temp = null;
            buf_printf(temp, "[");
            for (size_t offset = 0; offset < get_type_size(typ); offset++)
            {
                byte b = *(val + offset);
                buf_printf(temp, "%d", b);
                if (offset < get_type_size(typ) - 1)
                {
                    buf_printf(temp, ",");
                }
            }
            buf_printf(temp, "]");

            snprintf(debug_print_buffer, debug_print_buffer_size, "struct '%s', bytes: %s", 
                typ->symbol->name, temp);

            buf_free(temp);
        }
        break;

        case TYPE_ARRAY:
        case TYPE_LIST:
        case TYPE_FUNCTION:

        default:
        {
            fatal("unimplemented");
        }
        break;
    }
    
    // zabezpieczenie
    debug_print_buffer[debug_print_buffer_size - 1] = 0;

    return debug_print_buffer;
}

void eval_function(symbol *function_sym, byte *ret_value);

memory_arena *vm_global_memory;
hashmap global_identifiers;

// const strings zrobić osobno
byte *push_global_identifier(const char *name, byte* init_val, size_t val_size)
{
    assert(null == map_get(&global_identifiers, (void *)name));
    
    byte *result = push_size(vm_global_memory, val_size);
    copy_vm_val(result, init_val, val_size);

    map_put(&global_identifiers, (void *)name, (void *)result);

#if DEBUG_BUILD
    map_put(&debug_names_dict, (void *)result, (void *)name);
#endif

    return result;
}

enum
{
    MAX_VM_STACK_SIZE = megabytes(1),
    MAX_VM_METADATA_COUNT = MAX_VM_STACK_SIZE / 4, // raczej nie będziemy zapychać stacku jednobajtowymi zmiennymi
};

byte vm_stack[MAX_VM_STACK_SIZE];
byte *last_used_vm_stack_byte = vm_stack;
vm_value_meta stack_metadata[MAX_VM_METADATA_COUNT];
size_t vm_metadata_count = 0;

bool is_on_stack(byte *ptr)
{
    bool result = (ptr >= (byte *)&vm_stack && ptr < (byte *)&vm_stack + MAX_VM_STACK_SIZE);
    return result;
}

byte *enter_vm_stack_scope(void)
{
    byte *marker = last_used_vm_stack_byte;
    return marker;
}

void leave_vm_stack_scope(byte *marker)
{
    assert(marker >= vm_stack);
    assert(marker <= last_used_vm_stack_byte);
    assert(vm_metadata_count > 0);

    for (byte *byte_to_clean = marker + 1;
        byte_to_clean <= last_used_vm_stack_byte;
        byte_to_clean++)
    {
        *byte_to_clean = 0;
    }
    last_used_vm_stack_byte = marker;

    for (int64_t index = vm_metadata_count - 1; index >= 0; index--)
    {
        vm_value_meta *m = &stack_metadata[index];
        assert(m->stack_ptr);
        if ((byte *)m->stack_ptr > marker)
        {
            *m = (vm_value_meta){0};
            vm_metadata_count--;
        }
        else
        {
            break;
        }
    }
}

byte *push_identifier_on_stack(const char *name, type *type)
{
    size_t size = get_type_size(type);
    if (name) 
    { 
        assert_is_interned(name); 
    }

    byte *result = null;
    int64_t stack_size = last_used_vm_stack_byte - vm_stack;
    if (stack_size + size < MAX_VM_STACK_SIZE
        && vm_metadata_count < MAX_VM_METADATA_COUNT)
    {
        last_used_vm_stack_byte++;
        copy_vm_val(last_used_vm_stack_byte, null, size);
        result = last_used_vm_stack_byte;
        last_used_vm_stack_byte += size /*- 1*/;

        stack_metadata[vm_metadata_count] = (vm_value_meta){
            .name = name,
            .stack_ptr = result,
        };
        vm_metadata_count++;
    }
    else
    {
        // tutaj powinniśmy rzucić błędem i przerwać program
        fatal("stack overflow");
    }

    return result;
}

byte *get_vm_variable(const char *name)
{
    assert(name);
    assert_is_interned(name); 

    byte *result = null;

    // zastanowić się, czy chained hashmap nie byłoby szybsze 
    // w większości wypadków nie będziemy tutaj trafiać - co może powodować sprawdzanie zbyt wielu wartości w słowniku
    result = map_get(&global_identifiers, name);

    if (result == null) 
    {
        if (vm_stack != last_used_vm_stack_byte)
        {
            assert(vm_metadata_count != 0);
            // ze względu na to, że w funkcji nazwa parametru może być taka sama 
            // jak w zewnętrznym scope w którym została wezana
            // - trzeba iść od tyłu
            for (int64_t index = vm_metadata_count - 1; index >= 0; index--)
            {
                vm_value_meta *m = &stack_metadata[index];
                assert(m->stack_ptr);
                assert((byte *)m->stack_ptr <= last_used_vm_stack_byte);
                if (m->name == name)
                {
                    result = m->stack_ptr;
                    break;
                }
            }
        }
    }
    else
    {
        debug_breakpoint;
    }

    if (result == null)
    {
        fatal("no variable with name '%s'", name);
    }

    return result;
}

vm_value_meta *get_metadata_by_ptr(byte *ptr)
{
    if (vm_stack != last_used_vm_stack_byte)
    {
        assert(vm_metadata_count != 0);
        for (int64_t index = vm_metadata_count - 1; index >= 0; index--)
        {
            vm_value_meta *m = &stack_metadata[index];
            if (m->stack_ptr == ptr)
            {
                return m;
            }
        }
    }
    fatal("no variable on address '%p' on the stack", ptr);
    return null;
}

void eval_unary_op(byte *dest, token_kind operation, byte *operand, type *operand_type)
{
    assert(dest);
    assert(operand);
    assert(operand_type);

    if (operand_type == type_int)
    {
        *(int32_t *)dest = (int32_t)eval_long_unary_op(operation, *(int32_t *)operand);
    }
    else if (operand_type == type_uint
        || operand_type == type_bool
        || operand_type == type_char)
    {
        *(uint32_t *)dest = (uint32_t)eval_ulong_unary_op(operation, *(uint32_t *)operand);
    }
    else if (operand_type == type_long)
    {
        *(int64_t *)dest = eval_long_unary_op(operation, *(int64_t *)operand);
    }
    else if (operand_type == type_ulong)
    {
        *(uint64_t *)dest = eval_ulong_unary_op(operation, *(uint32_t *)operand);
    }
    else if (operand_type == type_float)
    {
        *(float *)dest = eval_float_unary_op(operation, *(float *)operand);
    }
    else
    {
        fatal("unsupported type");
    }
}

void eval_binary_op(byte *dest, token_kind op, byte *left, byte *right, type *operands_type)
{
    assert(dest);
    assert(left);
    assert(right);
    assert(operands_type);

    if (operands_type== type_int)
    {
        *(int32_t *)dest = (int32_t)eval_long_binary_op(op, *(int32_t *)left, *(int32_t *)right);
    }
    else if (operands_type== type_uint 
        || operands_type== type_bool 
        || operands_type== type_char)
    {
        *(uint32_t *)dest = (uint32_t)eval_ulong_binary_op(op, *(uint32_t *)left, *(uint32_t *)right);
    }
    else if (operands_type== type_long)
    {
        *(int64_t *)dest = eval_long_binary_op(op, *(int64_t *)left, *(int64_t *)right);
    }
    else if (operands_type== type_ulong)
    {
        *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left, *(uint64_t *)right);
    }
    else if (operands_type== type_float)
    {
        *(float *)dest = eval_float_binary_op(op, *(float *)left, *(float *)right);
    }
    else
    {
        fatal("unsupported type");
    }
}

void perform_cast(byte *new_val, type_kind new_type, byte *old_val, type *old_type)
{
    assert(new_val);
    assert(new_type);
    assert(old_val);
    assert(old_type);

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
            if (old_type == type_int)
            {
                *(int32_t *)new_val = (int32_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(int32_t *)new_val = (int32_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(int32_t *)new_val = (int32_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(int32_t *)new_val = (int32_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(int32_t *)new_val = (int32_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(int32_t *)new_val = (int32_t)*(uintptr_t *)old_val;
            }
        } 
        break;
        case TYPE_LONG:
        {
            if (old_type == type_int)
            {
                *(int64_t *)new_val = (int64_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(int64_t *)new_val = (int64_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(int64_t *)new_val = (int64_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(int64_t *)new_val = (int64_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(int64_t *)new_val = (int64_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(int64_t *)new_val = (int64_t)*(uintptr_t *)old_val;
            }
        }
        break;
        case TYPE_CHAR:
        case TYPE_BOOL:
        case TYPE_UINT:
        {
            if (old_type == type_int)
            {
                *(uint32_t *)new_val = (uint32_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(uint32_t *)new_val = (uint32_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(uint32_t *)new_val = (uint32_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(uint32_t *)new_val = (uint32_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(uint32_t *)new_val = (uint32_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(uint32_t *)new_val = (uint32_t)*(uintptr_t *)old_val;
            }
        }
        break;
        case TYPE_ULONG:
        {
            if (old_type == type_int)
            {
                *(uint64_t *)new_val = (uint64_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(uint64_t *)new_val = (uint64_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(uint64_t *)new_val = (uint64_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(uint64_t *)new_val = (uint64_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(uint64_t *)new_val = (uint64_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(uint64_t *)new_val = (uint64_t)*(uintptr_t *)old_val;
            }
        }
        break;
        case TYPE_FLOAT:
        {
            if (old_type == type_int)
            {
                *(float *)new_val  = (float)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(float *)new_val  = (float)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(float *)new_val  = (float)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(float *)new_val  = (float)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(float *)new_val  = (float)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(float *)new_val  = (float)*(uintptr_t *)old_val;
            }
        }
        break;
        case TYPE_POINTER:
        {
            if (old_type == type_int)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(uintptr_t *)new_val = (uintptr_t)*(uintptr_t *)old_val;;
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

byte *eval_expression(expr *exp)
{
    assert(exp);
    assert(exp->resolved_type);
    
    // w niektórych case'ach result jest nadpisany - można to będzie później usprawnić
    byte *result = push_identifier_on_stack(null, exp->resolved_type);

    switch (exp->kind)
    {
        case EXPR_INT:
        {    
            if (exp->resolved_type->size == 4)
            {
                *((uint32_t *)result) = exp->integer_value;
            }
            else
            {
                *((uint64_t *)result) = exp->integer_value;
            }
        }
        break;
        case EXPR_FLOAT:
        {
            assert(sizeof(double) == sizeof(exp->resolved_type->size));
            *((float *)result) = exp->float_value;
        }
        break;
        case EXPR_CHAR:
        case EXPR_STRING:
        {
            assert(sizeof(uintptr_t) == sizeof(exp->string_value));
            *((uintptr_t *)result) = (uintptr_t)exp->string_value;
        }
        break;
        case EXPR_NULL:
        {
            assert(type_bool->size == sizeof(byte));
            *result = 0;
        }
        break;
        case EXPR_BOOL:
        {
            assert(type_bool->size == sizeof(uint32_t));
            if (exp->bool_value)
            {
                *((uint32_t *)result) = 1;
            }
            else
            {
                *((uint32_t *)result) = 0;
            }
        }
        break;
        case EXPR_NAME:
        {
            assert_is_interned(exp->name);

            result = get_vm_variable(exp->name);            
            assert(result);

            debug_vm_print(exp->pos, "read var '%s' from stack, value %s", 
                exp->name, debug_print_vm_value(result, exp->resolved_type));
        }
        break;
        case EXPR_UNARY:
        {
            assert(exp->unary.operand->resolved_type);

            byte *operand = eval_expression(exp->unary.operand);       
            
            if (exp->unary.operator == TOKEN_MUL) // pointer dereference
            {       
                assert(exp->resolved_type->kind == TYPE_POINTER);
                assert(exp->resolved_type->pointer.base_type);

                debug_vm_print(exp->pos, "deref ptr %s", debug_print_vm_value(operand, exp->resolved_type->pointer.base_type));

                result = (byte *)*(uintptr_t *)operand;
                
                debug_vm_print(exp->pos, "result is val %s", debug_print_vm_value(result, exp->resolved_type));
            }
            else if (exp->unary.operator == TOKEN_BITWISE_AND) // address of
            {
                debug_vm_print(exp->pos, "address of val %s", debug_print_vm_value(operand, exp->resolved_type->pointer.base_type));

                copy_vm_val(result, (byte *)&operand, sizeof(byte *));

                debug_vm_print(exp->pos, "result is ptr %s", debug_print_vm_value(result, exp->resolved_type));
            }
            else if (exp->unary.operator == TOKEN_INC || exp->unary.operator == TOKEN_DEC)
            {
                // te operatory zmieniają wartość, do której się odnosiły               
                eval_unary_op(operand, exp->unary.operator, operand, exp->unary.operand->resolved_type);

                copy_vm_val(result, operand, get_type_size(exp->resolved_type));
            }
            else
            {
                eval_unary_op(result, exp->unary.operator, operand, exp->unary.operand->resolved_type);
            }

            debug_vm_print(exp->pos, "operation %s, result %s", 
                get_token_kind_name(exp->unary.operator), debug_print_vm_value(result, exp->resolved_type));
        }
        break;
        case EXPR_BINARY:
        {
            byte *left = eval_expression(exp->binary.left);
            byte *right = eval_expression(exp->binary.right);
            
            type *left_t = exp->binary.left->resolved_type;
            assert(left_t);
            type *right_t = exp->binary.right->resolved_type;
            assert(right_t);
            assert(compare_types(left_t, right_t));

            size_t left_size = get_type_size(left_t);
            size_t right_size = get_type_size(right_t);
            
            eval_binary_op(result, exp->binary.operator, left, right, left_t);

            debug_vm_print(exp->pos, "operation %s, result %s", 
                get_token_kind_name(exp->binary.operator), debug_print_vm_value(result, exp->resolved_type));
        }
        break;
        case EXPR_TERNARY:
        {
            byte *val = eval_expression(exp->ternary.condition);
            if (*val)
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

                assert(exp->call.args[1]->resolved_type);
                byte *val = eval_expression(exp->call.args[1]);
                char *val_str = debug_print_vm_value(val, exp->call.args[1]->resolved_type);

                printf("--------------------------- PRINTF CALL: format: %s, vals: %s\n", format, val_str);                
            }
            else if (exp->call.resolved_function->name == str_intern("assert"))
            {
                assert(exp->call.args_num == 1);
                assert(exp->call.args[0]->resolved_type);
                byte *val = eval_expression(exp->call.args[0]);
                bool passed = is_non_zero(val, get_type_size(exp->call.args[0]->resolved_type));
                printf("--------------------------- ASSERT: %s\n", passed ? "PASSED" : "FAILED");
                if (false == passed)
                {
                    failed_asserts++;
                }
            }
            else
            {
                assert(exp->call.resolved_function);
                assert(exp->call.resolved_function->type);

                debug_vm_print(exp->pos, "FUNCTION CALL - %s - enter", exp->call.resolved_function->name);

                byte **arg_vals = null;
                for (size_t i = 0; i < exp->call.args_num; i++)
                {
                    expr *arg_expr = exp->call.args[i];
                    byte *arg_val = eval_expression(arg_expr);
                    buf_push(arg_vals, arg_val);
                }

                assert(buf_len(arg_vals) == exp->call.args_num);
                assert(exp->call.args_num == exp->call.resolved_function->type->function.param_count);

                result = push_identifier_on_stack(null, exp->resolved_type);

                byte *marker = enter_vm_stack_scope();
                {
                    for (size_t i = 0; i < buf_len(arg_vals); i++)
                    {
                        const char *name = exp->call.resolved_function->decl->function.params.params[i].name;
                        type *t = exp->call.resolved_function->type->function.param_types[i];
                        
                        byte *arg_val = push_identifier_on_stack(name, t);
                        copy_vm_val(arg_val, arg_vals[i], get_type_size(t));
                    }

                    eval_function(exp->call.resolved_function, result);
                }
                leave_vm_stack_scope(marker);
                buf_free(arg_vals);

                debug_vm_print(exp->pos, "FUNCTION CALL - %s - exit", exp->call.resolved_function->name);
                debug_vm_print(exp->pos, "returned value from function call: %s", debug_print_vm_value(result, exp->resolved_type));
            }
        }
        break;
        case EXPR_FIELD:
        {
            byte *val = eval_expression(exp->field.expr);

            type *aggr_type = exp->field.expr->resolved_type;
            while (aggr_type->kind == TYPE_POINTER)
            {
                debug_vm_print(exp->pos, "auto deref ptr %s", debug_print_vm_value(val, aggr_type));
                aggr_type = aggr_type->pointer.base_type;                
                (uintptr_t)val = *(uintptr_t *)val;
            }

            size_t field_offset = get_field_offset(aggr_type, exp->field.field_name);
            result = val + field_offset;
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
            size_t size = get_type_size(exp->size_of.resolved_type);
            assert(get_type_size(exp->resolved_type) == sizeof(size_t));
            copy_vm_val(result, (byte *)&size, sizeof(size_t));
        }
        break;
        case EXPR_CAST:
        {
            byte *old_val = eval_expression(exp->cast.expr);
            perform_cast(result, exp->cast.resolved_type->kind, old_val, exp->cast.expr->resolved_type);
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            type *aggr_type = exp->resolved_type;
            assert(aggr_type->kind == TYPE_STRUCT || aggr_type->kind == TYPE_UNION);
            
            for (size_t i = 0; i < exp->compound.fields_count; i++)
            {
                compound_literal_field *f = exp->compound.fields[i];
                byte *f_val = eval_expression(f->expr);
                f->expr->resolved_type;

                size_t offset = 0;
                if (f->field_name)
                {
                    offset = get_field_offset(aggr_type, f->field_name);
                }
                else
                {
                    offset = get_field_offset_by_index(aggr_type, i);
                }
                
                copy_vm_val(result + offset, f_val, get_type_size(f->expr->resolved_type));                               
            }            
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

    assert(result);
    return result;
}

bool break_loop = false;
bool continue_loop = false;
bool return_func = false;

void eval_statement(stmt *st, byte *opt_ret_value);

void eval_statement_block(stmt_block block, byte *opt_ret_value)
{
    if (block.stmts_count > 0)
    {
        debug_vm_print(block.stmts[0]->pos, "BLOCK SCOPE - start");
        
        byte *marker = enter_vm_stack_scope();
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

void eval_statement(stmt *st, byte *opt_ret_value)
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
            byte *val = eval_expression(st->return_stmt.ret_expr);
            
            size_t type_size = get_type_size(st->return_stmt.ret_expr->resolved_type);
            copy_vm_val(opt_ret_value, val, type_size);

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
            decl *dec = st->decl_stmt.decl;
            assert(dec);
            assert(dec->resolved_type);
            assert(compare_types(dec->variable.expr->resolved_type, dec->resolved_type));
            
            if (dec->kind == DECL_VARIABLE)
            {                
                byte *new_val = eval_expression(dec->variable.expr);
                
                if (false == is_on_stack(new_val))
                {
                    byte *stack_val = push_identifier_on_stack(dec->name, dec->resolved_type);
                    copy_vm_val(stack_val, new_val, get_type_size(dec->resolved_type));
                    new_val = stack_val;
                }

                vm_value_meta *m = get_metadata_by_ptr(new_val);
                m->name = dec->name;

                debug_vm_print(dec->pos, "declaration of %s, init value %s", 
                    dec->name, debug_print_vm_value(new_val, dec->resolved_type));
            }
            else
            {
                fatal("shouldn't be possible");
            }
        }
        break;
        case STMT_IF_ELSE:
        {
            byte *cond_var = eval_expression(st->if_else.cond_expr);
            type *cond_type = st->if_else.cond_expr->resolved_type;
            assert(cond_var);
            assert(cond_type);

            debug_vm_print(st->pos, "IF - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));

            if (is_non_zero(cond_var, get_type_size(st->if_else.cond_expr->resolved_type)))
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

            type *cond_type = st->while_stmt.cond_expr->resolved_type;
            assert(cond_type);

            size_t cond_var_size = get_type_size(cond_type);
            byte *cond_var = eval_expression(st->while_stmt.cond_expr);

            debug_vm_print(st->pos, "WHILE - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));
            
            byte *marker = enter_vm_stack_scope();
            while (is_non_zero(cond_var, cond_var_size))
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

                cond_var = eval_expression(st->while_stmt.cond_expr);

                debug_vm_print(st->pos, "WHILE - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));
            }
            leave_vm_stack_scope(marker);

            debug_vm_print(st->pos, "WHILE - end");
        }
        break;
        case STMT_DO_WHILE:
        {
            debug_vm_print(st->pos, "DO WHILE - start");

            type *cond_type = st->do_while_stmt.cond_expr->resolved_type;
            assert(cond_type);

            size_t cond_var_size = get_type_size(cond_type);
            byte *cond_var = null;

            byte *marker = enter_vm_stack_scope();
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

                cond_var = eval_expression(st->do_while_stmt.cond_expr);

                debug_vm_print(st->pos, "DO WHILE - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));
            }           
            while (is_non_zero(cond_var, cond_var_size));
            leave_vm_stack_scope(marker);

            debug_vm_print(st->pos, "DO WHILE - end");
        }
        break;
        case STMT_FOR:
        {
            debug_vm_print(st->pos, "FOR - start");

            eval_statement(st->for_stmt.init_stmt, null);

            type *cond_type = st->for_stmt.cond_expr->resolved_type;
            assert(cond_type);

            size_t cond_var_size = get_type_size(cond_type);
            byte *cond_var = eval_expression(st->for_stmt.cond_expr);

            debug_vm_print(st->pos, "FOR - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));

            byte *marker = enter_vm_stack_scope();
            while (is_non_zero(cond_var, cond_var_size))
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
                cond_var = eval_expression(st->for_stmt.cond_expr);

                debug_vm_print(st->pos, "FOR - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));
            }
            leave_vm_stack_scope(marker);          

            debug_vm_print(st->pos, "FOR - end");
        }
        break;
        case STMT_ASSIGN:
        {   
            assert(is_assign_operation(st->assign.operation));
            
            byte *new_val = eval_expression(st->assign.value_expr);
            byte *old_val = eval_expression(st->assign.assigned_var_expr);
            
            type *new_val_t = st->assign.value_expr->resolved_type;
            type *old_val_t = st->assign.assigned_var_expr->resolved_type;

            assert(new_val_t);
            assert(old_val_t);
            assert(compare_types(new_val_t, old_val_t));

            if (st->assign.operation != TOKEN_ASSIGN)
            {
                token_kind op = get_assignment_operation_token(st->assign.operation);
                eval_binary_op(new_val, op, old_val, new_val, new_val_t);

                debug_vm_print(st->assign.assigned_var_expr->pos, "operation %s for assignment, result %s",
                    get_token_kind_name(op), debug_print_vm_value(new_val, new_val_t));
            }

            debug_vm_print(st->assign.assigned_var_expr->pos, "copied new value %s over old value %s", 
                debug_print_vm_value(new_val, new_val_t), debug_print_vm_value(old_val, old_val_t));
            
            copy_vm_val(old_val, new_val, get_type_size(old_val_t));
        }
        break;
        case STMT_SWITCH:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_EXPR:
        {
            assert(st->expr->resolved_type);
            byte *result = eval_expression(st->expr);
            debug_vm_print(st->expr->pos, "expression as statement, result %s",
                debug_print_vm_value(result, st->expr->resolved_type));
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
        size_t size = get_type_size(sym->type);
        switch (sym->kind)
        {
            case SYMBOL_VARIABLE:
            {
                assert(sym->decl->kind == DECL_VARIABLE);
                
                byte *result = eval_expression(sym->decl->variable.expr);

                push_global_identifier(sym->name, result, size);
            }
            break;
            case SYMBOL_CONST:
            {                
                push_global_identifier(sym->name, (byte *)sym->val, size);
            }
            break;           
        }
    }
}

void eval_function(symbol *function_sym, byte *ret_value)
{
    assert(function_sym);
    assert(function_sym->state == SYMBOL_RESOLVED);
    assert(function_sym->kind == SYMBOL_FUNCTION);

    eval_statement_block(function_sym->decl->function.stmts, ret_value);

    if (return_func)
    {
        return_func = false;
    }
}

void treewalk_interpreter_test(void)
{
    vm_global_memory = allocate_memory_arena(kilobytes(5));
    map_grow(&global_identifiers, 16);

#if DEBUG_BUILD    
    map_grow(&debug_names_dict, 16);
#endif

    decl **all_declarations = null;
    parse_file("test/sizeof.txt", &all_declarations);
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

    if (failed_asserts > 0)
    {
        fatal("\nNumber of failed assertions: %d\n", failed_asserts);
    }

    debug_breakpoint;
}