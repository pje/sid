#ifndef SRC_LIST_NODE_H
#define SRC_LIST_NODE_H

#include <stdbool.h>
#include "note.h"

// a node in a doubly-linked list

// BEGIN "generic section" for this cursed language
#ifndef NODE_DATA
#define NODE_DATA struct note
#endif
// END "generic section"

typedef struct node node;

struct node {
  NODE_DATA data;
  node *previous;
  node *next;
  unsigned int key;
};

#endif /* SRC_LIST_NODE_H */
