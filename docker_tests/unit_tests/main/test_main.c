#include <stdio.h>
#include <stdlib.h>
#include "unity.h"

// Test function declarations
void test_circular_buffer(void);
void test_version(void);
void test_temperature(void);

// Main test runner using Unity framework
void setUp(void)
{
  // Called before each test
}

void tearDown(void)
{
  // Called after each test
}

int main(int argc, char *argv[])
{
  printf("Starting ESP32 Unit Tests...\n");

  UNITY_BEGIN();

  // Run test groups
  test_circular_buffer();
  test_version();
  test_temperature();

  return UNITY_END();
}
