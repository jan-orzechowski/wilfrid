typedef uint64_t word;
typedef uint64_t op;

typedef enum op_kind
{
    OP_RETURN = 0,
    OP_HALT = 0,

    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,

    OP_UNARY_ADD,
    OP_UNARY_SUB,
    OP_UNARY_NOT,
    OP_UNARY_BITWISE_NOT,

    OP_ADD,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_MOD,
    OP_XOR, // ^
    OP_BITWISE_OR, // |
    OP_BITWISE_AND, // &
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,

    OP_EQ, // ==
    OP_NEQ, // !=
    OP_GT, // >
    OP_LT, // <
    OP_GEQ, // >=
    OP_LEQ, // <=
    OP_AND,
    OP_OR,

    OP_NULL,
    OP_FALSE,
    OP_TRUE,

    OP_JUMP_IF_FALSE,
    OP_PRINT,
    OP_STORE,
} op_kind;

#define op_name_macro(name) [ name ] = #name
const char *op_names[] = {
    op_name_macro(OP_RETURN),
    op_name_macro(OP_HALT),
    op_name_macro(OP_SET_GLOBAL),
    op_name_macro(OP_GET_GLOBAL),
    op_name_macro(OP_SET_LOCAL),
    op_name_macro(OP_GET_LOCAL),

    op_name_macro(OP_UNARY_ADD),
    op_name_macro(OP_UNARY_SUB),
    op_name_macro(OP_UNARY_NOT),
    op_name_macro(OP_UNARY_BITWISE_NOT),

    op_name_macro(OP_ADD),
    op_name_macro(OP_SUB),
    op_name_macro(OP_DIV),
    op_name_macro(OP_MUL),
    op_name_macro(OP_MOD),
    op_name_macro(OP_XOR),
    op_name_macro(OP_BITWISE_OR),
    op_name_macro(OP_BITWISE_AND),
    op_name_macro(OP_LEFT_SHIFT),
    op_name_macro(OP_RIGHT_SHIFT),

    op_name_macro(OP_EQ),
    op_name_macro(OP_NEQ),
    op_name_macro(OP_GT),
    op_name_macro(OP_LT),
    op_name_macro(OP_GEQ),
    op_name_macro(OP_LEQ),
    op_name_macro(OP_AND),
    op_name_macro(OP_OR),

    op_name_macro(OP_NULL),
    op_name_macro(OP_FALSE),
    op_name_macro(OP_TRUE),

    op_name_macro(OP_PRINT),
    op_name_macro(OP_JUMP_IF_FALSE),
    op_name_macro(OP_STORE),
};
#undef op_name_macro

#if DEBUG_BUILD
#define assert_is_interned(ptr) assert(str_intern((const char *)ptr) == (const char *)ptr)
#define save_debug_string(str) map_put(&debug_strings, str, str);
#elif
#define assert_is_interned 
#define save_debug_string
#endif

word *code;
int *lines; // odpowiadające instrukcjom - potem zmienić na run-length encoding

int current_line;

void emit(word w, int line)
{
    buf_push(code, w);
    buf_push(lines, line);
}

void emit_words(op o1, int line1, op o2, int line2)
{
    buf_push(code, o1);
    buf_push(code, o2);
    buf_push(lines, line1);
    buf_push(lines, line2);
}

void print_ops(void)
{
    for (size_t i = 0; i < buf_len(code); i++)
    {
        printf("%08lld - %s\n", code[i], op_names[code[i]]);
        if (code[i] == OP_STORE)
        {
            i++;
            assert(i < buf_len(code));
            printf("store    - %lld\n", code[i]);
        }
    }
}

memory_arena *globals_memory;
memory_arena *locals_memory;
// heap to po prostu xcalloc / free

hashmap globals_dict;
hashmap locals_dict;

#if DEBUG_BUILD
hashmap globals_names_dict;
hashmap locals_names_dict;
hashmap debug_strings;
#endif

