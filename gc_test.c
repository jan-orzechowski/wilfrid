#include "utils.h"

#define define_struct(name) typedef struct name name; struct name

define_struct(allocation_metadata)
{
    size_t size; // najwyższy bit jest tagged
};

define_struct(something_to_gc)
{
    int value;
    something_to_gc* gc_obj_ptr;
};

int first;
uintptr_t __begin = (uintptr_t)&first;

// porządek zmiennych globalnych, jeśli polegają na sobie, może być
// rozwiązany tak samo jak porządek deklaracji - wystarczy, że 
// begin i end będą na właściwych miejscach
int costam0 = 2;
something_to_gc* static_gc_to_save;
int costam1 = 3;
int costam2 = 4;
int costam3 = 5;
int costam4 = 6;
int costam5 = 7;
int costam6 = 8;
int costam7 = 9;
int costam8 = 10;
int costam9 = 11;
int costam10 = 12;
int costam11 = 13;
int costam12 = 14;

int __last;
uintptr_t __end = (uintptr_t)&__last;

uintptr_t stack_approximate_beginning;

void gc_init()
{
    int value_on_stack = 1;
    stack_approximate_beginning = (uintptr_t)&value_on_stack;
}

define_struct(non_gc_object)
{
    int value;
    non_gc_object* non_gc_obj_ptr;
    something_to_gc* gc_obj_ptr;
};

allocation_metadata** allocations;
allocation_metadata** gc_allocations;

// dwie cyfry heksadecymalne - np. 0xFF -> jeden bajt, 0-255, więc 64 bity, czyli 8 bajtów -> 16 cyfr
//#define ptr_tag 0x0000000000002137
#define alive_tag 0x1000000000000000
#define tag(v, t) ((v) = ((v) | (t)))
#define untag(v, t) ((v) = ((v) & (~0 & ~(t))))

uintptr_t min_ptr = UINT64_MAX;
uintptr_t max_ptr = 0;

void* alloc_wrapper(size_t num_bytes, bool garbage_collect)
{
    assert(num_bytes <= 0x7FFFFFFFFFFFFFFF);

    void* memory = xcalloc(num_bytes + sizeof(allocation_metadata));

    allocation_metadata* hdr = (allocation_metadata*)memory;
    hdr->size = num_bytes;

    assert((hdr->size & alive_tag) == 0);

    uintptr_t obj_ptr = (uintptr_t)((char*)memory + sizeof(allocation_metadata));

    if (garbage_collect)
    {
        if (obj_ptr < min_ptr)
        {
            min_ptr = obj_ptr;
        }

        if (obj_ptr > max_ptr)
        {
            max_ptr = obj_ptr;
        }

        buf_push(gc_allocations, hdr);
    }
    else
    {
        buf_push(allocations, hdr);
    }

    return (void*)obj_ptr;
}

typedef enum garbage_collected
{
    GARBAGE_COLLECT_TRUE,
    GARBAGE_COLLECT_FALSE
} garbage_collected;

void* gc_alloc(size_t num_bytes)
{
    return alloc_wrapper(num_bytes, GARBAGE_COLLECT_TRUE);
}

void* alloc(size_t num_bytes)
{
    return alloc_wrapper(num_bytes, GARBAGE_COLLECT_FALSE);
}

void free_wrapper(void* ptr)
{
    if (ptr)
    {
        free(ptr);
        for (size_t i = 0; i < buf_len(allocations); i++)
        {
            if (allocations[i] == ptr)
            {
                buf_remove_at(allocations, i);
                break;
            }
        }
    }
}

void scan_for_pointers(uintptr_t memory_block_begin, size_t byte_count)
{   
    size_t gc_count = buf_len(gc_allocations);
    if (buf_len(gc_allocations) > 0)
    {        
        uintptr_t memory_block_end = ((uintptr_t)memory_block_begin + align_down(byte_count, 8));
        size_t debug_size = memory_block_end - memory_block_begin;

        for (uintptr_t ptr = memory_block_begin;
            ptr < memory_block_end;
            ptr++)
        {
            uintptr_t potential_managed_ptr = *((uintptr_t*)ptr);
            if (potential_managed_ptr <= max_ptr 
                && potential_managed_ptr >= min_ptr)
            {
                for (size_t i = 0; i < gc_count; i++)
                {
                    // to koniecznie trzeba zastąpić hash tablicą...
                    allocation_metadata* hdr = gc_allocations[i];
                    uintptr_t obj_ptr = (uintptr_t)((char*)hdr + sizeof(allocation_metadata));
                    if (potential_managed_ptr == obj_ptr)
                    {
                        debug_breakpoint;
                        scan_for_pointers(obj_ptr, hdr->size);
                        tag(hdr->size, alive_tag);
                        break;
                    }
                }
            }
        }
    }  
}

