#include "utils.h"

int *stack;
int stack_size;
int ip;
int sp;

int *vm_output;

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
                ip = code[++ip] - 1;
            }
            break;
            case PUSH:
            {
                stack[++sp] = code[++ip];
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

        opcode = code[++ip];
    }

    debug_breakpoint;
}

void stack_vm_test(int *code)
{
    buf_push(code, PUSH);
    buf_push(code, 1);
    buf_push(code, PUSH);
    buf_push(code, 2);
    buf_push(code, ADD);
    buf_push(code, BRANCH);
    buf_push(code, 9);
    buf_push(code, ADD);
    buf_push(code, 4);
    buf_push(code, POP);

    stack_size = 1024;
    stack = xmalloc(sizeof(int)  *stack_size);

    run_vm(code);

    buf_free(code);
    free(stack);
}

void parsing_and_vm_test(char *expr, int value)
{
    lex(expr, null);

    //parse_expr();
    buf_push(code, POP);

    stack_size = 1024;
    stack = xmalloc(sizeof(int)  *stack_size);

    buf_free(vm_output);

    run_vm(code);

    assert(vm_output[0] == value);

    buf_free(code);
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