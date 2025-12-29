#!/usr/bin/env python3
"""
Script to automatically increment the patch version in version.h
This script is called during the build process to ensure each build has a unique version.
"""

import re
import os
import sys
from datetime import datetime

def increment_patch_version():
    """Increment the patch version in include/version.h"""

    version_file = os.path.join(os.path.dirname(__file__), '..', 'include', 'version.h')

    if not os.path.exists(version_file):
        print(f"Error: {version_file} not found")
        return False

    # Read the current version file
    with open(version_file, 'r') as f:
        content = f.read()

    # Find and increment the patch version
    patch_pattern = r'#define FIRMWARE_VERSION_PATCH (\d+)'
    match = re.search(patch_pattern, content)

    if not match:
        print("Error: Could not find FIRMWARE_VERSION_PATCH in version.h")
        return False

    current_patch = int(match.group(1))
    new_patch = current_patch + 1

    # Replace the patch version
    new_content = re.sub(patch_pattern, f'#define FIRMWARE_VERSION_PATCH {new_patch}', content)

    # Also update the version string
    # First extract major and minor versions
    major_match = re.search(r'#define FIRMWARE_VERSION_MAJOR (\d+)', new_content)
    minor_match = re.search(r'#define FIRMWARE_VERSION_MINOR (\d+)', new_content)

    if major_match and minor_match:
        major = major_match.group(1)
        minor = minor_match.group(1)
        new_version_string = f'#define FIRMWARE_VERSION_STRING "{major}.{minor}.{new_patch}"'
        version_string_pattern = r'#define FIRMWARE_VERSION_STRING ".*"'
        new_content = re.sub(version_string_pattern, new_version_string, new_content)

    # Write back the updated content
    with open(version_file, 'w') as f:
        f.write(new_content)

    print(f"Incremented patch version: {current_patch} -> {new_patch}")
    return True

if __name__ == "__main__":
    if increment_patch_version():
        print("Version incremented successfully")
        sys.exit(0)
    else:
        print("Failed to increment version")
        sys.exit(1)
