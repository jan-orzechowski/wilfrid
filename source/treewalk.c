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
char *debug_print_buffer;
size_t debug_print_buffer_size = 1000;
#define save_debug_string(str) map_put(&debug_strings, str, str);
#define debug_vm_print(...) _vm_debug_printf(__VA_ARGS__)
#define debug_vm_simple_print(...) printf(__VA_ARGS__)
#define debug_print_vm_value(val, typ) _debug_print_vm_value(val, typ)
#else
#define save_debug_string
#define debug_vm_print
#define debug_vm_simple_print
#define debug_print_vm_value
#endif

void runtime_error(source_pos pos, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char *message_buf = null;
    buf_printf(message_buf, "RUNTIME ERROR ");
    print_source_pos(&message_buf, pos);
    buf_printf(message_buf, ": ");

    printf("\n\n%s", message_buf);
    vprintf(format, args);
    printf("\n");
    buf_free(message_buf);
    
    va_end(args);
    exit(1);
}

typedef struct vm_value_meta
{
    char *name;
    void *stack_ptr;
} vm_value_meta;

typedef char byte;

byte *eval_expression(expr *exp);

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

typedef ___list_hdr___ vm_list_header;

#if DEBUG_BUILD
char *_debug_print_vm_value(byte *val, type *typ)
{    
    assert(typ);
    assert(val);

    if (debug_print_buffer == null)
    {
        debug_print_buffer = xcalloc(debug_print_buffer_size * sizeof(char));
    }
    
    switch (typ->kind)
    {
        case TYPE_INT:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%i", *(int32_t *)val);
        }
        break;
        case TYPE_ENUM:
        case TYPE_LONG:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%lld", *(int64_t *)val);
        }
        break;
        case TYPE_CHAR:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%u", *(uint8_t *)val);
        }
        break;
        case TYPE_BOOL:
        case TYPE_NULL:
        case TYPE_UINT:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%u", *(uint32_t *)val);
        }
        break;
        case TYPE_ULONG:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%llu", *(uint64_t *)val);
        }
        break;
        case TYPE_POINTER:
        {
            snprintf(debug_print_buffer, debug_print_buffer_size, "%p (%llu)", (void *)val, (uint64_t)val);
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
                buf_printf(temp, "%u", b);
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
        {
            char *temp = null;
            buf_printf(temp, "[");            
            size_t array_size = get_type_size(typ->array.base_type) * typ->array.size;
            for (size_t offset = 0; offset < array_size; offset++)
            {
                byte b = *(val + offset);
                buf_printf(temp, "%u", b);
                if (offset < array_size - 1)
                {
                    buf_printf(temp, ",");
                }
            }
            buf_printf(temp, "]");

            snprintf(debug_print_buffer, debug_print_buffer_size, "%s, bytes: %s",
                pretty_print_type_name(typ, false), temp);

            buf_free(temp);
        }
        break;
        case TYPE_LIST:
        {            
            vm_list_header *hdr = *(vm_list_header **)val;
            if (hdr == null)
            {
                snprintf(debug_print_buffer, debug_print_buffer_size, "%s, uninitialized",
                    pretty_print_type_name(typ, false));
            }
            else
            {
                char *temp = null;
                buf_printf(temp, "dynamic (len: %zu, cap: %zu) [", hdr->length, hdr->capacity);
                
                size_t list_size = get_type_size(typ->list.base_type) * hdr->length;
                byte *buffer = (byte *)hdr->buffer;

                for (size_t offset = 0; offset < list_size; offset++)
                {
                    byte b = *(buffer + offset);
                    buf_printf(temp, "%d", b);
                    if (offset < list_size - 1)
                    {
                        buf_printf(temp, ",");
                    }
                }
                buf_printf(temp, "]");

                snprintf(debug_print_buffer, debug_print_buffer_size, "%s, bytes: %s",
                    pretty_print_type_name(typ, false), temp);

                buf_free(temp);
            }
        }
        break;

        case TYPE_FUNCTION:

        default:
        {
            fatal("unimplemented");
        }
        break;
    }
    
    // zabezpieczenie
    debug_print_buffer[debug_print_buffer_size - 1] = 0;

    return xprintf("%s", debug_print_buffer);
}
#endif

void eval_function(symbol *function_sym, byte *ret_value);

memory_arena *vm_global_memory;
hashmap global_identifiers;

