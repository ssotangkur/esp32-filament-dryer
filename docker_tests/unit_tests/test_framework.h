#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Simple test framework macros
#define TEST_ASSERT_EQUAL(expected, actual)                                                                      \
  do                                                                                                             \
  {                                                                                                              \
    if ((expected) != (actual))                                                                                  \
    {                                                                                                            \
      printf("FAIL: %s:%d - Expected %zu, got %zu\n", __FILE__, __LINE__, (size_t)(expected), (size_t)(actual)); \
      failures++;                                                                                                \
    }                                                                                                            \
    else                                                                                                         \
    {                                                                                                            \
      printf("PASS: %s:%d\n", __FILE__, __LINE__);                                                               \
      tests_run++;                                                                                               \
    }                                                                                                            \
  } while (0)

#define TEST_ASSERT_TRUE(condition)                                     \
  do                                                                    \
  {                                                                     \
    if (!(condition))                                                   \
    {                                                                   \
      printf("FAIL: %s:%d - Condition is false\n", __FILE__, __LINE__); \
      failures++;                                                       \
    }                                                                   \
    else                                                                \
    {                                                                   \
      printf("PASS: %s:%d\n", __FILE__, __LINE__);                      \
      tests_run++;                                                      \
    }                                                                   \
  } while (0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                                                    \
  do                                                                                                         \
  {                                                                                                          \
    float diff = (expected) - (actual);                                                                      \
    if (diff < 0)                                                                                            \
      diff = -diff;                                                                                          \
    if (diff > (delta))                                                                                      \
    {                                                                                                        \
      printf("FAIL: %s:%d - Expected %f ± %f, got %f\n", __FILE__, __LINE__, (expected), (delta), (actual)); \
      failures++;                                                                                            \
    }                                                                                                        \
    else                                                                                                     \
    {                                                                                                        \
      printf("PASS: %s:%d\n", __FILE__, __LINE__);                                                           \
      tests_run++;                                                                                           \
    }                                                                                                        \
  } while (0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) TEST_ASSERT_TRUE((actual) > (threshold))

#define TEST_ASSERT_LESS_THAN(threshold, actual) TEST_ASSERT_TRUE((actual) < (threshold))

#define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT_TRUE((ptr) != NULL)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) TEST_ASSERT_TRUE(strcmp((expected), (actual)) == 0)

// Global test counters
extern int tests_run;
extern int failures;

// Test function runner macro
#define RUN_TEST(test_func)                \
  do                                       \
  {                                        \
    printf("Running %s...\n", #test_func); \
    test_func();                           \
  } while (0)

#endif // TEST_FRAMEWORK_H
