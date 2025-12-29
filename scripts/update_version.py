#!/usr/bin/env python3
"""
Script to update version.json with current build information.
This script should be run after a successful build to update the version metadata.
"""

import json
import datetime
import subprocess
import os
import sys
import re

def get_git_info():
    """Get git commit hash and version info."""
    try:
        # Get short commit hash
        commit = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode().strip()

        # Get version tag if available
        try:
            version = subprocess.check_output(['git', 'describe', '--tags', '--dirty']).decode().strip()
        except subprocess.CalledProcessError:
            version = "1.0.0"

        return commit, version
    except subprocess.CalledProcessError:
        return "unknown", "1.0.0"

def get_current_version():
    """Read the current version from include/version.h"""
    version_file = os.path.join(os.path.dirname(__file__), '..', 'include', 'version.h')

    if not os.path.exists(version_file):
        print(f"Warning: {version_file} not found, using default version")
        return "1.0.0"

    try:
        with open(version_file, 'r') as f:
            content = f.read()

        # Extract version components
        major_match = re.search(r'#define FIRMWARE_VERSION_MAJOR (\d+)', content)
        minor_match = re.search(r'#define FIRMWARE_VERSION_MINOR (\d+)', content)
        patch_match = re.search(r'#define FIRMWARE_VERSION_PATCH (\d+)', content)

        if major_match and minor_match and patch_match:
            major = major_match.group(1)
            minor = minor_match.group(1)
            patch = patch_match.group(1)
            return f"{major}.{minor}.{patch}"
        else:
            print("Warning: Could not parse version from version.h, using default")
            return "1.0.0"
    except Exception as e:
        print(f"Warning: Error reading version.h: {e}, using default version")
        return "1.0.0"

def update_version_json(version=None, description="ESP32 filament dryer firmware"):
    """Update the version.json file with current build information."""

    # If no version provided, read it from version.h
    if version is None:
        version = get_current_version()

    commit, git_version = get_git_info()

    data = {
        "version": version,
        "build_date": datetime.date.today().isoformat(),
        "git_commit": commit,
        "git_version": git_version,
        "description": description,
        "target": "esp32s3"
    }

    # Ensure build directory exists
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build', 'esp32s3')
    os.makedirs(build_dir, exist_ok=True)

    version_file = os.path.join(build_dir, 'version.json')

    with open(version_file, 'w') as f:
        json.dump(data, f, indent=2)

    print(f"Updated {version_file} with:")
    print(f"  Version: {version}")
    print(f"  Build Date: {data['build_date']}")
    print(f"  Git Commit: {commit}")
    print(f"  Git Version: {git_version}")

if __name__ == "__main__":
    # Allow custom version and description from command line
    version = sys.argv[1] if len(sys.argv) > 1 else None
    description = sys.argv[2] if len(sys.argv) > 2 else "ESP32 filament dryer firmware with improved versioning"

    update_version_json(version, description)