byte *push_global_identifier(source_pos pos, const char *name, byte* init_val, size_t val_size)
{
    assert(null == map_get(&global_identifiers, name));
    
    if (val_size > vm_global_memory->block_size)
    {
        runtime_error(pos, "Currently max size for global variables is %zu. Tried to allocate %zu for '%s' variable",
            vm_global_memory->block_size, val_size, name);
    }

    byte *result = push_size(vm_global_memory, val_size);
    if (init_val)
    {
        copy_vm_val(result, init_val, val_size);
    }

    map_put(&global_identifiers, name, result);

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

    for (byte *byte_to_clean = marker + 1;
        byte_to_clean <= last_used_vm_stack_byte;
        byte_to_clean++)
    {
        *byte_to_clean = 0;
    }
    last_used_vm_stack_byte = marker;

    if (vm_metadata_count > 0)
    {
        for (int64_t index = vm_metadata_count - 1; index >= 0; index--)
        {
            vm_value_meta *m = &stack_metadata[index];
            assert(m->stack_ptr);
            if ((byte *)m->stack_ptr > marker)
            {
                *m = (vm_value_meta){ 0 };
                vm_metadata_count--;
            }
            else
            {
                break;
            }
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
        last_used_vm_stack_byte += (size - 1);

        stack_metadata[vm_metadata_count] = (vm_value_meta){
            .name = name,
            .stack_ptr = result,
        };
        vm_metadata_count++;
    }
    else
    {
        runtime_error((source_pos){ 0 }, "Stack overflow");
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
        runtime_error((source_pos){ 0 }, "Variable with name '%s' doesn't exist", name);
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

    if (operand_type == type_char)
    {
        *(uint8_t *)dest = (uint8_t)eval_ulong_unary_op(operation, *(uint8_t *)operand);
    }
    else if (operand_type == type_int)
    {
        *(int32_t *)dest = (int32_t)eval_long_unary_op(operation, *(int32_t *)operand);
    }
    else if (operand_type == type_uint
        || operand_type == type_bool
        || operand_type == type_char)
    {
        *(uint32_t *)dest = (uint32_t)eval_ulong_unary_op(operation, *(uint32_t *)operand);
    }
    else if (operand_type == type_long 
        || operand_type->kind == TYPE_ENUM)
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
        illegal_op_flag = true;
    }
}

void eval_binary_op(byte *dest, token_kind op, byte *left, byte *right, type *left_t, type *right_t)
{
    assert(dest);
    assert(left);
    assert(right);
    assert(left_t);
    assert(right_t);
    
    if (left_t != right_t 
        && (left_t->kind == TYPE_POINTER || right_t->kind == TYPE_POINTER))
    {
        if (left_t->kind == TYPE_POINTER && right_t->kind == TYPE_POINTER)
        {
            *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left, *(uint64_t *)right);
        }
        else if (left_t->kind == TYPE_POINTER && right_t == type_null)
        {
            *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left, 0);
        }
        else if (right_t->kind == TYPE_POINTER && left_t == type_null)
        {
            *(uint64_t *)dest = eval_ulong_binary_op(op, 0, *(uint64_t *)right);
        }        
        else
        {
            assert(left_t->kind == TYPE_POINTER || right_t->kind == TYPE_POINTER);

            if (left_t->kind == TYPE_POINTER)
            {
                assert(*(uint64_t *)left > *(uint64_t *)right);
                size_t type_size = get_type_size(left_t->pointer.base_type);
                *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left, (*(uint64_t *)right) * type_size);
            }
            else
            {
                assert(*(uint64_t *)right > *(uint64_t *)left);
                size_t type_size = get_type_size(right_t->pointer.base_type);
                *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left * type_size, (*(uint64_t *)right));
            }
        }
    }
    else if (left_t == type_char)
    {
        *(uint8_t *)dest = (uint8_t)eval_ulong_binary_op(op, *(uint8_t *)left, *(uint8_t *)right);
    }
    else if (left_t == type_int)
    {
        *(int32_t *)dest = (int32_t)eval_long_binary_op(op, *(int32_t *)left, *(int32_t *)right);
    }
    else if (left_t == type_uint || left_t == type_bool)
    {
        *(uint32_t *)dest = (uint32_t)eval_ulong_binary_op(op, *(uint32_t *)left, *(uint32_t *)right);
    }
    else if (left_t == type_long || left_t->kind == TYPE_ENUM)
    {
        *(int64_t *)dest = eval_long_binary_op(op, *(int64_t *)left, *(int64_t *)right);
    }
    else if (left_t == type_ulong || left_t->kind == TYPE_POINTER)
    {
        *(uint64_t *)dest = eval_ulong_binary_op(op, *(uint64_t *)left, *(uint64_t *)right);
    }
    else if (left_t == type_float)
    {
        *(float *)dest = eval_float_binary_op(op, *(float *)left, *(float *)right);
    }
    else 
    {
        illegal_op_flag = true;
    }
}

void perform_cast(source_pos pos, byte *new_val, type* new_type, byte *old_val, type *old_type)
{
    assert(new_val);
    assert(new_type);
    assert(old_val);
    assert(old_type);

    bool invalid_cast = false;

    switch (new_type->kind)
    {
        case TYPE_INCOMPLETE:
        case TYPE_COMPLETING:
        case TYPE_NONE:
        {
            fatal("this shouldn't happen");
            invalid_cast = true;
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
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
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
            else if (old_type->kind == TYPE_ENUM)
            {
                *(int32_t *)new_val = (int32_t)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(int32_t *)new_val = (int32_t)*(uint8_t *)old_val;
            }
            else 
            { 
                invalid_cast = true;
            }
        } 
        break;
        case TYPE_ENUM:
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
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
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
            else if (old_type->kind == TYPE_ENUM)
            {
                *(int64_t *)new_val = (int64_t)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(int64_t *)new_val = (int64_t)*(uint8_t *)old_val;
            }
            else
            {
                invalid_cast = true;
            }
        }
        break;
        case TYPE_CHAR:
        {
             if (old_type == type_int)
            {
                *(uint8_t *)new_val = (uint8_t)*(int32_t *)old_val;
            }
            else if (old_type == type_uint)
            {
                *(uint8_t *)new_val = (uint8_t)*(uint32_t *)old_val;
            }
            else if (old_type == type_long)
            {
                *(uint8_t *)new_val = (uint8_t)*(int64_t *)old_val;
            }
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
            {
                *(uint8_t *)new_val = (uint8_t)*(uint64_t *)old_val;
            }
            else if (old_type == type_float)
            {
                *(uint8_t *)new_val = (uint8_t)*(float *)old_val;
            }
            else if (old_type->kind == TYPE_POINTER)
            {
                *(uint8_t *)new_val = (uint8_t)*(uintptr_t *)old_val;
            }
            else if (old_type->kind == TYPE_ENUM)
            {
                *(uint8_t *)new_val = (uint8_t)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(uint8_t *)new_val = (uint8_t)*(uint8_t *)old_val;
            }
            else
            {
                invalid_cast = true;
            }
        }
        break;
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
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
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
            else if (old_type->kind == TYPE_ENUM)
            {
                *(uint32_t *)new_val = (uint32_t)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(uint32_t *)new_val = (uint32_t)*(uint8_t *)old_val;
            }
            else
            {
                invalid_cast = true;
            }
        }
        break;
        case TYPE_NULL:
        case TYPE_ULONG:
        case TYPE_POINTER:
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
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
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
            else if (old_type->kind == TYPE_ENUM)
            {
                *(uint64_t *)new_val = (uint64_t)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(uint64_t *)new_val = (uint64_t)*(uint8_t *)old_val;
            }
            else
            {
                invalid_cast = true;
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
            else if (old_type == type_ulong || old_type->kind == TYPE_NULL)
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
            else if (old_type->kind == TYPE_ENUM)
            {
                *(float *)new_val = (float)*(int64_t *)old_val;
            }
            else if (old_type == type_char)
            {
                *(float *)new_val = (float)*(uint8_t *)old_val;
            }
            else
            {
                invalid_cast = true;
            }
        }
        break;
     
        case TYPE_STRUCT:
        case TYPE_UNION: 
        case TYPE_ARRAY: 
        case TYPE_LIST: 
        case TYPE_FUNCTION: 
        
        default:
        {
            invalid_cast = true;
        }
        break;
    }

    if (invalid_cast)
    {
        runtime_error(pos, "Invalid cast. Tried to cast %s to %s",
            pretty_print_type_name(old_type, false),
            pretty_print_type_name(new_type, false));
    }
}

