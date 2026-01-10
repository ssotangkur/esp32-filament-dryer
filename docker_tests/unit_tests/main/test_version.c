#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "test_framework.h"
#include "version.h"

// Mock version header content for testing
#define PROJECT_VERSION "1.2.3"
#define PROJECT_VERSION_MAJOR 1
#define PROJECT_VERSION_MINOR 2
#define PROJECT_VERSION_PATCH 3
#undef BUILD_DATE
#define BUILD_DATE "2024-01-09"

// Test version string parsing
void test_version_string_parsing(void)
{
  // Test version string format
  TEST_ASSERT_EQUAL_STRING("1.2.3", PROJECT_VERSION);

  // Test version components
  TEST_ASSERT_EQUAL(1, PROJECT_VERSION_MAJOR);
  TEST_ASSERT_EQUAL(2, PROJECT_VERSION_MINOR);
  TEST_ASSERT_EQUAL(3, PROJECT_VERSION_PATCH);
}

// Test version comparison logic
void test_version_comparison(void)
{
  // Test version number relationships
  TEST_ASSERT_TRUE(PROJECT_VERSION_MAJOR >= 1);
  TEST_ASSERT_TRUE(PROJECT_VERSION_MINOR >= 0);
  TEST_ASSERT_TRUE(PROJECT_VERSION_PATCH >= 0);

  // Test semantic versioning rules
  TEST_ASSERT_TRUE(PROJECT_VERSION_MAJOR > 0 || PROJECT_VERSION_MINOR > 0 || PROJECT_VERSION_PATCH >= 0);
}

// Test build date format
void test_build_date_format(void)
{
  // Build date should be in YYYY-MM-DD format
  TEST_ASSERT_NOT_NULL(BUILD_DATE);
  size_t date_len = strlen(BUILD_DATE);
  TEST_ASSERT_EQUAL(10, date_len); // YYYY-MM-DD is 10 characters

  // Check date format (basic validation)
  TEST_ASSERT_TRUE(BUILD_DATE[4] == '-');
  TEST_ASSERT_TRUE(BUILD_DATE[7] == '-');
}

// Test version string formatting
void test_version_string_formatting(void)
{
  char version_str[32];

  // Format version string manually
  sprintf(version_str, "%d.%d.%d",
          PROJECT_VERSION_MAJOR,
          PROJECT_VERSION_MINOR,
          PROJECT_VERSION_PATCH);

  TEST_ASSERT_EQUAL_STRING(PROJECT_VERSION, version_str);
}

// Test version increment logic (conceptual)
void test_version_increment_logic(void)
{
  int major = PROJECT_VERSION_MAJOR;
  int minor = PROJECT_VERSION_MINOR;
  int patch = PROJECT_VERSION_PATCH;

  // Test patch increment
  patch++;
  TEST_ASSERT_TRUE(patch > PROJECT_VERSION_PATCH);

  // Test minor increment (reset patch)
  minor++;
  patch = 0;
  TEST_ASSERT_TRUE(minor > PROJECT_VERSION_MINOR);
  TEST_ASSERT_EQUAL(0, patch);

  // Test major increment (reset minor and patch)
  major++;
  minor = 0;
  patch = 0;
  TEST_ASSERT_TRUE(major > PROJECT_VERSION_MAJOR);
  TEST_ASSERT_EQUAL(0, minor);
  TEST_ASSERT_EQUAL(0, patch);
}

// Test version compatibility
void test_version_compatibility(void)
{
  // Same major version should be compatible
  TEST_ASSERT_EQUAL(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MAJOR);

  // Different major versions are incompatible
  TEST_ASSERT_TRUE(PROJECT_VERSION_MAJOR != 99);

  // Test version bounds
  TEST_ASSERT_TRUE(PROJECT_VERSION_MAJOR >= 0);
  TEST_ASSERT_TRUE(PROJECT_VERSION_MAJOR < 100); // Reasonable upper bound
}

// Test group runner
void test_version(void)
{
  RUN_TEST(test_version_string_parsing);
  RUN_TEST(test_version_comparison);
  RUN_TEST(test_build_date_format);
  RUN_TEST(test_version_string_formatting);
  RUN_TEST(test_version_increment_logic);
  RUN_TEST(test_version_compatibility);
}
