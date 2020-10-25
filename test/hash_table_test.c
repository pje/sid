#include "test_helper.h"
#include "../note.h"
#define HASH_TABLE_VAL struct note // easier to test than node*
#include "../hash_table.h"

int test_hash_table() {
  hash_table *h = hash_table_initialize(3);

  assert_int_eq(3, h->max_size);
  assert_int_eq(0, h->size);

  note note0 = { .number=100, .on_time=0, .off_time=0, .voiced_by_oscillator=0 };
  note note1 = { .number=101, .on_time=1, .off_time=10, .voiced_by_oscillator=1 };
  note note2 = { .number=102, .on_time=2, .off_time=20, .voiced_by_oscillator=2 };

  // get/set no collisions
  HASH_TABLE_VAL *result0 = hash_table_set(h, 100, note0);
  assert_not_null(result0);
  assert_long_eq((long)result0, (long)hash_table_get(h, 100));
  assert_int_eq(1, h->size);

  HASH_TABLE_VAL *result1 = hash_table_set(h, 100, note0); // set should be idempotent
  assert_not_null(result1);
  assert_long_eq((long)result1, (long)hash_table_get(h, 100));
  assert_int_eq(1, h->size);                               // set should be idempotent

  HASH_TABLE_VAL *result2 = hash_table_set(h, 101, note1);
  assert_not_null(result2);
  assert_long_eq((long)result2, (long)hash_table_get(h, 101));
  assert_int_eq(2, h->size);

  HASH_TABLE_VAL *result3 = hash_table_set(h, 102, note2);
  assert_not_null(result3);
  assert_long_eq((long)result3, (long)hash_table_get(h, 102));
  assert_int_eq(3, h->size);

  result0 = hash_table_get(h, 100);
  assert_not_null(result0);
  assert_int_eq(100, result0->number);


  result1 = hash_table_get(h, 101);
  assert_not_null(result1);
  assert_int_eq(101, result1->number);

  result2 = hash_table_get(h, 102);
  assert_not_null(result2);
  assert_int_eq(102, result2->number);

  result3 = hash_table_get(h, 69); // shouldn't exist
  assert_null(result3);

  // get/set with collisions
  hash_table_free(h);
  h = hash_table_initialize(3);

  hash_table_set(h, 0, note0); // hashes to 0
  result0 = hash_table_get(h, 0);
  assert_not_null(result0);
  assert_int_eq(100, result0->number);
  assert_int_eq(100, h->array[0].unwrap.value.number);

  hash_table_set(h, 3, note1); // hashes to 0
  result1 = hash_table_get(h, 3);
  assert_not_null(result1);
  assert_int_eq(101, result1->number);
  assert_int_eq(101, h->array[1].unwrap.value.number);

  hash_table_set(h, 6, note2); // hashes to 0
  result2 = hash_table_get(h, 6);
  assert_not_null(result2);
  assert_int_eq(102, result2->number);
  assert_int_eq(102, h->array[2].unwrap.value.number);

  // remove, collisions
  hash_table_free(h);
  h = hash_table_initialize(3);
  maybe_hash_table_val maybe0 = {.exists=false};
  maybe_hash_table_val maybe1 = {.exists=false};
  maybe_hash_table_val maybe2 = {.exists=false};
  maybe_hash_table_val maybe3 = {.exists=false};
  unsigned int collision_index_0 = 0; // starting here, insert three collisions into the array
  unsigned int collision_index_1 = 1;
  unsigned int collision_index_2 = 2;
  hash_table_set(h, 0, note0); // hashes to 0
  hash_table_set(h, 3, note1); // hashes to 0
  hash_table_set(h, 6, note2); // hashes to 0

  maybe0 = hash_table_remove(h, 0);
  assert_true(maybe0.exists);
  assert_int_eq(2, h->size);
  assert_int_eq(100, maybe0.unwrap.number);
  assert_true(h->array[collision_index_0].exists);
  assert_int_eq(102, h->array[collision_index_0].unwrap.value.number);    // assert that we "patched the hole" using the final collision, leaving a contiguous span of two elements having the same hash value, with no nulls
  assert_true(h->array[collision_index_1].exists);
  assert_int_eq(101, h->array[collision_index_1].unwrap.value.number);
  assert_false(h->array[collision_index_2].exists);

  maybe1 = hash_table_remove(h, 6);
  assert_true(maybe1.exists);
  assert_int_eq(1, h->size);
  assert_int_eq(102, maybe1.unwrap.number);
  assert_true(h->array[collision_index_0].exists);
  assert_int_eq(101, h->array[collision_index_0].unwrap.value.number);    // assert that we "patched the hole" using the final collision
  assert_false(h->array[collision_index_1].exists);
  assert_false(h->array[collision_index_2].exists);

  maybe2 = hash_table_remove(h, 3);
  assert_true(maybe2.exists);
  assert_int_eq(0, h->size);
  assert_int_eq(101, maybe2.unwrap.number);
  assert_false(h->array[collision_index_0].exists);
  assert_false(h->array[collision_index_1].exists);
  assert_false(h->array[collision_index_2].exists);

  // load factor
  hash_table_free(h);
  h = hash_table_initialize(3);
  hash_table_set(h, 0, note0);
  hash_table_set(h, 3, note1);
  hash_table_set(h, 6, note2);

  maybe0 = hash_table_remove(h, 0);
  assert_true(maybe0.exists);
  assert_float_eq(2/3.0, hash_table_load_factor(h));

  maybe1 = hash_table_remove(h, 3);
  assert_true(maybe1.exists);
  assert_float_eq(1/3.0, hash_table_load_factor(h));

  maybe2 = hash_table_remove(h, 6);
  assert_true(maybe2.exists);
  assert_float_eq(0.0, hash_table_load_factor(h));

  maybe3 = hash_table_remove(h, 69); // shouldn't exist, shouldn't change load factor
  assert_false(maybe3.exists);
  assert_float_eq(0.0, hash_table_load_factor(h));

  hash_table_free(h);

  return 0;
}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0); // disable buffering on stdout

  test_hash_table();

  printf("\n");
  return 0;
}