byte *eval_binary_expression(expr *exp, byte *result)
{
    assert(exp->kind == EXPR_BINARY);

    type *left_t = exp->binary.left->resolved_type;
    type *right_t = exp->binary.right->resolved_type;
    assert(left_t);
    assert(right_t);
    assert(compare_types(left_t, right_t));
    assert(compare_types(left_t, exp->resolved_type));

    size_t left_size = get_type_size(left_t);
    size_t right_size = get_type_size(right_t);
    assert(left_size == right_size);

    byte *left = null;
    byte *right = null;

    // short-circuit evaluation of logical and/or
    if (exp->binary.operator == TOKEN_AND || exp->binary.operator == TOKEN_OR)
    {
        assert(exp->resolved_type == type_bool);
        left = eval_expression(exp->binary.left);
        if (exp->binary.operator == TOKEN_AND)
        {
            if ((*(uint32_t *)left) == 0)
            {
                (*(uint32_t *)result) = 0;
                debug_vm_print(exp->pos, "short circuit &&, result false");
                return result;
            }
        }
        else
        {
            if ((*(uint32_t *)left) != 0)
            {
                (*(uint32_t *)result) = 1;
                debug_vm_print(exp->pos, "short circuit &&, result true");
                return result;
            }
        }

        right = eval_expression(exp->binary.right);
    }
    else
    {
        left = eval_expression(exp->binary.left);
        right = eval_expression(exp->binary.right);
    }
    
    eval_binary_op(result, exp->binary.operator, left, right, left_t, right_t);

    if (illegal_op_flag)
    {
        runtime_error(exp->pos, "Illegal operation %s on %s and %s types",
            get_token_kind_name(exp->binary.operator),
            pretty_print_type_name(left_t, false),
            pretty_print_type_name(right_t, false));
    }

    debug_vm_print(exp->pos, "operation %s on %s and %s, result %s",
        get_token_kind_name(exp->binary.operator),
        debug_print_vm_value(left, left_t),
        debug_print_vm_value(right, right_t),
        debug_print_vm_value(result, exp->resolved_type));

    return result;
}

byte *eval_stub_expression(byte *result, expr *exp);

byte *eval_printf_call(expr *exp, byte *result)
{
    assert(exp->kind == EXPR_CALL);
    assert(exp->call.resolved_function->name == str_intern("printf"));

    assert(exp->call.args_num >= 1);
    assert(exp->call.args[0]->kind);

    type *format_arg_type = exp->call.args[0]->resolved_type;
    assert(format_arg_type->kind == TYPE_POINTER
        && format_arg_type->pointer.base_type->kind == TYPE_CHAR);

    byte *format_arg = eval_expression(exp->call.args[0]);
    char *format = *(char **)format_arg;
    char *orig_format = format;

    byte **arg_vals = null;
    type **arg_types = null;
    for (size_t i = 1; i < exp->call.args_num; i++)
    {
        expr *arg_expr = exp->call.args[i];
        byte *arg_val = eval_expression(arg_expr);
        buf_push(arg_vals, arg_val);
        buf_push(arg_types, arg_expr->resolved_type);
    }

    char *output = null;

    size_t current_arg_index = 0;
    size_t spec_start_index = 0;
    size_t spec_end_index = 0;

    bool not_enough_arguments = false;
    type *mismatching_type = null;
    type *expected_type = null;
    bool unknown_specifier = false;

    while (*format)
    {        
        if (*format != '%')
        {
            buf_printf(output, "%c", *format);
            format++;
            continue;
        }         

        spec_start_index = (format - orig_format) + 1;
        if (*(format + 1) == 0)
        {
            unknown_specifier = true;
            spec_end_index = spec_start_index;
            break;
        }
        else if (*(format + 1) == '%')
        {
            buf_printf(output, "%%");
            format += 2;
        }
        else if (*(format + 1) == 'i' || *(format + 1) == 'd')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_INT)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%i", *(int32_t *)val);
            }
            else
            {
                mismatching_type = t;
                expected_type = type_int;
            }
            current_arg_index++;
            format += 2;
        }
        else if (*(format + 1) == 'c')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_CHAR)
            {
                byte *val = arg_vals[current_arg_index];
                if (val != 0)
                {
                    buf_printf(output, "%c", *(char *)val);
                }               
            }
            else
            {
                mismatching_type = t;
                expected_type = type_uint;
            }
            current_arg_index++;
            format += 2;
        }
        else if (*(format + 1) == 's')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_POINTER 
                && t->pointer.base_type
                && t->pointer.base_type->kind == TYPE_CHAR)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%s", *(char **)val);
            }
            else
            {
                mismatching_type = t;
                expected_type = type_void;
            }
            current_arg_index++;
            format += 2;
        }
        else if (*(format + 1) == 'u')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_CHAR)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%u", *(uint8_t *)val);
            }
            else if (t->kind == TYPE_BOOL)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%s", (*(uint32_t *)val) ? "true" : "false");
            }
            else if (t->kind == TYPE_NULL || t->kind == TYPE_UINT)
            {
                byte *val = arg_vals[current_arg_index];                
                buf_printf(output, "%u", *(uint32_t *)val);
            }
            else
            {
                mismatching_type = t;
                expected_type = type_uint;
            }
            current_arg_index++;
            format += 2;
        }
        else if (*(format + 1) == 'p')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_NULL || t->kind == TYPE_POINTER)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%p", (void *)val);
            }
            else
            {
                mismatching_type = t;
                expected_type = type_void;
            }
            current_arg_index++;
            format += 2;
        }
        else if (*(format + 2) == 0)
        {
            unknown_specifier = true;
            spec_end_index = spec_start_index + 1;
            break;
        }
        else if (*(format + 1) == 'f' && *(format + 2) == 'f')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_FLOAT)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%ff", *(float *)val);
            }
            else
            {
                mismatching_type = t;
                expected_type = type_float;
            }
            current_arg_index++;
            format += 3;
        }
        else if (*(format + 3) == 0)
        {
            unknown_specifier = true;
            spec_end_index = spec_start_index + 1;
            break;
        }
        else if (*(format + 1) == 'l' && *(format + 2) == 'l' && *(format + 3) == 'd')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_LONG || t->kind == TYPE_ENUM)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%lld", *(int64_t *)val); 
            }
            else
            {
                mismatching_type = t;
                expected_type = type_long;
            }
            current_arg_index++;
            format += 4;
        }
        else if (*(format + 1) == 'l' && *(format + 2) == 'l' && *(format + 3) == 'u')
        {
            if (current_arg_index + 1 > buf_len(arg_types))
            {
                not_enough_arguments = true;
                break;
            }

            type *t = arg_types[current_arg_index];
            if (t->kind == TYPE_ULONG)
            {
                byte *val = arg_vals[current_arg_index];
                buf_printf(output, "%llu", *(uint64_t *)val);                
            }
            else
            {
                mismatching_type = t;
                expected_type = type_ulong;
            }
            current_arg_index++;
            format += 4;
        }
        else
        {
            unknown_specifier = true;
            spec_end_index = spec_start_index + 1;
            break;
        }
    }

    bool too_many_arguments = false;
    if (buf_len(arg_types) > current_arg_index)
    {
        too_many_arguments = true;
    }

    buf_free(arg_vals);
    buf_free(arg_types);

    result = push_identifier_on_stack(null, exp->resolved_type);

    if (false == (unknown_specifier || too_many_arguments || not_enough_arguments || mismatching_type))
    {
        int32_t characters_written = buf_len(output);
        assert(exp->resolved_type == type_int);
        copy_vm_val(result, (byte *)&characters_written, sizeof(int32_t));

#if DEBUG_BUILD
        debug_vm_simple_print("--------------------------- PRINTF CALL: format: %s, output: %s\n", orig_format, output);
        debug_breakpoint;
#else
        printf("%s", output);
#endif
        buf_free(output);
        return result;
    }
    else
    {
        buf_free(output);
    }

    if (unknown_specifier)
    {
        assert(spec_start_index > 0);
        assert(spec_end_index >= spec_start_index);
        size_t length = spec_end_index - spec_start_index;
        assert(length <= 3);
        if (length == 0)
        {
            runtime_error(exp->pos, "Singular '%%' in printf is not allowed. Use '%%%%' or type specifier instead");
        }
        else
        {
            char buffer[5] = { 0 };
            strncpy((char *)buffer, orig_format + spec_start_index, length);
            buffer[4] = 0;
            runtime_error(exp->pos, "Unrecognized type specifer in the printf call: %%%s", buffer);
        }        
    }
    else if (too_many_arguments)
    {
        runtime_error(exp->pos, "There are more arguments passed to the printf call than type specifiers");
    }
    else if (not_enough_arguments)
    {
        runtime_error(exp->pos, "There are more type specifiers in the printf call than passed arguments");
    }
    else if (mismatching_type)
    {        
        runtime_error(exp->pos, 
            "A type specifier in the printf call doesn't match the passed argument: expected %s, got %s",
            expected_type == type_void ? "pointer" : pretty_print_type_name(expected_type, false),
            pretty_print_type_name(mismatching_type, false));
    }

    return result;
}

