#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if !defined(null)
#define null 0
#endif 

#if !defined(offsetof)
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#if !defined(assert)
#define assert(condition) { if (!(condition)) { perror("Assert failed"); exit(1); } }
#endif 

#if !defined(max)
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif 

#if !defined(min)
#define max(a,b) ((a) < (b) ? (a) : (b))
#endif 

#if !defined(align_down)
#define align_down(num, align) ((num) & ~((align) - 1))
#endif

#if !defined(align_up)
#define align_up(num, align) (align_down((num) + (align) - 1, (align)))
#endif 

#if !defined(align_down_ptr)
#define align_down_ptr(ptr, align) ((void *)align_down((uintptr_t)(ptr), (align)))
#endif 

#if !defined(align_up_ptr)
#define align_up_ptr(ptr, align) ((void *)align_up((uintptr_t)(ptr), (align)))
#endif 

typedef struct ___alloc_hdr___ {
  size_t size;
} ___alloc_hdr___;

#define ___alive_tag___ 0x1000000000000000
#define ___tag___(v, t) ((v) = ((v) | (t)))
#define ___untag___(v, t) ((v) = ((v) & (~0 & ~(t))))

#define ___get_obj_ptr___(hdr_ptr) (void*)((char*)(hdr_ptr) + sizeof(___alloc_hdr___))
#define ___get_hdr_ptr___(obj_ptr) (___alloc_hdr___*)((char*)(obj_ptr) - sizeof(___alloc_hdr___))

typedef struct ___alloc_map_value___ ___alloc_map_value___;
struct ___alloc_map_value___ {
  uintptr_t key;
  uintptr_t value;
  ___alloc_map_value___ *next;
};

typedef struct ___alloc_map___ {
  ___alloc_map_value___ **values;
  size_t total_count;
  size_t used_capacity;
  size_t capacity;
} ___alloc_map___;

uint64_t ___hash_ptr___(uint64_t ptr) {
  ptr *= 0xff51afd7ed558ccd;
  ptr ^= ptr >> 32;
  return ptr;
}

uintptr_t ___alloc_map_get___(___alloc_map___ *map, uintptr_t key) {
  ___alloc_map_value___ *val = null;
  size_t index = (size_t)___hash_ptr___(key) & (map->capacity - 1);
  val = map->values[index];
  while (val) {
    if (val->key == key) {
      return val->value;
    }
    if (val->next) {
      val = val->next;
    }
    else {
      break;
    }
  }
  return null;
}

void ___alloc_map_delete___(___alloc_map___ *map, uintptr_t key)
{
  if (key == null) {
    return;
  }
  size_t index = (size_t)___hash_ptr___(key) & (map->capacity - 1);
  ___alloc_map_value___ *val = map->values[index];
  bool found = false;
  ___alloc_map_value___ *prev_val = null;
  size_t chain_length = 0;
  while (val) {
    chain_length++;
    if (val->key == key) {
      found = true;
      if (val->next) {
        if (prev_val) {
          prev_val->next = val->next;
        }
        else {
          map->values[index] = val->next;
        }
      }
      else {
        if (prev_val) {
          prev_val->next = null;
        }
        if (chain_length == 1) {
          map->values[index] = null;
          map->used_capacity--;
        }
      }
      free(val);
      map->total_count--;
      break;
    }
    else {
      prev_val = val;
      val = val->next;
    }
  }
}

void ___alloc_map_put___(___alloc_map___ *map, uintptr_t key, uintptr_t value);

void ___alloc_map_grow___(___alloc_map___ *map, size_t new_capacity) {
  bool free_values = (map->capacity > 0);
  new_capacity = max(16, new_capacity);
  ___alloc_map___ new_map = {
    .values = calloc(1, new_capacity * sizeof(___alloc_map_value___ *)),
    .capacity = new_capacity,
  };
  for (size_t i = 0; i < map->capacity; i++) {
    ___alloc_map_value___ *val = map->values[i];
    while (val) {
      ___alloc_map_put___(&new_map, val->key, val->value);
      ___alloc_map_value___ *temp = val->next;
      free(val);
      val = temp;
    }
  }
  if (free_values) {
    free(map->values);
  }
  *map = new_map;
}

