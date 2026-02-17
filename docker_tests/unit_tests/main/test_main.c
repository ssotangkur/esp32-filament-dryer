#include <stdio.h>
#include <stdlib.h>
#include "unity.h"

// Test function declarations
void test_circular_buffer_real(void);
void test_circular_buffer_subscriber_system_real(void);
// void test_temp(void);              // Disabled: requires temp.c with many dependencies (ui/subjects.c, etc.)
void test_version(void);
// void test_temperature(void);        // Disabled: test_temperature.c not compiled (standalone, can be re-enabled)
void test_controller_group_runner(void);
void test_linked_list_all(void);

// Global setUp and tearDown functions for Unity
void setUp(void) {
    // Optional: Add global setup logic here if needed for all tests.
    // For now, it remains empty to satisfy the linker.
}

void tearDown(void) {
    // Optional: Add global teardown logic here if needed for all tests.
    // For now, it remains empty to satisfy the linker.
}

// Main test runner using Unity framework
int main(int argc, char *argv[])
{
  printf("Starting ESP32 Unit Tests...\n");

  UNITY_BEGIN();

  // Run all test groups
  test_linked_list_all();
  test_circular_buffer_real();
  test_circular_buffer_subscriber_system_real();
  // test_temp();         // Disabled: requires temp.c with many dependencies
  test_version();
  // test_temperature();  // Disabled: test file not compiled
  test_controller_group_runner();

  return UNITY_END();
}
