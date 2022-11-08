#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
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
        new_hdr = (buffer_header*)xrealloc(__buf_header(buf), new_size);
    }
    else
    {
        new_hdr = (buffer_header*)xmalloc(new_size);
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
    char* new_str = (char*)xmalloc(len + 1);
    memcpy(new_str, start, len);
    new_str[len] = 0; // upewniamy się, że mamy null terminator

    buf_push(interns, ((intern_str){.len = len, .ptr = new_str}));

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