void ___alloc_map_put___(___alloc_map___ *map, uintptr_t key, uintptr_t value) {
  size_t index = (size_t)___hash_ptr___(key) & (map->capacity - 1);
  ___alloc_map_value___ *val = map->values[index];
  if (val) {
    ___alloc_map_value___ *last_val = val;
    while (val) {
      last_val = val;
      val = val->next;
    }
    last_val->next = calloc(1, sizeof(___alloc_map_value___));
    last_val->next->key = key;
    last_val->next->value = value;
  }
  else {
    if (2 * map->used_capacity >= map->capacity) {
      ___alloc_map_grow___(map, 2 * map->capacity);
    }
    ___alloc_map_value___ *new_value = calloc(1, sizeof(___alloc_map_value___));
    new_value->key = key;
    new_value->value = value;
    index = (size_t)___hash_ptr___(key) & (map->capacity - 1);
    val = map->values[index];
    if (val) {
      val->next = new_value;
    }
    else {
      map->values[index] = new_value;
      map->used_capacity++;
    }
  }

  map->total_count++;
}

void ___alloc_map_free___(___alloc_map___ *map) {
  for (size_t i = 0; i < map->capacity; i++) {
    ___alloc_map_value___ *val = map->values[i];
    while (val) {
      ___alloc_map_value___ *temp = val->next;
      free(___get_hdr_ptr___(val->key));
      free(val);
      val = temp;
    }
  }
  free(map->values);
  map->total_count = 0;
  map->used_capacity = 0;
  map->capacity = 0;
}

___alloc_map___ *___allocs___;
___alloc_map___ *___gc_allocs___;

uintptr_t ___stack_begin___;

void *___calloc_wrapper___(size_t num_bytes, bool gc) {
  void *memory = calloc(1, num_bytes + sizeof(___alloc_hdr___));
  if (!memory) {
    perror("Allocation failed");
    exit(1);
  }
  ___alloc_hdr___ *hdr = (___alloc_hdr___ *)memory;
  hdr->size = num_bytes;
  uintptr_t obj_ptr = (uintptr_t)___get_obj_ptr___(memory);
  if (gc)
  {
    ___alloc_map_put___(___gc_allocs___, obj_ptr, obj_ptr);
  } else
  {
    ___alloc_map_put___(___allocs___, obj_ptr, obj_ptr);
  }
  return (void *)obj_ptr;
}

void* ___alloc___(size_t num_bytes) {
  return ___calloc_wrapper___(num_bytes, false);
}

void *___realloc_wrapper___(void *ptr, size_t num_bytes, bool gc) { 
  if (ptr == null) {
    return ___calloc_wrapper___(num_bytes, gc);
  }    
  if (gc) {
      ___alloc_map_delete___(___gc_allocs___, (uintptr_t)ptr);
  }
  else {
      ___alloc_map_delete___(___allocs___, (uintptr_t)ptr);
  }
  ptr = realloc(___get_hdr_ptr___(ptr), num_bytes + sizeof(___alloc_hdr___));
  if (!ptr) {
    perror("Reallocation failed");
    exit(1);
  }
  uintptr_t obj_ptr = (uintptr_t)___get_obj_ptr___(ptr);
  if (gc) {
      ___alloc_map_put___(___gc_allocs___, (uintptr_t)obj_ptr, (uintptr_t)obj_ptr);
  }
  else {
      ___alloc_map_put___(___allocs___, (uintptr_t)obj_ptr, (uintptr_t)obj_ptr);
  }
  ___alloc_hdr___ *hdr = (___alloc_hdr___ *)ptr;
  hdr->size = num_bytes;
  return (void *)obj_ptr;
}

