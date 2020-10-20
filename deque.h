#ifndef DEQUE_H
#define DEQUE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// A deque (double-ended queue) implemented using a fixed-size doubly-linked list
// backed by a ring buffer array. (Could easily be changed to auto-expand in size.)
//
// Similar to Scala's [ArrayDeque](https://www.scala-lang.org/api/current/scala/collection/mutable/ArrayDeque.html),
// but with some optimizations for O(1) deletion at the cost of worse space performance.
//
// Implements O(1) prepend, append, remove_first, remove_last, find(i)*, remove(i)*
//
// [*]: if `node_index_function` isn't implemented, `find(i)` and remove(i) are O(n).

// BEGIN the "generic section" for this cursed language
#include "note.h"
#define NODE_TYPE struct note
// END the "generic section" for this cursed language

typedef struct node node;
typedef struct deque deque;

// function that takes a NODE_TYPE and returns a unique array index for that
// node. The index will be used as for this node's slot in the deque's
// underlying array. (must be idempotent!)
typedef size_t (node_index_function_t)(deque *l, NODE_TYPE *n);
// function that takes a node (for printing) and returns void
typedef void (node_print_function_t)(node *n);
// function that takes a deque (for printing) and returns void
typedef void (list_print_function_t)(deque *l);

struct node {
  NODE_TYPE *data;
  node *previous;
  node *next;
  size_t index;
};

struct deque {
  node *first;
  node *last;
  size_t length;
  int max_length;
  int start_index;
  int end_index;
  node **data_array;
  node_index_function_t *node_index_function;
  node_print_function_t *node_print_function;
  list_print_function_t *root_print_function;
};

void deque_initialize(deque *list, size_t max_length, node_index_function_t *node_indexer, node_print_function_t *node_printer);
void deque_append(deque *list, NODE_TYPE *node_data);
void deque_prepend(deque *list, NODE_TYPE *node_data);
NODE_TYPE *deque_remove_first(deque *list);
NODE_TYPE *deque_remove_last(deque *list);
NODE_TYPE *deque_remove(deque *list, size_t n);
void deque_inspect(deque *list);
void deque_empty(deque *list);
void deque_free(deque *list);
bool is_empty(deque *list);
// "private" below
static node *_deque_find(node *list, size_t n);
static NODE_TYPE *_deque_remove_helper(deque *list, node *n);
static void _note_node_print_function(node *n);
static void _deque_inspect_nodes(node *n, node_print_function_t *print_node);
static void _deque_root_print_function(deque *l);
static int _deque_end_index_plus(deque *list, signed int i);
static int _deque_start_index_minus(deque *list, signed int i);
static int _deque_start_index_plus(deque *list, signed int i);

// BEGIN the section of code that stands for "generic" in this cursed language
size_t _note_indexer(deque *list, note *n) {
  if (NULL == n) {
    fprintf(stderr, "note_indexer was passed a NULL note pointer.");
    exit(EXIT_FAILURE);
  }

  return n->number;
}

static void _note_node_print_function(node *n) {
  if (n == NULL) {
    printf("NULL");
    return;
  }
  node *next = n->next;
  node *previous = n->previous;

  char data_as_string[5];
  char next_as_string[5];
  char previous_as_string[5];

  n->data != NULL ? snprintf(data_as_string, sizeof(data_as_string), "%i", n->data->number) : snprintf(data_as_string, sizeof(data_as_string), "%s", "NULL");
  next != NULL && next->data != NULL ? snprintf(next_as_string, sizeof(next_as_string), "%i", next->data->number) : snprintf(next_as_string, sizeof(next_as_string), "%s", "NULL");
  previous != NULL && previous->data != NULL ? snprintf(previous_as_string, sizeof(previous_as_string), "%i", previous->data->number) : snprintf(previous_as_string, sizeof(previous_as_string), "%s", "NULL");

  printf("(#%s %s ☚ ☛ %s)", data_as_string, previous_as_string, next_as_string);
}
// END the section of code that stands for "generic" in this cursed language

// O(n)
void deque_inspect(deque *list) {
  list->root_print_function(list);
  _deque_inspect_nodes(list->first, list->node_print_function);
  printf("\n");
}

// O(1)
void deque_empty(deque *list) {
  list->length = 0;
  list->start_index = 0;
  list->end_index = 0;
  list->first = NULL;
  list->last = NULL;
  list->data_array = (node **) malloc(sizeof(node *) * list->max_length);
}

// O(n)
void deque_initialize(deque *list, size_t max_length, node_index_function_t *node_indexer, node_print_function_t *node_printer) {
  list->max_length = max_length;
  list->start_index = 0;
  list->end_index = 0;
  list->first = NULL;
  list->last = NULL;
  list->data_array = (node **) malloc(sizeof(node *) * list->max_length);
  list->node_index_function = node_indexer;
  list->node_print_function = node_printer;
  list->root_print_function = _deque_root_print_function;
  deque_empty(list);
}

