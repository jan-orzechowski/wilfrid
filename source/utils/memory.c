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
        perror("xcalloc failed");
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

typedef struct memory_arena_block memory_arena_block;
struct memory_arena_block
{    
    void *base_address;
    size_t max_size;
    size_t current_size;
    memory_arena_block *next;
};

typedef struct memory_arena memory_arena;
struct memory_arena
{
    size_t stack_frames;
    size_t memory_block_count;
    memory_arena_block *first_block;
    memory_arena_block *last_block;
    size_t block_size;
};

memory_arena_block *allocate_new_block(memory_arena *arena)
{
    memory_arena_block *new_block = xcalloc(arena->block_size + sizeof(memory_arena_block));
    new_block->base_address = (char *)new_block + sizeof(memory_arena_block);
    new_block->max_size = arena->block_size;
    new_block->current_size = 0;
   
    if (arena->last_block)
    {
        arena->last_block->next = new_block;
        arena->last_block = new_block;
    }
    else
    {
        arena->first_block = new_block;
        arena->last_block = new_block;
    }

    arena->memory_block_count++;

    return new_block;
}

memory_arena *allocate_memory_arena(size_t block_size)
{
    memory_arena *arena = xcalloc(sizeof(memory_arena));
    arena->block_size = block_size;
    allocate_new_block(arena);
    return arena;
}

size_t get_remaining_space_in_block(memory_arena_block *block)
{
    size_t result = block->max_size - block->current_size;
    return result;
}

void free_memory_arena(memory_arena *arena)
{
    if (arena == null)
    {
        return;
    }

    memory_arena_block *block_to_delete = arena->first_block;
    while (block_to_delete)
    {
        memory_arena_block *temp = block_to_delete->next;
        free(block_to_delete);
        block_to_delete = temp;
    }

    free(arena);
}

#define push_struct(arena, type) (type*)push_size(arena, sizeof(type))

void *push_size(memory_arena *arena, size_t size)
{
    if (size > arena->block_size)
    {
        fatal("allocation is too large");
    }

    memory_arena_block *block = arena->last_block;
    if (block->max_size - block->current_size < size)
    {
        block = allocate_new_block(arena);
    }

    void *result = (char *)block->base_address + block->current_size;
    block->current_size += size;    
    return result;
}

typedef struct arena_stack_marker
{
    memory_arena_block *block;
    void *address;
} arena_stack_marker;

arena_stack_marker push_arena_stack(memory_arena *arena)
{
    arena->stack_frames++;
    void *current_address = (char*)arena->last_block->base_address + arena->last_block->current_size;
    arena_stack_marker result = 
    { 
        .block = arena->last_block,
        .address = current_address  
    };
    return result;
}

size_t get_arena_total_size(memory_arena *arena)
{
    size_t size = 0;
    memory_arena_block *block = arena->first_block;
    while (block)
    {
        size += block->max_size;
        block = block->next;
    }
    return size;
}

size_t get_arena_used_size(memory_arena *arena)
{
    size_t size = 0;
    memory_arena_block *block = arena->first_block;
    while (block)
    {
        size += block->current_size;
        block = block->next;
    }
    return size;
}

void zero_memory_arena_block(memory_arena_block *block, void *from_address)
{
    void *block_current_address = (char *)block->base_address + block->current_size;
    void *block_max_address = (char *)block->base_address + block->max_size;
    assert(block_current_address < block_max_address);
    assert(from_address > block->base_address);
    assert(from_address < block_max_address);

    memset(from_address, 0, (char *)block_current_address - (char *)from_address);
    block->current_size = (char *)from_address - (char *)block->base_address - 1;
}

void pop_arena_stack(memory_arena *arena, arena_stack_marker marker)
{    
    assert(arena->stack_frames > 0);
    arena->stack_frames--;

    if (marker.block == arena->last_block)
    {
        zero_memory_arena_block(marker.block, marker.address);
    }
    else
    {
        zero_memory_arena_block(marker.block, marker.address);

        memory_arena_block *block_to_delete = marker.block->next;
        while (block_to_delete)
        {
            memory_arena_block *temp = block_to_delete->next;
            free(block_to_delete);
            arena->memory_block_count--;
            block_to_delete = temp;
        }
        marker.block->next = null;
        arena->last_block = marker.block;
    }
}

#define push_to_stack_or_list(elem, use_stack, stack, stack_len, stack_cap, list) \
    if (use_stack) \
    { \
        if (stack_len < stack_cap) \
        { \
            stack[stack_len] = elem; \
            stack_len++; \
        } \
        else \
        { \
            use_stack = false; \
            __buf_fit(list, stack_len * 2); \
            for (size_t i = 0; i < stack_len; i++) \
            { \
                buf_push(list, stack[i]); \
            } \
            buf_push(list, elem); \
        } \
    } \
    else \
    { \
        buf_push(list, elem); \
    }

#define copy_stack_or_list_to_arena(dest_ptr, dest_count, elem_size, use_stack, stack, stack_len, list) \
    if (use_stack) \
    { \
        dest_count = stack_len; \
        dest_ptr = push_size(arena, elem_size * dest_count); \
        for (size_t i = 0; i < stack_len; i++) \
        { \
            dest_ptr[i] = stack[i]; \
        } \
    } \
    else \
    { \
        dest_count = buf_len(list); \
        if (dest_count > 0) \
        { \
            dest_ptr = copy_buf_to_arena(arena, list); \
        } \
        buf_free(list);\
    }
