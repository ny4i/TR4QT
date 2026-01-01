# Changelog

All notable changes to TR4QT will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [3.16.0] - 2026-01-01

### Added
- Serial port dropdown with auto-detection in Radio Configuration dialog
- Automatic scanning of available serial ports using Qt SerialPort
- "Refresh" button to manually rescan for new USB devices
- Auto-refresh timer (every 5 seconds) when dialog is visible
- Port descriptions shown in dropdown (e.g., "COM3 (USB Serial Port)")
- Manual entry field as fallback for ports not auto-detected

### Changed
- Radio Configuration dialog now prefers dropdown selection over manual entry
- Serial port field updated with dual input: dropdown + manual entry
- Platform-specific placeholder text for better UX
- Auto-start/stop refresh timer on dialog show/hide

### Dependencies
- Added Qt6::SerialPort module to project
- Updated Windows CI to include qtserialport module

## [3.15.1] - 2026-01-01

### Fixed
- Windows serial port configuration - auto-format numeric port entries (e.g., "4" → "COM4")
- Windows users can now enter just the port number and it will be automatically formatted as COMn

### Changed
- Permanently disabled Linux ARM64 build in CI/CD workflow
- Updated serial port field placeholder text with platform-specific hints (Windows: "COM1 or just 1", Unix: "/dev/ttyUSB0")

## [3.15.0] - 2026-01-01

### Changed
- Cq/s&p mode tracking in qso records and udp broadcasts

## [3.14.0] - 2026-01-01

### Changed
- Serial port configuration settings (data bits, stop bits, parity)
- Windows program icon and version synchronization

## [3.13.0] - 2026-01-01

### Changed
- Band map timeout settings and function keys reference window

## [3.12.0] - 2026-01-01

### Changed
- Radio configuration improvements

## [3.11.0] - 2026-01-01

### Changed
- Auto s&p mode with vfo movement detection

## [3.10.9] - 2026-01-01

### Changed
- Pass numerical cty version directly in downloadfinished signal

### Fixed
- Ed persistent cty.dat update notification

## [3.10.8] - 2026-01-01

### Fixed
- Ed download dialogs - single dialog waits for user ok

## [3.10.7] - 2026-01-01

### Changed
- Streamlined download dialogs - status bar feedback

## [3.10.6] - 2026-01-01

### Changed
- Cty.dat update notification at startup

## [3.10.5] - 2025-12-31

### Changed
- Dx cluster modern formatting and layout improvements
- Dx cluster and band map improvements

## [3.10.4] - 2025-12-31

### Fixed
- Test failures in v3.10.3

## [3.10.3] - 2025-12-31

### Changed
- Station info display with us call area coordinates

## [3.10.2] - 2025-12-31

### Fixed
- Minimum window width with dynamic calculation

## [3.10.1] - 2025-12-31

### Changed
- Esc key handling and scp font size improvements

### Fixed
- Band buttons and worker thread shutdown issues

## [3.10.0] - 2025-12-31

### Changed
- Florida qso party implementation with qso party base class

## [3.9.0] - 2025-12-31

### Changed
- Super check partial (scp) real-time callsign matching
- Contest-specific band filtering and title bar improvements

## [3.8.4] - 2025-12-30

### Changed
- Critical bug fixes and improvements

## [3.8.3] - 2025-12-30

### Changed
- Dx cluster and band map click behavior improvements

## [3.8.1] - 2025-12-30

### Changed
- Streamline adif import workflow with checkbox rescore

### Fixed
- Macos app bundling for macs without homebrew

## [3.8.0] - 2025-12-29

### Added
- Grid_square and iota_reference fields

## [3.7.3] - 2025-12-29

### Fixed
- Contest type registry lookup

## [3.7.2] - 2025-12-29

### Fixed
- Windows ci mingw path detection

## [3.7.0] - 2025-12-29

### Added
- Hamlib debug logging checkbox in preferences

### Changed
- Bundle cty.dat and add cross-platform zip extraction

### Fixed
- Windows qt 6.10+ entry point linking
- Windows mingw dll deployment

## [3.6.3] - 2025-12-29

### Changed
- Multiplier window improvements and window menu checkmarks
- Disable linux arm64 builds except manual workflow dispatch