byte *eval_function_call(expr *exp, byte *result)
{
    assert(exp->kind == EXPR_CALL);
    assert(exp->call.resolved_function);
    
    symbol *function = exp->call.resolved_function;

    if (function->name == str_intern("printf"))
    {
        eval_printf_call(exp, result);
    }
    else if (function->name == str_intern("assert"))
    {
        assert(exp->call.args_num == 1);
        assert(exp->call.args[0]->resolved_type);
        byte *val = eval_expression(exp->call.args[0]);
        bool passed = is_non_zero(val, get_type_size(exp->call.args[0]->resolved_type));
        debug_vm_simple_print("--------------------------- ASSERT: %s\n", passed ? "PASSED" : "FAILED");
        if (false == passed)
        {
            failed_asserts++;
            runtime_error(exp->pos, "ASSERTION FAILED");            
        }
    }
    else if (function->name == str_intern("gc"))
    {
        assert(exp->call.args_num == 0);
        debug_vm_simple_print("--------------------------- GC CALL\n");

        if (___gc_allocs___->total_count > 0)
        {            
            memory_arena_block *block = vm_global_memory->first_block;
            while (block)
            {
                if (block->current_size > 0)
                {
                    ___scan_for_pointers___((uintptr_t)block->base_address, block->current_size);
                }
                block = block->next;
            }

            uintptr_t stack_begin = (uintptr_t)&vm_stack;
            uintptr_t stack_end = (uintptr_t)last_used_vm_stack_byte;
            ___scan_for_pointers___(stack_begin, stack_end - stack_begin);

            ___mark_heap___();
            ___sweep___();
        }

        return result;
    }
    else if (function->name == str_intern("query_gc_total_memory"))
    {
        assert(exp->call.args_num == 0);
        size_t val = query_gc_total_memory();
        copy_vm_val(result, (byte *)&val, sizeof(size_t));
        return result;
    }
    else if (function->name == str_intern("query_gc_total_count"))
    {
        assert(exp->call.args_num == 0);        
        size_t val = query_gc_total_count();
        copy_vm_val(result, (byte *)&val, sizeof(size_t));
        return result;
    }
    else if (function->name == str_intern("allocate"))
    {
        assert(exp->call.args_num == 1);
        assert(exp->call.args[0]->resolved_type);

        byte *val = eval_expression(exp->call.args[0]);
        size_t size = *(int64_t *)val;
        assert(size < megabytes(100));

        uintptr_t ptr = (uintptr_t)___alloc___(size);
        copy_vm_val(result, (byte *)&ptr, sizeof(uintptr_t));

        debug_vm_print(exp->pos, "allocation at %p, via 'allocate', size %zu",
            (void *)ptr, size);
    }
    else if (function->decl->function.is_extern)
    {
        runtime_error(exp->pos, 
            "Extern functions are not supported in the interpreter. Tried to call '%s'", 
            function->name);
    }
    else
    {
        assert(function);
        assert(function->type);

        debug_vm_print(exp->pos, "FUNCTION CALL - %s - enter", function->name);

        byte **arg_vals = null;
        type **arg_types = null;
        char **arg_names = null;
        if (exp->call.method_receiver)
        {
            byte *arg_val = eval_expression(exp->call.method_receiver);
            buf_push(arg_vals, arg_val);
            buf_push(arg_types, exp->call.method_receiver->resolved_type);
            buf_push(arg_names, function->decl->function.method_receiver->name);
        }

        for (size_t i = 0; i < exp->call.args_num; i++)
        {
            expr *arg_expr = exp->call.args[i];
            byte *arg_val = eval_expression(arg_expr);
            buf_push(arg_vals, arg_val);
            buf_push(arg_types, function->type->function.param_types[i]);
            buf_push(arg_names, function->decl->function.params.params[i].name);
        }

        assert(buf_len(arg_vals) == exp->call.args_num + (exp->call.method_receiver ? 1 : 0));
        assert(exp->call.args_num == function->type->function.param_count);

        result = push_identifier_on_stack(null, exp->resolved_type);

        byte *marker = enter_vm_stack_scope();
        {
            for (size_t i = 0; i < buf_len(arg_vals); i++)
            {
                const char *name = arg_names[i];
                type *type = arg_types[i];
                byte *arg_val = push_identifier_on_stack(name, type);
                copy_vm_val(arg_val, arg_vals[i], get_type_size(type));
            }

            eval_function(function, result);
        }
        leave_vm_stack_scope(marker);

        buf_free(arg_vals);
        buf_free(arg_types);
        buf_free(arg_names);

        debug_vm_print(exp->pos, "FUNCTION CALL - %s - exit", function->name);
        debug_vm_print(exp->pos, "returned value from function call: %s", debug_print_vm_value(result, exp->resolved_type));
    }

    return result;
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
        {
            assert(sizeof(uint8_t) == sizeof(exp->string_value[0]));
            *((uint8_t *)result) = (uint8_t)(exp->string_value[0]);
        }
        break;
        case EXPR_STRING:
        {
            assert(sizeof(uintptr_t) == sizeof(exp->string_value));
            *((uintptr_t *)result) = (uintptr_t)exp->string_value;
        }
        break;
        case EXPR_NULL:
        {
            assert(type_null->size == sizeof(uintptr_t));
            *((uintptr_t *)result) = 0;
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
            type *operand_t = exp->unary.operand->resolved_type;

            if (exp->unary.operator == TOKEN_DEREFERENCE)
            {       
                assert(operand_t->kind == TYPE_POINTER);
                assert(operand_t->pointer.base_type);
                
                result = (byte *)*(uintptr_t *)operand;
                
                debug_vm_print(exp->pos, "deref ptr %s, result is %s", 
                    debug_print_vm_value(operand, operand_t),
                    debug_print_vm_value(result, exp->resolved_type));                
            }
            else if (exp->unary.operator == TOKEN_ADDRESS_OF)
            {
                assert(exp->resolved_type->kind == TYPE_POINTER);
                assert(exp->resolved_type->pointer.base_type);

                copy_vm_val(result, (byte *)&operand, sizeof(byte *));
            
                debug_vm_print(exp->pos, "address of val %s, result is ptr %s",
                    debug_print_vm_value(operand, operand_t),
                    debug_print_vm_value(result, exp->resolved_type));
            }            
            else
            {
                eval_unary_op(result, exp->unary.operator, operand, operand_t);

                if (illegal_op_flag)
                {
                    runtime_error(exp->pos, "Illegal operation %s on %s type",
                        get_token_kind_name(exp->binary.operator),
                        pretty_print_type_name(operand_t, false));
                }

                debug_vm_print(exp->pos, "unary operation %s on %s, result is %s",
                    get_token_kind_name(exp->unary.operator),
                    debug_print_vm_value(operand, operand_t),
                    debug_print_vm_value(result, exp->resolved_type));
            }
        }
        break;
        case EXPR_BINARY:
        {
            result = eval_binary_expression(exp, result);
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
            result = eval_function_call(exp, result);
        }
        break;
        case EXPR_FIELD:
        {
            assert(exp->field.expr->resolved_type);
            if (exp->field.expr->resolved_type->kind == TYPE_ENUM)
            {
                type *enum_type = exp->field.expr->resolved_type;
                char *key = exp->field.field_name;
                void *val_ptr = map_get(&enum_type->enumeration.values, key);
                assert(val_ptr);
                int64_t val = *(int64_t *)val_ptr;
                
                // tu jest zduplikowany result, po to, żeby można było nadpisać type jako long
                result = push_identifier_on_stack(null, type_long);
                copy_vm_val(result, (byte *)&val, sizeof(int64_t));
            }
            else
            {
                byte *aggr = eval_expression(exp->field.expr);
                type *aggr_type = exp->field.expr->resolved_type;
                
                while (aggr_type->kind == TYPE_POINTER)
                {
                    debug_vm_print(exp->pos, "auto deref ptr %s", debug_print_vm_value(aggr, aggr_type));
                    aggr_type = aggr_type->pointer.base_type;                    
                    aggr = (byte*)*(uintptr_t *)aggr;
                }

                if (aggr == null)
                {
                    runtime_error(exp->pos, "Tried to access value by a null pointer");
                    break;
                }

                size_t field_offset = get_field_offset(aggr_type, exp->field.field_name);
                result = aggr + field_offset;

                debug_vm_print(exp->pos, "accessing field %s of %s at address %p plus offset %d",
                    exp->field.field_name,
                    pretty_print_type_name(exp->field.expr->resolved_type, false),
                    (void *)aggr,
                    field_offset);
            }
        }
        break;
        case EXPR_INDEX:
        {
            byte *arr = eval_expression(exp->index.array_expr);
            type *arr_type = exp->index.array_expr->resolved_type;

            assert(arr_type->kind == TYPE_ARRAY || arr_type->kind == TYPE_POINTER);
            
            if (arr_type->kind == TYPE_POINTER)
            {
                debug_vm_print(exp->pos, "index expr - auto deref ptr %s", debug_print_vm_value(arr, arr_type));
                arr = (byte *)*(uintptr_t *)arr;
            }

            if (arr == null)
            {
                runtime_error(exp->pos, "Tried to access value by a null pointer");
                break;
            }

            byte *ind = eval_expression(exp->index.index_expr);
            type *ind_type = exp->index.index_expr->resolved_type;
            size_t element_index = 0;
            if (get_type_size(ind_type) == 8)
            {
                element_index = *(uint64_t *)ind;
            }
            else if (get_type_size(ind_type) == 4)
            {
                element_index = *(uint32_t *)ind;
            }
            else
            {
                fatal("this shouldn't happen");
            }

            size_t index_offset = get_array_index_offset(arr_type, element_index);
            result = arr + index_offset;
        }
        break;
        case EXPR_NEW:
        {
            type *t = exp->new.resolved_type;
            assert(t);

            size_t size = 0;
            if (t->kind == TYPE_ARRAY && t->array.size == 0)
            {                
                byte *runtime_size = eval_expression(exp->new.type->array.size_expr);
                // to powinno być załatwione jakoś lepiej 
                // - przez upewnienie się, że rozmiar w sizeof jest zawsze long albo coś
                if (exp->new.type->array.size_expr->resolved_type->size == 4)
                {
                    size = *(uint32_t *)runtime_size;
                }
                else
                {
                    size = *(uint64_t *)runtime_size;
                }
            }
            else
            {
                size = get_type_size(exp->new.resolved_type);
            }
            assert(size);
            
            uintptr_t ptr = (uintptr_t)___calloc_wrapper___(size, false);
            copy_vm_val(result, (byte *)&ptr, sizeof(uintptr_t));

            debug_vm_print(exp->pos, "allocation at %p, type %s, size %zu", 
                (void *)ptr,
                pretty_print_type_name(exp->new.resolved_type, false),
                size);
        }
        break;
        case EXPR_AUTO:
        {
            assert(exp->auto_new.resolved_type);
            size_t size = get_type_size(exp->auto_new.resolved_type);
            uintptr_t ptr = (uintptr_t)___calloc_wrapper___(size, true);
            copy_vm_val(result, (byte *)&ptr, sizeof(uintptr_t));

            debug_vm_print(exp->pos, "GC allocation at %p, type %s, size %zu",
                (void *)ptr,
                pretty_print_type_name(exp->auto_new.resolved_type, false),
                size);
        }
        break;
        case EXPR_SIZE_OF_TYPE:
        {
            assert(exp->size_of_type.resolved_type);
            size_t size = get_type_size(exp->size_of_type.resolved_type);
            copy_vm_val(result, (byte *)&size, sizeof(size_t));
        }
        break;
        case EXPR_SIZE_OF:
        {
            assert(exp->size_of.expr->resolved_type);
            size_t size = get_type_size(exp->size_of.expr->resolved_type);
            copy_vm_val(result, (byte *)&size, sizeof(size_t));
        }
        break;
        case EXPR_CAST:
        {
            byte *old_val = eval_expression(exp->cast.expr);
            perform_cast(exp->pos, result, exp->cast.resolved_type, old_val, exp->cast.expr->resolved_type);
        }
        break;
        case EXPR_COMPOUND_LITERAL:
        {
            type *typ = exp->resolved_type;
            if (typ->kind == TYPE_ARRAY)
            {
                for (size_t i = 0; i < exp->compound.fields_count; i++)
                {
                    compound_literal_field *f = exp->compound.fields[i];
                    byte *f_val = eval_expression(f->expr);

                    size_t offset = 0;
                    if (f->field_index >= 0)
                    {
                        offset = get_array_index_offset(typ, f->field_index);
                    }
                    else
                    {
                        assert(f->field_index == -1);
                        offset = get_array_index_offset(typ, i);
                    }

                    copy_vm_val(result + offset, f_val, get_type_size(f->expr->resolved_type));
                }
            }
            else
            {
                assert(typ->kind == TYPE_STRUCT || typ->kind == TYPE_UNION);
                for (size_t i = 0; i < exp->compound.fields_count; i++)
                {
                    compound_literal_field *f = exp->compound.fields[i];
                    byte *f_val = eval_expression(f->expr);

                    size_t offset = 0;
                    if (f->field_name)
                    {
                        offset = get_field_offset(typ, f->field_name);
                    }
                    else
                    {
                        offset = get_field_offset_by_index(typ, i);
                    }

                    copy_vm_val(result + offset, f_val, get_type_size(f->expr->resolved_type));
                }
            }                   
        }
        break;
        case EXPR_STUB:
        {
            result = eval_stub_expression(result, exp);
        }
        break;

        case EXPR_NONE:
        invalid_default_case;
    }

    assert(result);
    return result;
}

