# Changelog

All notable changes to TR4QT will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Permanently disabled Linux ARM64 build in CI/CD workflow

## [3.15.0] - 2026-01-01

### Added
- CQ/S&P mode tracking in QSO records
- `is_run_qso` column in database (Migration 6)
- `IsRunMode` field in UDP RadioInfo broadcasts for N1MM+ compatibility
- Operating mode indicator in UDP broadcasts (CQ vs S&P)

### Changed
- QSO records now track whether logged in CQ/Run mode or S&P mode
- UDP broadcast manager includes operating mode state

## [3.14.0] - 2026-01-01

### Added
- Windows program icon and version information in .exe properties
- Serial port configuration settings (data bits, stop bits, parity)
- Windows resource file (`tr4qt.rc`) for executable metadata

### Changed
- Improved serial port control for radios requiring custom settings
- Windows installer now includes proper application metadata

## [3.13.0] - 2026-01-01

### Added
- Band map timeout settings (configurable spot expiration)
- Function keys reference window (F1-F12 quick reference)
- Preferences dialog section for band map configuration

### Changed
- Band map spots now expire after configurable timeout (default: 15 minutes)
- Function key window accessible via Help menu or F1

## [3.12.0] - 2026-01-01

### Changed
- Radio configuration dialog improvements
- Enhanced serial port settings UI
- Better organization of radio control settings

## [3.11.0] - 2026-01-01

### Added
- AUTO S&P mode with VFO movement detection
- Automatic mode switching when tuning radio

### Changed
- Operating mode (CQ/S&P) now responds to VFO frequency changes
- Improved mode detection logic

## [3.10.9] - 2026-01-01

### Fixed
- CTY.DAT update notification showing repeatedly after update
- Download finished signal now passes numerical CTY version

### Changed
- Streamlined CTY.DAT update flow

## [3.10.8] - 2026-01-01

### Fixed
- Download dialogs now wait for user acknowledgment
- Single modal dialog for download completion

### Changed
- Improved download feedback workflow

## [3.10.7] - 2026-01-01

### Changed
- Streamlined download dialogs with status bar feedback
- Reduced modal dialog interruptions

## [3.10.6] - 2026-01-01

### Added
- CTY.DAT update notification at startup
- Automatic check for country file updates on launch

## [3.10.5] - 2025-12-31

### Added
- DX Cluster modern formatting and layout improvements
- Enhanced spot display with better readability

### Changed
- Band map visual improvements
- DX Cluster window layout refinements

## [3.10.4] - 2025-12-31

### Fixed
- Test failures in v3.10.3
- Unit test suite stability improvements

## [3.10.3] - 2025-12-31

### Added
- Station info display with US call area coordinates
- Automatic coordinate lookup for US stations

### Changed
- Improved station information display

## [3.10.2] - 2025-12-31

### Fixed
- Minimum window width calculation (dynamic sizing)
- Window resize behavior improvements

## [3.10.1] - 2025-12-31

### Added
- ESC key handling improvements
- SCP (Super Check Partial) font size configuration

### Fixed
- Band button state synchronization issues
- Worker thread shutdown race conditions

### Changed
- Enhanced keyboard shortcut handling

## [3.10.0] - 2025-12-31

### Added
- Florida QSO Party contest support
- QSO Party base class for state/regional contests
- County-based multiplier system
- In-state vs out-of-state exchange handling

### Changed
- Contest architecture now supports QSO Party style events

---

## Maintenance Notes

### Version Numbering
- **Major.Minor.Patch** (e.g., 3.15.0)
- **Major**: Breaking changes or major feature releases
- **Minor**: New features, enhancements
- **Patch**: Bug fixes, small improvements

### Updating This Changelog
When releasing a new version:

1. Move items from `[Unreleased]` to a new version section
2. Add version number and date: `## [X.Y.Z] - YYYY-MM-DD`
3. Categorize changes:
   - **Added**: New features
   - **Changed**: Changes to existing functionality
   - **Deprecated**: Soon-to-be removed features
   - **Removed**: Removed features
   - **Fixed**: Bug fixes
   - **Security**: Security fixes
4. Update version in `src/core/Constants.h`
5. Update version in `installer/tr4qt.nsi`
6. Update version in `src/CMakeLists.txt`
7. Update version in `resources/tr4qt.rc`

### Links
[Unreleased]: https://github.com/ny4i/TR4QT/compare/v3.15.0...HEAD
[3.15.0]: https://github.com/ny4i/TR4QT/releases/tag/v3.15.0
[3.14.0]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.13.0]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.12.0]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.11.0]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.9]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.8]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.7]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.6]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.5]: https://github.com/ny4i/TR4QT/compare/v3.10.4...v3.15.0
[3.10.4]: https://github.com/ny4i/TR4QT/releases/tag/v3.10.4
[3.10.3]: https://github.com/ny4i/TR4QT/releases/tag/v3.10.3
[3.10.2]: https://github.com/ny4i/TR4QT/releases/tag/v3.10.2
[3.10.1]: https://github.com/ny4i/TR4QT/releases/tag/v3.10.1
[3.10.0]: https://github.com/ny4i/TR4QT/compare/v3.9.0...v3.10.0