## [3.6.2] - 2025-12-29

### Changed
- Window persistence and map viewer improvements

## [3.5.1] - 2025-12-29

### Added
- In-app map viewer with auto-refresh

### Fixed
- Windows ci - add qt webengine modules

## [3.4.0] - 2025-12-29

### Added
- Us states map for was (worked all states) tracking

## [3.3.5] - 2025-12-29

### Fixed
- K4 discovery with per-interface sockets and subnet broadcast

## [3.3.4] - 2025-12-29

### Fixed
- K4 radio discovery - use same socket for send and receive

## [3.3.3] - 2025-12-29

### Fixed
- Critical tls plugin deployment - https downloads now work

## [3.3.2] - 2025-12-28

### Fixed
- Contest registry lookup - remove mode suffix from contest id

## [3.3.1] - 2025-12-28

### Fixed
- Windows qsqlite driver deployment

## [3.3.0] - 2025-12-28

### Added
- Fatal error handling for database driver failures

## [3.2.0] - 2025-12-28

### Added
- K4 radio network discovery

## [3.1.0] - 2025-12-28

### Changed
- Cabrillo: sort operators by qso count descending

## [3.0.2] - 2025-12-28

### Fixed
- Schema.sql fallback paths for test environment

## [3.0.1] - 2025-12-28

### Fixed
- Winter field day scoring to use geographic distance

## [3.0.0] - 2025-12-28

### Changed
- Remove windeployqt, use explicit deterministic deployment
- Complete explicit deployment across all platforms
- Make windows style plugin optional

## [2.99.5] - 2025-12-28

### Changed
- Manually copy qsqlite.dll plugin for windows

## [2.99.4] - 2025-12-28

### Added
- Sql plugin to windows deployment

## [2.99.3] - 2025-12-28

### Changed
- Make mingw dll copy non-fatal, add diagnostics

## [2.99.2] - 2025-12-28

### Fixed
- Qt6 compatibility for older versions

## [2.99.1] - 2025-12-28

### Fixed
- Windows deployment - include mingw runtime dlls

## [2.99.0] - 2025-12-28

### Added
- Sidebar navigation for preferences dialog

## [2.98.5] - 2025-12-28

### Fixed
- Ci build: skip auto-bundling in github actions

## [2.98.4] - 2025-12-28

### Fixed
- Arrl field day zones, add web dashboard mode groups, automate macos bundling

## [2.98.1] - 2025-12-28

### Fixed
- Sections map zoom reset on auto-refresh

## [2.98.0] - 2025-12-28

### Added
- Chloropleth map for arrl sections with geojson polygons

## [2.97.1] - 2025-12-28

### Added
- Dynamic mode group display in bandsummarygrid

## [2.97.0] - 2025-12-28

### Added
- Mode group infrastructure for mixed-mode contests

## [2.96.6] - 2025-12-28

### Added
- Contest class field and field day scoring improvements

## [2.95.11] - 2025-12-28

### Changed
- Comprehensive widget background rendering fix for macos

## [2.95.10] - 2025-12-28

### Fixed
- Blank widget displays on some macos systems

## [2.95.9] - 2025-12-27

### Fixed
- Qtdbus framework dependency on libdbus to use @rpath

## [2.95.8] - 2025-12-27

### Fixed
- All manually copied libraries to use @rpath

## [2.95.7] - 2025-12-27

### Fixed
- Missing libdbus dependency in macos bundle

## [2.95.6] - 2025-12-27

### Fixed
- Missing qtdbus framework in macos bundle

## [2.95.5] - 2025-12-27

### Fixed
- Missing libbrotlicommon dependency in macos bundle

## [2.95.4] - 2025-12-27

### Fixed
- Macos dmg creation - use hdiutil instead of macdeployqt -dmg

## [2.95.3] - 2025-12-27

### Fixed
- Macos code signing - sign all components individually

## [2.95.2] - 2025-12-27

### Added
- Nsis windows installer to ci build

## [2.95.1] - 2025-12-27

### Changed
- Build hamlib from source to avoid code signature issues
- Temporarily exclude linux from releases (package issue tbd)

