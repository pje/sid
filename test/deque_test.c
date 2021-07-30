#include "test_helper.h"
#include "../src/deque.h"

static void test_deque_initialize() {
  deque *dq = deque_initialize(3, stdout, _note_indexer, _note_node_print_function);

  assert_int_eq(3, dq->max_length);
  assert_int_eq(0, deque_length(dq));
  assert_int_eq(3, dq->ht->max_size);
  assert_not_null(dq->node_print_function);

  NOTE_FIXTURES;

  node node0 = { .data=note0, .previous=NULL, .next=NULL, .key=0 };
  node node1 = { .data=note1, .previous=NULL, .next=NULL, .key=1 };
  node node2 = { .data=note2, .previous=NULL, .next=NULL, .key=2 };

  node0.previous=NULL; node0.next=&node1;
  node1.previous=&node0; node1.next=&node2;
  node2.previous=&node1; node2.next=NULL;

  // set up the deque without prepend/append
  dq->ht->array[0] = (maybe_hash_table_element) { .exists=true, .unwrap={ .key=node0.key, .value=node0 } };
  dq->ht->array[1] = (maybe_hash_table_element) { .exists=true, .unwrap={ .key=node1.key, .value=node1 } };
  dq->ht->array[2] = (maybe_hash_table_element) { .exists=true, .unwrap={ .key=node2.key, .value=node2 } };
  dq->ht->size = 3;

  dq->first = &node0;
  dq->last = &node2;

  assert_long_eq((long)&node0, (long)dq->first);
  assert_long_eq((long)&node2, (long)dq->last);
  assert_int_eq(3, deque_length(dq));

  // traversing forward
  assert_int_eq(100, dq->first->data.number);
  assert_int_eq(101, dq->first->next->data.number);
  assert_int_eq(102, dq->first->next->next->data.number);
  assert_null(dq->first->next->next->next);

  // traversing backward
  assert_int_eq(102, dq->last->data.number);
  assert_int_eq(101, dq->last->previous->data.number);
  assert_int_eq(100, dq->last->previous->previous->data.number);
  assert_null(dq->last->previous->previous->previous);
}

static void test_deque_append_replace() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  NOTE_FIXTURES;

  deque_append_replace(dq, note0);
  assert_not_null(dq->first);
  assert_not_null(dq->last);
  assert_int_eq(1, deque_length(dq));
  assert_null(dq->last->next);
  assert_null(dq->last->previous);
  assert_int_eq(100, dq->last->data.number);
  assert_not_null(dq->last);

  deque_append_replace(dq, note1);
  assert_int_eq(101, dq->last->data.number);
  assert_int_eq(2, deque_length(dq));
  assert_not_null(dq->first->next);
  assert_int_eq(101, dq->first->next->data.number);
  assert_not_null(dq->last->previous);
  assert_int_eq(100, dq->last->previous->data.number);
  assert_not_null(dq->last);

  deque_append_replace(dq, note2);
  assert_int_eq(102, dq->last->data.number);
  assert_int_eq(3, deque_length(dq));
  assert_not_null(dq->last);

  deque_append_replace(dq, note2); // since we use a hashmap, append should *replace* in this case.
  assert_int_eq(102, dq->last->data.number);
  assert_int_eq(3, deque_length(dq));
  assert_not_null(dq->last);
  assert_true(dq->last != dq->last->previous); // ensure we don't have self-referential linked list nodes

  fprintf(stdout, "\n");
  deque_inspect(dq);
}

static void test_deque_prepend_replace() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  assert_int_eq(127, dq->max_length);
  assert_int_eq(0, deque_length(dq));
  assert_not_null(dq->node_print_function);

  NOTE_FIXTURES;

  deque_prepend_replace(dq, note2);
  assert_int_eq(102, dq->first->data.number);
  assert_not_null(dq->first);
  assert_not_null(dq->first);
  assert_not_null(dq->last);

  assert_long_eq((long)dq->last, (long)dq->first);
  assert_null(dq->first->next);
  assert_null(dq->first->previous);
  deque_prepend_replace(dq, note1);

  assert_int_eq(101, dq->first->data.number);
  assert_not_null(dq->first);
  assert_not_null(dq->last->previous);
  assert_int_eq(101, dq->last->previous->data.number);

  deque_prepend_replace(dq, note0);
  assert_int_eq(100, dq->first->data.number);
  assert_int_eq(3, deque_length(dq));

  deque_prepend_replace(dq, note0); // since we use a hashmap, append should *replace* in this case.
  assert_int_eq(100, dq->first->data.number);
  assert_int_eq(3, deque_length(dq));
  assert_not_null(dq->first);
  assert_true(dq->first != dq->first->next); // ensure we don't have self-referential linked list nodes

  fprintf(stdout, "\n");
  deque_inspect(dq);
}

