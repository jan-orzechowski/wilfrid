#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

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
    // pierwsze 128 wartości jest dla characters
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_NAME,
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

void next_token()
{
    token.start = stream;
    switch (*stream)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            // idziemy po następnych
            int val = 0;
            while (isdigit(*stream))
            {
                val *= 10; // dotychczasową wartość traktujemy jako 10 razy większą - bo znaleźliśmy kolejne miejsce dziesiętne
                val += *stream++ - '0'; // przerabiamy char na integer
            }
            token.kind = TOKEN_INT;
            token.val = val;
        }
        break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
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
        default:
        {
            token.kind = *stream++;
        }
        break;
    }

    token.end = stream;
    stream++; 

    tok* new_tok = xmalloc(sizeof(token));
    memcpy(new_tok, &token, sizeof(token));
    buf_push(all_tokens, new_tok); 
}

void init_stream(const char* str)
{
    stream = str;
    next_token();
}

int main(int argc, char** argv)
{
    stretchy_buffers_test();
    intern_str_test();

    const char* parsing_test = "AA 12A BB A21 CC";

    init_stream(parsing_test);
    while (token.kind)
    {
        next_token();
    }

    return 0;
}