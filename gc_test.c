#include "utils.h"

define_struct(allocation_header)
{
    size_t size; // najwyższy bit jest tagged
};

define_struct(something_to_gc)
{
    int test_number;
    something_to_gc* gc_obj_ptr;
};

typedef struct globals_wrapper
{
    something_to_gc* static_gc_to_save_1;
    something_to_gc* static_gc_to_save_2;
    something_to_gc* static_gc_to_save_3;
} globals_wrapper;
struct globals_wrapper globals = {(something_to_gc*)666,(something_to_gc*)777,(something_to_gc*)888};

uintptr_t test_gc_ptr;

uintptr_t stack_approximate_beginning;

void gc_init()
{
    int value_on_stack = 1;
    stack_approximate_beginning = (uintptr_t)&value_on_stack;
}

define_struct(non_gc_object)
{
    int test_number;
    non_gc_object* non_gc_obj_ptr;
    something_to_gc* gc_obj_ptr;
};

// dwie cyfry heksadecymalne - np. 0xFF -> jeden bajt, 0-255, więc 64 bity, czyli 8 bajtów -> 16 cyfr
#define alive_tag 0x1000000000000000
#define tag(v, t) ((v) = ((v) | (t)))
#define untag(v, t) ((v) = ((v) & (~0 & ~(t))))

uintptr_t min_ptr = UINT64_MAX;
uintptr_t max_ptr = 0;

#define get_obj_ptr(hdr_ptr) (void*)((char*)(hdr_ptr) + sizeof(allocation_header))
#define get_hdr_ptr(obj_ptr) (allocation_header*)((char*)(obj_ptr) - sizeof(allocation_header))

chained_hashmap* allocations;
chained_hashmap* gc_allocations;

void* alloc_wrapper(size_t num_bytes, bool garbage_collect)
{
    assert(num_bytes <= 0x7FFFFFFFFFFFFFFF);

    void* memory = xcalloc(num_bytes + sizeof(allocation_header));

    allocation_header* hdr = (allocation_header*)memory;
    hdr->size = num_bytes;

    assert((hdr->size & alive_tag) == 0);

    uintptr_t obj_ptr = (uintptr_t)get_obj_ptr(memory);

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

        map_add_to_chain(gc_allocations, (void*)obj_ptr, (void*)obj_ptr);
    }
    else
    {
        map_add_to_chain(allocations, (void*)obj_ptr, (void*)obj_ptr);
    }

    return (void*)obj_ptr;
}

typedef enum garbage_collected
{
    GARBAGE_COLLECT_TRUE = 1,
    GARBAGE_COLLECT_FALSE = 0
} garbage_collected;

void* gc_alloc(size_t num_bytes)
{
    return alloc_wrapper(num_bytes, GARBAGE_COLLECT_TRUE);
}

void* alloc(size_t num_bytes)
{
    return alloc_wrapper(num_bytes, GARBAGE_COLLECT_FALSE);
}

//void free_wrapper(void* ptr)
//{
//    if (ptr)
//    {
//        delete_from_chain(allocations, ptr);
//    }
//}

