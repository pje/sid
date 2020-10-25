#ifndef SRC_HASH_TABLE_H
#define SRC_HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// A simple fixed-size hash table using open addressing.
// intended for storing small-ish data, because all data is stored
// directly in the hash's array. no pointers.

// begin "generic" section
#include "node.h"

#ifndef HASH_TABLE_KEY
#define HASH_TABLE_KEY unsigned char
#endif

#ifndef HASH_TABLE_VAL
#define HASH_TABLE_VAL node
#endif

// end "generic" section

struct hash_table_element {
  HASH_TABLE_KEY key;
  HASH_TABLE_VAL value;
};
typedef struct hash_table_element hash_table_element;

struct maybe_hash_table_element {
  bool exists; // if `exists` is not `true`, then the value returned by `unwrap` is undefined.
  hash_table_element unwrap;
};
typedef struct maybe_hash_table_element maybe_hash_table_element;

struct maybe_hash_table_val {
  bool exists; // if `exists` is not `true`, then the value returned by `unwrap` is undefined.
  HASH_TABLE_VAL unwrap;
};
typedef struct maybe_hash_table_val maybe_hash_table_val;

struct maybe_uint {
  bool exists;
  unsigned int unwrap;
};
typedef struct maybe_uint maybe_uint;

struct hash_table {
  maybe_hash_table_element *array;
  unsigned int size;
  unsigned int max_size;
};
typedef struct hash_table hash_table;

hash_table *hash_table_initialize(unsigned int max_size);
void hash_table_free(hash_table *h);
HASH_TABLE_VAL *hash_table_set(hash_table *h, HASH_TABLE_KEY key, HASH_TABLE_VAL value);
HASH_TABLE_VAL *hash_table_get(hash_table *h, HASH_TABLE_KEY key);
maybe_hash_table_val hash_table_remove(hash_table *h, HASH_TABLE_KEY key);
void hash_table_empty(hash_table *h);
void hash_table_element_inspect(const hash_table_element *e);
void hash_table_inspect(const hash_table *h);
float hash_table_load_factor(hash_table *h);
float hash_table_collision_ratio(hash_table *h);
static HASH_TABLE_KEY hash(const hash_table *h, HASH_TABLE_KEY key);
static unsigned int hash_table_num_collisions(hash_table *h);
static maybe_uint _find_slot(const hash_table *h, HASH_TABLE_KEY key);

hash_table *hash_table_initialize(unsigned int max_size) {
  hash_table *h = (hash_table *) malloc(sizeof(hash_table));
  h->max_size = max_size;
  h->array = (maybe_hash_table_element *) malloc(sizeof(maybe_hash_table_element) * max_size);
  hash_table_empty(h);
  return h;
}

static HASH_TABLE_KEY hash(const hash_table *h, HASH_TABLE_KEY key) {
  return(key % h->max_size); // yes folks it's just that simple
}

HASH_TABLE_VAL *hash_table_get(hash_table *h, HASH_TABLE_KEY key) {
  maybe_uint slot = _find_slot(h, key);

  if (slot.exists) {
    maybe_hash_table_element element = h->array[slot.unwrap];
    if (element.exists) {
      return &h->array[slot.unwrap].unwrap.value;
    }
  }

  return NULL;
}

HASH_TABLE_VAL *hash_table_set(hash_table *h, HASH_TABLE_KEY key, HASH_TABLE_VAL value) {
  maybe_uint slot = _find_slot(h, key);

  if (slot.exists) {
    if (!h->array[slot.unwrap].exists) { // increment hashtable size only if nothing was already there
      h->size++;
    }
    maybe_hash_table_element e = { .exists=true, .unwrap={ .key=key, .value=value } };
    h->array[slot.unwrap] = e;
    return &h->array[slot.unwrap].unwrap.value;
  } else {
    return NULL; // array is full. value was not set.
  }
}

static maybe_uint _find_slot(const hash_table *h, HASH_TABLE_KEY key) {
  unsigned int index = hash(h, key);

  for (unsigned int i = index, max = index + h->max_size; i < max; i++) {
    maybe_hash_table_element maybe = h->array[i % h->max_size];

    if (!maybe.exists || maybe.unwrap.key == key) {
      return (maybe_uint){ .exists=true, .unwrap=i % h->max_size };
    }
  }

  // if we get to this point, the hash table is full and `key` ain't in it.
  // that's bad obviously, but since we're a fixed-size hash table, best to just
  // signal that failure to the caller so they can choose which element to
  // replace.
  return (maybe_uint){ .exists=false }; // alternative: `return index;`
}

maybe_hash_table_val hash_table_remove(hash_table *h, HASH_TABLE_KEY key) {
  unsigned int index = hash(h, key);
  maybe_hash_table_element null_element = { .exists=false };
  maybe_hash_table_val found = { .exists=false };
  unsigned int found_index = 0;
  maybe_hash_table_element swap_places_with = { .exists=false };
  unsigned int swap_places_with_index = 0;

  for (unsigned int i = index, max = index + h->max_size; i < max; i++) {
    unsigned int this_index = i % h->max_size;
    maybe_hash_table_element maybe = h->array[this_index];

    if (!maybe.exists) {
      break;
    }
    if (maybe.unwrap.key == key) {
      found = (maybe_hash_table_val) { .exists=true, .unwrap=maybe.unwrap.value };
      found_index = this_index;
    }
    if (hash(h, maybe.unwrap.key) == hash(h, key)) {
      swap_places_with.exists = true;
      swap_places_with.unwrap = maybe.unwrap;
      swap_places_with_index = this_index;
    }
  }

  if (found.exists) {
    if (swap_places_with.exists) {
      h->array[found_index] = swap_places_with;
      h->array[swap_places_with_index] = null_element;
    } else {
      h->array[found_index] = null_element;
    }
    h->size--;
  }

  return found;
}

float hash_table_load_factor(hash_table *h) {
  return((float)h->size / h->max_size);
}

static unsigned int hash_table_num_collisions(hash_table *h) {
  unsigned int num_collisions = 0;
  for (unsigned int i = 0; i < h->size; i++) {
    if (h->array[i].exists) {
      hash_table_element e = h->array[i].unwrap;

      if (hash(h, e.key) != i) {
        maybe_hash_table_element in_our_spot = h->array[hash(h, e.key)];

        if (in_our_spot.exists) {
          num_collisions++;
        }
      }
    }
  }

  return num_collisions;
}

float hash_table_collision_ratio(hash_table *h) {
  return((float) hash_table_num_collisions(h) / (float)h->size);
}

void hash_table_empty(hash_table *h) {
  for (unsigned int i = 0; i < h->max_size; i++) {
    h->array[i].exists = false;
  }
  h->size = 0;
}

void hash_table_element_inspect(const hash_table_element *e) {
  if (e) {
    // fprintf(stdout, "{k:%u,v:%u} ", e->key, e->value.data.number);
    fprintf(stdout, "{k:%u,v:*},", e->key);
  } else {
    fprintf(stdout, "{},");
  }
}

void hash_table_inspect(const hash_table *h) {
  fprintf(stdout, "h(%u/%u): { ", h->size, h->max_size);

  for (unsigned int i = 0; i < h->max_size; i++) {
    if (h->array[i].exists) {
      hash_table_element_inspect(&h->array[i].unwrap);
    } else {
      hash_table_element_inspect(NULL);
    }
  }

  fprintf(stdout, " }\n");
}

void hash_table_free(hash_table *h) {
  free(h->array);
  free(h);
}

#endif /* SRC_HASH_TABLE_H */