### Fixed
- Macos code signature crash
- Macos build - remove hamlib signature before macdeployqt

## [2.95.0] - 2025-12-27

### Changed
- Refactor webserver to pull model architecture

## [2.94.0] - 2025-12-27

### Changed
- Web dashboard enhancements and radio control improvements

## [2.93.0] - 2025-12-27

### Changed
- Critical: add emergency adif fallback for database failures

## [2.92.0] - 2025-12-27

### Added
- Dialoghelper for centralized dialog logging and fix scalability issues

## [2.91.4] - 2025-12-27

### Added
- Dialog logging for debugging

## [2.91.3] - 2025-12-27

### Added
- Duplicate detection to rescore contest

## [2.91.2] - 2025-12-27

### Fixed
- Contest reopening with scalable parsing

## [2.91.1] - 2025-12-27

### Fixed
- Duplicate qso handling

## [2.90.1] - 2025-12-26

### Fixed
- Macos deployment target for sonoma compatibility

## [2.90.0] - 2025-12-26

### Added
- Cw sender abstraction with factory pattern

## [2.89.1] - 2025-12-26

### Changed
- Merge remote changes

## [2.89.0] - 2025-12-26

### Added
- Editable cw macro buttons with persistence

## [2.88.1] - 2025-12-26

### Fixed
- Ci workflow build failures

## [2.88.0] - 2025-12-26

### Added
- Github actions ci for multi-platform builds
- 5 new amateur radio contests

## [2.87.9] - 2025-12-26

### Fixed
- Windows build and window flashing issues

## [2.87.7] - 2025-12-26

### Changed
- Remove scrollbar debug logging

## [2.87.6] - 2025-12-26

### Added
- 4th decimal place to bandmap frequency display

## [2.87.5] - 2025-12-26

### Fixed
- Bandmap horizontal scrollbar flickering

## [2.87.4] - 2025-12-26

### Fixed
- Bandmap horizontal scrollbar with two-pass layout calculation

## [2.87.3] - 2025-12-26

### Fixed
- Bandmap status text flickering on startup

## [2.87.2] - 2025-12-26

### Fixed
- Bandmap horizontal scrollbar and suppress qcustomplot warnings

## [2.87.1] - 2025-12-26

### Added
- Resizable columns, global pgup/pgdn, auto-reconnect, and flashing radio indicator

## [2.85.2] - 2025-12-26

### Fixed
- Points all column to show qso points sum, not final score

## [2.85.1] - 2025-12-26

### Added
- Rescore contest menu item

## [2.85.0] - 2025-12-26

### Changed
- Mark multipliers when logging, simplify score display

## [2.84.1] - 2025-12-26

### Fixed
- Multiplier tracking for allbands vs perband scope

## [2.84.0] - 2025-12-26

### Changed
- Calculate final score using contest formula (qso points × multipliers)

## [2.83.7] - 2025-12-26

### Changed
- Remove unused auto send cw appsettings methods

## [2.83.6] - 2025-12-26

### Fixed
- Wpm display to follow auto send cw action state

## [2.83.5] - 2025-12-26

### Changed
- Auto send cw always defaults to enabled on startup

## [2.83.4] - 2025-12-26

### Changed
- Calculate next serial number from qsos on startup

## [2.83.3] - 2025-12-26

### Fixed
- Received serial number display and datachanged warnings

## [2.83.2] - 2025-12-26

### Fixed
- Serial number display using database field (proper fix)

## [2.83.1] - 2025-12-26

### Changed
- Move send morse to window menu and fix serial number display

## [2.83.0] - 2025-12-26

### Added
- Auto send cw toggle and rename dx mults to mults

## [2.82.2] - 2025-12-26

### Changed
- Stack date and time vertically in radio status widget

## [2.82.1] - 2025-12-26

### Changed
- Adjust log levels for better log clarity

## [2.82.0] - 2025-12-26

### Added
- Configurable wpm increment and auto-send cw on enter

## [2.81.1] - 2025-12-26

### Fixed
- Scientific notation in bandmapwidget frequency log messages

## [2.81.0] - 2025-12-26

### Added
- Comprehensive morse code sending features

## [2.80.1] - 2025-12-25

