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
#elif
#define save_debug_string
#define debug_vm_print
#endif

typedef void *address;

typedef struct vm_value
{
    type *type;
    union
    {
        uint32_t uint_value;
        int32_t int_value;
        uint64_t long_value;
        float float_value;
        void *ptr_value;
    };
} vm_value;

typedef struct vm_stack_value
{
    const char *name;
    vm_value val;
} vm_stack_value;

memory_arena *vm_global_memory;
hashmap global_identifiers;

/*
    do przemyślenia... jak to przechowywać? czy alokować tu jeszcze vm_value osobno?
*/
address define_global_identifier(const char *name, size_t size) // czy podawać typ?
{
    // to będzie trafione jeśli dwa razy występuje taki sam const string w kodzie źródłowym...
    assert(0 == map_get(&global_identifiers, (void *)name));

    void *result = push_size(vm_global_memory, size);
    map_put(&global_identifiers, (void *)name, result);

#if DEBUG_BUILD
    map_put(&debug_names_dict, result, (void *)name);
#endif

    return (address)result;
}

enum
{
    MAX_VM_STACK_LENGTH = 2048
};

vm_stack_value vm_stack[MAX_VM_STACK_LENGTH];
vm_stack_value *last_vm_stack_value = vm_stack;

vm_stack_value *enter_vm_stack_scope(void)
{
    vm_stack_value *marker = last_vm_stack_value;
    return marker;
}

void leave_vm_stack_scope(vm_stack_value *marker)
{
    assert(marker >= vm_stack);
    assert(marker <= last_vm_stack_value);
    for (vm_stack_value *val_to_clean = marker + 1; // samego markera nie czyścimy
        val_to_clean <= last_vm_stack_value;
        val_to_clean++)
    {
        *val_to_clean = (vm_stack_value){0};
    }
    last_vm_stack_value = marker;
}

