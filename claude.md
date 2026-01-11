# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## 🏗️ BUILD PHILOSOPHY - APPLIES TO ALL PROJECTS

### Explicit > Implicit (when implicit is unreliable)

**NEVER use "automatic" deployment tools that don't work reliably.**

Common unreliable tools:
- ❌ `windeployqt` (Windows) - Silently fails to copy plugins, inconsistent
- ❌ `macdeployqt` (macOS) - Misses dependencies, gets paths wrong
- ❌ `linuxdeploy` (Linux) - Complex, hard to debug, inconsistent
- ❌ **ALL OF THEM** - None bundle Qt TLS plugins by default (critical for HTTPS!)

**Why these tools are problematic:**
1. **Silent failures** - Missing dependencies without clear errors
2. **False confidence** - "I used the official tool, so it should work"
3. **Wastes time** - Hours debugging what the tool "forgot"
4. **Not deterministic** - Same inputs can produce different outputs
5. **Hard to debug** - Don't know what they did vs. didn't do

**Instead: Be Explicit**

✅ **List every dependency explicitly**
✅ **Copy every file with clear commands**
✅ **Verify what was deployed**
✅ **Clear errors when something fails**

**Example (Windows):**
```bash
# BAD: Hope windeployqt works
windeployqt tr4qt.exe --sql  # Maybe copies SQL plugin? Maybe not?

# GOOD: Explicit
cp Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll ...  # Know exactly what's copied
mkdir sqldrivers && cp qsqlite.dll sqldrivers/  # SQL plugin always included
ls -la sqldrivers/  # Verify it's there
```

**Example (macOS):**
```bash
# BAD: Trust macdeployqt to bundle everything
macdeployqt tr4qt.app
# Result: App launches but HTTPS fails with "No functional TLS backend was found"
# macdeployqt silently forgot TLS plugins!

# GOOD: Explicit TLS plugin bundling
macdeployqt tr4qt.app
mkdir -p tr4qt.app/Contents/PlugIns/tls
cp /opt/homebrew/opt/qtbase/share/qt/plugins/tls/*.dylib tr4qt.app/Contents/PlugIns/tls/
ls -la tr4qt.app/Contents/PlugIns/tls/  # Verify all 3 TLS backends present
# Result: App works perfectly, HTTPS downloads succeed
```

