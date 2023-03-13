//#include <stddef.h>
#include <stdlib.h> // calloc, realloc
#include <stdio.h>
//#include <stdarg.h>
#include <string.h> // memcpy
//#include <stdint.h>
#include <stdbool.h>

#define null 0
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#define max(a,b) ((a) > (b) ? (a) : (b))

void* ___alloc___(size_t num_bytes)
{
    void* ptr = calloc(1, num_bytes);
    if (!ptr)
    {
        perror("allocation failed");
        exit(1);
    }
    return ptr;
}

void* ___realloc___(void* ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr)
    {
        perror("reallocation failed");
        exit(1);
    }
    return ptr;
}

void ___free___(void* ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

void* ___managed_alloc___(size_t num_bytes)
{
    void* ptr = calloc(1, num_bytes);
    if (!ptr)
    {
        perror("allocation failed");
        exit(1);
    }
    return ptr;
}

void* ___managed_realloc___(void* ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr)
    {
        perror("reallocation failed");
        exit(1);
    }
    return ptr;
}

void ___managed_free___(void* ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

typedef struct ___list_hdr___
{
    bool is_managed;
    size_t length;
    size_t capacity;
    char* buffer;
} ___list_hdr___;

___list_hdr___* ___list_initialize___(size_t initial_capacity, size_t element_size, bool managed)
{    
    ___list_hdr___* hdr = 0;
    if (managed)
    {   
        hdr = (___list_hdr___*)___managed_alloc___(sizeof(___list_hdr___));
        hdr->buffer = (char*)___managed_alloc___(initial_capacity * element_size);
    }
    else
    {
        hdr = (___list_hdr___*)___alloc___(sizeof(___list_hdr___));
        hdr->buffer = (char*)___alloc___(initial_capacity * element_size);
    }
    hdr->length = 0;
    hdr->capacity = initial_capacity;
    hdr->is_managed = managed;
    return hdr;
}

void ___list_grow___(___list_hdr___* hdr, size_t new_length, size_t element_size)
{
    if (hdr)
    {
        size_t new_capacity = max(1 + 2 * hdr->capacity, new_length);    
        if (hdr->is_managed)
        {
            hdr->buffer = (char*)___managed_realloc___(hdr->buffer, new_capacity * element_size);
        }
        else
        {
            hdr->buffer = (char*)___realloc___(hdr->buffer, new_capacity * element_size);
        }                
        hdr->capacity = new_capacity;
    }   
}

bool ___check_list_fits___(___list_hdr___* hdr, size_t increase)
{
    if (hdr)
    {
        if (hdr->length + increase < hdr->capacity)
        {
            return true;
        }
    }
    return false;
}

void ___list_fit___(___list_hdr___* hdr, size_t increase, size_t element_size)
{
    if (hdr)
    {
        if (false == ___check_list_fits___(hdr, increase))
        {            
            ___list_grow___(hdr, hdr->length + increase, element_size);
        }
    }
}

void ___list_remove_at___(___list_hdr___* hdr, size_t element_size, size_t index)
{
    if (hdr && hdr->length > index)
    {
        memcpy(
            hdr->buffer + (index * element_size),
            hdr->buffer + ((hdr->length - 1) * element_size),
            element_size);

        memset(hdr->buffer + ((hdr->length - 1) * element_size), 0, element_size);

        hdr->length--;
    }
}

void ___list_free_internal__(___list_hdr___* hdr)
{
    if (hdr)
    {
        if (hdr->is_managed)
        {
            ___managed_free___(hdr->buffer);
            ___managed_free___(hdr);
        }
        else
        {
            ___free___(hdr->buffer);
            ___free___(hdr);
        }        
    }
}

#define ___list_free___(hdr) \
    ((hdr) ? (___list_free_internal__(hdr), (hdr) = null) : 0)

#define ___list_add___(hdr, new_element, element_type) \
    (___list_fit___((hdr), 1, sizeof(new_element)), \
    ((element_type*)(hdr)->buffer)[hdr->length++] = (new_element))

#define ___get_list_capacity___(hdr) ((hdr) ? (hdr->capacity) : 0)

#define ___get_list_length___(hdr) ((hdr) ? (hdr->length) : 0)

#undef bool
#undef false
#undef true
#undef offsetof
#undef max
#undef min
#undef NULL