// O(1)
void deque_free(deque *list) {
  if (list->data_array) { free(list->data_array); }
  // free(list);
}

// O(1)
bool is_empty(deque *list) {
  return(!!(list->first));
}

// O(1)
void deque_append(deque *list, NODE_TYPE *node_data) {
  node *new_node = (struct node*) malloc(sizeof(struct node));

  size_t index = list->node_index_function
    ? list->node_index_function(list, node_data)
    : _deque_end_index_plus(list, 1);

  new_node->index = index;
  list->data_array[index] = new_node;

  new_node->next = NULL;
  new_node->previous = NULL;
  new_node->data = node_data;

  node *last = list->last;
  list->length++;
  list->end_index = index;
  list->last = new_node;
  if (!last) {
    list->first = new_node;
    return;
  }
  last->next = new_node;
  new_node->previous = last;
}

// O(1)
void deque_prepend(deque *list, NODE_TYPE *node_data) {
  node *new_node = (struct node*) malloc(sizeof(struct node));

  size_t index = list->node_index_function
    ? list->node_index_function(list, node_data)
    : _deque_start_index_minus(list, 1);

  new_node->index = index;
  list->data_array[index] = new_node;

  new_node->next = NULL;
  new_node->previous = NULL;
  new_node->data = node_data;

  node *first = list->first;
  list->length++;
  list->start_index = index;
  list->first = new_node;
  if (!first) {
    list->last = new_node;
    return;
  }
  first->previous = new_node;
  new_node->next = first;
}

// O(1)
NODE_TYPE *deque_remove_first(deque *list) {
  node *n = list->first;

  if (n->next) {
    list->first = n->next;
    list->length--;
  } else {
    list->first = NULL;
    list->last = NULL;
    list->length = 0;
  }

  return n->data;
}

// O(1)
NODE_TYPE *deque_remove_last(deque *list) {
  node *n = list->last;

  if (n->previous) {
    list->last = n->previous;
    list->length--;
  } else {
    list->first = NULL;
    list->last = NULL;
    list->length = 0;
  }

  return n->data;
}

// O(n)
NODE_TYPE *deque_find(deque *list, size_t n) {
  node *result = NULL;

  if (n == 0) {
    result = list->first;
  } else if (n == list->length - 1) {
    result = list->last;
  } else {
    result = _deque_find(list->first, n);
  }

  if (result == NULL) { return NULL; }

  return result->data;
}

// O(n)
NODE_TYPE *deque_remove(deque *list, size_t n) {
  if (n == list->length - 1) { return deque_remove_last(list); }
  if (n == 0) { return deque_remove_first(list); }

  node *nth = _deque_find(list->first, n);

  return _deque_remove_helper(list, nth);
}

// O(1)
NODE_TYPE *deque_remove_by_index(deque *list, size_t i) {
  if (i > (list->max_length - 1)) {
    fprintf(stderr, "deque_remove_by_index: attempt to access out-of-bounds array index %zu", i);
    return NULL;
  }

  return _deque_remove_helper(list, list->data_array[i]);
}

// O(1)
NODE_TYPE *deque_find_by_index(deque *list, size_t i) {
  if (i > (list->max_length - 1)) {
    fprintf(stderr, "deque_find_by_index: attempt to access out-of-bounds index %zu", i);
    return NULL;
  }

  node *result = list->node_index_function
    ? list->data_array[i]
    : list->data_array[_deque_start_index_plus(list, i)];

  return(result ? result->data : NULL);
}

// private below

static node *_deque_find(node *nd, size_t n) {
  if (NULL == nd) { return NULL; }
  if (0 == n) { return nd; }

  return _deque_find(nd->next, --n);
}

static NODE_TYPE *_deque_remove_helper(deque *list, node *n) {
  if (NULL == n) { return NULL; }

  node *next = n->next;
  node *previous = n->previous;

  if (previous) {
    previous->next = next;
  }
  if (next) {
    next->previous = previous;
  }

  if (list->first == n) { list->first = next; }
  if (list->last == n) { list->last = previous; }

  list->length--;
  list->data_array[n->index] = NULL;

  return n->data;
}

static void _deque_inspect_nodes(node *n, node_print_function_t *print_node) {
  if (n == NULL) {
    return;
  }
  print_node(n);
  if (n->next) {
    printf("-");
    _deque_inspect_nodes(n->next, print_node);
  } else {
    return;
  }
}

static void _deque_root_print_function(deque *l) {
  printf("deque(%zu): ", l->length);
}

static int _deque_start_index_minus(deque *list, signed int i) {
  int result = (list->start_index - i) % (list->max_length);
  return (result < 0) ? list->max_length + result : result;
}

static int _deque_start_index_plus(deque *list, signed int i) {
  int result = (list->start_index + i) % (list->max_length);
  return (result < 0) ? list->max_length + result : result;
}

static int _deque_end_index_plus(deque *list, signed int i) {
  int result = (list->end_index + i) % (list->max_length);
  return (result < 0) ? list->max_length + result : result;
}

#endif /* DEQUE_H */
