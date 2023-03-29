// do przeniesienia do mojego języka

#if 0

typedef enum instruction
{
    HALT = 0,
    ADD,
    POP,
    PUSH,
    SUB,
    DIV,
    MUL,
    PRINT,
    BRANCH,
}
instruction;

int *stack;
int stack_size;
int ip;
int sp;

int *vm_output;

int *vm_code;

void run_vm(int *code)
{
    size_t code_size = buf_len(code);

    instruction opcode = (instruction)code[0];
    ip = 0;
    sp = -1;
    while (opcode && ip < code_size)
    {
        switch (opcode)
        {
            case BRANCH:
            {
                // argumentem jest miejsce, do którego mamy przeskoczyć
                ip = vm_code[++ip] - 1;
            }
            break;
            case PUSH:
            {
                stack[++sp] = vm_code[++ip];
            }
            break;
            case POP:
            {
                int value = stack[sp--];
                buf_push(vm_output, value);
            }
            break;
            case ADD:
            {
                int a = stack[sp--];
                int b = stack[sp--];
                stack[++sp] = b + a;
            }
            break;
            case SUB:
            {
                int a = stack[sp--];
                int b = stack[sp--];
                stack[++sp] = b - a;
            }
            break;
            case MUL:
            {
                int a = stack[sp--];
                int b = stack[sp--];
                stack[++sp] = b  *a;
            }
            break;
            case DIV:
            {
                int a = stack[sp--];
                int b = stack[sp--];
                stack[++sp] = b / a;
            }
            break;
            case HALT:
            {
                return;
            }
            break;
        }

        opcode = vm_code[++ip];
    }

    debug_breakpoint;
}

void stack_vm_test(void)
{
    buf_push(vm_code, PUSH);
    buf_push(vm_code, 1);
    buf_push(vm_code, PUSH);
    buf_push(vm_code, 2);
    buf_push(vm_code, ADD);
    buf_push(vm_code, BRANCH);
    buf_push(vm_code, 9);
    buf_push(vm_code, ADD);
    buf_push(vm_code, 4);
    buf_push(vm_code, POP);

    stack_size = 1024;
    stack = xmalloc(sizeof(int)  *stack_size);

    run_vm(vm_code);

    buf_free(vm_code);
    free(stack);
}

void parsing_and_vm_test(char *expr, int value)
{
    lex(expr, null);

    //parse_expr();
    buf_push(vm_code, POP);

    stack_size = 1024;
    stack = xmalloc(sizeof(int)  *stack_size);

    buf_free(vm_output);

    run_vm(vm_code);

    assert(vm_output[0] == value);

    buf_free(vm_code);
    buf_free(vm_output);
    free(stack);
}

#define PARSING_TEST(expr) { int val = (expr); parsing_and_vm_test(#expr, val); } 

void vm_test(void)
{
    PARSING_TEST(2 + 2);
    PARSING_TEST((12    + 4) + 28 - 14 + (8 - 4) / 2 + (2 * 2 - 1 * 4));
    PARSING_TEST(2 +-2 / -2);
}

#endif