#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define debug_breakpoint {int x = 0;}

#define is_power_of_2(x) (((x) != 0) && ((x) & ((x) - 1)) == 0)
#define align_down(num, align) ((num) & ~((align) - 1))
#define align_up(num, align) (align_down((num) + (align) - 1, (align)))
#define align_down_ptr(ptr, align) ((void *)align_down((uintptr_t)(ptr), (align)))
#define align_up_ptr(ptr, align) ((void *)align_up((uintptr_t)(ptr), (align)))

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

void* xcalloc(size_t num_bytes)
{
    void* ptr = calloc(num_bytes, sizeof(char));
    if (!ptr)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void* xmempcy(void* src, size_t size)
{
    void* dest = xmalloc(size);
    memcpy(dest, src, size);
    return dest;
}

#define kilobytes(n) (1024 * n)
#define megabytes(n) (kilobytes(n) * 1024)
#define gigabytes(n) (megabytes(n) * 1024)
#define terabytes(n) (gigabytes(n) * 1024)

typedef struct memory_arena
{
    void* base_address;
    size_t max_size;
    size_t current_size;
} memory_arena;

memory_arena* allocate_memory_arena(size_t size)
{
    memory_arena* result = (memory_arena*)xcalloc(size);
    result->base_address = result;
    result->max_size = size;
    result->current_size = sizeof(memory_arena);
    return result;
}

void free_memory_arena(memory_arena* arena)
{
    free(arena);
}

#define push_struct(arena, type) (type*)push_size(arena, sizeof(type))

void* push_size(memory_arena* arena, size_t size)
{
    assert(arena->current_size + size < arena->max_size);

    void* result = (void*)((char*)arena->base_address + arena->current_size);
    arena->current_size += size;

    return result;
}

void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);

    debug_breakpoint;

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
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)

#define __buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define __buf_fit(b, n) (__buf_fits((b), (n)) ? 0 : ((b) = __buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_push(b, x) (__buf_fit((b), 1), (b)[__buf_header(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(__buf_header(b)), (b) = NULL) : 0)

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
        new_hdr = __buf_header(buf);
        new_hdr = (buffer_header*)xrealloc(new_hdr, new_size);
    }
    else
    {
        new_hdr = (buffer_header*)xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

#define copy_buf_to_arena(arena, buf) __copy_buf_to_arena((arena), (buf), sizeof(*(buf)))

void copy_test();

void* __copy_buf_to_arena(memory_arena* arena, const void* buf, size_t elem_size)
{
    void* dest = NULL;    
    if (buf)
    {
        buffer_header* hdr = __buf_header(buf);
        dest = push_size(arena, hdr->len * elem_size);
        memcpy(dest, buf, hdr->len * elem_size);
    }

    return dest;
}

void copy_test()
{
    memory_arena* test = allocate_memory_arena(kilobytes(1));

    int* buffer = 0;
    for (int i = 0; i < 128; i++)
    {
        buf_push(buffer, i);
    }

    int* new_array = (int*)copy_buf_to_arena(test, buffer);

    for (int i = 0; i < 128; i++)
    {
        if (new_array[i] != i)
        {
            debug_breakpoint;
        }
    }

    free(test);
    buf_free(buffer);
}

typedef struct intern_str
{
    size_t len;
    char* ptr;
}
intern_str;

intern_str* interns;
memory_arena* string_arena;

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
    char* new_str = (char*)push_size(string_arena, len + 1);
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

