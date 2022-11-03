#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

void* xrealloc(void* ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr)
    {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void* xmalloc(size_t num_bytes)
{
    void* ptr = malloc(num_bytes);
    if (!ptr)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    
    va_end(args);
    exit(1);
}

typedef struct buffer_header
{
    size_t len;
    size_t cap;
    char buf[0];
} 
buffer_header;

// offsetof zwraca pozycję pola w ramach structu
// chodzi o to, że z buffer_header korzystamy tak, jak ze zwykłego void* 
// ale potajemnie mamy zaalokowane zawsze trochę więcej, z przodu
#define __buf_header(b) ((buffer_header*)((char*)(b) - offsetof(buffer_header, buf)))
#define buf_len(b) ((b) ? __buf_header(b)->len : 0)
#define buf_cap(b) ((b) ? __buf_header(b)->cap : 0)

#define __buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define __buf_fit(b, n) (__buf_fits((b), (n)) ? 0 : ((b) = __buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_push(b, x) (__buf_fit((b), 1), (b)[__buf_header(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(__buf_header(b)), (b) = NULL) : 0)

#define debug_breakpoint {int x = 0;}

// do debugowania - w watch window makra nie działają...
buffer_header* __get_buf_header(void* ptr)
{
   buffer_header* header = (buffer_header*)((char*)ptr - offsetof(buffer_header, buf));
   return header;
}

void* __buf_grow(const void* buf, size_t new_len, size_t elem_size)
{
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(buffer_header, buf) + new_cap * elem_size;
    buffer_header* new_hdr;
    if (buf)
    {
        new_hdr = xrealloc(__buf_header(buf), new_size);
    }
    else
    {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

typedef struct intern_str
{
    size_t len;
    char* ptr;
}
intern_str;

intern_str* interns;

const char* str_intern_range(const char* start, const char* end)
{
    size_t len = end - start;
    for (size_t i = 0; i < buf_len(interns); i++)
    {
        // musimy wcześniej sprawdzić len, by uniknąć sytuacji, w której tylko prefix się zgadza - strncmp nie sprawdza długości
        if (interns[i].len == len && strncmp(interns[i].ptr, start, len) == 0)
        {
            return interns[i].ptr;
        }
    }

    // jeśli nie znaleźliśmy
    char* new_str = xmalloc(len + 1);
    memcpy(new_str, start, len);
    new_str[len] = 0; // upewniamy się, że mamy null terminator
    
    buf_push(interns, ((intern_str){ .len = len, .ptr = new_str}));
    
    return new_str;
}

const char* str_intern(const char* str)
{
    return str_intern_range(str, str + strlen(str));
}

void intern_str_test()
{
    char x[] = "hello";
    char y[] = "hello";

    assert(x != y);

    const char* px = str_intern(x);
    const char* py = str_intern(y);
    assert(px == py);
    
    char z[] = "hello!"; 
    const char* pz = str_intern(z);
    assert(pz != px);
}

void stretchy_buffers_test()
{
    intern_str* str = 0;

    assert(buf_len(str) == 0);

    for (size_t index = 0; index < 1024; index++)
    {
        buf_push(str, ((intern_str){ index, NULL }));
    }

    assert(buf_len(str) == 1024);

    for (size_t index = 0; index < buf_len(str); index++)
    {
        assert(str[index].len == index);
    }

    buf_free(str);

    assert(buf_len(str) == 0);
}

typedef enum token_kind
{
    TOKEN_EOF = 0,
    // pierwsze 128 wartości jest dla characters
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV
    // ...
} 
token_kind;

typedef struct tok
{
    token_kind kind;
    const char* start;
    const char* end;
    union
    {
        int val;
        const char* name;
    };
}  
tok;

char* stream;
tok token;
tok** all_tokens;
int lexed_token_index;

void next_token()
{
    bool discard_token = false;
    token.start = stream;
    switch (*stream)
    {
        case '0': case '1': case '2': case '3': case '4': case '5': 
        case '6': case '7': case '8': case '9':
        {
            // idziemy po następnych
            int val = 0;
            while (isdigit(*stream))
            {
                val *= 10; // dotychczasową wartość traktujemy jako 10 razy większą - bo znaleźliśmy kolejne miejsce dziesiętne
                val += *stream++ - '0'; // przerabiamy char na integer
            }
            stream--; // w ostatnim przejściu pętli posunęliśmy się o 1 za daleko
            token.kind = TOKEN_INT;
            token.val = val;
        }
        break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': 
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
        case 's': case 't': case 'u': case 'v': case 'w': case 'x': 
        case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': 
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
        case 'Y': case 'Z':
        case '_':
        {
            // zaczęliśmy od litery - dalej idziemy po cyfrach, literach i _
            while (isalnum(*stream) || *stream == '_')
            {
                stream++;
            }
            token.kind = TOKEN_NAME;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '+':
        {
            token.kind = TOKEN_ADD;
        }
        break;
        case '-':
        {
            token.kind = TOKEN_SUB;
        }
        break;
        case '*':
        {
            token.kind = TOKEN_MUL;
        }
        break;
        case '/':
        {
            token.kind = TOKEN_DIV;
        }
        break;
        case '(': 
        {
            token.kind = TOKEN_LEFT_PAREN;
        }
        break;
        case ')':
        {
            token.kind = TOKEN_RIGHT_PAREN;
        }
        break;
        case ' ':
        {
            discard_token = true;
        }
        break;
        default:
        {
            token.kind = *stream++;
        }
        break;
    }

    token.end = stream;
    stream++; 

    if (false == discard_token)
    {
        tok* new_tok = xmalloc(sizeof(token));
        memcpy(new_tok, &token, sizeof(token));
        buf_push(all_tokens, new_tok);
    }
}

void init_stream(const char* str)
{
    stream = str;
    buf_free(all_tokens);    
    next_token();
}

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

int* code;
int code_size;

void get_first_lexed_token()
{
    token = *all_tokens[0];
    lexed_token_index = 1;
}

void next_lexed_token()
{
    if (lexed_token_index + 1 < buf_len(all_tokens))
    {
        tok* next_token = all_tokens[lexed_token_index];
        if (next_token)
        {
            token = *next_token;
            lexed_token_index++;
        }
    }
    else
    {
        token.kind = TOKEN_EOF;
    }
}

bool is_token_kind(token_kind kind)
{
    bool result = (token.kind == kind);
    return result;
}

bool match_token_kind(token_kind kind)
{
    bool result = (token.kind == kind);
    if (result)
    {
        next_lexed_token();
    }
    return result;
}

bool expect_token_kind(token_kind kind)
{
    bool result = match_token_kind(kind);
    if (false == result)
    {
        fatal("expected token of different kind: %d", kind);
    }
    return result;
}

/*
    expr3 = INT | '(' expr ')'
    expr2 = { [+-] } expr2 | expr3
    expr1 = expr2 { [/*] expr2 }
    expr0 = expr1 { [+-] expr1 }
    expr = expr0
*/

void parse_expr();

void parse_expr3()
{
    if (is_token_kind(TOKEN_INT))
    {
        int value = token.val;
        next_lexed_token();
        buf_push(code, PUSH);
        buf_push(code, value);
    }
    else
    {
        if (is_token_kind(TOKEN_LEFT_PAREN))
        {
            next_lexed_token();
            parse_expr();
            if (expect_token_kind(TOKEN_RIGHT_PAREN))
            {
                // skończyliśmy
            }
        }
    }
}

void parse_expr2()
{
    if (is_token_kind(TOKEN_ADD) || is_token_kind(TOKEN_SUB))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr2();
        if (operation == TOKEN_SUB)
        {
            buf_push(code, PUSH);
            buf_push(code, -1);
            buf_push(code, MUL);
        }           
    }
    else
    {
        parse_expr3();
    }
}

void parse_expr1()
{
    parse_expr2();
    while (is_token_kind(TOKEN_MUL) || is_token_kind(TOKEN_DIV))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr2();
        if (operation == TOKEN_MUL)
        {
            buf_push(code, MUL);
        }
        else
        {
            buf_push(code, DIV);
        }
    }
}

void parse_expr0()
{
    parse_expr1();
    while (is_token_kind(TOKEN_ADD) || is_token_kind(TOKEN_SUB))
    {
        token_kind operation = token.kind;
        next_lexed_token();
        parse_expr1(); 
        if (operation == TOKEN_ADD)
        {
            buf_push(code, ADD);
        }
        else
        {
            buf_push(code, SUB);
        }
    }
}

void parse_expr()
{
    parse_expr0();
}

int* stack;
int stack_size;
int ip;
int sp;

int* vm_output;

void run_vm(int* code)
{
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

void stack_vm_test()
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
   
    code_size = buf_len(code);

    stack_size = 1024;
    stack = xmalloc(sizeof(int) * stack_size);

    run_vm(code);

    debug_breakpoint;

    buf_free(code);
    free(stack);
}

int parsing_test(char* expr, int value)
{
    init_stream(expr);
    while (token.kind)
    {
        next_token();
    }

    get_first_lexed_token();
    parse_expr();
    buf_push(code, POP);
    code_size = buf_len(code);

    stack_size = 1024;
    stack = xmalloc(sizeof(int) * stack_size);

    buf_free(vm_output);

    run_vm(code);

    assert(vm_output[0] == value);

    buf_free(code);
    buf_free(vm_output);
    free(stack);
}

#define PARSING_TEST(expr) { int val = (expr); parsing_test(#expr, val); } 

int main(int argc, char** argv)
{
    stretchy_buffers_test();
    intern_str_test();

    stack_vm_test();
    
    PARSING_TEST(2 + 2);
    PARSING_TEST((12    + 4) + 28 - 14 + (8 - 4) / 2 + (2 * 2 - 1 * 4));
    PARSING_TEST(2 +-2 / -2);

    return 0;
}