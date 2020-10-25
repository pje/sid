#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <stdio.h>
#include <stdbool.h>

// here's the test framework

#define EPSILON 0.00001

#define varname(var) #var

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

#define assert_true(thing) {                                                   \
  if (thing != true) {                                                         \
    printf("\n%s:%d in %s(): \n\tfailure: expected `%s' to be true, got %d\n", \
            __FILE__, __LINE__, __func__, varname(thing), thing);              \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

#define assert_false(thing) {                                                  \
  if (thing != false) {                                                        \
    printf("\n%s:%d in %s(): \n\tfailure: expected `%s' to be false, got %d\n",\
            __FILE__, __LINE__, __func__, varname(thing), thing);              \
  } else {                                                                     \
    printf(".");                                                               \
  }                                                                            \
}

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

#endif /* TEST_HELPER_H */