byte *eval_stub_expression(byte *result, expr *exp)
{
    assert(exp->kind == EXPR_STUB);
    assert(exp->resolved_type);

    expr *orig_exp = exp->stub.original_expr;

    switch (exp->stub.kind)
    {
        case STUB_EXPR_CAST:
        {
            byte *old_val = eval_expression(orig_exp);
            perform_cast(orig_exp->pos, result, exp->resolved_type, old_val, orig_exp->resolved_type);

            debug_vm_print(exp->pos, "implicit cast %s (value: %s) to %s (result value: %s)",
                pretty_print_type_name(orig_exp->resolved_type, false),
                debug_print_vm_value(old_val, orig_exp->resolved_type),
                pretty_print_type_name(exp->resolved_type, false),
                debug_print_vm_value(result, exp->resolved_type));
        }
        break;
        case STUB_EXPR_POINTER_ARITHMETIC_BINARY:
        {
            assert(orig_exp->kind == EXPR_BINARY);
            
            bool is_ptr_left = exp->stub.left_is_pointer;
            expr *left = orig_exp->binary.left;
            expr *right = orig_exp->binary.right;
            token_kind op = orig_exp->binary.operator;

            byte *ptr_val = eval_expression(is_ptr_left ? left : right);
            byte *int_val = eval_expression(is_ptr_left ? right : left);            
            type *ptr_type = is_ptr_left ? left->resolved_type : right->resolved_type;
            type *int_type = is_ptr_left ? right->resolved_type : left->resolved_type;

            assert(ptr_type->kind == TYPE_POINTER);
            size_t ptr_base_type_size = get_type_size(ptr_type->pointer.base_type);
            assert(ptr_base_type_size != 0);

            uintptr_t old_ptr_val = *(uintptr_t*)ptr_val;
            int64_t int_operand = 0;
            perform_cast(orig_exp->pos, (byte *)&int_operand, type_long, int_val, int_type);
            
            uintptr_t new_ptr_val = 0;
            assert(op == TOKEN_ADD || op == TOKEN_SUB);
            if (op == TOKEN_ADD)
            {
                new_ptr_val = old_ptr_val + (int_operand * ptr_base_type_size);
            }
            else
            {
                new_ptr_val = old_ptr_val - (int_operand * ptr_base_type_size);
            }

            copy_vm_val(result, (byte *)&new_ptr_val, sizeof(uintptr_t));

            assert((*(uintptr_t *)result - old_ptr_val) == (int_operand * ptr_base_type_size)
                || (*(uintptr_t *)result - old_ptr_val) == -(int64_t)(int_operand * ptr_base_type_size));

            debug_vm_print(exp->pos, 
                "pointer arithmetic operation %s on %s and %s (base type size: %zu), result %s",
                get_token_kind_name(op),
                debug_print_vm_value(ptr_val, ptr_type),
                debug_print_vm_value(int_val, int_type),
                ptr_base_type_size, 
                debug_print_vm_value(result, exp->resolved_type)
            );
        }
        break;
        case STUB_EXPR_POINTER_ARITHMETIC_INC:
        {
            byte *ptr_val = eval_expression(orig_exp);
            type *ptr_type = orig_exp->resolved_type;
            assert(ptr_type->kind == TYPE_POINTER);
            size_t ptr_base_type_size = get_type_size(ptr_type->pointer.base_type);
            assert(ptr_base_type_size != 0);

            uintptr_t old_ptr_val = *(uintptr_t *)ptr_val;
            uintptr_t new_ptr_val = 0;
            if (exp->stub.is_inc)
            {
                new_ptr_val = old_ptr_val + ptr_base_type_size;
            }
            else
            {
                new_ptr_val = old_ptr_val - ptr_base_type_size;
            }

            // inc/dec zmienia wartość
            copy_vm_val(ptr_val, (byte *)&new_ptr_val, sizeof(uintptr_t));

            debug_vm_print(exp->pos,
                "pointer arithmetic operation %s on %s (base type size: %zu), result %s",
                exp->stub.is_inc ? "++" : "--",
                debug_print_vm_value((byte*)&old_ptr_val, ptr_type),
                ptr_base_type_size,
                debug_print_vm_value(ptr_val, exp->resolved_type)
            );
        }
        break;
        case STUB_EXPR_LIST_CAPACITY:
        case STUB_EXPR_LIST_LENGTH:
        {
            assert(orig_exp->kind == EXPR_CALL);
            assert(orig_exp->call.args_num == 0);
            assert(orig_exp->call.method_receiver->resolved_type);

            expr *list_expr = orig_exp->call.method_receiver;
            byte *receiver = eval_expression(list_expr);
            if (*(uintptr_t *)receiver == 0)
            {
                runtime_error(orig_exp->pos, "Tried to call method on a uninitialized list");
            }

            vm_list_header *hdr = *(vm_list_header **)receiver;

            bool get_len = (exp->stub.kind == STUB_EXPR_LIST_LENGTH);
            size_t value = get_len 
                ? ___get_list_length___(hdr) 
                : ___get_list_capacity___(hdr);
            
            copy_vm_val(result, (byte *)&value, sizeof(size_t));
        }
        break;
        case STUB_EXPR_LIST_FREE:
        {
            assert(orig_exp);
            byte *list = eval_expression(orig_exp);
            if (*(uintptr_t *)list == 0)
            {
                runtime_error(orig_exp->pos, "Tried to free a uninitialized list");
            }

            vm_list_header *hdr = *(vm_list_header **)list;
            ___list_free___(hdr);

            uintptr_t zero = 0;
            copy_vm_val(list, (byte *)&zero, sizeof(uintptr_t));
        }
        break;
        case STUB_EXPR_LIST_REMOVE_AT:
        {
            // not implemented in cgen
            fatal("unimplemented");
        }
        break;
        case STUB_EXPR_LIST_NEW:
        case STUB_EXPR_LIST_AUTO:
        {            
            assert(orig_exp->resolved_type);
            assert(orig_exp->resolved_type->kind == TYPE_LIST);
            assert(orig_exp->resolved_type->list.base_type);

            bool managed = (exp->stub.kind == STUB_EXPR_LIST_AUTO);
            size_t element_size = get_type_size(exp->resolved_type->list.base_type);

            assert(managed || orig_exp->kind == EXPR_NEW);
            assert(false == managed || orig_exp->kind == EXPR_AUTO);

            vm_list_header *ptr = ___list_initialize___(8, element_size, managed);
            copy_vm_val(result, (byte *)&ptr, sizeof(vm_list_header *));
        }
        break;
        case STUB_EXPR_LIST_ADD:
        {
            assert(orig_exp->kind == EXPR_CALL);
            assert(orig_exp->call.args_num == 1);
            assert(orig_exp->call.method_receiver->resolved_type);
            
            expr *list_expr = orig_exp->call.method_receiver;
            expr *arg_expr = orig_exp->call.args[0];

            assert(list_expr->resolved_type);
            assert(list_expr->resolved_type->kind == TYPE_LIST);            
            assert(list_expr->resolved_type->list.base_type);

            byte *receiver = eval_expression(list_expr);
            byte *arg = eval_expression(arg_expr);

            if (*(uintptr_t *)receiver == 0)
            {
                runtime_error(orig_exp->pos, "Tried to add an element to a uninitialized list");
            }
            
            vm_list_header *hdr = *(vm_list_header **)receiver;
            size_t elem_size = get_type_size(list_expr->resolved_type->list.base_type);
            
            ___list_fit___(hdr, 1, elem_size);
            
            byte *new_elem = (byte *)(hdr->buffer + (hdr->length * elem_size));
            copy_vm_val(new_elem, arg, elem_size);
            hdr->length++;
        }
        break;
        case STUB_EXPR_LIST_INDEX:
        {
            assert(orig_exp->kind == EXPR_INDEX);
            assert(orig_exp->index.array_expr);
            
            byte *list_val = eval_expression(orig_exp->index.array_expr);          
            type *list_typ = orig_exp->index.array_expr->resolved_type;
            assert(list_typ);
            assert(list_typ->kind == TYPE_LIST);
            
            byte *index_val = eval_expression(orig_exp->index.index_expr);
            type *index_typ = orig_exp->index.index_expr->resolved_type;
            assert(index_typ);

            vm_list_header *hdr = *(vm_list_header **)list_val;
            size_t index = *(size_t *)index_val;

            size_t element_index = 0;
            if (get_type_size(index_typ) == 8)
            {
                element_index = *(uint64_t *)index_val;
            }
            else if (get_type_size(index_typ) == 4)
            {
                element_index = *(uint32_t *)index_val;
            }
            else
            {
                fatal("this shouldn't happen");
            }

            size_t index_offset = get_type_size(list_typ->list.base_type) * element_index;
            result = hdr->buffer + index_offset;
        }
        break;
        case STUB_EXPR_NONE:
        invalid_default_case;
    }

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
            if (st->return_stmt.ret_expr)
            {
                assert(opt_ret_value);
                byte *val = eval_expression(st->return_stmt.ret_expr);

                size_t type_size = get_type_size(st->return_stmt.ret_expr->resolved_type);
                copy_vm_val(opt_ret_value, val, type_size);
            }            

            debug_vm_print(st->pos, "return statement");
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
            assert(dec->kind == DECL_VARIABLE);

            if (dec->variable.expr)
            {
                byte *new_val = eval_expression(dec->variable.expr);
                byte *stack_val = push_identifier_on_stack(dec->name, dec->resolved_type);
                copy_vm_val(stack_val, new_val, get_type_size(dec->resolved_type));
                new_val = stack_val;

                vm_value_meta *m = get_metadata_by_ptr(new_val);
                m->name = dec->name;

                debug_vm_print(dec->pos, "declaration of %s, init value %s",
                    dec->name, debug_print_vm_value(new_val, dec->resolved_type));
            }
            else
            {
                byte *stack_val = push_identifier_on_stack(dec->name, dec->resolved_type);

                vm_value_meta *m = get_metadata_by_ptr(stack_val);
                m->name = dec->name;

                debug_vm_print(dec->pos, "declaration of %s, no init value", dec->name);
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
                eval_binary_op(new_val, op, old_val, new_val, old_val_t, new_val_t);

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
            debug_vm_print(st->pos, "SWITCH - start");

            type *cond_type = st->switch_stmt.var_expr->resolved_type;
            assert(cond_type);

            size_t cond_var_size = get_type_size(cond_type);
            byte *cond_var = eval_expression(st->switch_stmt.var_expr);
            int64_t cond_var_value = *(int64_t *)cond_var;

            debug_vm_print(st->pos, "SWITCH - condition evaluated as: %s", debug_print_vm_value(cond_var, cond_type));

            if (cond_var_value == 1)
            {
                debug_breakpoint;
            }

            int64_t case_index_to_eval = -1;
            for (size_t i = 0; i < st->switch_stmt.cases_num; i++)
            {
                switch_case *c = st->switch_stmt.cases[i];
                if (c->is_default)
                {
                    case_index_to_eval = i;
                    break;
                }
                
                for (size_t j = 0; j < c->cond_exprs_num; j++)
                {
                    int64_t val = c->cond_exprs_vals[j];                                        
                    if (val == cond_var_value)
                    {
                        case_index_to_eval = i;
                        break;
                    }                    
                }

                if (case_index_to_eval != -1)
                {
                    break;
                }
            }

            while (case_index_to_eval != -1)
            {
                switch_case *c = st->switch_stmt.cases[case_index_to_eval];                
                eval_statement_block(c->stmts, opt_ret_value);
                
                if (return_func)
                {
                    debug_vm_print(st->pos, "SWITCH - return");
                    break;
                }

                if (break_loop)
                {
                    debug_vm_print(st->pos, "SWITCH - break");
                    // wewnątrz switch case traktujemy to jako przerwanie pętli - chyba inaczej niż w c...
                    //break_loop = false;
                    break;
                }

                // co z continue?
                
                if (c->fallthrough)
                {
                    if (case_index_to_eval + 1 < st->switch_stmt.cases_num)
                    {
                        case_index_to_eval++;
                    }
                    else
                    {
                        case_index_to_eval = -1;
                    }
                }
                else
                {
                    case_index_to_eval = -1;
                }
            }

            debug_vm_print(st->pos, "SWITCH - end");
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
            if (st->delete.expr->kind == EXPR_STUB)
            {
                eval_stub_expression(null, st->delete.expr);
            }
            else
            {
                byte *obj = eval_expression(st->delete.expr);
                uintptr_t ptr = *(uintptr_t *)obj;
                if (ptr != 0)
                {              
                    debug_vm_print(st->pos, "free allocation at: %p", (void *)ptr);
                    ___free___((void *)ptr);
                }
            }
        }
        break;
        case STMT_INC:
        {
            assert(st->inc.operator == TOKEN_INC || st->inc.operator == TOKEN_DEC);
            assert(st->inc.operand->resolved_type);

            if (st->inc.operand->kind == EXPR_STUB)
            {
                eval_stub_expression(null, st->inc.operand);
            }
            else
            {
                byte *operand = eval_expression(st->inc.operand);
                type *operand_t = st->inc.operand->resolved_type;

                eval_unary_op(operand, st->inc.operator, operand, operand_t);

                debug_vm_print(st->pos, "operation %s, result %s",
                    get_token_kind_name(st->inc.operator),
                    debug_print_vm_value(operand, operand_t));
            }          
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
        source_pos pos = sym->decl->pos;
        switch (sym->kind)
        {
            case SYMBOL_VARIABLE:
            {
                assert(sym->decl->kind == DECL_VARIABLE);
                if (sym->decl->variable.expr)
                {
                    byte *result = eval_expression(sym->decl->variable.expr);
                    push_global_identifier(pos, sym->name, result, size);
                }
                else
                {
                    push_global_identifier(pos, sym->name, 0, size);
                }
            }
            break;
            case SYMBOL_CONST:
            {
                push_global_identifier(pos, sym->name, (byte *)&sym->val, size);
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

void run_interpreter(symbol **resolved_decls)
{
    if (buf_len(errors) > 0)
    {
        return;
    }
    
    ___gc_init___();

#if DEBUG_BUILD
    printf("\n=== TREEWALK INTERPRETER RUN ===\n\n");
#endif

    eval_global_declarations(resolved_decls);

    symbol *main = get_entry_point(resolved_decls);
    assert(main);

    if (buf_len(errors) > 0)
    {
        print_errors_to_console();
        return;
    }

    eval_function(main, null);

#if DEBUG_BUILD
    printf("\n=== FINISHED INTERPRETER RUN ===\n\n");

    if (failed_asserts > 0)
    {
        fatal("\nNumber of failed assertions: %d\n", failed_asserts);
    }
#endif

    ___clean_memory___();

    debug_breakpoint;
}