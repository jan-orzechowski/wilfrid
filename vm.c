#include "utils.h"

int* stack;
int stack_size;
int ip;
int sp;

int* vm_output;

void run_vm(int* code)
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
                stack[++sp] = b * a;
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

void stack_vm_test(int* code)
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
    stack = xmalloc(sizeof(int) * stack_size);

    run_vm(code);

    buf_free(code);
    free(stack);
}