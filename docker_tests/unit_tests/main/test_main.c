#include <stdio.h>
#include <stdlib.h>
#include "unity.h"

// Test function declarations
void test_circular_buffer_real(void);
void test_temp(void);
void test_version(void);
void test_temperature(void);
void test_controller_group_runner(void); // New: Controller test runner

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
  test_circular_buffer_real();
  test_temp();
  test_version();
  test_temperature();
  test_controller_group_runner(); // New: Call controller tests

  return UNITY_END();
}
