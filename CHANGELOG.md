# Changelog

All notable changes to the ESP32 filament dryer firmware will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-12-29

### Added
- **Firmware Versioning System**: Complete semantic versioning implementation
  - Dedicated `version.h` header with semantic version components
  - Version comparison functions for proper OTA update checking
  - CMake auto-generation of build metadata (git commit, build date)
  - Version API functions for runtime access
- **OTA Improvements**: Enhanced over-the-air update system
  - Proper semantic version comparison instead of string comparison
  - Improved version checking and update detection
- **Build System Enhancements**: Automated version metadata generation
  - CMake integration for version header generation
  - Python script for version.json updates
  - Git integration for commit tracking
- **Development Tools**: Added local OTA server and development scripts
  - Node.js OTA server for local firmware hosting
  - Version update scripts for CI/CD integration

### Changed
- **Version Management**: Moved from hardcoded strings to structured semantic versioning
- **OTA Logic**: Replaced simple string comparison with proper semantic version comparison
- **Build Process**: Added automated version metadata generation

### Technical Details
- Firmware version components: MAJOR=1, MINOR=0, PATCH=0
- Target platform: ESP32-S3
- Build system: ESP-IDF v5.5.1 with CMake
- Version format: Semantic versioning 2.0.0 compliant

---

## Versioning Guidelines

This project follows semantic versioning:

- **MAJOR**: Breaking changes (API changes, major rewrites)
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

### Release Process

1. Update version numbers in `include/version.h`
2. Update `CHANGELOG.md` with changes
3. Create git tag: `git tag -a v1.2.3 -m "Release version 1.2.3"`
4. Build and test firmware
5. Push tag: `git push origin --tags`

### Development

- Use `scripts/update_version.py` to update version metadata after builds
- Version information is automatically included in OTA server responses
- Firmware version is accessible at runtime via version API functions
