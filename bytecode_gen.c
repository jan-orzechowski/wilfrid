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

void print_ops(void)
{
    for (size_t i = 0; i < buf_len(code); i++)
    {
        printf("%04d - %s\n", code[i], op_names[code[i]]);
    }
}

void run_vm(op *code);

void bytecode_gen_test(void)
{
    emit_op(OP_PUSH, 0);
    emit_op(12, 0);
    emit_op(OP_PUSH, 0);
    emit_op(3, 0);
    emit_op(OP_INT_ADD, 0);
    emit_op(OP_RETURN, 0);
    emit_op(OP_HALT, 0);

    print_ops();

    run_vm(code);
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
    while (opcode && ip < code_size)
    {
        switch (opcode)
        {
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
                free(stack);
                return;
            }
            break;
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