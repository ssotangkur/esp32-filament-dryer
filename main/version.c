#include "version.h"
#include <string.h>
#include <stdio.h>

// Get firmware version as string
const char *get_firmware_version_string(void)
{
  return FIRMWARE_VERSION_STRING;
}

// Get firmware version as structured data
firmware_version_t get_firmware_version(void)
{
  firmware_version_t version = {
      .major = FIRMWARE_VERSION_MAJOR,
      .minor = FIRMWARE_VERSION_MINOR,
      .patch = FIRMWARE_VERSION_PATCH};
  return version;
}

// Get complete firmware information
firmware_info_t get_firmware_info(void)
{
  firmware_info_t info = {
      .version = get_firmware_version(),
      .git_commit = GIT_COMMIT,
      .build_date = BUILD_DATE,
      .target = "esp32s3"};
  return info;
}

// Parse version string into components
firmware_version_t parse_version_string(const char *version_str)
{
  firmware_version_t version = {0, 0, 0};

  if (version_str == NULL)
  {
    return version;
  }

  // Parse format: "MAJOR.MINOR.PATCH"
  sscanf(version_str, "%hhu.%hhu.%hhu",
         &version.major, &version.minor, &version.patch);

  return version;
}

// Compare two version strings semantically
// Returns: 1 if version1 > version2, -1 if version1 < version2, 0 if equal
int compare_versions(const char *version1, const char *version2)
{
  if (version1 == NULL || version2 == NULL)
  {
    return 0;
  }

  firmware_version_t v1 = parse_version_string(version1);
  firmware_version_t v2 = parse_version_string(version2);

  // Compare major version
  if (v1.major > v2.major)
    return 1;
  if (v1.major < v2.major)
    return -1;

  // Major versions equal, compare minor
  if (v1.minor > v2.minor)
    return 1;
  if (v1.minor < v2.minor)
    return -1;

  // Minor versions equal, compare patch
  if (v1.patch > v2.patch)
    return 1;
  if (v1.patch < v2.patch)
    return -1;

  // Versions are equal
  return 0;
}

// Check if remote version is newer than current version
bool is_version_newer(const char *current_version, const char *remote_version)
{
  return compare_versions(remote_version, current_version) > 0;
}