### Fixed
- Numeric frequency entry - move to correct handler

## [2.80.0] - 2025-12-25

### Added
- Numeric frequency entry shortcut

## [2.79.2] - 2025-12-25

### Fixed
- Adif export - remove operator from header

## [2.79.1] - 2025-12-25

### Fixed
- Dxccrepository performance issue

## [2.79.0] - 2025-12-25

### Changed
- Improve exchange memory and log clearing

## [2.78.1] - 2025-12-25

### Fixed
- Dxcc entities table creation for existing databases

## [2.78.0] - 2025-12-25

### Changed
- Complete dxcc entity mapping in global database

## [2.77.0] - 2025-12-25

### Fixed
- Adif exports to use official contest ids and dxcc entity codes

## [2.75.0] - 2025-12-25

### Added
- Guid support, udp rebroadcast, data integrity checking, and ui improvements

## [2.70.1] - 2025-12-25

### Fixed
- Operator field not loading from database
- Operator field persistence and add comprehensive qso persistence test

## [2.70.0] - 2025-12-25

### Added
- Qso points calculation, fix edit qso, and add selectable message boxes

## [2.68.0] - 2025-12-25

### Fixed
- Exchange validation, contest scoring, and section mapping

## [2.67.0] - 2025-12-25

### Added
- Comprehensive test coverage for exchange parsers

## [2.66.7] - 2025-12-25

### Changed
- Update wfd transmitter limit to 1-99 per official 2025 rules

## [2.66.6] - 2025-12-25

### Changed
- Improve wfd class validation error message

## [2.66.5] - 2025-12-25

### Fixed
- Wfd class validation to accept all 4 categories

## [2.66.4] - 2025-12-25

### Fixed
- Wfd validator to be order-agnostic and log all status messages

## [2.66.3] - 2025-12-25

### Changed
- Centralize arrl section validation in arrlsectionhelper

## [2.66.2] - 2025-12-25

### Fixed
- Smart exchange parser to use contest-specific section validation

## [2.66.1] - 2025-12-25

### Fixed
- Home class validation in wfd exchange parser

## [2.66.0] - 2025-12-25

### Added
- Smart exchange parser with automatic field reordering

## [2.65.1] - 2025-12-25

### Added
- Keyboard shortcut and clarify radio reconnect

## [2.65.0] - 2025-12-25

### Added
- Table column definitions for all contests

## [2.64.1] - 2025-12-25

### Fixed
- Arrl sweepstakes contest not loading from database

## [2.64.0] - 2025-12-25

### Added
- Dynamic qso table columns for contests with many exchange fields

## [2.63.0] - 2025-12-25

### Added
- Arrl sweepstakes contest (cw and ssb)

## [2.62.1] - 2025-12-25

### Added
- Arrl section field to qso records

## [2.62.0] - 2025-12-25

### Added
- County field to qso records for arrl section mapping

## [2.61.0] - 2025-12-25

### Added
- Phase 1 exchange validation with real-time visual feedback

## [2.60.1] - 2025-12-25

### Added
- Comprehensive test coverage for all arrl section counties

## [2.60.0] - 2025-12-25

### Added
- Comprehensive arrl section helper for state/county mapping

## [2.59.3] - 2025-12-25

### Fixed
- Log auto-scroll and selection on startup

## [2.59.2] - 2025-12-25

### Changed
- Update building.md with comprehensive multi-platform instructions

## [2.59.1] - 2025-12-25

### Added
- Comprehensive us zone tests and fix hawaii/alaska dxcc handling

## [2.59.0] - 2025-12-25

### Added
- Built-in us call area zone logic with proper cty.dat priority

## [2.58.0] - 2025-12-25

### Added
- Zone lookup and display from cty.dat

### Changed
- Update version to 2.58.0

## [2.57.4] - 2025-12-25

### Changed
- Use band edge frequencies for manual selection

## [2.57.3] - 2025-12-25

### Fixed
- Frequency display without radio connection

## [2.57.2] - 2025-12-25

### Fixed
- Frequency not set on contest activation without radio

## [2.57.1] - 2025-12-25

### Added
- Band up/down support and move dupe warning to status bar

## [2.57.0] - 2025-12-25

