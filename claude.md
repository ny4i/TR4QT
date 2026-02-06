# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## 🏗️ BUILD PHILOSOPHY - APPLIES TO ALL PROJECTS

### Explicit > Implicit (when implicit is unreliable)

**NEVER use "automatic" deployment tools** (`windeployqt`, `macdeployqt`, `linuxdeploy`) - they silently fail (missing plugins/TLS backends).

**Instead: Be Explicit**
- List every dependency explicitly
- Copy files with clear commands
- Verify what was deployed
- See `docs/macos-deployment.md` and `docs/windows-deployment.md` for examples

## ⚠️ CRITICAL REMINDERS

### 🔌 HARDWARE MUST Run in Worker Threads

**CRITICAL**: ALL hardware communication (radios, amplifiers, rotators, keyers, etc.) MUST run in a dedicated worker thread. The main UI thread MUST NEVER block on external hardware I/O.

**Why this matters**:
- Hardware I/O is unpredictable (timeouts, slow responses, network latency)
- Blocking the main thread freezes the entire UI
- Windows is especially sensitive to main thread blocking (shows "Not Responding")
- Network I/O (TCP/UDP) can stall for seconds on packet loss

**The Pattern** (see `RadioController` as reference):
```cpp
// ✅ CORRECT: Controller wraps device in worker thread
class HardwareController : public QObject {
    Q_OBJECT
public:
    explicit HardwareController(QObject* parent = nullptr);
    ~HardwareController() override;

public slots:
    void connectDevice(const Config& config);
    void disconnectDevice();

signals:
    void connectionStatusChanged(bool connected);
    void stateUpdated(const DeviceState& state);

private:
    QThread m_workerThread;           // Worker thread owned by controller
    IDeviceInterface* m_device;       // Device lives in worker thread
};

// In constructor:
m_device->moveToThread(&m_workerThread);
m_workerThread.start();

// In destructor:
m_workerThread.quit();
m_workerThread.wait();
```

**Wrong Pattern**:
```cpp
// ❌ WRONG: Device created on main thread
void MainWindow::connectAmplifier() {
    m_amplifier = AmplifierFactory::create(config, this);  // Lives on UI thread!
    // All polling timers and I/O block the UI
}
```

**Correct Pattern**:
```cpp
// ✅ CORRECT: Controller manages worker thread
void MainWindow::connectAmplifier() {
    m_amplifierController = new AmplifierController(this);  // Controller on main thread
    m_amplifierController->connectDevice(config);           // Device on worker thread
    // All I/O happens on worker thread, signals update UI
}
```

**Checklist for new hardware support**:
1. Create a `*Controller` class that wraps the device interface
2. Controller owns a `QThread m_workerThread`
3. Device is moved to worker thread via `moveToThread()`
4. All I/O happens on worker thread
5. Communicate via signals/slots (Qt handles cross-thread automatically)
6. Clean up thread in destructor: `quit()` then `wait()`