void* ___realloc___(void* ptr, size_t num_bytes) {
  return ___realloc_wrapper___(ptr, num_bytes, false);
}

void ___free___(void* ptr) {
  if (ptr) {
    ___alloc_map_delete___(___allocs___, (uintptr_t)ptr);
    free(___get_hdr_ptr___(ptr));
  }
}

void* ___managed_alloc___(size_t num_bytes) {
  return ___calloc_wrapper___(num_bytes, true);
}

void* ___managed_realloc___(void* ptr, size_t num_bytes) {
  return ___realloc_wrapper___(ptr, num_bytes, true);
}

void ___managed_free___(void* ptr) {
  if (ptr) {
    ___alloc_map_delete___(___gc_allocs___, (uintptr_t)ptr);
    free(___get_hdr_ptr___(ptr));
  }
}

void ___gc_init___() {
  int value_on_stack = 1;
  ___stack_begin___ = (uintptr_t)&value_on_stack;
  ___gc_allocs___ = calloc(1, sizeof(___alloc_map___));
  ___allocs___ = calloc(1, sizeof(___alloc_map___));
  ___alloc_map_grow___(___gc_allocs___, 16);
  ___alloc_map_grow___(___allocs___, 16);
}

void ___scan_for_pointers___(uintptr_t memory_block_begin, size_t byte_count) {
  uintptr_t memory_block_end = ((uintptr_t)memory_block_begin + align_down(byte_count, 8));
  for (uintptr_t ptr = memory_block_begin; ptr < memory_block_end; ptr += sizeof(uintptr_t)) {
    uintptr_t potential_ptr = *((uintptr_t *)ptr);
    uintptr_t obj_ptr = 0;
    if (potential_ptr != 0) {
      obj_ptr = (uintptr_t)___alloc_map_get___(___gc_allocs___, potential_ptr);
    }
    if (obj_ptr) {
      ___alloc_hdr___ *hdr = ___get_hdr_ptr___(obj_ptr);
      if (hdr->size & ___alive_tag___) {
          continue;
      }
      size_t size = hdr->size;
      ___tag___(hdr->size, ___alive_tag___);
      ___scan_for_pointers___(obj_ptr, size);
    }
  }
}

void ___mark_stack___(void) {
  int value_on_stack = 0;
  uintptr_t stack_begin = ___stack_begin___;
  uintptr_t stack_end = (uintptr_t)&value_on_stack;
  if (stack_begin > stack_end) {
    uintptr_t temp = stack_end;
    stack_end = stack_begin;
    stack_begin = temp;
  }
  ___scan_for_pointers___(stack_begin, stack_end - stack_begin);
}

void ___mark_heap___(void) {
  for (size_t i = 0; i < ___allocs___->capacity; i++) {
    ___alloc_map_value___ *val = ___allocs___->values[i];
    while (val) {
      uintptr_t obj_ptr = val->value;
      ___alloc_hdr___ *hdr = ___get_hdr_ptr___(obj_ptr);
      ___scan_for_pointers___((uintptr_t)obj_ptr, hdr->size);
      val = val->next;
    }
  }
}

typedef struct ___list_hdr___ {
  bool is_managed;
  size_t length;
  size_t capacity;
  char* buffer;
} ___list_hdr___;

___list_hdr___* ___list_initialize___(size_t initial_capacity, size_t element_size, bool managed) {  
  ___list_hdr___* hdr = 0;
  if (managed) {   
    hdr = (___list_hdr___*)___managed_alloc___(sizeof(___list_hdr___));
    hdr->buffer = (char*)___managed_alloc___(initial_capacity * element_size);
  }
  else {
    hdr = (___list_hdr___*)___alloc___(sizeof(___list_hdr___));
    hdr->buffer = (char*)___alloc___(initial_capacity * element_size);
  }
  hdr->length = 0;
  hdr->capacity = initial_capacity;
  hdr->is_managed = managed;
  return hdr;
}

