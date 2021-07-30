#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "../src/util.h"

// here's the test framework

volatile unsigned int TEST_FAILURE_COUNT = 0;
#define EPSILON 0.00001
#define varname(var) #var

#define reset_color() {                                                        \
  printf("\033[0m");                                                           \
}

#define color_red() {                                                          \
  printf("\033[0;31m");                                                        \
}

#define color_green() {                                                        \
  printf("\033[0;32m");                                                        \
}

#define assert_null(thing) {                                                   \
  if (thing != NULL) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure! expected `%s' to be NULL\n",         \
            __FILE__, __LINE__, __func__, varname(thing));                     \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_not_null(thing) {                                               \
  if (thing == NULL) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure! did not expect `%s' to be NULL\n",   \
            __FILE__, __LINE__, __func__, varname(thing));                     \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_true(thing) {                                                   \
  if (thing != true) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure! expected `%s' to be true, got %d\n", \
            __FILE__, __LINE__, __func__, varname(thing), thing);              \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_false(thing) {                                                  \
  if (thing != false) {                                                        \
    printf("\n%s:%d in %s(): \n\tfailure! expected `%s' to be false, got %d\n",\
            __FILE__, __LINE__, __func__, varname(thing), thing);              \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_float_eq(expected, actual) {                                    \
  double diff = fabs(expected - actual);                                       \
  if (diff > EPSILON) {                                                        \
    printf("\n%s:%d in %s(): \n\tfailure! expected %lf, got %lf\n",            \
      __FILE__, __LINE__, __func__, expected, actual);                         \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_int_eq(expected, actual) {                                      \
  if (expected != actual) {                                                    \
    printf("\n%s:%d in %s(): \n\tfailure! expected %i, got %i\n",              \
      __FILE__, __LINE__, __func__, expected, actual);                         \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_byte_eq(expected, actual) {                                     \
  if (expected != actual) {                                                    \
    printf("\n%s:%d in %s(): \n\tfailure! expected: ",                         \
      __FILE__, __LINE__, __func__);                                           \
    color_green();                                                             \
    printf("0B");                                                              \
    print_byte_in_binary(expected);                                            \
    reset_color();                                                             \
    printf("\n\t              got: ");                                         \
    color_red()                                                                \
    printf("0B");                                                              \
    print_byte_in_binary(actual);                                              \
    reset_color();                                                             \
    printf("\n");                                                              \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_long_eq(expected, actual) {                                     \
  if (expected != actual) {                                                    \
    printf("\n%s:%d in %s(): \n\tfailure! expected %zu, got %zu\n",            \
      __FILE__, __LINE__, __func__, expected, actual);                         \
    TEST_FAILURE_COUNT++;                                                      \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define NOTE_FIXTURES                                                          \
  note note0 = {.number=100, .on_time=1, .off_time=2, .voiced_by_oscillator=0};\
  note note1 = {.number=101, .on_time=2, .off_time=3, .voiced_by_oscillator=1};\
  note note2 = {.number=102, .on_time=3, .off_time=4, .voiced_by_oscillator=2};

#endif /* TEST_HELPER_H */
