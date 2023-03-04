//#include <stddef.h>
#include <stdlib.h> // calloc, realloc
#include <stdio.h>
//#include <stdarg.h>
#include <string.h> // memcpy
//#include <stdint.h>
#include <stdbool.h>

#define null 0
#define offsetof(s,m) ((size_t)&(((s*)0)->m))

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
    char buffer[0];
} ___list_hdr___;

___list_hdr___* ___get_list_hdr___(void* list)
{
    return ((___list_hdr___*)((char*)(list) - offsetof(___list_hdr___, buffer)));
}

size_t ___get_list_allocation_size___(size_t new_capacity, size_t element_size)
{
    size_t result = offsetof(___list_hdr___, buffer) + new_capacity * element_size;
    return result;
}

void* ___list_initialize___(size_t initial_capacity, size_t element_size, bool managed)
{
    size_t new_size = ___get_list_allocation_size___(initial_capacity, element_size);
    ___list_hdr___* hdr = 0;
    if (managed)
    {
        hdr = (___list_hdr___*)___managed_alloc___(new_size);
    }
    else
    {
        hdr = (___list_hdr___*)___alloc___(new_size);
    }
    hdr->length = 0;
    hdr->capacity = initial_capacity;
    hdr->is_managed = managed;
    return hdr->buffer;
}

void* ___list_grow___(void* list, size_t new_length, size_t element_size)
{
    if (list)
    {
        ___list_hdr___* hdr = ___get_list_hdr___(list);
        size_t new_capacity = max(1 + 2 * hdr->capacity, new_length);
        size_t new_size = ___get_list_allocation_size___(new_capacity, element_size);        
        if (hdr->is_managed)
        {
            hdr = (___list_hdr___*)___managed_realloc___(hdr, new_size);
        }
        else
        {
            hdr = (___list_hdr___*)___realloc___(hdr, new_size);
        }                
        hdr->capacity = new_capacity;
        return hdr->buffer;
    }
    else
    {
        return 0;
    }
}

size_t ___get_list_length___(void* list)
{
    if (list)
    {
        return ___get_list_hdr___(list)->length;
    }
    else
    {
        return 0;
    }
}

size_t ___get_list_capacity___(void* list)
{
    if (list)
    {
        return ___get_list_hdr___(list)->capacity;
    }
    else
    {
        return 0;
    }
}

bool ___check_list_fits___(void* list, size_t increase)
{
    if (list)
    {
        ___list_hdr___* hdr = ___get_list_hdr___(list);
        if (hdr->length + increase < hdr->capacity)
        {
            return true;
        }
    }
    return false;
}

void* ___list_fit___(void* list, size_t increase, size_t element_size)
{
    if (list)
    {
        if (false == ___check_list_fits___(list, increase))
        {
            list = ___list_grow___(list, ___get_list_length___(list) + increase, element_size);
        }
    }   
    return list;
}

void ___list_free___(void* list)
{
    if (list)
    {
        ___list_hdr___* hdr = ___get_list_hdr___(list);
        if (hdr->is_managed)
        {
            ___managed_free___(hdr);
        }
        else
        {
            ___free___(hdr);
        }        
    }
}

void* ___list_add___(void* list, size_t element_size, void* new_element)
{
    if (list)
    {
        list = ___list_fit___(list, 1, element_size);

        ___list_hdr___* hdr = ___get_list_hdr___(list);
        memcpy(hdr->buffer + (element_size * hdr->length), new_element, element_size);
        hdr->length++;
    }   
    return list;
}

void* ___list_add_at_index___(void* list, size_t element_size, void* new_element, size_t index)
{    
    if (list)
    {
        ___list_hdr___* hdr = ___get_list_hdr___(list);
        if (hdr->capacity < index)
        {
            list = ___list_fit___(list, index - hdr->capacity, element_size);
            hdr = ___get_list_hdr___(list);
            hdr->length = index + 1;
        }

        memcpy(hdr->buffer + (element_size * index), new_element, element_size);
        if (index > hdr->length)
        {
            hdr->length = index + 1;
        }
    }
    return list;
}

void ___list_remove___(void* list, size_t element_size, size_t index)
{
    if (list && ___get_list_length___(list) > index)
    {
        ___list_hdr___* hdr = ___get_list_hdr___(list);

        memcpy(
            hdr->buffer + (index * element_size), 
            hdr->buffer + ((hdr->length - 1) * element_size), 
            element_size);

        memset(hdr->buffer + ((hdr->length - 1) * element_size), 0, element_size);
        
        hdr->length--;
    }
}

#undef bool
#undef false
#undef true
#undef offsetof
#undef max
#undef min
#undef NULL