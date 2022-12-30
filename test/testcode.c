#include "stdio.h"

// FORWARD DECLARATIONS

typedef struct tree tree;
typedef struct node node;
void push_node(node *n);
node *pop_node(void);
void _depth_first(void);
void _depth_first_rec(node *n);
void depth_first_traversal(tree *t);
void push_to_queue(node *n);
node *pop_from_queue(void);
void _breadth_first(void);
void breadth_first_search(tree *t);
int search_for_value(node *n, int value);
void check_depth(node *n);
int get_tree_depth(tree *t);
void check_path_sum(node *n);
int get_max_path_sum(tree *t);
void invert_left_right(node *n);
void invert_binary_tree(tree *t);
void print_node(node *n);
void print_tree(tree *t);
void tree_test(void);
void assert(int b);

// DECLARATIONS

struct tree {
  node *root;
};
struct node {
  int value;
  node *left;
  node *right;
};
enum { MAX_STACK = 1024 };
node *(stack[MAX_STACK]);
node *last_node;
int node_count = 1;
void push_node(node *n){
  if ((node_count) < (MAX_STACK)) {
    node_count++;
    last_node = n;
  }
};
node *pop_node(void){
  if ((node_count) > (0)) {
    node_count--;
    return last_node;
  }
  else 
  {
    return 0;
  }
};
void _depth_first(void){
  node *n = pop_node();
  if (n) {
    if (n->right) {
      push_node(n->right);
    }
    if (n->left) {
      push_node(n->left);
    }
    printf("%lld ", n->value);
  }
};
void _depth_first_rec(node *n){
  printf("%lld ", n->value);
  if (n->left) {
    _depth_first_rec(n->left);
  }
  if (n->right) {
    _depth_first_rec(n->right);
  }
};
void depth_first_traversal(tree *t){
  push_node(t->root);
  printf("\ntree depth first traversal: ", 0);
  _depth_first_rec(t->root);
};
node *(cirular_queue[MAX_STACK]);
int max_index = (MAX_STACK) - (1);
int first_index;
int last_index;
int element_count;
void push_to_queue(node *n){
  if ((element_count) < (MAX_STACK)) {
    cirular_queue[last_index] = n;
    last_index++;
    if ((last_index) > (max_index)) {
      last_index = 0;
    }
    element_count++;
  }
};
node *pop_from_queue(void){
  if ((element_count) <= (0)) {
    return 0;
  }
  node *result = cirular_queue[first_index];
  element_count--;
  first_index++;
  if ((first_index) > (max_index)) {
    first_index = 0;
  }
  return result;
};
void _breadth_first(void){
  node *n = pop_from_queue();
  printf("%lld ", n->value);
  if (n) {
    if (n->left) {
      push_to_queue(n->left);
    }
    if (n->right) {
      push_to_queue(n->right);
    }
  }
};
void breadth_first_search(tree *t){
  push_to_queue(t->root);
  printf("\ntree breadth first traversal: ", 0);
  while ((element_count) > (0)) {
    _breadth_first();
  }
};
int search_for_value(node *n, int value){
  if ((n) == (0)) {
    return 0;
  }
  else 
  if ((n->value) == (value)) {
    return 1;
  }
  else 
  if (search_for_value(n->left, value)) {
    return 1;
  }
  else 
  if (search_for_value(n->right, value)) {
    return 1;
  }
  else 
  {
    return 0;
  }
};
int max_depth;
int current_depth;
void check_depth(node *n){
  current_depth++;
  if (n->left) {
    check_depth(n->left);
  }
  if (n->right) {
    check_depth(n->right);
  }
  current_depth--;
  if ((current_depth) > (max_depth)) {
    max_depth = current_depth;
  }
};
int get_tree_depth(tree *t){
  max_depth = 0;
  current_depth = 0;
  check_depth(t->root);
  printf("\ntree depth: %lld", max_depth);
  return max_depth;
};
int max_path_sum;
int current_path_sum;
void check_path_sum(node *n){
  if ((n) == (0)) {
    return;
  }
  current_path_sum += n->value;
  if (n->left) {
    check_path_sum(n->left);
  }
  if (n->right) {
    check_path_sum(n->right);
  }
  if (((n->left) == (0)) && ((n->right) == (0))) {
    if ((current_path_sum) > (max_path_sum)) {
      max_path_sum = current_path_sum;
    }
  }
  current_path_sum -= n->value;
};
int get_max_path_sum(tree *t){
  max_path_sum = 0;
  current_path_sum = 0;
  check_path_sum(t->root);
  printf("\ntree max path sum: %lld", max_path_sum);
  return max_path_sum;
};
void invert_left_right(node *n){
  if (n) {
    node *temp = n->right;
    n->right = n->left;
    n->left = temp;
    invert_left_right(n->left);
    invert_left_right(n->right);
  }
};
void invert_binary_tree(tree *t){
  invert_left_right(t->root);
};
void print_node(node *n){
  printf("( %lld ", n->value);
  if (n->left) {
    print_node(n->left);
  }
  if (n->right) {
    print_node(n->right);
  }
  printf(")", 0);
};
void print_tree(tree *t){
  printf("\n", 0);
  print_node(t->root);
};
void tree_test(void){
  node a = (node ){1};
  node b = (node ){2};
  node c = (node ){3};
  node d = (node ){4};
  node e = (node ){5};
  node f = (node ){6};
  node g = (node ){7};
  node h = (node ){8};
  a.left = &(b);
  a.right = &(c);
  b.left = &(d);
  b.right = &(e);
  d.left = &(g);
  d.right = &(h);
  c.right = &(f);
  tree t = (tree ){0};
  t.root = &(a);
  push_node(t.root);
  node *result = pop_node();
  assert((result) == (t.root));
  push_to_queue(&(a));
  push_to_queue(&(b));
  push_to_queue(&(c));
  assert((pop_from_queue()) == (&(a)));
  assert((pop_from_queue()) == (&(b)));
  assert((pop_from_queue()) == (&(c)));
  printf(" // binary tree test // ", 0);
  breadth_first_search(&(t));
  depth_first_traversal(&(t));
  assert(search_for_value(t.root, 7));
  assert((0) == (search_for_value(t.root, 9)));
  assert((0) == (search_for_value(t.root, 0)));
  int depth = get_tree_depth(&(t));
  assert((depth) == (3));
  int sum = get_max_path_sum(&(t));
  printf("\noriginal tree:", 0);
  print_tree(&(t));
  invert_binary_tree(&(t));
  printf("\ninverted tree:", 0);
  print_tree(&(t));
};
void assert(int b){
  if ((b) == (0)) {
    int y = 0;
    int x = (10) / (y);
  }
};
int main(int argc, char** argv) { tree_test(); }