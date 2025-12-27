# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## ⚠️ CRITICAL REMINDERS

### Always Bump Version Before Building
**NEVER build without updating the version constant first!**

Before making any code changes:
1. Edit `/src/core/Constants.h`
2. Increment `APP_VERSION` appropriately
3. Update the comment with what's changing
4. Then make your code changes
5. Build with `make tr4qt -j4`
6. Commit with version in message

This prevents confusion about which version is running.

## Version Management

**CRITICAL**: The version number must be updated with every release commit.

Location: `/src/core/Constants.h`
```cpp
constexpr const char* APP_VERSION = "X.Y.Z";
```

Convention:
- **Major (X)**: Major features or breaking changes
- **Minor (Y)**: New features, enhancements
- **Patch (Z)**: Bug fixes, small improvements

Update process:
1. Increment version in Constants.h
2. Update comment to describe the change
3. Include version in commit message: "Feature description - vX.Y.Z"
4. Rebuild: `cmake --build build`

Example:
```cpp
constexpr const char* APP_VERSION = "2.58.0";  // Zone lookup from cty.dat, manual band selection fixes
```

## Build Process

Standard build:
```bash
cmake --build build
```

Clean rebuild:
```bash
rm -rf build && cmake -B build && cmake --build build
```

Run after build:
```bash
# IMPORTANT: Always kill any running instances before starting a new one
pkill -9 tr4qt
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

**CRITICAL**: Always `pkill -9 tr4qt` before running the application to avoid multiple instances running simultaneously.

## Testing

Run tests:
```bash
cd build && ctest --output-on-failure
```

## Git Workflow

Commit message format:
```
Brief description of change - vX.Y.Z

Detailed explanation of what changed and why.

Changes:
- Bullet point list of specific changes
- Include file locations if helpful

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

## Pre-Release Checklist

**CRITICAL**: Before creating any release tag, ALWAYS run through this checklist!

Release tags trigger GitHub Actions builds for all platforms. A failed build wastes time and creates confusion.

### 1. Verify Qt Module Dependencies Match
Compare CMakeLists.txt with Windows CI configuration:

```bash
# Check what Qt components TR4QT requires
grep -A 10 "find_package(Qt6 REQUIRED COMPONENTS" CMakeLists.txt

# Check what Windows CI installs
grep -A 5 "modules:" .github/workflows/build.yml
```

**Rule**: Every Qt component in CMakeLists.txt (except base modules) must be listed in the Windows CI `modules:` line.

