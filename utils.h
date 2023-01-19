#pragma once

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

void fatal(const char* format, ...)
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

char* xprintf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    size_t length = 1 + vsnprintf(NULL, 0, format, args);
    va_end(args);

    char* str = xmalloc(length);
    
    va_start(args, format);
    vsnprintf(str, length, format, args);
    va_end(args);
    
    return str;
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
#define buf_remove_at(b, i) ((b) && buf_len(b) > (i) ? ((b)[i] = (b)[buf_len(b) - 1], (b)[buf_len(b) - 1] = 0, __buf_header(b)->len--) : 0) 

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

#define buf_printf(b, ...) ((b) = __buf_printf((b), __VA_ARGS__))

char* __buf_printf(char* buf, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    size_t current_cap = buf_cap(buf) - buf_len(buf);
    size_t str_length = 1 + vsnprintf(buf_end(buf), current_cap, format, args);
    va_end(args);

    if (str_length > current_cap)
    {
        __buf_fit(buf, str_length + buf_len(buf));

        va_start(args, format);
        size_t new_cap = buf_cap(buf) - buf_len(buf);
        str_length = 1 + vsnprintf(buf_end(buf), new_cap, format, args);
        assert(str_length <= new_cap);
        va_end(args);
    }

    __buf_header(buf)->len += str_length - 1;
    
    return buf;
}

#define copy_buf_to_arena(arena, buf) __copy_buf_to_arena((arena), (buf), sizeof(*(buf)))

void copy_test(void);

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

void copy_test(void)
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

void buf_remove_at_test()
{
    int* integers = 0;
    assert(buf_len(integers) == 0);
    buf_push(integers, 0);
    buf_push(integers, 1);
    buf_push(integers, 2);
    assert(buf_len(integers) == 3);
    buf_remove_at(integers, 1);
    assert(buf_len(integers) == 2);
    assert(integers[2] == 0);
    assert(integers[1] == 2);
    buf_remove_at(integers, 1);
    assert(buf_len(integers) == 1);
    assert(integers[1] == 0);
    buf_remove_at(integers, 0);
    assert(buf_len(integers) == 0);
    buf_remove_at(integers, 0);
}

uint64_t hash_uint64(uint64_t x)
{
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

uint64_t hash_ptr(void* ptr)
{
    return hash_uint64((uintptr_t)ptr);
}

uint64_t hash_bytes(const char* buf, size_t len)
{
    // FNV hash
    uint64_t x = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++)
    {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

typedef struct hashmap
{
    void** keys;
    void** values;
    size_t count;
    size_t capacity;
} hashmap;

void* map_get(hashmap* map, void* key)
{
    if (map->count == 0)
    {
        return null;
    }

    assert(is_power_of_2(map->capacity));

    size_t index = (size_t)hash_ptr(key);
    assert(map->count < map->capacity);
    while (true)
    {
        index &= map->capacity - 1;
        if (map->keys[index] == key)
        {
            return map->values[index];
        }
        else if (!map->keys[index])
        {
            return null;
        }
        index++;
    }
    return null;
}

void map_put(hashmap* map, void* key, void* val);

void map_grow(hashmap* map, size_t new_capacity)
{
    new_capacity = max(16, new_capacity);
    hashmap new_map = {
        .keys = xcalloc(new_capacity * sizeof(void*)),
        .values = xcalloc(new_capacity * sizeof(void*)),
        .capacity = new_capacity,
    };
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->keys[i])
        {
            map_put(&new_map, map->keys[i], map->values[i]);
        }
    }
    free(map->keys);
    free(map->values);
    *map = new_map;
}

void map_put(hashmap* map, void* key, void* val)
{
    assert(key);
    assert(val);
    // dzięki temu mamy mniejsze prawdopodobieństwo kolizji
    if (2 * map->count >= map->capacity)
    {
        map_grow(map, 2 * map->capacity);
    }

    assert(2 * map->count < map->capacity);
    assert(is_power_of_2(map->capacity));

    size_t index = (size_t)hash_ptr(key);
    while (true)
    {
        index &= map->capacity - 1;
        if (!map->keys[index])
        {
            map->count++;
            map->keys[index] = key;
            map->values[index] = val;
            return;
        }
        else if (map->keys[index] == key)
        {
            map->values[index] = val;
            return;
        }
        index++;
    }
}

void map_test(void)
{
    hashmap map = { 0 };
    enum { N = 2048 };

    for (size_t i = 1; i < N; i++)
    {
        map_put(&map, (void*)i, (void*)(i + 1));
    }

    for (size_t i = 1; i < N; i++)
    {
        void* val = map_get(&map, (void*)i);
        assert(val == (void*)(i + 1));
    }

    debug_breakpoint;
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

void intern_str_test(void)
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

void stretchy_buffers_test(void)
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

    char* char_buf = 0;
    buf_printf(char_buf, "One: %d\n", 1);
    assert(strcmp(char_buf, "One: 1\n") == 0);
    buf_printf(char_buf, "Hex: 0x%x\n", 0x12345678);
    assert(strcmp(char_buf, "One: 1\nHex: 0x12345678\n") == 0);

    buf_remove_at_test();
}