static void test_deque_remove_first() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  NOTE_FIXTURES;

  deque_append_replace(dq, note0);
  deque_append_replace(dq, note1);
  deque_append_replace(dq, note2);

  maybe_node_data removed = deque_remove_first(dq);
  assert_true(removed.exists);
  assert_int_eq(100, removed.unwrap.number);
  assert_not_null(dq->first);                  // assert the `first` pointer got reset correctly
  assert_int_eq(101, dq->first->data.number);  // assert the `first` pointer got reset correctly
  assert_int_eq(2, deque_length(dq));

  removed = deque_remove_first(dq);
  assert_true(removed.exists);
  assert_int_eq(101, removed.unwrap.number);
  assert_not_null(dq->first);
  assert_int_eq(102, dq->first->data.number);
  assert_int_eq(1, deque_length(dq));

  removed = deque_remove_first(dq);
  assert_true(removed.exists);
  assert_int_eq(102, removed.unwrap.number);
  assert_null(dq->first);
  assert_int_eq(0, deque_length(dq));
}

static void test_deque_remove_last() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  NOTE_FIXTURES;

  deque_prepend_replace(dq, note2);
  deque_prepend_replace(dq, note1);
  deque_prepend_replace(dq, note0);

  maybe_node_data removed = deque_remove_last(dq);
  assert_true(removed.exists);
  assert_int_eq(102, removed.unwrap.number);
  assert_not_null(dq->last);                  // assert the `first` pointer got reset correctly
  assert_int_eq(101, dq->last->data.number);  // assert the `first` pointer got reset correctly
  assert_int_eq(2, deque_length(dq));

  removed = deque_remove_last(dq);
  assert_true(removed.exists);
  assert_int_eq(101, removed.unwrap.number);
  assert_not_null(dq->last);
  assert_int_eq(100, dq->last->data.number);
  assert_int_eq(1, deque_length(dq));

  removed = deque_remove_last(dq);
  assert_true(removed.exists);
  assert_int_eq(100, removed.unwrap.number);
  assert_null(dq->last);
  assert_int_eq(0, deque_length(dq));
}

static void test_deque_remove_by_key() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  NOTE_FIXTURES;

  deque_append_replace(dq, note0);
  deque_append_replace(dq, note1);
  deque_append_replace(dq, note2);

  maybe_node_data removed = deque_remove_by_key(dq, 10); // non-existent thing should be null
  assert_false(removed.exists);
  assert_int_eq(3, deque_length(dq));

  removed = deque_remove_by_key(dq, 100);
  assert_true(removed.exists);
  assert_int_eq(100, removed.unwrap.number);
  assert_int_eq(2, deque_length(dq));
  assert_not_null(dq->first);
  assert_int_eq(101, dq->first->data.number);
  assert_not_null(dq->first->next);
  assert_int_eq(102, dq->first->next->data.number);

  removed = deque_remove_by_key(dq, 101);
  assert_true(removed.exists);
  assert_int_eq(101, removed.unwrap.number);
  assert_int_eq(1, deque_length(dq));

  removed = deque_remove_by_key(dq, 102);
  assert_true(removed.exists);
  assert_int_eq(102, removed.unwrap.number);
  assert_int_eq(0, deque_length(dq));
}

static void test_deque_find_by_key() {
  deque *dq = deque_initialize(127, stdout, _note_indexer, _note_node_print_function);

  NOTE_FIXTURES;

  deque_append_replace(dq, note0);
  deque_append_replace(dq, note1);
  deque_append_replace(dq, note2);
  assert_int_eq(3, deque_length(dq));

  note *found = deque_find_by_key(dq, 100);
  assert_not_null(found);
  assert_int_eq(100, found->number);
  found = deque_find_by_key(dq, 101);
  assert_not_null(found);
  assert_int_eq(101, found->number);
  found = deque_find_by_key(dq, 102);
  assert_not_null(found);
  assert_int_eq(102, found->number);
  found = deque_find_by_key(dq, 3);
  assert_null(found);

  deque_empty(dq);
  deque_append_replace(dq, note0);
  deque_append_replace(dq, note1);
  deque_append_replace(dq, note2);
  assert_int_eq(3, deque_length(dq));
}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0); // disable buffering on stdout

  test_deque_initialize();
  test_deque_append_replace();
  test_deque_prepend_replace();
  test_deque_remove_first();
  test_deque_remove_last();
  test_deque_remove_by_key();
  test_deque_find_by_key();

  printf("\n");

  return TEST_FAILURE_COUNT;
}