void scan_range_for_pointers(uintptr_t memory_block_begin, uintptr_t memory_block_end)
{
    scan_for_pointers(memory_block_begin, memory_block_end - memory_block_begin);
}

void mark(void)
{
    int stack_var = 0;
    uintptr_t stack_approximate_end = (uintptr_t)&stack_var;

    // stack rośnie w drugą stronę
    assert(stack_approximate_beginning > stack_approximate_end);

    scan_range_for_pointers(stack_approximate_end, stack_approximate_beginning);

    // zmienne statycznie zaalokowane
    scan_range_for_pointers(__begin, __end);

    // no i to, co sami zaalokowaliśmy
    size_t allocations_count = buf_len(allocations);
    for (size_t i = 0; i < allocations_count; i++)
    {
        allocation_metadata* hdr = allocations[i];
        uintptr_t obj_ptr = (uintptr_t)((char*)hdr + sizeof(allocation_metadata));
        scan_for_pointers(obj_ptr, hdr->size);
    }
}

void sweep(void)
{
    size_t gc_obj_count = buf_len(gc_allocations);
    for (size_t index = 0; index < gc_obj_count; index++)
    {
        allocation_metadata* hdr = gc_allocations[index];
        if (false == (hdr->size & alive_tag))
        {
            free(hdr);
            buf_remove_at(gc_allocations, index);
            gc_obj_count--;
            index--; // żeby nie przeskoczyć o 1 w wyniku usunięcia
        }
        else
        {
            untag(hdr->size, alive_tag);
        }
    }
}

void gc(void)
{
    mark();
    sweep();
}

void print_values_from_list(bool gc)
{
    allocation_metadata** buf = gc ? gc_allocations : allocations;
    printf(gc ? "\nGC: \n" : "\nNON GC: \n");
    for (size_t i = 0; i < buf_len(buf); i++)
    {
        int value = 0;
        allocation_metadata* hdr = buf[i];
        void* obj_ptr = (void*)((char*)hdr + sizeof(allocation_metadata));
        if (gc)
        {
            value = ((something_to_gc*)obj_ptr)->value;
        }
        else
        {
            value = ((non_gc_object*)obj_ptr)->value;
        }

        printf("obj value: %i, %d\n", value, hdr->size);
    }
}

void gc_test(void)
{
    something_to_gc* gc_to_save = null;

    uintptr_t gc_to_save_addr = (uintptr_t)&gc_to_save;

    for (size_t i = 0; i < 1000; i++)
    {
        non_gc_object* nongc = alloc(sizeof(non_gc_object));
        nongc->value = i + 1;
        something_to_gc* gc = gc_alloc(sizeof(something_to_gc));
        int option = i % 3;
        switch (option)
        {
            case 0:
            {
                // nie powinniśmy skasować
                nongc->gc_obj_ptr = gc;                
                gc->value = i;
            } 
            break;
            case 1:
            {
                // tutaj też powinniśmy skasować
                something_to_gc* gc2 = gc_alloc(sizeof(something_to_gc));
                gc2->value = i;
                gc2->gc_obj_ptr = gc;
            }
            break;
            case 2:
            {
                gc_to_save = gc;
                gc->value = i;

                if (static_gc_to_save == 0)
                {
                    static_gc_to_save = gc;
                }
            }
            break;
        }       
    }

    size_t gc_count_before = buf_len(gc_allocations);

    debug_breakpoint;

    gc();

    size_t gc_count_after = buf_len(gc_allocations);

    print_values_from_list(true);
    print_values_from_list(false);

    debug_breakpoint;
}