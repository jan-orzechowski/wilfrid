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

    OP_ADD,
    OP_SUB,
    OP_DIV,
    OP_MUL,
    OP_MOD,

    OP_UNARY_ADD,
    OP_UNARY_SUB,
    OP_UNARY_NOT,
    OP_UNARY_BITWISE_NOT,
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
    op_name_macro(OP_ADD),
    op_name_macro(OP_SUB),
    op_name_macro(OP_DIV),
    op_name_macro(OP_MUL),
    op_name_macro(OP_MOD),
    op_name_macro(OP_UNARY_ADD),
    op_name_macro(OP_UNARY_SUB),
    op_name_macro(OP_UNARY_NOT),
    op_name_macro(OP_UNARY_BITWISE_NOT),
    op_name_macro(OP_PRINT),
    op_name_macro(OP_JUMP_IF_FALSE),
    op_name_macro(OP_STORE),
};
#undef op_name_macro

#if DEBUG_BUILD
#define assert_is_interned(ptr) assert(str_intern((const char *)ptr) == (const char *)ptr)
#elif
#define assert_is_interned 
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

hashmap globals;

void set_global(char *name, int value_index)
{
    map_put(&globals, (void *)name, (void *)value_index);
}

void *get_global(void *name)
{
    void *result = map_get(&globals, name);
    assert(result);
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
            case OP_GET_GLOBAL:
            case OP_GET_LOCAL:
            {               
                word name_ptr = stack[sp--];
                word value = (word)map_get(&globals, (void *)name_ptr);
                exec_printf("\nget variable: %s, value: %lld", (char *)name_ptr, value);
            }
            break;
            case OP_SET_GLOBAL:
            case OP_SET_LOCAL:
            {
                word name_ptr = stack[sp--];
                word value = stack[sp--];
                
                set_global((void *)name_ptr, value);
                assert_is_interned(name_ptr);

                exec_printf("\nset variable: %s, value: %lld", (char *)name_ptr, value);
            }
            break;  
            case OP_ADD: binary_op_case(int, +);
            case OP_SUB: binary_op_case(int, -);
            case OP_MUL: binary_op_case(int, *);
            case OP_DIV: binary_op_case(int, /);
            case OP_MOD: binary_op_case(int, %);
            case OP_UNARY_ADD: unary_op_case(int, +);
            case OP_UNARY_SUB: unary_op_case(int, -);
            case OP_UNARY_NOT: unary_op_case(int, !);
            case OP_UNARY_BITWISE_NOT: unary_op_case(int, ~);
            case OP_HALT:
            {
                break;
            }
            break;
            invalid_default_case;
        }

        opcode = code[++ip];
    }


    printf("\n=== VM RUN FINISHED === \n\n");

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
        case TOKEN_ADD:         emit(OP_UNARY_ADD, line);            break;
        case TOKEN_SUB:         emit(OP_UNARY_SUB, line);            break;
        case TOKEN_NOT:         emit(OP_UNARY_NOT, line);            break;
        case TOKEN_BITWISE_NOT: emit(OP_UNARY_BITWISE_NOT, line);    break;
        default:                fatal("operation not implemented");     break;
    }
}

void emit_binary_operator(token_kind operator, int line)
{
    switch (operator)
    {
        case TOKEN_ADD: emit(OP_ADD, line); break;
        case TOKEN_SUB: emit(OP_SUB, line); break;
        case TOKEN_MUL: emit(OP_MUL, line); break;
        case TOKEN_DIV: emit(OP_DIV, line); break;
        case TOKEN_MOD: emit(OP_MOD, line); break;
        //case TOKEN_BITWISE_AND: return left & right;
        //case TOKEN_BITWISE_OR: return left | right;
        //case TOKEN_LEFT_SHIFT: return left << right;
        //case TOKEN_RIGHT_SHIFT: return left >> right;
        //case TOKEN_XOR: return left ^ right;
        //case TOKEN_EQ: return left == right;
        //case TOKEN_NEQ: return left != right;
        //case TOKEN_LT: return left < right;
        //case TOKEN_LEQ: return left <= right;
        //case TOKEN_GT: return left > right;
        //case TOKEN_GEQ: return left >= right;
        //case TOKEN_AND: return left && right;
        //case TOKEN_OR: return left || right;
        invalid_default_case;
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
            fatal("unimplemented");
        }
        break;
        case EXPR_NULL:
        {
            fatal("unimplemented");
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
            emit(OP_STORE, 0);
            emit((word)exp->name, 0);
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

void emit_declaration(decl *de)
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
            emit_expression(de->variable.expr);
            emit(OP_STORE, 0);
            emit((word)de->name, 0);
            emit(OP_SET_LOCAL, 0);
        }
        break;
        case DECL_CONST:
        {
            emit_expression(de->const_decl.expr);
            emit(OP_STORE, 0);
            emit((word)de->name, 0);
            emit(OP_SET_GLOBAL, 0);
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
            emit_declaration(st->decl_stmt.decl);
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
            fatal("unimplemented");
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
            fatal("unimplemented");
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
#if 0
    bytecode_gen_test_simple();
#else 
    char *test_strs[] = {
        "const i = 1024",
        "fn f() { let x := 2 }",
#if 0
        "fn main() { \
            let x := 42 + 1\
            let y := (12 / 3) * 3 + 2\
            let z := 25 % (3 * 7 + 24 / 8)\
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