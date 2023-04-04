typedef uint8_t op;

typedef enum op_kind
{
    OP_RETURN = 0,
    OP_HALT = 0,
    OP_POP,
    OP_PUSH,

    OP_INT_ADD,
    OP_INT_SUB,
    OP_INT_DIV,
    OP_INT_MUL,
    OP_INT_MOD,

    OP_PRINT,
    OP_BRANCH,
    OP_CONSTANT,
} op_kind;

#define op_name_macro(name) [ name ] = #name
const char *op_names[] = {
    op_name_macro(OP_RETURN),
    op_name_macro(OP_HALT),
    op_name_macro(OP_POP),
    op_name_macro(OP_PUSH),
    op_name_macro(OP_INT_ADD),
    op_name_macro(OP_INT_SUB),
    op_name_macro(OP_INT_DIV),
    op_name_macro(OP_INT_MUL),
    op_name_macro(OP_INT_MOD),
    op_name_macro(OP_PRINT),
    op_name_macro(OP_BRANCH),
    op_name_macro(OP_CONSTANT),
};
#undef op_name_macro

op *code;
int *lines; // odpowiadające instrukcjom - potem zmienić na run-length encoding

int current_line;

void emit_op(op o, int line)
{
    buf_push(code, o);
    buf_push(lines, line);
}

void emit_ops(op o1, int line1, op o2, int line2)
{
    buf_push(code, o1);
    buf_push(code, o2);
    buf_push(lines, line1);
    buf_push(lines, line2);
}

enum { MAX_CONSTANTS = 256 };
int64_t constants[MAX_CONSTANTS];
uint8_t constants_count;

void print_ops(void)
{
    for (size_t i = 0; i < buf_len(code); i++)
    {
        printf("%04d - %s\n", code[i], op_names[code[i]]);
        if (code[i] == OP_CONSTANT)
        {
            i++;
            assert(i < buf_len(code));
            printf("cnst - %d - %lld\n", code[i], constants[code[i]]);
        }
    }
}

uint8_t add_constant(int64_t value)
{
    if (constants_count >= MAX_CONSTANTS)
    {
        fatal("Codegen: index in constants array exceeds the maximum of %d", MAX_CONSTANTS);
    }
    int value_index = constants_count;
    constants[value_index] = value;
    constants_count++;
    return value_index;
}

int *stack;
int stack_size;
int ip;
int sp;

int *vm_output;

#define pop() stack[sp--]
#define push(val) stack[++sp] = val

// trik z do while jest wykorzystany po to, by lista statementów mogła być wykorzystana także w miejscu
// w którym można dać jedynie pojedynczy statement
#define binary_op(type, op) \
    do { \
        type a = stack[sp--];\
        type b = stack[sp--];\
        stack[++sp] = b op a;\
        printf("\noperation %s, result %d\n", #op, stack[sp]);\
    } while (false)

#define binary_op_case(type, op) { binary_op(type, op); } break;

void run_vm(op *code)
{
    stack_size = 1024;
    stack = xcalloc(stack_size);
    
    size_t code_size = buf_len(code);

    op opcode = (op_kind)code[0];
    ip = 0;
    sp = -1;

    printf("\n=== VM RUN: === \n");
    while (opcode && ip < code_size)
    {
        switch (opcode)
        {
            case OP_CONSTANT:
            {
                ip++;
                size_t index = code[ip];
                if (index > MAX_CONSTANTS)
                {
                    fatal("Execution: index in constants array exceeds the maximum of %d", MAX_CONSTANTS);
                    break;
                }
                int64_t value = constants[index];
                push(value);
                
                printf("\nconstant: %lld\n", value);
            } 
            break;
            case OP_BRANCH:
            {
                // argumentem jest miejsce, do którego mamy przeskoczyć
                ip = code[++ip] - 1;
            }
            break;
            case OP_PUSH:
            {
                stack[++sp] = code[++ip];
            }
            break;
            case OP_POP:
            {
                int value = stack[sp--];
                buf_push(vm_output, value);
            }
            break;
            case OP_INT_ADD: binary_op_case(int, +);        
            case OP_HALT:
            {
                break;
            }
            break;
            invalid_default_case;
        }

        opcode = code[++ip];
    }

    debug_breakpoint;

    free(stack);
}

#undef binary_op
#undef binary_op_case
#undef pop
#undef push

void emit_operator(token_kind operator, int line)
{
    switch (operator)
    {
        case TOKEN_ADD: emit_op(OP_INT_ADD, line); break;
        case TOKEN_SUB: emit_op(OP_INT_SUB, line); break;
        case TOKEN_MUL: emit_op(OP_INT_MUL, line); break;
        case TOKEN_DIV: emit_op(OP_INT_DIV, line); break;
        case TOKEN_MOD: emit_op(OP_INT_MOD, line); break;
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
            emit_op(OP_CONSTANT, exp->pos.line);
            emit_op(add_constant(exp->number_value), exp->pos.line);
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
            fatal("unimplemented");
        }
        break;
        case EXPR_UNARY:
        {
            fatal("unimplemented");
        }
        break;
        case EXPR_BINARY:
        {
            emit_expression(exp->binary.left);
            emit_expression(exp->binary.right);
            emit_operator(exp->binary.operator, exp->pos.line);
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
        }
        break;
        case DECL_CONST:
        {
            debug_breakpoint;
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
            emit_op(OP_CONSTANT, 0);
            emit_op(add_constant(sym->val), 0);
        } 
        break;
        case SYMBOL_FUNCTION: 
        {
            for (size_t i = 0; i < sym->decl->function.stmts.stmts_count; i++)
            {
                stmt *st = sym->decl->function.stmts.stmts[i];
                emit_statement(st);
            }
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
    emit_op(OP_CONSTANT, 0);
    emit_op(add_constant(12), 0);
    emit_op(OP_CONSTANT, 0);
    emit_op(add_constant(3), 0);
    emit_op(OP_INT_ADD, 0);
    emit_op(OP_RETURN, 0);
    emit_op(OP_HALT, 0);

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
        "fn main() { let x := 42 + 1 }"
    };
    size_t str_count = sizeof(test_strs) / sizeof(test_strs[0]);
    symbol **resolved = test_resolve_decls(test_strs, str_count, false);

    for (size_t i = 0; i < buf_len(resolved); i++)
    {
        symbol *sym = resolved[i];
        emit_symbol(sym);
    }
    emit_op(OP_HALT, 0);

    printf("\nEmitted instructions:\n\n");
    print_ops();
    printf("\n");

    run_vm(code);
#endif
}