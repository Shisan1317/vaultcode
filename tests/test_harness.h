#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef bool (*test_fn_t)(void);

typedef struct {
    const char *name;
    test_fn_t   function;
} test_case_t;

#define TEST_ASSERT(condition)                                                \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "    assertion failed: %s (%s:%d)\n",           \
                    #condition, __FILE__, __LINE__);                          \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_EQ_INT(expected, actual)                                  \
    do {                                                                      \
        int test_expected = (expected);                                       \
        int test_actual = (actual);                                           \
        if (test_expected != test_actual) {                                   \
            fprintf(stderr,                                                   \
                    "    expected: %d, actual: %d (%s:%d)\n",                \
                    test_expected, test_actual, __FILE__, __LINE__);           \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_EQ_SIZE(expected, actual)                                 \
    do {                                                                      \
        size_t test_expected = (expected);                                    \
        size_t test_actual = (actual);                                        \
        if (test_expected != test_actual) {                                   \
            fprintf(stderr,                                                   \
                    "    expected: %zu, actual: %zu (%s:%d)\n",              \
                    test_expected, test_actual, __FILE__, __LINE__);           \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_STREQ(expected, actual)                                   \
    do {                                                                      \
        const char *test_expected = (expected);                               \
        const char *test_actual = (actual);                                   \
        bool test_equal =                                                     \
            (test_expected == NULL && test_actual == NULL) ||                 \
            (test_expected != NULL && test_actual != NULL &&                  \
             strcmp(test_expected, test_actual) == 0);                        \
        if (!test_equal) {                                                    \
            fprintf(stderr,                                                   \
                    "    expected: \"%s\", actual: \"%s\" (%s:%d)\n",      \
                    test_expected ? test_expected : "(null)",                \
                    test_actual ? test_actual : "(null)",                    \
                    __FILE__, __LINE__);                                      \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_NULL(value)                                               \
    do {                                                                      \
        const void *test_actual = (value);                                    \
        if (test_actual != NULL) {                                            \
            fprintf(stderr,                                                   \
                    "    expected: NULL, actual: %p (%s:%d)\n",              \
                    (void *)test_actual, __FILE__, __LINE__);                  \
            return false;                                                     \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_NOT_NULL(value)                                           \
    do {                                                                      \
        const void *test_actual = (value);                                    \
        if (test_actual == NULL) {                                            \
            fprintf(stderr,                                                   \
                    "    expected: non-NULL, actual: NULL (%s:%d)\n",         \
                    __FILE__, __LINE__);                                      \
            return false;                                                     \
        }                                                                     \
    } while (0)

#endif /* TEST_HARNESS_H */
