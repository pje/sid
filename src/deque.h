#ifndef SRC_DEQUE_H
#define SRC_DEQUE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "hash_table.h"

// A deque (double-ended queue) implemented using a fixed-size doubly-linked list
// backed by a simple fixed-size hash table using open addressing
//
// Implements O(1) average-case prepend, append, remove_first, remove_last, find(i)*, remove(i)*
//
// [*]: if `node_index_function` isn't implemented, `find(i)` and remove(i) are O(n).

typedef struct deque deque;

// function that takes a NODE_DATA and returns a unique array index for that
// node. The index will be used as for this node's slot in the deque's
// underlying array. (must be idempotent!)
typedef unsigned int (node_index_function_t)(deque *dq, NODE_DATA n);
// function that takes a node (for printing) and returns void
typedef void (node_print_function_t)(node *n, FILE *stream);
// function that takes a deque (for printing) and returns void
typedef void (deque_print_function)(deque *dq);

struct deque {
  node *first;
  node *last;
  unsigned int max_length;
  hash_table *ht;
  FILE *stream;
  node_index_function_t *node_index_function;
  node_print_function_t *node_print_function;
  deque_print_function *root_print_function;
};

struct maybe_node_data {
  bool exists; // if `exists` is not `true`, then the value returned by `unwrap` is undefined.
  NODE_DATA unwrap;
};
typedef struct maybe_node_data maybe_node_data;

deque *deque_initialize(unsigned int max_length, FILE *stream, node_index_function_t *node_indexer, node_print_function_t *node_printer);
unsigned int deque_length(deque *dq);
void deque_append_replace(deque *dq, NODE_DATA node_data);
void deque_prepend_replace(deque *dq, NODE_DATA node_data);
maybe_node_data deque_remove_first(deque *dq);
maybe_node_data deque_remove_last(deque *dq);
maybe_node_data deque_remove_by_key(deque *dq, unsigned int k);
NODE_DATA *deque_find_by_key(deque *dq, unsigned int k);
node *deque_find_node_by_key(deque *dq, unsigned int k);
void deque_inspect(deque *dq);
void deque_empty(deque *dq);
void deque_free(deque *dq);
// "private" below
static maybe_node_data _deque_remove_helper(deque *dq, unsigned int key);
static void _note_node_print_function(node *n, FILE *stream);
static void _deque_inspect_nodes(deque *dq, node *n, node_print_function_t *print_node, FILE *stream, unsigned int iterations);
static void _deque_root_print_function(deque *l);

// BEGIN the section of code that stands for "generic" in this cursed language
unsigned int _note_indexer(__attribute__ ((unused)) deque *dq, note n) {
  return n.number;
}

NODE_DATA node_data_init() {
  NODE_DATA nd = {.number=0, .on_time=0, .off_time=0, .voiced_by_oscillator=0};
  return(nd);
}

static void _note_node_print_function(node *n, FILE *stream) {
  if (!n) {
    fprintf(stream, "%s", "NONE");
    return;
  }
  node *next = n->next;
  node *previous = n->previous;

  char data_as_string[5];
  char next_as_string[5];
  char previous_as_string[5];

  snprintf(data_as_string, sizeof(data_as_string), "%i", n->data.number);

  if (next) {
    snprintf(next_as_string, sizeof(next_as_string), "%i", next->data.number);
  } else {
    snprintf(next_as_string, sizeof(next_as_string), "%s", "_");
  }

  if (previous) {
    snprintf(previous_as_string, sizeof(previous_as_string), "%i", previous->data.number);
  } else {
    snprintf(previous_as_string, sizeof(previous_as_string), "%s", "_");
  }

  fprintf(stream, "(#%s %s< >%s)", data_as_string, previous_as_string, next_as_string);
}
// END the section of code that stands for "generic" in this cursed language

// O(n)
void deque_inspect(deque *dq) {
  _deque_root_print_function(dq);
  _deque_inspect_nodes(dq, dq->first, dq->node_print_function, dq->stream, 0);
  fprintf(dq->stream, "%s", "\n");
}

// O(1)
void deque_empty(deque *dq) {
  dq->first = NULL;
  dq->last = NULL;
  hash_table_empty(dq->ht);
}

// O(n)
deque *deque_initialize(unsigned int max_length, FILE *stream, node_index_function_t *node_indexer, node_print_function_t *node_printer) {
  deque *dq = (deque *)malloc(sizeof(deque));
  dq->max_length = max_length;
  dq->first = NULL;
  dq->last = NULL;
  dq->ht = hash_table_initialize(dq->max_length); // sizeof(ht) + array[max_length] of nodes
  dq->stream = stream;
  dq->node_index_function = node_indexer;
  dq->node_print_function = node_printer;
  dq->root_print_function = _deque_root_print_function;
  deque_empty(dq);
  return dq;
}

// O(1)
unsigned int deque_length(deque *dq) {
  return dq->ht->size;
}

// O(1)
void deque_free(deque *dq) {
  if (dq->ht) {
    hash_table_free(dq->ht);
  }
  free(dq);
}

