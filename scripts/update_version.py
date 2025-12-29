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

def update_version_json(version="1.0.0", description="ESP32 filament dryer firmware"):
    """Update the version.json file with current build information."""

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
    version = sys.argv[1] if len(sys.argv) > 1 else "1.0.0"
    description = sys.argv[2] if len(sys.argv) > 2 else "ESP32 filament dryer firmware with improved versioning"

    update_version_json(version, description)