**Benefits of Explicit Deployment:**
- Deterministic (same result every time)
- Transparent (see exactly what's deployed)
- Debuggable (clear errors when missing)
- Maintainable (easy to add/remove dependencies)
- Reliable (no silent failures)

**This philosophy applies to ALL projects, not just TR4QT.**

If you encounter an "automatic" tool that doesn't work reliably:
1. Don't keep using it hoping it will work this time
2. Replace it with explicit, deterministic steps
3. Document exactly what needs to be deployed

## ⚠️ CRITICAL REMINDERS

### 🚨 ARCHITECTURE: MainWindow is 5,564 lines - STOP Adding Features

**MainWindow has exceeded all limits:**
- ✗ 500 lines (Yellow flag) - **EXCEEDED BY 11X**
- ✗ 1,000 lines (Red flag) - **EXCEEDED BY 5.5X**
- ✗ 1,500 lines (STOP) - **EXCEEDED BY 3.7X**

**BEFORE implementing any new feature, CHECK:**

1. **Will this add SQL to MainWindow?** → Create Repository/Service instead
2. **Will this add business logic to MainWindow?** → Create Service instead
3. **Will the event handler exceed 20 lines?** → Extract to Service method
4. **Does MainWindow already exceed 1,500 lines?** → STOP, extract services first

**See ARCHITECTURE_RULES.md for complete guidance on:**
- ❌ What's forbidden in MainWindow
- ✅ What's allowed in MainWindow
- 📐 TR4QT architecture layers
- 🎯 Where new features should go
- 🔍 Pre-feature checklist

**Example of WRONG approach (what we just did):**
```cpp
// ❌ MainWindow::onEditContestSettings() - 73 lines
void MainWindow::onEditContestSettings() {
    // SQL queries - belongs in Repository
    db.execute("UPDATE contests SET ...");

    // Business logic loops - belongs in Service
    for (int row = 0; row < count; ++row) {
        qso.exchangeSent = formatExchange(...);
        db.execute("UPDATE qsos SET ...");
    }
}
```

**Example of CORRECT approach:**
```cpp
// ✅ MainWindow delegates to Service
void MainWindow::onEditContestSettings() {
    auto result = m_contestService->showEditExchangeDialog(m_currentContestDbId);
    if (!result.success) showError(result.error);
}

// Service handles business logic
class ContestService {
    Result<void> showEditExchangeDialog(int contestId);
    // Encapsulates: validation, database updates, model refresh
};
```

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

### Adding New Qt Modules - Update CI Configuration!
**CRITICAL**: When adding new Qt modules to CMakeLists.txt, you MUST update the Windows CI configuration!

Why this matters:
- macOS: `brew install qt@6` includes ALL modules (won't catch the issue)
- Windows: `install-qt-action` only installs base modules + explicitly listed modules
- If you forget to update CI, Windows builds will fail with "Qt6ModuleName not found"

**Workflow when adding a new Qt module:**

1. Add to `/CMakeLists.txt`:
   ```cmake
   find_package(Qt6 REQUIRED COMPONENTS
       Core Widgets Network
       NewModule  # ← Added here
   )
   ```

2. Add to `/src/CMakeLists.txt`:
   ```cmake
   target_link_libraries(tr4qt PRIVATE
       Qt6::Core Qt6::Widgets
       Qt6::NewModule  # ← Added here
   )
   ```

3. **CRITICAL**: Add to `/.github/workflows/build.yml` Windows CI:
   ```yaml
   - name: Install Qt
     uses: jurplel/install-qt-action@v4
     with:
       modules: 'qtwebsockets qthttpserver qtnewmodule'  # ← Added here
   ```

**Base modules** (always installed, don't need to be listed in CI):
- Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**Additional modules** (MUST be explicitly listed in CI):
- WebEngine, WebEngineWidgets, WebChannel, Positioning, WebSockets, HttpServer
- Multimedia, Charts, 3D, etc.

**Verification after adding:**
```bash
# Check CMakeLists.txt matches Windows CI
grep -A 10 "find_package(Qt6 REQUIRED COMPONENTS" CMakeLists.txt
grep "modules:" .github/workflows/build.yml
```

**Real example that broke CI:**
- Added Qt WebEngineWidgets for map viewer in CMakeLists.txt
- Forgot to add `qtwebengine qtwebchannel qtpositioning` to Windows CI
- macOS build succeeded (all modules available), Windows failed
- Fixed by adding modules to `install-qt-action` configuration

This prevents confusion about which version is running.

### Check for Existing Data Before Loading/Creating

**CRITICAL**: Before loading data from files or creating new instances, ALWAYS check if the data already exists in the application.

**The Problem**:
- Loading duplicate copies of data wastes memory
- Creates synchronization issues (which copy is authoritative?)
- Can cause race conditions if one copy is updated
- Violates Single Source of Truth principle

**Examples of what to check**:
- ✅ Does MainWindow already have `m_countryFile` loaded?
- ✅ Is there a GlobalDatabase with DXCC entities?
- ✅ Does a singleton already exist for this data?
- ✅ Can I get a pointer/reference to existing data?

**Bad Pattern** (what NOT to do):
```cpp
// ADIFImportDialog.cpp
void ADIFImportDialog::onImportClicked() {
    // ❌ BAD: Loading own copy of cty.dat from filesystem
    CountryFile countryFile;
    countryFile.loadFromFile("~/.tr4qt/cty.dat");

    // ❌ Creates duplicate, wastes memory, no sync with MainWindow's copy
}
```

**Good Pattern** (what TO do):
```cpp
// ADIFImportDialog.h
class ADIFImportDialog : public QDialog {
public:
    // ✅ GOOD: Receive pointer to existing data
    ADIFImportDialog(CountryFile* countryFile, QWidget* parent);

private:
    CountryFile* m_countryFile;  // Pointer to MainWindow's CountryFile
};

// Usage in MainWindow
ADIFImportDialog dialog(&m_countryFile, this);
```

**Checklist before loading/creating data**:
1. Search codebase for existing instances (`grep -r "CountryFile m_"`)
2. Check for singletons (`SomeClass::instance()`)
3. Check GlobalDatabase for tables with the data
4. Ask: "Who is the authoritative owner of this data?"
5. Pass pointer/reference instead of creating duplicate

**Real-world example**:
- **Issue**: ADIFImportDialog was loading cty.dat from filesystem
- **Problem**: MainWindow already had `m_countryFile` loaded
- **Fix**: Pass `&m_countryFile` to dialog constructor
- **Benefit**: No duplicate load, always uses same data, no sync issues

**Why this matters**:
- Thread safety: Multiple copies can get out of sync during reload
- Memory efficiency: Large files (like cty.dat) shouldn't be duplicated
- Single Source of Truth: One authoritative copy prevents conflicts
- Performance: Loading from disk is slow, reuse what's in memory

**This principle applies to ALL projects, not just TR4QT.**

### ALWAYS Use DialogHelper for User Dialogs

**CRITICAL**: ALL dialog messages MUST go through DialogHelper. Never use QMessageBox directly.

**Why this matters**:
- All dialogs are automatically logged (message + user response)
- Text is selectable/copyable by default for error messages
- Consistent user experience across the application
- Easier debugging via log analysis

**Pattern to follow**:
```cpp
// ❌ NEVER do this:
QMessageBox::question(this, "Title", "Message");
QMessageBox::information(this, "Title", "Message");
QMessageBox::warning(this, "Title", "Message");
QMessageBox::critical(this, "Title", "Error");

// ✅ ALWAYS use DialogHelper:
#include "../../utils/DialogHelper.h"

// Question dialog (returns QMessageBox::StandardButton)
QMessageBox::StandardButton reply = DialogHelper::question(
    this,
    "Confirm Action",
    "Are you sure you want to proceed?"
);
if (reply == QMessageBox::Yes) {
    // User clicked Yes
}

// Information
DialogHelper::information(this, "Success", "Operation completed successfully.");

// Warning
DialogHelper::warning(this, "Warning", "This action cannot be undone.");

// Critical error
DialogHelper::critical(this, "Error", QString("Failed: %1").arg(errorMessage));
```

**What DialogHelper does automatically**:
1. Logs the dialog message to debug log
2. Logs user's response (Yes/No/Cancel/etc.)
3. Makes text selectable in error dialogs
4. Consistent styling across the app

**Files**:
- `/src/utils/DialogHelper.h` - Dialog wrapper with logging
- `/src/utils/DialogHelper.cpp` - Implementation

**Note**: Using the return type `QMessageBox::StandardButton` is correct - that's what DialogHelper returns. The key is calling `DialogHelper::question()` instead of `QMessageBox::question()`.

**This pattern applies to all projects**: If one dialog is logged, ALL dialogs must be logged via a common helper class.

### ALWAYS Avoid Magic Numbers

**CRITICAL**: NEVER use literal numbers directly in code. Always use named constants.

**Why this matters**:
- Magic numbers are unclear: What does `12` mean? Field width? Buffer size? Timeout?
- Hard to maintain: Change one occurrence, might miss others
- Error-prone: Easy to use wrong value (typo, copy-paste)
- Poor readability: Named constants document intent

**Pattern to follow**:
```cpp
// ❌ BAD: Magic numbers
QString formatted = QString("%1 %2").arg(callsign, -12).arg(frequency, 10);
m_label->setMaximumHeight(45);
QTimer::singleShot(3000, this, &Widget::refresh);

// ✅ GOOD: Named constants
const int CALLSIGN_FIELD_WIDTH = 12;
const int FREQUENCY_FIELD_WIDTH = 10;
QString formatted = QString("%1 %2")
    .arg(callsign, -CALLSIGN_FIELD_WIDTH)
    .arg(frequency, FREQUENCY_FIELD_WIDTH);

const int LABEL_HEIGHT = m_label->fontMetrics().height() + 10;  // Font height + padding
m_label->setMaximumHeight(LABEL_HEIGHT);

const int REFRESH_INTERVAL_MS = 3000;  // 3 seconds
QTimer::singleShot(REFRESH_INTERVAL_MS, this, &Widget::refresh);
```

**Guidelines**:
1. **Name meaningfully**: `CALLSIGN_FIELD_WIDTH` not `WIDTH_1`
2. **Calculate when possible**: Use `fontMetrics().height()` instead of hardcoded pixels
3. **Add comments**: Explain what the value represents and why it's that value
4. **Use `const` or `constexpr`**: Prevent accidental modification
5. **Scope appropriately**: File-level for reused values, function-level for one-time use

**Common magic numbers to avoid**:
- Field widths and spacing (use named constants)
- Timeout values (name with units: `_MS`, `_SECONDS`)
- Buffer sizes (document why that size)
- Pixel dimensions (calculate from font metrics when possible)
- Array indices (use enum or named constants)

**Exceptions** (when literals are OK):
- Mathematical constants: `0`, `1`, `2` in formulas (e.g., `x * 2`)
- Boolean values: `true`, `false`
- Null/empty checks: `nullptr`, `0`, `-1` for "not found"
- Loop counters: `for (int i = 0; i < n; i++)`

**Real-world examples from TR4QT**:
```cpp
// ❌ Before: Magic 3
QString formatted = QString("%1%2 %3%4%5")
    .arg(freqStr, 10)
    .arg(QString(3, ' '))  // What is 3?
    .arg(callsign, -12);

// ✅ After: Named constant
const int CALLSIGN_INDENT = 3;  // Spaces between frequency and callsign
QString formatted = QString("%1%2 %3%4%5")
    .arg(freqStr, 10)
    .arg(QString(CALLSIGN_INDENT, ' '))
    .arg(callsign, -12);
```

**This principle applies to ALL projects, not just TR4QT.**

### ALWAYS Use ThemeManager for Colors

**CRITICAL**: NEVER use hardcoded hex color codes in UI code. Always use `ThemeManager::instance().color(ColorRole::...)`.

**Why this matters**:
- Enables theme support (TR4W Default, Dark Mode, High Contrast, Custom)
- Allows user customization via Custom theme
- Ensures consistent color management across the application
- Hardcoded colors break when user changes theme

**Pattern to follow**:
```cpp
// ❌ BAD: Hardcoded color
m_label->setStyleSheet("QLabel { color: #006600; }");

// ✅ GOOD: ThemeManager color
QString color = ThemeManager::instance().colorName(ColorRole::LotwUserText);
m_label->setStyleSheet(QString("QLabel { color: %1; }").arg(color));

// ✅ EVEN BETTER: Use QColor directly
QColor color = ThemeManager::instance().color(ColorRole::LotwUserText);
m_label->setPalette(QPalette(color));
```

**Available color roles** (see `ThemeManager.h`):
- Display: `VfoBackground`, `VfoText`, `WindowBackground`, `TextDisplayBackground`
- Status: `ConnectedStatus`, `DisconnectedStatus`, `FrozenIndicator`
- Functional: `DupeText`, `NewMultiplierBackground`, `WorkedStationText`, `MultiplierText`, `LotwUserText`
- Spot Aging: `NewSpotText`, `NewSpotBackground`, `AgingSpotText`, `AgingSpotBackground`
- UI: `PrimaryText`, `SecondaryText`, `HoverHighlight`, `BorderColor`

**Allowed hardcoded colors** (won't trigger error):
- Theme definitions in `ThemeManager.cpp`: `colors[ColorRole::VfoBackground] = QColor("#00FFFF");`
- Documented defaults: `QString defaultColor = "#006400";  // Theme default: dark green`

**Prevention**:
The git pre-commit hook enforces this rule:
- Located at `.git/hooks/pre-commit`
- **Blocks commits** containing hardcoded hex colors (e.g., `= "#0066cc"`)
- Excludes allowed patterns (ThemeManager.cpp, "// Theme default" comments)
- Can bypass with `git commit --no-verify` if intentional (not recommended)

**Hook Bug Fix (2026-01-06)**:
The original hook had a bug where `head -5` returns exit code 0 even with no input, causing false positives. Fixed by capturing output in a variable and checking if non-empty:

```bash
# ❌ OLD (buggy): head -5 returns 0 even with no output, triggers false positive
if echo "$STAGED_FILES" | xargs grep ... | head -5; then

# ✅ NEW (fixed): Capture output, only trigger if non-empty
HEX_COLOR_MATCHES=$(echo "$STAGED_FILES" | xargs grep ... | head -5)
if [ -n "$HEX_COLOR_MATCHES" ]; then
    echo "$HEX_COLOR_MATCHES"  # Display actual matches
```

This fix is applied to `.git/hooks/pre-commit` locally. The check is TR4QT-specific (not in general `scripts/pre-commit.sample`).

**This applies to ALL Qt projects with theme support.**

### NEVER Use setParent(nullptr) on Qt Widgets

**CRITICAL**: In Qt, calling `setParent(nullptr)` on a widget creates a TOP-LEVEL WINDOW. This is a dangerous API design that "fails open" (shows unwanted windows) rather than "fails closed".

**The Problem**:
Qt intentionally makes widgets with `parent = nullptr` into top-level windows, and it generates **no warnings**. This causes bizarre bugs where blank floating windows appear with fragments of UI text.

**Real bug in TR4QT**:
```cpp
// ❌ BUG: This created blank floating windows showing "Both:", "0", etc.
void BandSummaryGrid::rebuildGrid() {
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->setParent(nullptr);  // CREATES TOP-LEVEL WINDOW!
        }
        delete item;
    }
}
```

**Symptoms**:
- Blank Qt windows appearing intermittently on startup
- Windows showing small text fragments from your UI
- Appears on both macOS and Windows
- Intermittent (race condition - depends on paint events)

**What to do instead**:
```cpp
// ✅ GOOD: Delete the widget
widget->deleteLater();

// ✅ GOOD: Hide the widget
widget->hide();

// ✅ GOOD: Keep as child but remove from layout
// (Layout takes care of removal automatically)

// ✅ GOOD: If you REALLY want top-level, be explicit
widget->setParent(nullptr);
widget->setWindowFlags(Qt::Window);  // Make intent clear
widget->setAttribute(Qt::WA_DeleteOnClose);
widget->show();  // Explicitly show
```

**Prevention**:
A git pre-commit hook catches this pattern:
- Located at `.git/hooks/pre-commit`
- Blocks commits containing `setParent(nullptr)`
- Can bypass with `git commit --no-verify` if intentional

**Why Qt does this**:
Qt considers `setParent(nullptr)` a valid way to "promote" a widget to top-level (e.g., turning an embedded widget into a floating dialog). But it's far too easy to do accidentally, and the failure mode is terrible.

**This applies to ALL Qt projects.**

## Version Management

**CRITICAL**: The version number must be updated with every release commit IN FOUR PLACES!

Locations:
1. **Application**: `/src/core/Constants.h`
   ```cpp
   constexpr const char* APP_VERSION = "X.Y.Z";
   ```

2. **Windows Installer**: `/installer/tr4qt.nsi`
   ```nsis
   !define APPVERSION "X.Y.Z"
   ```

3. **macOS Bundle**: `/src/CMakeLists.txt`
   ```cmake
   MACOSX_BUNDLE_SHORT_VERSION_STRING "X.Y.Z"
   MACOSX_BUNDLE_BUNDLE_VERSION "X.Y.Z"
   ```

4. **Windows Resource File**: `/resources/tr4qt.rc`
   ```c
   #define VER_FILEVERSION             X,Y,Z,0
   #define VER_FILEVERSION_STR         "X.Y.Z.0\0"
   #define VER_PRODUCTVERSION          X,Y,Z,0
   #define VER_PRODUCTVERSION_STR      "X.Y.Z\0"
   ```

Convention:
- **Major (X)**: Major features or breaking changes
- **Minor (Y)**: New features, enhancements
- **Patch (Z)**: Bug fixes, small improvements

Update process:
1. Increment version in Constants.h
2. Update comment to describe the change
3. **Update version in tr4qt.nsi to match**
4. **Update version in CMakeLists.txt to match**
5. **Update version in tr4qt.rc to match (4 places!)**
6. **Update CHANGELOG.md**: Move items from `[Unreleased]` to new version section with date
7. Include version in commit message: "Feature description - vX.Y.Z"
8. Rebuild: `cmake --build build`

Example (Constants.h):
```cpp
constexpr const char* APP_VERSION = "3.8.1";  // Streamlined ADIF import with checkbox rescore
```

Example (tr4qt.nsi):
```nsis
!define APPVERSION "3.8.1"
```

Example (CMakeLists.txt):
```cmake
MACOSX_BUNDLE_SHORT_VERSION_STRING "3.8.1"
MACOSX_BUNDLE_BUNDLE_VERSION "3.8.1"
```

Example (tr4qt.rc):
```c
#define VER_FILEVERSION             3,8,1,0
#define VER_FILEVERSION_STR         "3.8.1.0\0"
#define VER_PRODUCTVERSION          3,8,1,0
#define VER_PRODUCTVERSION_STR      "3.8.1\0"
```

**Why all four files?**
- Constants.h: Used by the running application for "About" dialog, window titles, etc.
- tr4qt.nsi: Used by the Windows installer filename and registry entries
- CMakeLists.txt: Used by macOS Info.plist (shown in crash reports and About dialog)
- tr4qt.rc: Used by Windows .exe Properties → Details tab (File Version, Product Version, Company)
- If they don't match, users and crash reports show inconsistent versions

### Dependency Version Requirements

**CRITICAL**: See `VERSION_REQUIREMENTS.md` for required Qt, Hamlib, and other dependency versions.

**Key principle**: CI builds MUST use the same dependency versions as local development to prevent "works on my machine" issues.

Real example of version mismatch failure (v3.31.10):
- Windows CI: Qt 6.7.2, Local dev: Qt 6.10.1
- Result: CI-built releases had broken QSO grid, wrong theme colors
- Users downloaded broken installers from GitHub Releases
- Fix: Updated CI to Qt 6.10.1 to match local dev

**Enforcement:**
- CI includes automated version validation (fails build on mismatch)
- Run `./scripts/check-versions.sh` locally to verify versions
- Update `VERSION_REQUIREMENTS.md` when changing Qt/Hamlib versions

**This applies to ALL projects**: Version parity between CI and local dev is non-negotiable.

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

Platform-Specific:
- [ ] **Linux**: Implement "Email Logs to Support" feature (currently macOS/Windows only)
  - Feature saves logs to Desktop as zip file
  - Shows in file manager for easy email attachment
  - See `MainWindow::onEmailLogsToSupport()` for reference implementation

UI Enhancements:
- [ ] **DialogHelper Cmd-C Support**: Add keyboard copy (Cmd-C/Ctrl-C) support for dialog text
  - Current: Right-click → Copy works (text selectable)
  - Desired: Cmd-C/Ctrl-C keyboard shortcut for copying dialog text
  - Challenge: QLabel doesn't support keyboard copy shortcuts natively
  - Potential solutions:
    1. Replace QLabel with QTextEdit in custom QMessageBox subclass
    2. Event filter approach (attempted but caused stability issues)
    3. Use Qt's TextBrowserInteraction (attempted but didn't work reliably)
  - Status: Deferred - not a show-stopper, right-click copy is functional

## Deployment Guides

Detailed deployment documentation is in the `docs/` folder:
- **macOS**: See `docs/macos-deployment.md` for app bundling, code signing, and DMG creation
- **Windows**: See `docs/windows-deployment.md` for DLL deployment and installer creation

**Key principle**: Never trust automatic deployment tools (windeployqt, macdeployqt). They silently miss dependencies. Use explicit file copying instead.

## Platform-Specific Notes

### macOS
- Qt::ALT maps to Option, Qt::CTRL maps to Command
- Paths use `~/Library/Application Support/TR4QT/`

### Windows
- Paths use `%LOCALAPPDATA%\TR4QT\`
- Avoid "interface" as parameter name (conflicts with COM headers)
