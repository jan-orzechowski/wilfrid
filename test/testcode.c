#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void* ___alloc_(size_t num_bytes)
{
    void* ptr = calloc(1, num_bytes);
    if (!ptr)
    {
        perror("allocation failed");
        exit(1);
    }
    return ptr;
}

void ___free_(void* ptr)
{
    // powinniśmy sprawdzić też naszą listę alokacji
    if (ptr)
    {
        free(ptr);
    }
}

// FORWARD DECLARATIONS

typedef struct node node;
typedef struct linked_list linked_list;
typedef struct memory memory;
void main(int arguments_count, char **arguments_array);
void linked_list_test(void);
void add_at_tail(linked_list *list, int value);
void add_at_head(linked_list *list, int value);
void print_linked_list(linked_list *list);
void remove_head(linked_list *list);
void remove_tail(linked_list *list);
void add_at_position(linked_list *list, int position, int value);
void remove_at_position(linked_list *list, int position);
void reverse(linked_list *list);

// DECLARATIONS

#line 1 "test/linked_lists.txt" 
struct node {
  #line 3 
  int value;
  node *next;
};
#line 7 
struct linked_list {
  #line 9 
  node *head;
  node *tail;
  int count;
};
#line 14 
struct memory {
  #line 16 
  void *address;
  int size;
};
#line 20 
void main(int arguments_count, char **arguments_array){
  #line 22 
  linked_list_test();
};
#line 25 
void linked_list_test(void){
  #line 27 
  linked_list (*list) = (linked_list*)___alloc_(sizeof(linked_list));
  #line 29 
  linked_list *list2 = 0;
  #line 31 
  add_at_tail(list, 4);
  add_at_tail(list, 5);
  add_at_tail(list, 6);
  add_at_head(list, 3);
  add_at_head(list, 2);
  add_at_head(list, 1);
  #line 38 
  print_linked_list(list);
  #line 40 
  remove_head(list);
  remove_tail(list);
  #line 43 
  print_linked_list(list);
  #line 45 
  add_at_position(list, 3, 9);
  add_at_position(list, 3, 8);
  remove_at_position(list, 4);
  #line 49 
  print_linked_list(list);
  #line 51 
  reverse(list);
  #line 53 
  print_linked_list(list);
};
#line 66 
void add_at_tail(linked_list *list, int value){
  #line 68 
  if (list->tail) {
    #line 70 
    node (*new_tail) = (node*)___alloc_(sizeof(node));
    new_tail->value = value;
    list->tail->next = new_tail;
    list->tail = new_tail;
    list->count++;
  } else 
  #line 77 
  {
    list->head = (node*)___alloc_(sizeof(node));
    list->head->value = value;
    list->tail = list->head;
    list->count++;
  }
};
#line 85 
void add_at_head(linked_list *list, int value){
  #line 87 
  if (list->head) {
    #line 89 
    node (*new_head) = (node*)___alloc_(sizeof(node));
    new_head->value = value;
    new_head->next = list->head;
    list->head = new_head;
    list->count++;
  } else 
  #line 96 
  {
    list->head = (node*)___alloc_(sizeof(node));
    list->head->value = value;
    list->tail = list->head;
    list->count++;
  }
};
#line 56 
void print_linked_list(linked_list *list){
  node (*n) = list->head;
  printf("\nlist: ", 0);
  while (n) {
    #line 61 
    printf("%d ", n->value);
    n = n->next;
  }
};
#line 145 
void remove_head(linked_list *list){
  #line 147 
  if ((list->count) == (0)) {
    #line 149 
    return;
  }
  #line 152 
  if ((list->count) == (1)) {
    #line 154 
    ___free_(list->head);
    list->head = 0;
    list->tail = 0;
    list->count = 0;
  } else 
  #line 160 
  {
    node (*old_head) = list->head;
    list->head = old_head->next;
    ___free_(old_head);
    list->count--;
  }
};
#line 168 
void remove_tail(linked_list *list){
  #line 170 
  if ((list->count) == (0)) {
    #line 172 
    return;
  }
  #line 175 
  if ((list->count) == (1)) {
    #line 177 
    ___free_(list->tail);
    list->head = 0;
    list->tail = 0;
    list->count = 0;
  } else 
  #line 183 
  {
    node (*n) = list->head;
    int counter = 0;
    while ((counter) < ((list->count) - (2))) {
      #line 188 
      n = n->next;
      counter++;
    }
    #line 192 
    ___free_(list->tail);
    n->next = 0;
    list->tail = n;
    list->count--;
  }
};
#line 104 
void add_at_position(linked_list *list, int position, int value){
  #line 106 
  if ((position) < (list->count)) {
    #line 108 
    int counter = 0;
    node (*n) = list->head;
    while ((counter) < ((position) - (1))) {
      #line 112 
      n = n->next;
      counter++;
    }
    #line 116 
    node (*node_after) = n->next;
    n->next = (node*)___alloc_(sizeof(node));
    n->next->value = value;
    n->next->next = node_after;
    list->count++;
  }
};
#line 124 
void remove_at_position(linked_list *list, int position){
  #line 126 
  if ((position) < (list->count)) {
    #line 128 
    int counter = 0;
    node (*n) = list->head;
    node *node_before = {0};
    while ((counter) < (position)) {
      #line 133 
      node_before = n;
      n = n->next;
      counter++;
    }
    #line 138 
    node (*node_after) = n->next;
    node_before->next = node_after;
    ___free_(n);
    list->count--;
  }
};
#line 199 
void reverse(linked_list *list){
  #line 201 
  if ((list->count) < (2)) {
    #line 203 
    return;
  }
  #line 206 
  node *prev = 0;
  node *curr = list->head;
  node *next = 0;
  #line 210 
  list->head = list->tail;
  list->tail = curr;
  #line 213 
  while (curr) {
    #line 215 
    next = curr->next;
    curr->next = prev;
    prev = curr;
    curr = next;
  }
};