**Base modules** (always installed, don't need to be listed):
- Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**Additional modules** (must be explicitly listed):
- HttpServer (currently required by TR4QT)
- WebSockets, WebView, Multimedia, Charts, 3D, etc. (add if needed)

**macOS note**: `brew install qt@6` includes all modules, so macOS builds won't catch this issue.

### 2. Verify Last CI Build Passed
```bash
gh run list --limit 3
```

Both macOS and Windows builds must show ✓ success. If either failed, fix the issue before tagging.

### 3. Version Already Bumped
Confirm Constants.h has been updated in the latest commit:
```bash
grep APP_VERSION src/core/Constants.h
git log -1 --oneline
```

Version in Constants.h should match the commit message.

### 4. Build and Test Locally
```bash
cmake --build build
cd build && ctest --output-on-failure
```

All tests should pass. If any fail, investigate before releasing.

### 5. Check for Uncommitted Changes
```bash
git status
```

Should show "nothing to commit, working tree clean".

### 6. Create Release Tag (Only After Above Checks Pass)
```bash
# Lightweight tag (testing)
git tag v2.95.0
git push origin v2.95.0

# Annotated tag (official releases)
git tag -a v2.95.0 -m "Release v2.95.0 - WebServer pull model refactoring"
git push origin v2.95.0
```

### 7. Monitor GitHub Actions
```bash
gh run watch
```

Watch the triggered build. Both platforms must succeed for a valid release.

### Common Issues Caught By This Checklist:
- ✅ Missing Qt modules in Windows CI (e.g., HttpServer)
- ✅ Version mismatch between code and tag
- ✅ Tests failing on specific platforms
- ✅ Build errors from uncommitted changes
- ✅ CI configuration drift from CMakeLists.txt

**Remember**: Release tags are permanent in the git history. It's better to spend 2 minutes verifying than to create a broken release tag!

## Common Patterns

### Radio State
Radio frequency/band/mode is stored in `m_currentState` (RadioState struct):
- `frequencyA`: freq_t (Hz)
- `bandA`: BandType enum
- `modeA`: ModeType enum

### Country Lookup
```cpp
CountryData countryData = m_countryFile.lookup(callsign);
if (countryData.isValid()) {
    int zone = countryData.cqZone;
    QString country = countryData.name;
    QString continent = continentToString(countryData.continent);
}
```

### Exchange Prediction
Uses InitialExchangeManager singleton:
```cpp
QString prediction = InitialExchangeManager::instance().predictExchange(
    callsign,
    m_activeContest,
    m_currentState.modeA
);
```

Strategy priority:
1. Exchange memory (exact match)
2. CTY.DAT lookup (zone/country)
3. Contest defaults (RST)

### Database Operations
Always check for active contest:
```cpp
if (m_hasActiveContest) {
    QSORepository repo;
    if (!repo.saveQSO(qso, m_currentContestDbId)) {
        LOG_WARN("Tag", QString("Failed: %1").arg(repo.lastError()));
    }
}
```

### Dialog Messages
**CRITICAL**: ALL dialog messages MUST go through DialogHelper. Never use QMessageBox directly.

This ensures:
- All dialogs are automatically logged (message + user response)
- Text is selectable/copyable by default for error messages
- Consistent user experience across the application
- Easier debugging via log analysis

Use DialogHelper for all user-facing dialogs:
```cpp
// Question dialog
QMessageBox::StandardButton reply = DialogHelper::question(
    this,
    "Confirm Action",
    "Are you sure you want to proceed?"
);

// Information
DialogHelper::information(this, "Success", "Operation completed successfully.");

// Warning
DialogHelper::warning(this, "Warning", "This action cannot be undone.");

// Critical error
DialogHelper::critical(this, "Error", QString("Failed: %1").arg(errorMessage));
```

**NEVER** use QMessageBox::question/information/warning/critical directly. Always use DialogHelper.

Files:
- `/src/utils/DialogHelper.h` - Dialog wrapper with logging
- `/src/utils/DialogHelper.cpp` - Implementation

**This pattern applies to all projects**: If one dialog is logged, ALL dialogs must be logged via a common helper class.

## Known Issues / Limitations

### Database Threading (Deferred until Networking Implementation)
**Issue**: Database singleton is not thread-safe for concurrent writes (discovered 2025-12-27)

**Status**: Deferred until TCP networking implementation
- Single-operator use: SAFE (99% of users)
- Multi-threaded concurrent writes: CRASHES
- Test coverage: `tests/test_qso_load_performance.cpp` (concurrent tests disabled)

**Performance Verified**:
- Single-threaded: 6,800+ QSOs/second
- Transaction time: <1ms average
- Database throughput: 24M+ QSOs/hour theoretical max

**Solution Required Before**:
- TCP-based networked TR4QT (multi-station logging)
- True multi-operator concurrent logging

**Implementation Options** (documented in `src/data/Database.cpp`):
1. Connection pool (thread_local connections)
2. Serialized access (QMutex)
3. Message queue architecture (recommended for TCP server)

**Reference**: See comprehensive TODO in `/src/data/Database.cpp` line 13

## Architecture Notes

### Contest System
- Base class: `ContestBase` (abstract)
- Contest implementations: `CQWWContest`, `CQWPXContest`, `WinterFieldDayContest`
- Auto-registration: `REGISTER_CONTEST(ClassName, "ID")`
- Factory access: `ContestRegistry::instance().createContest("ID")`

### Exchange Fields
Contests define received/sent exchange fields:
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override;
QList<ExchangeField> getSentExchangeFields() const override;
```

QSOTableModel adapts columns based on contest exchange fields.

### Duplicate Checking
Each contest defines duplicate rule:
```cpp
DuplicateCheckingRule getDuplicateCheckingRule() const override {
    return DuplicateCheckingRule::PerBandMode;
}
```

Options:
- `PerBandMode`: Same call/band/mode is dupe
- `AllBandMode`: Same call/mode across all bands
- `PerBand`: Same call/band (any mode)
- `AllBand`: Same call (once per contest)

## Key Files

### Core
- `/src/core/Constants.h` - **VERSION HERE**
- `/src/core/Types.h` - Enums and type definitions
- `/src/models/QSO.h` - QSO record structure

### UI
- `/src/ui/MainWindow.cpp` - Main application logic
- `/src/ui/models/QSOTableModel.cpp` - Log display

### Contests
- `/src/contests/ContestBase.h` - Contest interface
- `/src/contests/CQWWContest.cpp` - CQ WW implementation
- `/src/contests/CQWPXContest.cpp` - CQ WPX implementation

### Exchange System
- `/src/exchanges/InitialExchangeManager.cpp` - Exchange prediction
- `/src/data/ExchangeMemoryRepository.cpp` - Exchange memory

### Country Data
- `/src/utils/CountryFile.cpp` - CTY.DAT parser
- `~/.tr4qt/cty.dat` - Country file data

## Recent Changes (Session Summary)

### v2.57.x - Manual Band Selection
- Fixed focus issues (callsign field gets focus on startup)
- Fixed duplicate checking (band/mode stored as TEXT not enums)
- Implemented manual band selection when radio disconnected
- Added ALT-B/ALT-V (Option-B/Option-V on Mac) for band up/down
- Set frequency to band edge for manual selection (visual indicator)
- Fixed frequency display without radio connection

### v2.58.0 - Zone Lookup
- Implemented country/zone lookup from cty.dat when logging QSO
- Zones now auto-fill in exchange field (via InitialExchangeManager)
- Zones display in "Zn" column of QSO table
- Geographic fields populated: CQ Zone, ITU Zone, Country, Continent

## Known Issues / TODOs

From code comments:
- [ ] Calculate QSO points via contest (line 1385)
- [ ] Check for new multipliers (line 1386)
- [ ] Improve exchange prediction architecture (per plan file)

## Mac-Specific Notes

Keyboard shortcuts:
- Qt::ALT maps to Option (⌥) on Mac
- Qt::CTRL maps to Command (⌘) on Mac
- Displayed automatically in menus with correct symbols

Path handling:
- Use QDir::homePath() for cross-platform home directory
- Config: `~/.tr4qt/`
- Logs: `~/.tr4qt/logs/`
- Backups: `~/.tr4qt/backups/`