void deque_append_replace(deque *dq, NODE_DATA node_data) {
  int key = dq->node_index_function(dq, node_data);
  node *new_node = NULL;
  node *former_lasts_previous = dq->last ? dq->last->previous : NULL;

  while (new_node == NULL) {
    new_node = hash_table_set(
      dq->ht,
      key,
      (node){ .data=node_data, .previous=dq->last, .next=NULL, .key=key }
    );

    // the only way this returns NULL is if it fails to set, which means the
    // hash is out of space, so delete the oldest element first and try again
    // NB: if this loops more than once, something went extremely wrong
    if (!new_node) {
      deque_remove_first(dq);
    }
  }

  node *last = dq->last;

  if (!last) {
    dq->last = new_node;
    dq->first = new_node;
    return;
  }

  if (last->key == new_node->key) { // we replaced a node that had the same key.
    new_node->next = NULL;
    new_node->previous = former_lasts_previous;
  } else {
    last->next = new_node;
    new_node->previous = last;
  }

  dq->last = new_node;
}

// // O(1)
void deque_prepend_replace(deque *dq, NODE_DATA node_data) {
  int key = dq->node_index_function(dq, node_data);
  node *new_node = NULL;
  node *former_firsts_next = dq->first ? dq->first->next : NULL;

  while (new_node == NULL) {
    new_node = hash_table_set(
      dq->ht,
      key,
      (node){ .data=node_data, .previous=NULL, .next=dq->first, .key=key }
    );

    // the only way this returns NULL is if it fails to set, which means the
    // hash is out of space, so delete the newest element first and try again
    // NB: if this loops more than once, something went extremely wrong
    if (!new_node) {
      deque_remove_last(dq);
    }
  }

  node *first = dq->first;

  if (!first) {
    dq->first = new_node;
    dq->last = new_node;
    return;
  }

  if (first->key == new_node->key) { // we replaced a node that had the same key.
    new_node->next = former_firsts_next;
    new_node->previous = NULL;
  } else {
    first->previous = new_node;
    new_node->next = first;
  }

  dq->first = new_node;
}

// O(1)
maybe_node_data deque_remove_first(deque *dq) {
  node *first = dq->first;
  if (first == NULL) {
    return (maybe_node_data){ .exists=false };
  }
  node *next = dq->first->next;
  NODE_DATA value = first->data;

  maybe_hash_table_val removed = hash_table_remove(dq->ht, first->key);

  if (!removed.exists) { exit(EXIT_FAILURE); } // well this is unexpected!

  if (next) {
    dq->first = next;
  } else {
    dq->first = NULL;
    dq->last = NULL;
  }

  return (maybe_node_data){ .exists=true, .unwrap=value };
}

// O(1)
maybe_node_data deque_remove_last(deque *dq) {
  node *last = dq->last;
  if (last == NULL) {
    return (maybe_node_data){ .exists=false };
  }
  node *previous = dq->last->previous;

  NODE_DATA value = last->data;

  maybe_hash_table_val removed = hash_table_remove(dq->ht, last->key);

  if (!removed.exists) { exit(EXIT_FAILURE); } // well this is unexpected!

  if (previous) {
    dq->last = previous;
  } else {
    dq->first = NULL;
    dq->last = NULL;
  }

  return (maybe_node_data){ .exists=true, .unwrap=value };
}

// O(1)
maybe_node_data deque_remove_by_key(deque *dq, unsigned int k) {
  return _deque_remove_helper(dq, k);
}

// O(1)
NODE_DATA *deque_find_by_key(deque *dq, unsigned int k) {
  HASH_TABLE_VAL *result = hash_table_get(dq->ht, k);
  return(result ? &result->data : NULL);
}

// O(1)
node *deque_find_node_by_key(deque *dq, unsigned int k) {
  HASH_TABLE_VAL *result = hash_table_get(dq->ht, k);
  return(result ? result : NULL);
}

// private below
static maybe_node_data _deque_remove_helper(deque *dq, unsigned int k) {
  maybe_hash_table_val removed = hash_table_remove(dq->ht, k);
  if (!removed.exists) {
    return (maybe_node_data){ .exists=false };
  }

  node *next = removed.unwrap.next;
  node *previous = removed.unwrap.previous;

  if (previous) {
    previous->next = next;
  }
  if (next) {
    next->previous = previous;
  }

  if (previous == NULL) { // it was the first node
    dq->first = next;
  }
  if (next == NULL) { // it was the last node
    dq->last = previous;
  }

  return (maybe_node_data){ .exists=true, .unwrap=removed.unwrap.data };
}

static void _deque_inspect_nodes(deque *dq, node *n, node_print_function_t *print_node, FILE *stream, unsigned int iterations) {
  if (n == NULL) {
    return;
  }
  if (iterations > dq->max_length) {
    fprintf(stream, "⚠️ INFINITE LOOP DETECTED in deque!\n");
    return;
  }
  print_node(n, stream);
  if (n->next) {
    fprintf(stream, "%s", "-");
    _deque_inspect_nodes(dq, n->next, print_node, stream, ++iterations);
  } else {
    return;
  }
}

static void _deque_root_print_function(deque *dq) {
  fprintf(dq->stream, "dq(%d/%d): ", deque_length(dq), dq->max_length);
}

#endif /* SRC_DEQUE_H */