void *define_global(char *name, size_t size, void *initial_value)
{
    void *result = push_size(globals_memory, size);    
    if (initial_value)
    {
        memcpy_s(result, size, initial_value, size);       
    }
    else
    {
        memset(result, 0, size);
    }
    
    map_put(&globals_dict, (void *)name, result);

#if DEBUG_BUILD
    map_put(&globals_names_dict, result, (void *)name);
    map_put(&debug_strings, (void *)name, (void *)name);
#endif

    return result;
}

void *define_local(char *name, size_t size, void *initial_value)
{
    void *result = push_size(locals_memory, size);
    if (initial_value)
    {
        memcpy_s(result, size, initial_value, size);
    }
    else
    {
        memset(result, 0, size);
    }

    map_put(&locals_dict, (void *)name, result);

#if DEBUG_BUILD
    map_put(&locals_names_dict, result, (void *)name);
    map_put(&debug_strings, (void *)name, (void *)name);
#endif

    return result;
}


// te cztery funkcje poniżej są bezużyteczne - nie będziemy mieli name!
void set_global(char *name, size_t size, void* value)
{
    void *result = map_get(&globals_dict, name);
    assert(result);
    memcpy_s(result, size, value, size);
}

void set_local(char *name, size_t size, void *value)
{
    void *result = map_get(&locals_dict, name);
    assert(result);
    memcpy_s(result, size, value, size);
}

void *get_global(void *name)
{
    void *result = map_get(&globals_dict, name);
    return result;
}

void *get_local(void *name)
{
    void *result = map_get(&locals_dict, name);
    return result;
}

word *stack;
size_t stack_size;
ptrdiff_t ip;
ptrdiff_t sp;

#define exec_printf(str, ...) printf(str, __VA_ARGS__); print_stack();

void print_stack(void)
{
    printf("\n        [");
    if (sp >= 0)
    {
        for (word *ptr = stack; ptr != stack + sp + 1; ptr++)
        {
            printf("%lld", *ptr);
            if (ptr != stack + sp)
            {
                printf(", ");
            }
        }
    }
    printf("]");
}

#define stack_pop() stack[sp--]
#define stack_push(val) stack[++sp] = val

