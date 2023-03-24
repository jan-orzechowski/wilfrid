typedef struct buffer_header
{
    size_t len;
    size_t cap;
    char buf[0];
} buffer_header;

#define __buf_header(b) ((buffer_header*)((char*)(b) - offsetof(buffer_header, buf)))
#define buf_len(b) ((b) ? __buf_header(b)->len : 0)
#define buf_cap(b) ((b) ? __buf_header(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)

#define __buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define __buf_fit(b, n) (__buf_fits((b), (n)) ? 0 : ((b) = __buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_push(b, x) (__buf_fit((b), 1), (b)[__buf_header(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(__buf_header(b)), (b) = null) : 0)
#define buf_remove_at(b, i) ((b) && buf_len(b) > (i) ? ((b)[i] = (b)[buf_len(b) - 1], (b)[buf_len(b) - 1] = 0, __buf_header(b)->len--) : 0) 

buffer_header *__get_buf_header(void *ptr)
{
    buffer_header *header = (buffer_header *)((char *)ptr - offsetof(buffer_header, buf));
    return header;
}

void *__buf_grow(const void *buf, size_t new_len, size_t elem_size)
{
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(buffer_header, buf) + new_cap * elem_size;
    buffer_header *new_hdr;
    if (buf)
    {
        new_hdr = __buf_header(buf);
        new_hdr = (buffer_header *)xrealloc(new_hdr, new_size);
    }
    else
    {
        new_hdr = (buffer_header *)xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

#define buf_printf(b, ...) ((b) = __buf_printf((b), __VA_ARGS__))

char *__buf_printf(char *buf, const char *format, ...)
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

void *__copy_buf_to_arena(memory_arena *arena, const void *buf, size_t elem_size)
{
    void *dest = null;
    if (buf)
    {
        buffer_header *hdr = __buf_header(buf);
        dest = push_size(arena, hdr->len * elem_size);
        memcpy(dest, buf, hdr->len * elem_size);
    }

    return dest;
}