vm_value *define_identifier_on_stack(const char *name)
{    
    assert_is_interned(name);
    vm_value *result = null;
    int64_t stack_size = last_vm_stack_value - vm_stack;
    if (stack_size + 1 < MAX_VM_STACK_LENGTH)
    {        
        last_vm_stack_value++;
        last_vm_stack_value->name = name;
        result = &last_vm_stack_value->val;
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
    assert_is_interned(name); 
    if (vm_stack != last_vm_stack_value)
    {
        // pierwsza wartość to zawsze null ze względu na to jak działa define_identifier_on_stack
        // nieeleganckie, pomyśleć nad tym
        for (vm_stack_value *var = vm_stack + 1;
            var <= last_vm_stack_value;
            var++)
        {
            assert(var->name);
            assert(var->val.type);
            if (var->name == name)
            {
                return &var->val;
            }
        }
    }
    fatal("no variable with name '%s' on the stack", name);
    return null;
}

vm_value eval_expression(expr *exp)
{
    vm_value result = {0};
    assert(exp->resolved_type);
    result.type = exp->resolved_type;

    assert(exp);
    switch (exp->kind)
    {
        case EXPR_INT:
        {    
            // działa niezależnie od konkretnego typu
            result.long_value = exp->number_value;
        }
        break;
        case EXPR_FLOAT:
        {
            // czy to jest dobrze?
            result.float_value = exp->number_value;
        }
        break;
        case EXPR_CHAR:
        case EXPR_STRING:
        {
            result.ptr_value = exp->string_value;
        }
        break;
        case EXPR_NULL:
        {
            result.ptr_value = null;
        }
        break;
        case EXPR_BOOL:
        {
            fatal("unimplemented");
        }
        break;

        case EXPR_NAME:
        {
            assert_is_interned(exp->name);
            result = *get_vm_variable(exp->name);
        }
        break;
        case EXPR_UNARY:
        {
            vm_value operand = eval_expression(exp->unary.operand);

            // co z innymi typami? np. float, ptr, ulong itd.

            result.long_value = eval_int_unary_op(exp->unary.operator, operand.long_value);

            debug_vm_print(exp->pos, "operation %s, result %lld", get_token_kind_name(exp->unary.operator), result.long_value);
        }
        break;
        case EXPR_BINARY:
        {
            vm_value left = eval_expression(exp->binary.left);
            vm_value right = eval_expression(exp->binary.right);

            // co z innymi typami? np. float, ptr, ulong itd.

            result.long_value = eval_int_binary_op(exp->binary.operator, left.long_value, right.long_value);

            debug_vm_print(exp->pos, "operation %s, result %lld", get_token_kind_name(exp->binary.operator), result.long_value);
        }
        break;
        case EXPR_TERNARY:
        {
            vm_value val = eval_expression(exp->ternary.condition);
            if (val.uint_value != 0)
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
            fatal("unimplemented");
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
            fatal("unimplemented");
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

void eval_statement(stmt *st);

void eval_statement_block(stmt_block block)
{
    vm_stack_value *marker = enter_vm_stack_scope();
    for (size_t i = 0; i < block.stmts_count; i++)
    {
        debug_breakpoint;
        eval_statement(block.stmts[i]);
    }
    leave_vm_stack_scope(marker);
}

#define copy_val(addr, val) (*(vm_value *)(addr) = (val))

void eval_statement(stmt *st)
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
            fatal("unimplemented");
        }
        break;
        case STMT_BREAK:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_CONTINUE:
        {
            fatal("unimplemented");
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
                address memory_addr = define_identifier_on_stack(dec->name);
                assert(memory_addr);

                vm_value val = eval_expression(dec->variable.expr);
                // na razie bez structów na stacku
                assert(val.type->kind != TYPE_STRUCT);
                assert(val.type->kind != TYPE_UNION);
                
                copy_val(memory_addr, val);

                debug_vm_print(dec->pos, "declaration of %s, init value %lld", dec->name, val.long_value);
            }
            else
            {
                fatal("shouldn't be possible");
            }
        }
        break;
        case STMT_IF_ELSE:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_WHILE:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_DO_WHILE:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_FOR:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_ASSIGN:
        {   
            vm_value new_val = { 0 };
            assert(is_assign_operation(st->assign.operation));
            new_val = eval_expression(st->assign.value_expr);
            if (st->assign.operation != TOKEN_ASSIGN)
            {                
                new_val = eval_expression(st->assign.value_expr);
                vm_value old_val = eval_expression(st->assign.assigned_var_expr);

                token_kind op = get_assignment_operation_token(st->assign.operation);

                new_val.long_value = eval_int_binary_op(op, old_val.long_value, new_val.long_value);

                debug_vm_print(st->assign.assigned_var_expr->pos, 
                    "operation %s for assignment, result %lld", get_token_kind_name(op), new_val.long_value);
            }

            // a co, jesli tu będzie jakieś expr typu array[10]->member.other_member?
            //get_vm_variable(st.a)
            //copy_val
        }
        break;
        case STMT_SWITCH:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_EXPR:
        {
            fatal("unimplemented");
        }
        break;
        case STMT_BLOCK:
        {
            fatal("unimplemented");
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
            {
                size_t size = get_type_size(sym->type);
                address memory = define_global_identifier(sym->name, size);
                (*(uintptr_t *)memory) = sym->val;
                assert(sizeof(sym->val) == sizeof(uintptr_t));
            }
            break;
            case SYMBOL_CONST:
            {
                size_t size = get_type_size(sym->type);
                address memory = define_global_identifier(sym->name, size);
                (*(uintptr_t *)memory) = sym->val;
                assert(sizeof(sym->val) == sizeof(uintptr_t));
            }
            break;
        }
    }
}

void eval_symbol(symbol *sym)
{
    assert(sym);
    assert(sym->state == SYMBOL_RESOLVED);
    switch (sym->kind)
    {      
        case SYMBOL_VARIABLE:
        {
            size_t size = get_type_size(sym->type);
            address memory = define_global_identifier(sym->name, size);
            (*(uintptr_t *)memory) = sym->val;
            assert(sizeof(sym->val) == sizeof(uintptr_t));
        }
        break;
        case SYMBOL_CONST:
        {
            size_t size = get_type_size(sym->type);
            address memory = define_global_identifier(sym->name, size);
            (*(uintptr_t *)memory) = sym->val;
            assert(sizeof(sym->val) == sizeof(uintptr_t));
        }
        break;
        case SYMBOL_FUNCTION:
        {
            eval_statement_block(sym->decl->function.stmts);
        }
        break;
        case SYMBOL_TYPE:
        {
            // deklaracja uniona / structu - nie musimy nic robić
        }
        break;
        case SYMBOL_NONE:
        invalid_default_case;
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
    parse_file("test/treewalk.txt", &all_declarations);
    symbol **resolved = resolve(all_declarations, true);
    assert(all_declarations);
    assert(resolved);

    shorten_source_pos = true;

    eval_global_declarations(resolved);

    symbol *main = get_entry_point(resolved);
    assert(main);
    eval_symbol(main);

    shorten_source_pos = false;

    debug_breakpoint;
}