void ___list_grow___(___list_hdr___* hdr, size_t new_length, size_t element_size) {
  if (hdr) {
    size_t new_capacity = max(1 + 2 * hdr->capacity, new_length);  
    if (hdr->is_managed) {
      hdr->buffer = (char*)___managed_realloc___(hdr->buffer, new_capacity * element_size);
    }
    else {
      hdr->buffer = (char*)___realloc___(hdr->buffer, new_capacity * element_size);
    }        
    hdr->capacity = new_capacity;
  }   
}

bool ___check_list_fits___(___list_hdr___* hdr, size_t increase) {
  if (hdr) {
    if (hdr->length + increase < hdr->capacity) {
      return true;
    }
  }
  return false;
}

void ___list_fit___(___list_hdr___* hdr, size_t increase, size_t element_size) {
  if (hdr) {
    if (false == ___check_list_fits___(hdr, increase)) {      
      ___list_grow___(hdr, hdr->length + increase, element_size);
    }
  }
}

void ___list_remove_at___(___list_hdr___* hdr, size_t element_size, size_t index) {
  if (hdr && hdr->length > index) {
    memcpy(
      hdr->buffer + (index * element_size),
      hdr->buffer + ((hdr->length - 1) * element_size),
      element_size);
    memset(hdr->buffer + ((hdr->length - 1) * element_size), 0, element_size);
    hdr->length--;
  }
}

void ___list_free_internal__(___list_hdr___* hdr) {
  if (hdr) {
    if (hdr->is_managed) {
      ___managed_free___(hdr->buffer);
      ___managed_free___(hdr);
    }
    else {
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

void ___sweep___(void) {
  ___list_hdr___ *garbage = ___list_initialize___(2, sizeof(uintptr_t), false);
  for (size_t i = 0; i < ___gc_allocs___->capacity; i++) {
    ___alloc_map_value___ *val = ___gc_allocs___->values[i]; 
    while (val) {
      ___alloc_map_value___ *next = val->next;
      ___alloc_hdr___ *hdr = ___get_hdr_ptr___(val->value);
      if (false == (hdr->size & ___alive_tag___)) {
        ___list_add___(garbage, (uintptr_t)hdr, uintptr_t);
      }
      else {
        ___untag___(hdr->size, ___alive_tag___);
      }
      val = next;
    }
  }
  for (size_t i = 0; i < garbage->length; i++) {
    uintptr_t hdr_ptr = (uintptr_t)(((uintptr_t*)garbage->buffer)[i]);    
    ___alloc_map_delete___(___gc_allocs___, (uintptr_t)___get_obj_ptr___(hdr_ptr));
    free((void *)hdr_ptr);
  }
  ___list_free___(garbage);
}

size_t query_gc_total_memory(void) {
  size_t result = 0;
  for (size_t i = 0; i < ___gc_allocs___->capacity; i++) {
    ___alloc_map_value___ *val = ___gc_allocs___->values[i];
    while (val) {
      ___alloc_hdr___ *hdr = ___get_hdr_ptr___(val->value);
      result += hdr->size;
      val = val->next;
    }
  }
  return result;
}

size_t query_gc_total_count(void) {
    return ___gc_allocs___->total_count;
}

void gc(void) {
  if (___gc_allocs___->total_count > 0) {
    ___mark_stack___();
    ___mark_heap___();
    ___sweep___();
  }
}

void ___clean_memory___(void) {
  if (___gc_allocs___ != 0) {
    ___alloc_map_free___(___gc_allocs___);
    ___alloc_map_free___(___allocs___);
    free(___gc_allocs___);
    free(___allocs___);
    ___gc_allocs___ = 0;
    ___allocs___ = 0;
  }
}

void *allocate(size_t num_bytes) {  
  return ___alloc___(num_bytes);
}

void *reallocate(void *ptr, size_t num_bytes) {
  return ___realloc___(ptr, num_bytes);
}

#define ulong unsigned long
#define uint unsigned int