### Added
- Manual band/mode selection without radio

## [2.56.2] - 2025-12-25

### Fixed
- Duplicate checking to use string band/mode values

## [2.56.1] - 2025-12-25

### Fixed
- Callsign field focus on startup

## [2.56.0] - 2025-12-25

### Fixed
- Cqww and cqwpx scoring rules

## [2.55.0] - 2025-12-25

### Fixed
- Winter field day scoring and multipliers

## [2.54.0] - 2025-12-25

### Added
- Duplicate qso checking with visual warning

## [2.53.1] - 2025-12-25

### Fixed
- Contest type parsing in auto-reopen

## [2.53.0] - 2025-12-25

### Changed
- Auto-reopen last contest on startup

## [2.52.0] - 2025-12-25

### Changed
- Auto-fill rst in exchange parsing

## [2.51.0] - 2025-12-25

### Added
- Exchange auto-population and memory system

## [2.50.0] - 2025-12-25

### Added
- Phase 1: contest exchange validation

## [2.49.2] - 2025-12-25

### Fixed
- Operator field persistence and add log grid auto-scroll

## [2.49.1] - 2025-12-25

### Fixed
- Scientific notation in frequency display and add coding standards

## [2.49.0] - 2025-12-25

### Added
- Edit qso dialog, qcustomplot integration, and ui polish

## [2.48.0] - 2025-12-25

### Changed
- Replace qprogressbar s-meter with custom analog-style widget

## [2.47.3] - 2025-12-25

### Fixed
- Date/time display jumping and operator field initialization

## [2.47.2] - 2025-12-25

### Fixed
- Call field focus by only raising windows when child windows are activated

## [2.47.1] - 2025-12-25

### Fixed
- Call field focus issue, add s-meter debug logging, and enable window size persistence

## [2.47.0] - 2025-12-25

### Added
- S-meter (signal strength indicator) to radio control window

## [2.46.1] - 2025-12-25

### Changed
- Change radio state update log message to trace level

## [2.46.0] - 2025-12-25

### Added
- Rit/xit enable status polling and restructure widgets to two-row format

## [2.45.1] - 2025-12-25

### Fixed
- Logging issues and radio control window update behavior

## [2.45.0] - 2025-12-25

### Added
- Rit/xit offset display to radio control widget

## [2.44.0] - 2025-12-25

### Added
- Persistent spot storage with aging colors and band switching

## [2.42.0] - 2025-12-24

### Added
- Lotw user tracking and filtering for dx spots

## [2.41.0] - 2025-12-24

### Changed
- Convert all qdebug/qwarning calls to log_* macros

## [2.40.2] - 2025-12-24

### Fixed
- Qt 6.9 deprecation warnings

## [2.40.1] - 2025-12-24

### Changed
- Prioritize elecraft k4 at top of radio list

## [2.40.0] - 2025-12-24

### Added
- Radio status filtering (stable/beta/alpha/untested)

## [2.39.0] - 2025-12-24

### Changed
- Load all hamlib backends for complete radio enumeration

## [2.37.0] - 2025-12-24

### Added
- Phase 2 business logic unit tests

## [2.36.0] - 2025-12-24

### Added
- Comprehensive unit testing infrastructure
- Editable dx cluster server selector with validation and cc cluster support

## [2.35.0] - 2025-12-24

### Added
- Tr4w-style logging system with preferences ui

## [2.34.0] - 2025-12-24

### Fixed
- Bandmap scrollbar visibility and frequency formatting

## [2.33.0] - 2025-12-24

### Changed
- Parse cty version string from rss description field

## [2.32.0] - 2025-12-24

### Fixed
- Sql warnings and move program startup to first log entry

## [2.31.0] - 2025-12-24

### Fixed
- Qsqldatabase connection warnings in backupmanager

## [2.30.0] - 2025-12-24

### Changed
- Cty download via rss feed auto-detection

### Fixed
- Schema loading and backuprestoredialog segfault

## [2.29.0] - 2025-12-24

### Added
- Udp broadcast debug logging
- Graceful error handling for missing database schema

## [2.28.0] - 2025-12-24

### Added
- Bandmap sort options (frequency/callsign)

### Fixed
- Dx cluster download and schema.sql loading

