void *xrealloc(void *ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr)
    {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void *xmalloc(size_t num_bytes)
{
    void *ptr = malloc(num_bytes);
    if (!ptr)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void *xcalloc(size_t num_bytes)
{
    void *ptr = calloc(num_bytes, sizeof(char));
    if (!ptr)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void *xmempcy(void *src, size_t size)
{
    void *dest = xmalloc(size);
    memcpy(dest, src, size);
    return dest;
}

#define kilobytes(n) (1024 * n)
#define megabytes(n) (kilobytes(n) * 1024)
#define gigabytes(n) (megabytes(n) * 1024)
#define terabytes(n) (gigabytes(n) * 1024)

typedef struct memory_arena
{
    void *base_address;
    size_t max_size;
    size_t current_size;
} memory_arena;

memory_arena *allocate_memory_arena(size_t size)
{
    memory_arena *result = (memory_arena *)xcalloc(size);
    result->base_address = result;
    result->max_size = size;
    result->current_size = sizeof(memory_arena);
    return result;
}

void free_memory_arena(memory_arena *arena)
{
    free(arena);
}

#define push_struct(arena, type) (type*)push_size(arena, sizeof(type))

void *push_size(memory_arena *arena, size_t size)
{
    assert(arena->current_size + size < arena->max_size);

    void *result = (void *)((char *)arena->base_address + arena->current_size);
    arena->current_size += size;

    return result;
}
