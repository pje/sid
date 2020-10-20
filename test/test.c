#include "../util.h"
#include "../deque.h"
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define EPSILON 0.00001

#define varname(var) #var

#define assert_float_eq(expected, actual) {                                    \
  double diff = fabs(expected - actual);                                       \
  if (diff > EPSILON) {                                                        \
    printf("\n%s:%d in %s(): \n\tfailure: expected %lf, got %lf\n", __FILE__,  \
            __LINE__, __func__, expected, actual);                             \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_int_eq(expected, actual) {                                      \
  if (expected != actual) {                                                    \
    printf("\n%s:%d in %s(): \n\tfailure: expected %i, got %i\n", __FILE__,    \
            __LINE__, __func__, expected, actual);                             \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_long_eq(expected, actual) {                                     \
  if (expected != actual) {                                                    \
    printf("\n%s:%d in %s(): \n\tfailure: expected %zu, got %zu\n", __FILE__,  \
            __LINE__, __func__, expected, actual);                             \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_null(thing) {                                                   \
  if (thing != NULL) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure: expected `%s' to be NULL\n",         \
            __FILE__, __LINE__, __func__, varname(thing));                     \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_not_null(thing) {                                               \
  if (thing == NULL) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure: did not expect `%s' to be NULL\n",   \
            __FILE__, __LINE__, __func__, varname(thing));                     \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

int test_sine_waveform() {
  double result = sine_waveform(1.0, 0, 1.0, 0.0);
  assert_float_eq(0.0, result);

  result = sine_waveform(1.0, 0.25, 1.0, 0.0);
  assert_float_eq(1.0, result);

  result = sine_waveform(1.0, 0.5, 1.0, 0.0);
  assert_float_eq(0.0, result);

  result = sine_waveform(1.0, 0.75, 1.0, 0.0);
  assert_float_eq(-1.0, result);

  result = sine_waveform(1.0, 1.0, 1.0, 0.0);
  assert_float_eq(0.0, result);

  return 0;
}

int test_linear_envelope() {
  double result = 0;
  // attack
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 3.0, -1.0);
  assert_float_eq(0.3, result);

  // decay
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 12.5, -1.0);
  assert_float_eq(0.75, result);

  // sustain
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 20, -1.0);
  assert_float_eq(0.5, result);

  // release
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 20, 2.5);
  assert_float_eq(0.25, result);

  return 0;
}

int test_deque() {
  deque *dq = (deque *)malloc(sizeof(deque));
  deque_initialize(dq, 127, _note_indexer, _note_node_print_function);

  assert_not_null(dq->node_print_function);

  note note0 = { .number=100, .on_time=1, .off_time=2, .voiced_by_oscillator=0 };
  note note1 = { .number=101, .on_time=2, .off_time=3, .voiced_by_oscillator=1 };
  note note2 = { .number=102, .on_time=3, .off_time=4, .voiced_by_oscillator=2 };

  // append
  assert_long_eq(0L, dq->length);
  deque_append(dq, &note0);
  assert_not_null(dq->first);
  assert_not_null(dq->last);
  assert_null(dq->last->next);
  assert_null(dq->last->previous);
  assert_int_eq(100, dq->last->data->number);

  deque_append(dq, &note1);
  assert_int_eq(101, dq->last->data->number);
  assert_not_null(dq->first->next);
  assert_int_eq(101, dq->first->next->data->number);
  assert_not_null(dq->last->previous);
  assert_int_eq(100, dq->last->previous->data->number);

  deque_append(dq, &note2);
  assert_int_eq(102, dq->last->data->number);
  assert_long_eq(3L, dq->length);
  deque_inspect(dq);

  // prepend
  deque_empty(dq);
  assert_long_eq(0L, dq->length);

  deque_prepend(dq, &note2);
  assert_not_null(dq->first);
  assert_not_null(dq->last);
  assert_null(dq->first->next);
  assert_null(dq->first->previous);
  assert_int_eq(102, dq->first->data->number);

  deque_prepend(dq, &note1);
  assert_int_eq(101, dq->first->data->number);
  assert_not_null(dq->last->previous);
  assert_int_eq(101, dq->last->previous->data->number);

  deque_prepend(dq, &note0);
  assert_int_eq(100, dq->first->data->number);
  assert_long_eq(3L, dq->length);
  deque_inspect(dq);

  // traversing forward
  assert_int_eq(100, dq->first->data->number);
  assert_int_eq(101, dq->first->next->data->number);
  assert_int_eq(102, dq->first->next->next->data->number);
  assert_null(dq->first->next->next->next);

  // traversing backward
  assert_int_eq(102, dq->last->data->number);
  assert_int_eq(101, dq->last->previous->data->number);
  assert_int_eq(100, dq->last->previous->previous->data->number);
  assert_null(dq->last->previous->previous->previous);

  // remove_first
  deque_empty(dq);
  deque_prepend(dq, &note2);
  deque_prepend(dq, &note1);
  deque_prepend(dq, &note0);
  note *removed = deque_remove_first(dq);
  assert_int_eq(100, removed->number);
  assert_long_eq(2L, dq->length);

  removed = deque_remove_first(dq);
  assert_int_eq(101, removed->number);
  assert_long_eq(1L, dq->length);

  removed = deque_remove_first(dq);
  assert_int_eq(102, removed->number);
  assert_long_eq(0L, dq->length);

  // remove_last
  deque_empty(dq);
  deque_prepend(dq, &note2);
  deque_prepend(dq, &note1);
  deque_prepend(dq, &note0);
  removed = deque_remove_last(dq);
  assert_int_eq(102, removed->number);
  assert_long_eq(2L, dq->length);

  removed = deque_remove_last(dq);
  assert_int_eq(101, removed->number);
  assert_long_eq(1L, dq->length);

  removed = deque_remove_last(dq);
  assert_int_eq(100, removed->number);
  assert_long_eq(0L, dq->length);

  // find
  deque_empty(dq);
  deque_append(dq, &note0);
  deque_append(dq, &note1);
  deque_append(dq, &note2);
  assert_long_eq(3L, dq->length);

  note *found = NULL;
  found = deque_find(dq, 0);
  assert_not_null(found);
  assert_int_eq(100, found->number);
  found = deque_find(dq, 1);
  assert_not_null(found);
  assert_int_eq(101, found->number);
  found = deque_find(dq, 2);
  assert_not_null(found);
  assert_int_eq(102, found->number);
  found = deque_find(dq, 3);
  assert_null(found);

  // remove
  deque_empty(dq);
  deque_append(dq, &note0);
  deque_append(dq, &note1);
  deque_append(dq, &note2);
  assert_long_eq(3L, dq->length);

  removed = deque_remove(dq, 10);
  assert_null(removed);
  assert_long_eq(3L, dq->length);

  removed = deque_remove(dq, 0);
  assert_not_null(removed);
  assert_int_eq(100, removed->number);
  assert_long_eq(2L, dq->length);
  assert_not_null(dq->first);
  assert_int_eq(101, dq->first->data->number);
  assert_not_null(dq->first->next);
  assert_int_eq(102, dq->first->next->data->number);

  removed = deque_remove(dq, 1);
  assert_not_null(removed);
  assert_int_eq(102, removed->number);
  assert_long_eq(1L, dq->length);

  removed = deque_remove(dq, 0);
  assert_not_null(removed);
  assert_int_eq(101, removed->number);
  assert_long_eq(0L, dq->length);

  // find_by_index
  deque_empty(dq);
  deque_append(dq, &note0);
  deque_append(dq, &note1);
  deque_append(dq, &note2);
  assert_long_eq(3L, dq->length);

  found = deque_find_by_index(dq, 100);
  assert_not_null(found);
  assert_int_eq(100, found->number);
  found = deque_find_by_index(dq, 101);
  assert_not_null(found);
  assert_int_eq(101, found->number);
  found = deque_find_by_index(dq, 102);
  assert_not_null(found);
  assert_int_eq(102, found->number);
  found = deque_find_by_index(dq, 3);
  assert_null(found);

  deque_empty(dq);
  deque_append(dq, &note0);
  deque_append(dq, &note1);
  deque_append(dq, &note2);
  assert_long_eq(3L, dq->length);

  // remove_by_index
  removed = deque_remove_by_index(dq, 127); // expected to print a warning
  assert_null(removed);
  assert_long_eq(3L, dq->length);

  removed = deque_remove_by_index(dq, 100);
  assert_not_null(removed);
  assert_int_eq(100, removed->number);
  assert_long_eq(2L, dq->length);
  assert_not_null(dq->first);
  assert_int_eq(101, dq->first->data->number);
  assert_not_null(dq->first->next);
  assert_int_eq(102, dq->first->next->data->number);

  removed = deque_remove_by_index(dq, 101);
  assert_not_null(removed);
  assert_int_eq(101, removed->number);
  assert_long_eq(1L, dq->length);

  removed = deque_remove_by_index(dq, 102);
  assert_not_null(removed);
  assert_int_eq(102, removed->number);
  assert_long_eq(0L, dq->length);

  // TODO: ring buffer version tests
  deque_empty(dq);

  dq = (deque *)malloc(sizeof(deque));
  deque_initialize(dq, 127, _note_indexer, _note_node_print_function);

  assert_long_eq(126, _deque_start_index_minus(dq, 1));
  assert_long_eq(125, _deque_start_index_minus(dq, 2));
  assert_long_eq(124, _deque_start_index_minus(dq, 3));

  dq->node_index_function = NULL; // use the default ring buffer impl instead
  assert_long_eq(0L, dq->start_index);
  assert_long_eq(0L, dq->end_index);

  deque_prepend(dq, &note2);
  assert_long_eq(126L, dq->start_index);
  assert_long_eq(0L, dq->end_index);

  deque_prepend(dq, &note1);
  assert_long_eq(125L, dq->start_index);
  assert_long_eq(0L, dq->end_index);

  deque_prepend(dq, &note0);
  assert_long_eq(124L, dq->start_index);
  assert_long_eq(0L, dq->end_index);

  return 0;
}

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main() {
  signal(SIGSEGV, handler); // handle segfaults with a line number printer
  setvbuf(stdout, NULL, _IONBF, 0); // disable buffering on stdout

  test_sine_waveform();
  test_linear_envelope();
  test_deque();

  printf("\n");

  return 0;
}