## [2.27.0] - 2025-12-24

### Added
- Backup settings integration to preferencesdialog

### Changed
- Bandmap: revert to column-first layout with horizontal scrolling

## [2.26.0] - 2025-12-24

### Added
- Dx cluster server list download feature

### Fixed
- Bandmap scrollbar with row-first layout

## [2.25.0] - 2025-12-24

### Added
- Download cty.dat and fix bandmap display

### Changed
- Complete color customization - remaining widgets

## [2.24.1] - 2025-12-24

### Fixed
- Keyboard shortcuts to match tr4w umenu.pas exactly

## [2.24.0] - 2025-12-24

### Added
- Alt- and ctrl- keyboard shortcut menus like tr4w
- Color customization ui and integrate theme with widgets

## [2.23.0] - 2025-12-24

### Added
- Scrollbar support to bandmap for overflow content
- Thememanager core infrastructure for color customization

## [2.22.0] - 2025-12-24

### Added
- Searchable radio model selector for easier navigation

## [2.21.0] - 2025-12-24

### Added
- Enhanced radio enumeration logging for diagnostics

## [2.20.2] - 2025-12-24

### Fixed
- : disable band buttons when radio disconnected

## [2.20.1] - 2025-12-24

### Fixed
- : clear radio control display when disconnected

## [2.20.0] - 2025-12-24

### Added
- Bandmap dynamic multi-column layout

## [2.19.0] - 2025-12-24

### Added
- Dx cluster click-to-qsy feature

## [2.18.0] - 2025-12-24

### Added
- Contest factory pattern with auto-registration

## [2.17.0] - 2025-12-24

### Added
- Contest identifiers to existing contests

## [2.16.0] - 2025-12-24

### Added
- Udp multicast/broadcast messaging

## [2.14.0] - 2025-12-24

### Added
- Dx cluster preferences and band map enhancements

## [2.10.0] - 2025-12-23

### Added
- Custom tr4qt application icon

## [2.9.0] - 2025-12-23

### Added
- Dx cluster window with threaded telnet client

## [2.8.1] - 2025-12-23

### Fixed
- Cabrillo export to allow export without active contest

## [2.8.0] - 2025-12-23

### Added
- Multiplier widget

## [2.7.0] - 2025-12-23

### Added
- Radio control widget

## [2.6.0] - 2025-12-23

### Added
- Bandmap widget

## [2.5.0] - 2025-12-23

### Added
- Adif/cabrillo export and clear log

## [2.4.0] - 2025-12-23

### Added
- Contest-dependent table columns and fix time padding

## [2.3.0] - 2025-12-23

### Added
- Configurable font sizes for ui elements

## [2.2.0] - 2025-12-23

### Added
- Proper window resizing with layout scaling

## [2.1.0] - 2025-12-23

### Added
- Dynamic radio enumeration from hamlib

## [2.0.0] - 2025-12-23

### Added
- Winter field day contest

## [1.9.0] - 2025-12-23

### Added
- Cq wpx contest implementation

## [1.8.0] - 2025-12-23

### Added
- Contest chooser dialog

## [1.7.0] - 2025-12-23

### Added
- Preferences dialog with tabbed interface

## [1.6.0] - 2025-12-23

### Added
- Radio status grid at bottom of form

## [1.5.0] - 2025-12-23

### Added
- Initial release: TR4QT Phase 2 Complete
- Contest logging system with CQ WW contest support
- Radio control via Hamlib integration
- SQLite database with QSO persistence
- Band summary grid and QSO table display
- Station configuration and preferences dialog
- Country file support (CTY.DAT integration)
- Comprehensive radio enumeration and control
- Exchange validation and scoring system
- ADIF and Cabrillo export capabilities

### Notes
- This was the initial commit with 6,043 lines of code
- Represents "Phase 2 Complete" - earlier development (Phase 1) was not tracked in git
- First version of TR4QT to be committed to version control

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

---

## Project History

- **Initial Release**: v1.5.0 (2025-12-23)
- **Total Versions**: 214
- **Development Started**: December 23, 2025
- **Latest Version**: v3.15.0 (2026-01-01)
- **Active Development**: 214 releases in 10 days