// trik z do while jest wykorzystany po to, by lista statementów mogła być wykorzystana także w miejscu
// w którym można dać jedynie pojedynczy statement
#define unary_op(type, op) \
    do { \
        type x = stack[sp--]; \
        stack[++sp] = op x; \
        exec_printf("\nunary operation %s, result %lld", #op, stack[sp]); \
    } while (false)

#define binary_op(type, op) \
    do { \
        type a = stack[sp--]; \
        type b = stack[sp--]; \
        stack[++sp] = b op a; \
        exec_printf("\nbinary operation %s, result %lld", #op, stack[sp]); \
    } while (false)

#define unary_op_case(type, op) { unary_op(type, op); } break;
#define binary_op_case(type, op) { binary_op(type, op); } break;

void run_vm(op *code)
{
    stack_size = 1024;
    stack = xcalloc(stack_size * sizeof(word));
    
    size_t code_size = buf_len(code);

    op opcode = (op_kind)code[0];
    ip = 0;
    sp = -1;

    printf("\n=== VM RUN START === \n");
    while (opcode && ip < code_size)
    {
        switch (opcode)
        {          
            case OP_STORE:
            {                
                word value = code[++ip];
                stack_push(value);
                exec_printf("\npush on stack: %lld", value);
            }
            break;
            case OP_JUMP_IF_FALSE:
            {                
                word offset = code[++ip];
                ip += offset;
                exec_printf("\njump by offset: %lld", offset);
            }
            break;
             
            case OP_GET_GLOBAL: { fatal("unimplemented"); }
            case OP_GET_LOCAL:
            {
                void *ptr = (void *)stack[sp--];
                word value = *(word *)ptr; // structów i innych nie możemy póki co wrzucać na stack...
                stack_push(value);
                
#if DEBUG_BUILD                
                char *debug_name = (char *)map_get(&locals_names_dict, ptr);
                if (map_get(&debug_strings, (void *)value))
                {
                    exec_printf("\nget variable: %s, value: %lld (as string: '%s')", debug_name, value, (char *)value);
                }
                else
                {
                    exec_printf("\nget variable: %s, value: %lld", debug_name, value);
                }
#endif
            }
            break;
            case OP_SET_GLOBAL: { fatal("unimplemented"); }
            case OP_SET_LOCAL:
            {
                void *ptr = (void *)stack[sp--];
                word value = stack[sp--];
                
                memcpy_s(ptr, sizeof(word), (void*)&value, sizeof(word));

#if DEBUG_BUILD
                char *debug_name = (char *)map_get(&locals_names_dict, ptr);
                if (map_get(&debug_strings, (void *)value))
                {
                    exec_printf("\nset variable: %s, value: %lld (as string: '%s')", debug_name, value, (char *)value);
                }
                else
                {
                    exec_printf("\nset variable: %s, value: %lld", debug_name, value);
                }
#endif
            }
            break;




            case OP_NULL:
            {
                word value = 0;
                stack_push(value);
                exec_printf("\npush on stack: %lld (null)", value);
            }
            break;
            case OP_FALSE:
            {
                word value = 0;
                stack_push(value);
                exec_printf("\npush on stack: %lld (false)", value);
            }
            break;
            case OP_TRUE:
            {
                word value = 1;
                stack_push(1);
                exec_printf("\npush on stack: %lld (true)", value);
            }
            break;

            case OP_UNARY_ADD: unary_op_case(int, +);
            case OP_UNARY_SUB: unary_op_case(int, -);
            case OP_UNARY_NOT: unary_op_case(int, !);
            case OP_UNARY_BITWISE_NOT: unary_op_case(int, ~);

            case OP_ADD: binary_op_case(int, +);
            case OP_SUB: binary_op_case(int, -);
            case OP_MUL: binary_op_case(int, *);
            case OP_DIV: binary_op_case(int, /);
            case OP_MOD: binary_op_case(int, %);
            case OP_XOR: binary_op_case(int, ^);
            case OP_BITWISE_OR: binary_op_case(int, |);
            case OP_BITWISE_AND: binary_op_case(int, &);
            case OP_LEFT_SHIFT: binary_op_case(int, <<);
            case OP_RIGHT_SHIFT: binary_op_case(int, >>);
            case OP_EQ: binary_op_case(int, ==); 
            case OP_NEQ: binary_op_case(int, !=);
            case OP_GT: binary_op_case(int, >);
            case OP_LT: binary_op_case(int, <);
            case OP_GEQ: binary_op_case(int, >=);
            case OP_LEQ: binary_op_case(int, <=);
            case OP_AND: binary_op_case(int, &&);
            case OP_OR: binary_op_case(int, ||);

            case OP_HALT:
            {
                break;
            }
            break;
            invalid_default_case;
        }

        opcode = code[++ip];
    }


    printf("\n\n=== VM RUN FINISHED === \n\n");

    debug_breakpoint;

    free(stack);
}

#undef unary_op
#undef unary_op_case
#undef binary_op
#undef binary_op_case
#undef pop
#undef push

void emit_unary_operator(token_kind operator, int line)
{
    switch (operator)
    {
        case TOKEN_ADD:         emit(OP_UNARY_ADD, line); break;
        case TOKEN_SUB:         emit(OP_UNARY_SUB, line); break;
        case TOKEN_NOT:         emit(OP_UNARY_NOT, line); break;
        case TOKEN_BITWISE_NOT: emit(OP_UNARY_BITWISE_NOT, line); break;
        default:                fatal("operation not implemented"); break;
    }
}

void emit_binary_operator(token_kind operator, int line)
{
    switch (operator)
    {
        case TOKEN_ADD:         emit(OP_ADD, line); break;
        case TOKEN_SUB:         emit(OP_SUB, line); break;
        case TOKEN_MUL:         emit(OP_MUL, line); break;
        case TOKEN_DIV:         emit(OP_DIV, line); break;
        case TOKEN_MOD:         emit(OP_MOD, line); break;
        case TOKEN_BITWISE_AND: emit(OP_BITWISE_AND, line); break;
        case TOKEN_BITWISE_OR:  emit(OP_BITWISE_OR, line); break;
        case TOKEN_LEFT_SHIFT:  emit(OP_LEFT_SHIFT, line); break;
        case TOKEN_RIGHT_SHIFT: emit(OP_RIGHT_SHIFT, line); break;
        case TOKEN_XOR:         emit(OP_XOR, line); break;
        case TOKEN_EQ:          emit(OP_EQ, line); break;
        case TOKEN_NEQ:         emit(OP_NEQ, line); break;
        case TOKEN_LT:          emit(OP_LT, line); break;
        case TOKEN_LEQ:         emit(OP_LEQ, line); break;
        case TOKEN_GT:          emit(OP_GT, line); break;
        case TOKEN_GEQ:         emit(OP_GEQ, line); break;
        case TOKEN_AND:         emit(OP_AND, line); break;
        case TOKEN_OR:          emit(OP_OR, line); break;
        default:                fatal("operation not implemented");  break;
    }
}

void emit_expression(expr *exp)
{
    assert(exp);
    switch (exp->kind)
    {
        case EXPR_NONE:
        {
            fatal("codegen: none is not supported expression kind");
        }
        break;
        case EXPR_INT:
        {
            emit(OP_STORE, exp->pos.line);
            emit(exp->number_value, exp->pos.line);
        }
        break;
        case EXPR_FLOAT:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_CHAR:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_STRING:
        {            
            size_t len = strlen(exp->string_value);                       
            void *ptr = define_local(exp->string_value, len, exp->string_value); // zmienić na global potem
            emit(OP_STORE, exp->pos.line);
            emit((word)ptr, exp->pos.line);

            save_debug_string(exp->string_value);
        }
        break;
        case EXPR_NULL:
        {
            emit(OP_NULL, exp->pos.line);
        }
        break;
        case EXPR_BOOL:
        {
            if (exp->bool_value)
            {
                emit(OP_TRUE, exp->pos.line);
            }
            else
            {
                emit(OP_FALSE, exp->pos.line);
            }
        }
        break;

        case EXPR_NAME:
        {
            assert_is_interned(exp->name);
            emit(OP_STORE, 0);

            void *ptr = get_local(exp->name);
            if (ptr == null)
            {
                ptr = define_local(exp->name, sizeof(word), 0);
            }
            assert(ptr);

            save_debug_string(exp->name);

            emit((word)ptr, 0);
            emit(OP_GET_LOCAL, 0);
        }
        break;
        case EXPR_UNARY:
        {
            emit_expression(exp->unary.operand);
            emit_unary_operator(exp->unary.operator, exp->pos.line);
        }
        break;
        case EXPR_BINARY:
        {
            emit_expression(exp->binary.left);
            emit_expression(exp->binary.right);
            emit_binary_operator(exp->binary.operator, exp->pos.line);
        }
        break;
        case EXPR_TERNARY:
        {
            fatal("unimplemented");
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
            fatal("unimplemented");
        }
        break;
        case EXPR_STUB:
        {
            fatal("unimplemented");
        }
        break;
        invalid_default_case;
    }
}

void emit_declaration(decl *de, bool is_global_scope)
{
    assert(de);
    switch (de->kind)
    {
        case DECL_STRUCT:
        {
            fatal("unimplemented");
        }
        break;
        case DECL_UNION:
        {
            fatal("unimplemented");
        }
        break;
        case DECL_VARIABLE:
        {
            if (is_global_scope)
            {
                
            }
          
            // skąd wziąc size, mając tylko decl?
            size_t size = 10;
            void *ptr = define_local(de->name, size, 0);

            emit_expression(de->variable.expr);
            emit(OP_STORE, de->pos.line);
            emit((word)ptr, de->pos.line);
            emit(OP_SET_LOCAL, de->pos.line);
        }
        break;
        case DECL_CONST:
        {
            fatal("unimplemented");
            //emit_expression(de->const_decl.expr);
            //emit(OP_STORE, 0);
            //emit((word)de->name, 0);
            //emit(OP_SET_LOCAL, 0); // tymczasowe, zmienić na GLOBAL potem
        }
        break;
        case DECL_FUNCTION:
        {
            fatal("unimplemented");
        }
        break;
        case DECL_ENUM:
        {
            fatal("unimplemented");
        }
        break;
    }
}

void emit_statement(stmt *st);

void emit_statement_block(stmt_block block)
{
    // tutaj potrzebne jest jeszcze coś dla zaznaczenia scope
    for (size_t i = 0; i < block.stmts_count; i++)
    {
        emit_statement(block.stmts[i]);
    }
}

void emit_statement(stmt *st)
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
            emit_declaration(st->decl_stmt.decl, false);
        }
        break;
        case STMT_IF_ELSE:
        {
            emit_expression(st->if_else.cond_expr);
            
            emit(OP_JUMP_IF_FALSE, 0);
            emit(null, 0);
            size_t backpatch = buf_len(code);

            if (st->if_else.else_stmt)
            {
                emit_statement(st->if_else.else_stmt);
            }

            size_t current_op = buf_len(code);
            size_t difference = (current_op - backpatch);
            if (difference > UINT16_MAX)
            {
                fatal("difference in ops too large");
            }
            //backpatch = add_constant(difference);

            emit_statement_block(st->if_else.then_block);
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
            assert(is_assign_operation(st->assign.operation));
            if (st->assign.operation == TOKEN_ASSIGN)
            {
                emit_expression(st->assign.value_expr);
                
                // tutaj poprzednio było emit_expression(st->assign.assigned_var_expr)
                // i to było źle
                // OP_SET_LOCAL potrzebuje adresu, a nie wartości! emit_expression dla nazw zawsze ściąga wartość
                // jak to lepiej rozwiązać?

                // tymczasowe
                if (st->assign.assigned_var_expr->name)
                {
                    assert_is_interned(st->assign.assigned_var_expr->name);
                    emit(OP_STORE, 0);
                    emit((word)st->assign.assigned_var_expr->name, 0);
                }
                else
                {
                    fatal("unimplemented");
                    //emit_expression(st->assign.assigned_var_expr);
                }
                emit(OP_SET_LOCAL, 0);
            }
            else
            {
                emit_expression(st->assign.value_expr);
                emit_expression(st->assign.assigned_var_expr);
                switch (st->assign.operation)
                {
                    case TOKEN_ADD_ASSIGN: { emit_binary_operator(TOKEN_ADD, 0); } break;
                    case TOKEN_SUB_ASSIGN: { emit_binary_operator(TOKEN_SUB, 0); } break;
                    case TOKEN_OR_ASSIGN: { emit_binary_operator(TOKEN_OR, 0); } break;
                    case TOKEN_AND_ASSIGN: { emit_binary_operator(TOKEN_AND, 0); } break;
                    case TOKEN_BITWISE_OR_ASSIGN: { emit_binary_operator(TOKEN_BITWISE_OR, 0); } break;
                    case TOKEN_BITWISE_AND_ASSIGN: { emit_binary_operator(TOKEN_BITWISE_AND, 0); } break;
                    case TOKEN_XOR_ASSIGN: { emit_binary_operator(TOKEN_XOR, 0); } break;
                    case TOKEN_LEFT_SHIFT_ASSIGN: { emit_binary_operator(TOKEN_LEFT_SHIFT, 0); } break;
                    case TOKEN_RIGHT_SHIFT_ASSIGN: { emit_binary_operator(TOKEN_RIGHT_SHIFT, 0); } break;
                    case TOKEN_MUL_ASSIGN: { emit_binary_operator(TOKEN_MUL, 0); } break;
                    case TOKEN_DIV_ASSIGN: { emit_binary_operator(TOKEN_DIV, 0); } break;
                    case TOKEN_MOD_ASSIGN: { emit_binary_operator(TOKEN_MOD, 0); } break;
                }

                // tymczasowe
                if (st->assign.assigned_var_expr->name)
                {
                    assert_is_interned(st->assign.assigned_var_expr->name);
                    emit(OP_STORE, 0);
                    emit((word)st->assign.assigned_var_expr->name, 0);
                }
                else
                {
                    fatal("unimplemented");
                    //emit_expression(st->assign.assigned_var_expr);
                }
                emit(OP_SET_LOCAL, 0);
            }            
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

void emit_symbol(symbol *sym)
{
    assert(sym->state == SYMBOL_RESOLVED);
    switch (sym->kind)
    {
        case SYMBOL_NONE: 
        {
            fatal("codegen: none is not supported symbol kind");
        } 
        break;
        case SYMBOL_VARIABLE: 
        {
            fatal("unimplemented");
        }
        break;
        case SYMBOL_CONST: 
        {
            assert(sym->type == type_int);
            emit(OP_STORE, 0);
            emit(sym->val, 0);
        } 
        break;
        case SYMBOL_FUNCTION: 
        {
            emit_statement_block(sym->decl->function.stmts);
        }
        break;
        case SYMBOL_TYPE: 
        {
            // czyli deklaracja uniona/structu
            // chyba nie musimy nic robić?

            //fatal("unimplemented");
        }
        break;
        case SYMBOL_ENUM_CONST: 
        {
            fatal("unimplemented");
        }
        break;
        invalid_default_case;
    }
}

symbol **test_resolve_decls(char **decl_arr, size_t decl_arr_count, bool print_ast);

void bytecode_gen_test_simple(void)
{
    emit(OP_STORE, 0);
    emit(12, 0);
    emit(OP_STORE, 0);
    emit(3, 0);
    emit(OP_ADD, 0);
    emit(OP_RETURN, 0);
    emit(OP_HALT, 0);

    print_ops();

    run_vm(code);
}

void bytecode_gen_test(void)
{
    globals_memory = allocate_memory_arena(kilobytes(5));
    locals_memory = allocate_memory_arena(kilobytes(5));

#if DEBUG_BUILD
    map_grow(&globals_names_dict, 16);
    map_grow(&locals_names_dict, 16);
    map_grow(&debug_strings, 16);
#endif

#if 0
    bytecode_gen_test_simple();
#else 
    char *test_strs[] = {
#if 0
        "fn m() { let mx := 2\
            mx = mx + 2} ",
        "const i = 1024",
        "fn f() { let x := 2 }",
        "fn n() { \
            let x := 42 + 1\
            let y := (12 / 3) * 3 + 2\
            x = y + 25 % (3 * 7 + 24 / 8)\
            x += x / 15 }",
        "fn r() { let x := \"Some string\" } ",
#endif
        "struct node { val: int }",
        "fn o () { let x : node* = null let y = (x == null) }",
        //"fn o () { let x: long = 100 let y: int = 20, x = y + 20 }"
#if 0
            if (x == 43) {\
                x += 57\
            }\
            if (x == 99) {\
                x -= 20\
            }\
            x = x + 1\
        }",
#endif
    };
    size_t str_count = sizeof(test_strs) / sizeof(test_strs[0]);
    symbol **resolved = test_resolve_decls(test_strs, str_count, false);

    for (size_t i = 0; i < buf_len(resolved); i++)
    {
        symbol *sym = resolved[i];
        emit_symbol(sym);
    }
    emit(OP_HALT, 0);

    printf("\nEmitted instructions:\n\n");
    print_ops();
    printf("\n");

    run_vm(code);
#endif
}