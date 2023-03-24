﻿#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define null 0

#define assert(condition) { if (!(condition)) { int x = 0; int y = 1 / x; } }

#define debug_breakpoint { int x = 0; }
#define invalid_default_case default: { assert(0); } break;

#define is_power_of_2(x) (((x) != 0) && ((x) & ((x) - 1)) == 0)
#define align_down(num, align) ((num) & ~((align) - 1))
#define align_up(num, align) (align_down((num) + (align) - 1, (align)))
#define align_down_ptr(ptr, align) ((void *)align_down((uintptr_t)(ptr), (align)))
#define align_up_ptr(ptr, align) ((void *)align_up((uintptr_t)(ptr), (align)))

void fatal(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    printf("FATAL: ");
    vprintf(format, args);
    printf("\n");

    va_end(args);

    debug_breakpoint;

    exit(1);
}

#include "memory.c"

char *xprintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    size_t length = 1 + vsnprintf(null, 0, format, args);
    va_end(args);

    char *str = xmalloc(length);
    
    va_start(args, format);
    vsnprintf(str, length, format, args);
    va_end(args);
    
    return str;
}

char *make_str_range_copy(char *start, char *end)
{
    assert(end > start);
    size_t size = end - start;

    char *new_str = xcalloc((size + 1) * sizeof(char));

    memcpy(new_str, start, size);
    new_str[size] = 0;

    return new_str;
}

typedef struct string_ref
{
    char *str;
    size_t length;
} string_ref;

float get_random_01(void)
{
    return ((float)rand() / (float)RAND_MAX);
}

#include "stretchy_buffers.c"
#include "hashmap.c"
#include "interning.c"
#include "errors.c"
#include "file_handling.c"