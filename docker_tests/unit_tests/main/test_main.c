#include <stdio.h>
#include <stdlib.h>
#include "unity.h"

// Test function declarations
void test_circular_buffer_real(void);
void test_version(void);
void test_temperature(void);

// Main test runner using Unity framework
int main(int argc, char *argv[])
{
  printf("Starting ESP32 Unit Tests...\n");

  UNITY_BEGIN();

  // Run all test groups
  test_circular_buffer_real();
  test_version();
  test_temperature();

  return UNITY_END();
}