void scan_for_pointers(uintptr_t memory_block_begin, size_t byte_count)
{   
    size_t gc_count = buf_len(gc_allocations);
    if (buf_len(gc_allocations) > 0)
    {        
        uintptr_t memory_block_end = ((uintptr_t)memory_block_begin + align_down(byte_count, 8));
        size_t debug_size = memory_block_end - memory_block_begin;

        for (uintptr_t ptr = memory_block_begin;
            ptr < memory_block_end;
            ptr += sizeof(uintptr_t))
        {
            uintptr_t potential_managed_ptr = *((uintptr_t*)ptr);
            if (potential_managed_ptr > max_ptr || potential_managed_ptr < min_ptr)
            {
                continue;
            }
            
            uintptr_t obj_ptr = (uintptr_t)map_get_from_chain(gc_allocations, (void*)potential_managed_ptr);
            if (obj_ptr)
            {               
                allocation_header* hdr = get_hdr_ptr(obj_ptr);
                if (hdr->size & alive_tag)
                {
                    // mamy cykl - nie skanujemy dalej
                    debug_breakpoint;
                    return;
                }

                debug_breakpoint;
                scan_for_pointers(obj_ptr, hdr->size);
                tag(hdr->size, alive_tag);
                break;
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
    scan_for_pointers((uintptr_t)&globals, sizeof(globals_wrapper));

    // no i to, co sami zaalokowaliśmy
    for (size_t i = 0; i < allocations->capacity; i++)
    {
        hashmap_value* val = gc_allocations->values[i];
        while (val)
        {
            allocation_header* hdr = get_hdr_ptr(val->value);
            scan_for_pointers((uintptr_t)val->value, hdr->size);

            val = val->next;            
        }
    }
}

void sweep(void)
{
    for (size_t i = 0; i < gc_allocations->capacity; i++)
    {
        hashmap_value* val = gc_allocations->values[i];
        if (val)
        {
            hashmap_value* next = val->next;
            allocation_header* hdr = get_hdr_ptr(val->value);

            if (false == (hdr->size & alive_tag))
            {
                map_delete_from_chain(gc_allocations, val->value);
            }
            else
            {
                untag(hdr->size, alive_tag);
            }

            val = next;
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
    chained_hashmap* map = gc ? gc_allocations : allocations;
    printf(gc ? "\nGC: \n" : "\nNON GC: \n");
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->values[i])
        {
            hashmap_value* val = map->values[i];
            allocation_header* hdr = get_hdr_ptr(val->value);
            int number = 0;
            if (gc)
            {
                number = ((something_to_gc*)val->value)->test_number;
            }
            else
            {
                number = ((non_gc_object*)val->value)->test_number;
            }
            
            printf("obj number: %i, %zd\n", number, hdr->size);
        }       
    }
}

void gc_test(void)
{
    something_to_gc* stack_gc_to_save = null;

    allocations = xcalloc(sizeof(chained_hashmap));
    gc_allocations = xcalloc(sizeof(chained_hashmap));
    map_chain_grow(allocations, 16);
    map_chain_grow(gc_allocations, 16);

#if 1
    for (size_t i = 0; i < 1000; i++)
    {
        non_gc_object* nongc = alloc(sizeof(non_gc_object));
        nongc->test_number = i + 1;
        something_to_gc* gc = gc_alloc(sizeof(something_to_gc));
        int option = i % 3;
        switch (option)
        {
            case 0:
            {
                // nie powinniśmy skasować
                nongc->gc_obj_ptr = gc;                
                gc->test_number = i;
            } 
            break;
            case 1:
            {
                // tutaj też powinniśmy skasować
                something_to_gc* gc2 = gc_alloc(sizeof(something_to_gc));
                gc2->test_number = i;
                gc2->gc_obj_ptr = gc;
            }
            break;
            case 2:
            {
                stack_gc_to_save = gc;
                test_gc_ptr = (uintptr_t)stack_gc_to_save;
                globals.static_gc_to_save_1 = gc;
                gc->test_number = i;

                if (globals.static_gc_to_save_1 == 0)
                {
                    globals.static_gc_to_save_1 = gc;
                }
                else if (globals.static_gc_to_save_2 == 0)
                {
                    globals.static_gc_to_save_2 = gc;
                }
                else if (globals.static_gc_to_save_3 == 0)
                {
                    globals.static_gc_to_save_3 = gc;
                }
            }
            break;
        }       
    }
#endif

#if 0
    non_gc_object* nongc = alloc(sizeof(non_gc_object));
    nongc->value = 555;
    something_to_gc* gcobj = gc_alloc(sizeof(something_to_gc));
    globals.static_gc_to_save_1 = gcobj;
    stack_gc_to_save = gcobj;

    test_gc_ptr = (uintptr_t)stack_gc_to_save;
#endif

    size_t gc_count_before = gc_allocations->total_count;

    debug_breakpoint;

    gc();

    size_t gc_count_after = gc_allocations->total_count;

    print_values_from_list(true);
    print_values_from_list(false);

    debug_breakpoint;
}