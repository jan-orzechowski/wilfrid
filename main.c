#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct buffer_header
{
    size_t len;
    size_t cap;
    char buf[0];
} buffer_header;

// offsetof zwraca pozycję pola w ramach structu
// chodzi o to, że z buffer_header korzystamy tak, jak ze zwykłego void* 
// ale potajemnie mamy zaalokowane zawsze trochę więcej, z przodu
#define _buf_header(b) ((buffer_header*)((char*)(b) - offsetof(buffer_header, buf)))
#define buf_len(b) ((b) ? _buf_header(b)->len : 0)
#define buf_cap(b) ((b) ? _buf_header(b)->cap : 0)

#define _buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define _buf_fit(b, n) (_buf_fits((b), (n)) ? 0 : ((b) = _buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_push(b, x) (_buf_fit((b), 1), (b)[_buf_header(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(_buf_header(b)), (b) = NULL) : 0)

void* _buf_grow(const void* buf, size_t new_len, size_t elem_size)
{
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(buffer_header, buf) + new_cap * elem_size;
    buffer_header* new_hdr;
    if (buf)
    {
        new_hdr = realloc(_buf_header(buf), new_size);
    }
    else
    {
        new_hdr = malloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

struct intern_str
{
    size_t len;
    char* ptr;
};

int main(int argc, char** argv)
{
    printf("hello\n");

    struct intern_str* str = 0;

    assert(buf_len(str) == 0);

    for (size_t index = 0; index < 1024; index++)
    {
        buf_push(str, ((struct intern_str){ 1, NULL }));
    }

    assert(buf_len(str) == 1024);

    return 0;
}