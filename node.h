#ifndef NODE_H
#define NODE_H

#include <stdbool.h>

// a node in a doubly-linked list

// BEGIN "generic section" for this cursed language
#include "note.h"
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

#endif /* NODE_H */
