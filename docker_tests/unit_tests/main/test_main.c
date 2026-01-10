#include <stdio.h>
#include <stdlib.h>
#include "test_framework.h"

// Global test counters (defined once here)
int tests_run = 0;
int failures = 0;

// Test function declarations
void test_circular_buffer(void);
void test_version(void);
void test_temperature(void);

// Main test runner using simple framework
int main(int argc, char *argv[])
{
  printf("Starting ESP32 Unit Tests...\n");

  // Run test groups
  test_circular_buffer();
  test_version();
  test_temperature();

  printf("All unit tests completed: %d passed, %d failed\n", tests_run, failures);

  return failures;
}