**Real bug (Issue #69)**: KPA1500 amplifier ran 18 UDP commands every 250ms on main thread → Windows UI froze with "Not Responding". Fixed by moving to worker thread.

**This applies to ALL projects with hardware I/O.**

### ⚡ BATCH Expensive Operations on Main Thread

**CRITICAL**: When the main thread must perform expensive operations repeatedly (e.g., UI updates from state changes), ALWAYS batch multiple operations together to minimize expensive overhead.

**Why this matters**:
- Some operations have fixed overhead that dominates total time (parsing, memory allocation, serialization)
- Doing the operation once for N changes is far faster than doing it N times
- Windows is especially sensitive to main thread blocking
- Symptom: "Not Responding" even though individual operations are fast

**The Pattern** (SVG LED updates as reference):
```cpp
// ❌ WRONG: Reload renderer for EACH LED update
void updateLeds(const State& state) {
    for (const QString& ledId : allLeds) {
        if (shouldBeOn(ledId, state)) {
            setLedOn(ledId, color);  // Reloads entire SVG renderer!
        } else {
            setLedOff(ledId);  // Reloads entire SVG renderer!
        }
    }
    // Result: 29 LEDs × renderer reload = 29 × (delete, allocate, parse SVG)
    //         116 reloads/second → UI freeze
}

// ✅ CORRECT: Batch all updates, reload renderer ONCE
void updateLeds(const State& state) {
    m_svgPanel->beginBatch();  // Defer expensive operation

    for (const QString& ledId : allLeds) {
        if (shouldBeOn(ledId, state)) {
            setLedOn(ledId, color);  // Just modifies DOM
        } else {
            setLedOff(ledId);  // Just modifies DOM
        }
    }

    m_svgPanel->endBatch();  // Reload renderer ONCE for all changes
    // Result: 1 reload per update cycle = 4 reloads/second → smooth UI
}
```

**Batching Implementation Pattern**:
```cpp
class ExpensiveWidget {
private:
    bool m_batchMode{false};

public:
    void beginBatch() {
        m_batchMode = true;
    }

    void endBatch() {
        if (!m_batchMode) return;
        m_batchMode = false;

        // Do expensive operation ONCE
        reloadRenderer();
        update();
    }

    void updateElement(const QString& id, const QColor& color) {
        // Cheap operation (modify data structure)
        m_data[id] = color;

        // Skip expensive operation if batching
        if (m_batchMode) return;

        // Expensive operation (only when not batching)
        reloadRenderer();
        update();
    }
};
```

**When to use batching**:
1. Multiple related updates happen together (e.g., state change updates 20+ LEDs)
2. Each update triggers expensive operation (parsing, allocation, serialization)
3. Expensive operation doesn't depend on individual update order
4. Updates happen frequently (e.g., every 250ms from hardware polling)

**Debugging expensive operations**:
1. Add logging with timestamps to measure operation timing
2. Don't guess - trace with logs to find actual bottleneck
3. Look for operations called in loops (multiplication effect)
4. Check for unnecessary re-creation of heavy objects (parsers, renderers, etc.)

**Real bug (v3.40.36)**: Amplifier Control window updated 19 status LEDs + 10 SWR bargraph LEDs every 250ms. Each LED update reloaded entire SVG renderer (delete QSvgRenderer, allocate new one, parse full SVG XML). Result: 29 reloads × 4 updates/sec = 116 renderer reloads/second → Windows UI froze with "Not Responding". Fixed by batching all LED updates into single renderer reload: 29X performance improvement (116/sec → 4/sec).

**Key insight**: The work MUST happen on main thread (Qt requirement for widgets), but HOW MUCH work matters. Batch to minimize expensive overhead.

**This applies to ALL projects with repeated expensive operations.**

### 🚨 ARCHITECTURE: MainWindow Extraction In Progress

**MainWindow limits** (special case - main UI entry point has higher limits):
| Threshold | Normal Class | MainWindow |
|-----------|--------------|------------|
| Yellow | 500 | 2,000 |
| Red | 1,000 | 2,500 |
| STOP | 1,500 | **3,000** |

**Current status**: 5,050 lines (1.7X over 3,000 STOP limit) - extraction ongoing

**BEFORE implementing any new feature, CHECK:**

1. **Will this add SQL to MainWindow?** → Create Repository/Service instead
2. **Will this add business logic to MainWindow?** → Create Service instead
3. **Will the event handler exceed 20 lines?** → Extract to Service method
4. **Does MainWindow already exceed 3,000 lines?** → STOP, extract services first

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

### ALWAYS Use Explicit QSettings Constructor (macOS plist issue)

**CRITICAL**: On macOS, ALWAYS use `QSettings(APP_ORG, APP_NAME)` instead of `QSettings()`.

**The Problem**:
Using the default `QSettings()` constructor creates a DIFFERENT plist file than `QSettings(APP_ORG, APP_NAME)`:
- Default constructor: `com.tr4qt.tr4qt.TR4QT.plist` (WRONG)
- Explicit constructor: `com.tr4qt.TR4QT.plist` (CORRECT)

This causes preferences to be split across two files, making them appear "lost".

**Pattern to follow**:
```cpp
#include "../core/Constants.h"  // For APP_ORG, APP_NAME

// ❌ NEVER do this:
QSettings settings;

// ✅ ALWAYS do this:
QSettings settings(APP_ORG, APP_NAME);
```

**Files that have caused this bug**:
- ThemeManager.cpp (fixed v3.38.57)
- Prior fix (v3.38.54) missed ThemeManager

**macOS Preferences Cache**:
After manually editing plist files, run `pkill cfprefsd` to clear the macOS preferences daemon cache and force it to re-read from disk.

**Verification**:
```bash
# Should only show ONE plist file:
ls ~/Library/Preferences/ | grep -i tr4
# Expected: com.tr4qt.TR4QT.plist

# If you see com.tr4qt.tr4qt.TR4QT.plist, something is using default QSettings()
```

**This applies to ALL Qt projects on macOS.**

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
Git pre-commit hook (`.git/hooks/pre-commit`) blocks hardcoded hex colors. Bypass with `--no-verify` if intentional.

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
1. Increment version in Constants.h (e.g., `"3.8.1"`)
2. Update comment to describe the change
3. Update tr4qt.nsi: `!define APPVERSION "3.8.1"`
4. Update CMakeLists.txt: `MACOSX_BUNDLE_SHORT_VERSION_STRING` and `BUNDLE_VERSION`
5. Update tr4qt.rc: `VER_FILEVERSION`, `VER_FILEVERSION_STR`, `VER_PRODUCTVERSION`, `VER_PRODUCTVERSION_STR`
6. Update CHANGELOG.md: Move items from `[Unreleased]` to new version section
7. Include version in commit message: "Feature description - vX.Y.Z"
8. Rebuild: `cmake --build build`

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
# Use pkill WITHOUT -9 to allow graceful shutdown (saves window state/settings)
pkill tr4qt && sleep 1
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

**CRITICAL**: Always `pkill tr4qt` (without `-9`) before running the application. Using `-9` sends SIGKILL which bypasses `closeEvent()` and loses unsaved window state/settings.

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
```

**Note:** Git attribution disabled in settings.json (includeCoAuthoredBy=false, gitAttribution=false)

## Pre-Release Checklist

Before creating release tags (triggers GitHub Actions builds):

1. **Runner Status**: `./scripts/check-runners.sh` shows all runners healthy
2. **Qt Module Sync**: CMakeLists.txt Qt components must match `.github/workflows/build.yml` modules (except base: Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml)
3. **CI Passed**: `gh run list --limit 3` shows ✓ for macOS, Windows, and Linux
4. **Version Bumped**: `grep APP_VERSION src/core/Constants.h` matches commit message
5. **Tests Pass**: `cmake --build build && cd build && ctest --output-on-failure`
6. **Clean Tree**: `git status` shows no uncommitted changes
7. **Create Tag**: `git tag -a vX.Y.Z -m "Release vX.Y.Z - Description" && git push origin vX.Y.Z`
8. **Monitor**: `gh run watch` until all platforms succeed

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
See "ALWAYS Use DialogHelper for User Dialogs" above.

## Known Issues / Limitations

### Database Threading (Deferred until Networking Implementation)
**Issue**: Database singleton not thread-safe for concurrent writes (single-operator SAFE, multi-threaded CRASHES). Deferred until TCP networking. Performance: 6,800+ QSOs/sec single-threaded. See `/src/data/Database.cpp` line 13 for implementation options.

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

### Scoring System

**Key files for scoring:**
- `/src/services/ScoreCalculationService.cpp` - Central score calculation (used by both GUI and headless)
- `/src/contests/ContestBase.h` - Contest scoring interface
- `/src/controllers/QSOLogger.cpp` - Sets QSO.qsoPoints when logging

**Scoring architecture:**
1. **QSO Points**: `ContestBase::calculateQSOPoints(qso, myStation)` - called when logging
   - Most contests: Same continent = 1pt, Different continent = 3pt (for Phone)
   - CW contests add mode bonus (e.g., 2pt same continent for CW)
   - Uses `myStation.continent` vs `qso.continent` comparison

2. **Multipliers**: Each contest defines `getMultiplierTypes()` returning list of `MultiplierDefinition`:
   - `MultiplierType`: Zone, Country, Section, Prefix, State, etc.
   - `MultiplierScope`: `PerBand` (counts per band) or `AllBands` (counts once)
   - Example: CQ WW uses Zone + Country, both PerBand

3. **Total Score**: `ContestBase::calculateTotalScore(totalPoints, multiplierCounts)`
   - Most contests: `totalPoints × totalMultipliers`
   - ARRL SS: `totalPoints × sections` (2pts per QSO × sections)

**Critical fix (v3.40.31):**
- Per-band multipliers must track unique values per band, not globally
- `ScoreCalculationService` uses `QMap<MultiplierType, QMap<BandType, QSet<QString>>>` for PerBand
- `totalMultipliers` should sum `multiplierCounts` (not count `isMultiplier` flags)

**Station info for scoring:**
- `myStation` is built from cty.dat lookup on our callsign
- ALWAYS use cty.dat for `continent`, `cqZone`, etc. (not AppSettings which may be stale)
- `ContestManager::buildStationInfo()` at line 212-238 does the lookup

**Exchange format for Cabrillo:**
- `formatSentExchange(serialNumber, rst)` returns the exchange to store in QSO.exchangeSent
- Contests using serial numbers: prepend serial to `m_exchangeSent`
- `m_exchangeSent` is set via `setExchangeSent()` when contest is activated
- ARRL SS: `m_exchangeSent` = "M 62 WWA" (prec check section), format returns "1 M 62 WWA"

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
