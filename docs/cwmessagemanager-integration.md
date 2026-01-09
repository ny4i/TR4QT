# CWMessageManager Integration Guide

## Overview

The `CWMessageManager` class extracts CW messaging logic from `MainWindow`, providing a clean separation of concerns. This document explains how to integrate CWMessageManager into MainWindow.

## Files Created

- `/src/controllers/CWMessageManager.h` - Header with class definition
- `/src/controllers/CWMessageManager.cpp` - Implementation
- Updated `/src/CMakeLists.txt` - Added new files to build

## Architecture

### Responsibilities

**CWMessageManager handles:**
- Function key message lookup (F1-F12, Ctrl+F, Alt+F)
- CW template substitution via CWTemplateEngine
- Sending CW via RadioController
- Tracking last CW message (for = key repeat)
- Validating preconditions (radio connected, CW mode)

**MainWindow still handles:**
- UI updates (status bar, focus management)
- Dialog display (CWMessageEditorDialog, SendMorseDialog)
- Keyboard event handling
- AppSettings persistence
- Auto-send timing/triggering

## Integration Steps

### 1. Add Member Variable to MainWindow

```cpp
// MainWindow.h (in private section, near other controllers)
class MainWindow : public QMainWindow {
    // ...
private:
    // Controllers
    QSOLogger* m_qsoLogger;
    DataIntegrityManager* m_integrityManager;
    ContestManager* m_contestManager;
    CWMessageManager* m_cwMessageManager;  // NEW

    // Remove: QString m_lastCWMessage;  (now handled by CWMessageManager)
};
```

### 2. Initialize in Constructor

```cpp
// MainWindow.cpp constructor (after m_radio and m_activeContest are initialized)
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_cwMessageManager(nullptr)  // Initialize to nullptr
{
    // ... existing initialization ...

    // Initialize CW Message Manager
    CWMessageManager::Config cwConfig;
    cwConfig.radio = m_radio;
    cwConfig.contest = m_activeContest;
    m_cwMessageManager = new CWMessageManager(cwConfig);

    // ... rest of initialization ...
}
```

### 3. Update Contest Activation

When the contest changes, update the CWMessageManager's config:

```cpp
void MainWindow::activateContest(const ContestInfo& contestInfo) {
    // ... existing contest activation code ...

    // Update CW Message Manager with new contest
    if (m_cwMessageManager) {
        CWMessageManager::Config cwConfig;
        cwConfig.radio = m_radio;
        cwConfig.contest = m_activeContest;  // Updated contest pointer
        delete m_cwMessageManager;
        m_cwMessageManager = new CWMessageManager(cwConfig);
    }
}
```

**Note**: Alternative approach would be to add a `setContest()` method to CWMessageManager to avoid recreation.

### 4. Replace handleFunctionKey()

```cpp
// OLD CODE (delete this)
void MainWindow::handleFunctionKey(int fKey, bool ctrlPressed, bool altPressed) {
    bool isCQMode = (m_operatingMode == OperatingMode::CQ);
    QString messageTemplate;

    if (ctrlPressed) {
        messageTemplate = AppSettings::instance().getCtrlFMessage(fKey, isCQMode);
    } else if (altPressed) {
        messageTemplate = AppSettings::instance().getAltFMessage(fKey, isCQMode);
    } else {
        messageTemplate = isCQMode
            ? AppSettings::instance().getCQMessage(fKey)
            : AppSettings::instance().getSPMessage(fKey);
    }

    QString keyName = QString("F%1").arg(fKey);
    if (ctrlPressed) keyName = "Ctrl+" + keyName;
    if (altPressed) keyName = "Alt+" + keyName;
    QString modeStr = isCQMode ? "CQ" : "S&P";

    if (messageTemplate.isEmpty()) {
        LOG_INFO("MainWindow", QString("%1 (%2 mode): No message defined").arg(keyName).arg(modeStr));
        m_statusLabel->setText(QString("%1: No message defined").arg(keyName));
        return;
    }

    LOG_INFO("MainWindow", QString("%1 (%2 mode): Sending template: %3").arg(keyName).arg(modeStr).arg(messageTemplate));
    sendCWMessage(messageTemplate);
}

// NEW CODE (replace with this)
void MainWindow::handleFunctionKey(int fKey, bool ctrlPressed, bool altPressed) {
    // Build input for CW manager
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    // Send function key message
    CWMessageManager::Result result = m_cwMessageManager->sendFunctionKey(fKey, ctrlPressed, altPressed, input);

    // Update status bar
    m_statusLabel->setText(result.statusMessage);
}
```

### 5. Replace sendCWMessage()

```cpp
// OLD CODE (delete this - lines 5198-5245)
void MainWindow::sendCWMessage(const QString& messageTemplate) {
    // Check preconditions
    if (!m_radioConnected || !m_radio) {
        LOG_WARN("MainWindow", "Cannot send CW: radio not connected");
        m_statusLabel->setText("CW requires radio connection");
        return;
    }

    bool isCWMode = (m_currentState.modeA == ModeType::CW ||
                     m_currentState.modeA == ModeType::CWR);
    if (!isCWMode) {
        LOG_WARN("MainWindow", "Cannot send CW: not in CW mode");
        m_statusLabel->setText("CW requires CW mode");
        return;
    }

    // Build substitution context
    CWTemplateEngine::Context ctx;
    ctx.myCall = AppSettings::instance().getMyCallsign();
    ctx.hisCall = m_callsignEntry->text().trimmed().toUpper();
    ctx.qsoNumber = m_nextSerialNumber;
    ctx.mode = m_currentState.modeA;
    ctx.band = m_currentState.bandA;

    if (m_activeContest) {
        ctx.contestName = m_activeContest->getContestName();
        QString rst = RSTValidator::getDefault(ctx.mode);
        ctx.sentExchange = m_activeContest->formatSentExchange(ctx.qsoNumber, rst);
    }

    // Substitute template variables
    QString cwText = CWTemplateEngine::substitute(messageTemplate, ctx);

    // Send via radio
    int wpm = AppSettings::instance().getMorseWPM();
    m_radio->setCWSpeed(wpm);
    m_radio->sendCW(cwText);

    // Save for repeat (= key)
    m_lastCWMessage = cwText;

    m_statusLabel->setText(QString("Sending CW: %1").arg(cwText));
    LOG_INFO("MainWindow", QString("Sent CW: %1 (from template: %2)")
             .arg(cwText).arg(messageTemplate));
}

// NEW CODE (replace with this)
void MainWindow::sendCWMessage(const QString& messageTemplate) {
    // Build input for CW manager
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    // Send CW message
    CWMessageManager::Result result = m_cwMessageManager->sendCWMessage(messageTemplate, input);

    // Update status bar
    m_statusLabel->setText(result.statusMessage);
}
```

### 6. Replace = Key Repeat Logic

```cpp
// OLD CODE (delete from keyPressEvent, lines ~1456-1469)
case Qt::Key_Equal: {  // '=' key (repeat last CW message)
    if (m_lastCWMessage.isEmpty()) {
        LOG_INFO("MainWindow", "= key pressed, but no previous CW message to repeat");
        m_statusLabel->setText("No CW message to repeat");
        return;
    }

    if (!m_radioConnected || !m_radio) {
        return;
    }

    m_radio->sendCW(m_lastCWMessage);

    m_statusLabel->setText(QString("Repeating CW: %1").arg(m_lastCWMessage));
    LOG_INFO("MainWindow", QString("Repeated CW: %1 (via = key)").arg(m_lastCWMessage));
    return;
}

// NEW CODE (replace with this)
case Qt::Key_Equal: {  // '=' key (repeat last CW message)
    // Build input for CW manager
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    // Repeat last CW message
    CWMessageManager::Result result = m_cwMessageManager->repeatLastCWMessage(input);

    // Update status bar
    m_statusLabel->setText(result.statusMessage);
    return;
}
```

### 7. Update Auto-Send Code

The three auto-send locations (lines 2798, 3286, 3291) can continue calling `sendCWMessage()` since we've replaced it with the CWMessageManager wrapper.

**No changes needed** - the existing calls will work:

```cpp
// These calls continue to work (no changes needed)
sendCWMessage(qslMessage);                                      // Line 2798
sendCWMessage(AppSettings::instance().getSPCWExchange());       // Line 3286
sendCWMessage(AppSettings::instance().getCQCWExchange());       // Line 3291
```

### 8. Remove Old Code

Delete from MainWindow:
- ✅ Private methods: `handleFunctionKey()`, `sendCWMessage()` implementations (replace with wrappers)
- ✅ Member variable: `QString m_lastCWMessage;` (line 385 in MainWindow.h)

Keep in MainWindow:
- ✅ Public/private **declarations** of `handleFunctionKey()` and `sendCWMessage()` (as thin wrappers)
- ✅ Dialog methods: `onEditCWMessages()`, `onSendMorse()` (UI responsibility)

## Benefits

### Separation of Concerns
- **MainWindow**: UI, keyboard events, dialogs, auto-send triggers
- **CWMessageManager**: CW logic, template substitution, validation

### Testability
CWMessageManager can now be unit tested independently:

```cpp
// Example unit test
TEST_F(CWMessageManagerTest, SendFunctionKey_CQMode_F1) {
    CWMessageManager::Config config;
    config.radio = mockRadio;
    config.contest = mockContest;
    CWMessageManager manager(config);

    CWMessageManager::Input input;
    input.callsign = "W1AW";
    input.qsoNumber = 123;
    input.radioState.modeA = ModeType::CW;
    input.operatingMode = OperatingMode::CQ;

    auto result = manager.sendFunctionKey(1, false, false, input);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cwTextSent, "CQ TEST NY4I NY4I TEST");
}
```

### Reusability
Other components can now use CWMessageManager:
- Auto-CQ timer (future feature)
- Network logging station (future feature)
- Test utilities

## API Reference

### CWMessageManager::Input

```cpp
struct Input {
    QString callsign;            // Current callsign in entry field
    int qsoNumber;               // Current serial number
    RadioState radioState;       // Current radio state (for validation, context)
    OperatingMode operatingMode; // CQ vs S&P mode
};
```

### CWMessageManager::Result

```cpp
struct Result {
    bool success;           // true if CW sent successfully
    QString statusMessage;  // Always populated (for UI status bar)
    QString cwTextSent;     // Actual CW text sent (after substitution)
    QString errorMessage;   // Error message if success = false
};
```

### Key Methods

```cpp
// Handle function key press
Result sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed, const Input& input);

// Send CW message with template substitution
Result sendCWMessage(const QString& messageTemplate, const Input& input);

// Repeat last CW message (= key)
Result repeatLastCWMessage(const Input& input);

// Get last CW message sent (for debugging)
QString getLastCWMessage() const;
```

## Error Handling

CWMessageManager validates preconditions and returns descriptive errors:

```cpp
Result result = manager.sendCWMessage(template, input);
if (!result.success) {
    // result.errorMessage contains one of:
    // - "CW requires radio connection"
    // - "CW requires CW mode"
    m_statusLabel->setText(result.statusMessage);  // Same as errorMessage
}
```

## Logging

All CW operations are logged via the Logger framework:

```
[INFO] CWMessageManager: F1 (CQ mode): Sending template: CQ TEST \ \ TEST
[INFO] CWMessageManager: Sent CW: CQ TEST NY4I NY4I TEST (from template: CQ TEST \ \ TEST)
[INFO] CWMessageManager: Repeated CW: CQ TEST NY4I NY4I TEST
[WARN] CWMessageManager: Cannot send CW: radio not connected
[WARN] CWMessageManager: Cannot send CW: not in CW mode
```

## Future Enhancements

Potential improvements to CWMessageManager:

1. **Add setContest() method** to avoid recreating manager when contest changes
2. **Add setCWSpeed() override** for temporary speed changes
3. **Add stopCW() method** for Kill CW (Alt-K) feature
4. **Add auto-CQ support** with timing/repeat logic
5. **Add CW queue** for sending multiple messages sequentially

## Complete Example

Here's a complete before/after comparison:

### Before (MainWindow only)

```cpp
void MainWindow::handleFunctionKey(int fKey, bool ctrlPressed, bool altPressed) {
    bool isCQMode = (m_operatingMode == OperatingMode::CQ);
    QString messageTemplate = /* ... 15 lines of lookup logic ... */;
    QString keyName = /* ... 3 lines of formatting ... */;

    if (messageTemplate.isEmpty()) {
        /* ... 3 lines of logging/status ... */
        return;
    }

    sendCWMessage(messageTemplate);
}

void MainWindow::sendCWMessage(const QString& messageTemplate) {
    /* ... 47 lines of validation, substitution, sending ... */
}
```

### After (MainWindow + CWMessageManager)

```cpp
// MainWindow (thin wrapper)
void MainWindow::handleFunctionKey(int fKey, bool ctrlPressed, bool altPressed) {
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    auto result = m_cwMessageManager->sendFunctionKey(fKey, ctrlPressed, altPressed, input);
    m_statusLabel->setText(result.statusMessage);
}

void MainWindow::sendCWMessage(const QString& messageTemplate) {
    CWMessageManager::Input input;
    input.callsign = m_callsignEntry->text();
    input.qsoNumber = m_nextSerialNumber;
    input.radioState = m_currentState;
    input.operatingMode = m_operatingMode;

    auto result = m_cwMessageManager->sendCWMessage(messageTemplate, input);
    m_statusLabel->setText(result.statusMessage);
}

// CWMessageManager (handles all CW logic)
// - 47 lines of validation, substitution, sending
// - Now testable, reusable, maintainable
```

**Result**: 65+ lines of complex logic extracted from MainWindow, 8-line thin wrappers remain.

## Testing Checklist

After integration, verify:

- [ ] F1-F12 keys send CW messages in CQ mode
- [ ] F1-F12 keys send CW messages in S&P mode
- [ ] Ctrl+F1-F12 keys work in both modes
- [ ] Alt+F1-F12 keys work in both modes
- [ ] = key repeats last CW message
- [ ] = key shows "No CW message to repeat" when no previous message
- [ ] Auto-send works after logging QSO (QSL message)
- [ ] Auto-send works in S&P mode (S&P exchange)
- [ ] Auto-send works in CQ mode (CQ exchange)
- [ ] Status bar shows correct messages
- [ ] Logging shows correct INFO/WARN messages
- [ ] Error handling works (radio disconnected, not CW mode)
- [ ] Template substitution works ({MYCALL}, {HISCALL}, {NR}, etc.)
- [ ] CW speed matches WPM setting
- [ ] SendMorseDialog still works
- [ ] CWMessageEditorDialog still works

## Migration Notes

This is a **backward-compatible refactoring**:
- ✅ No changes to AppSettings (still reads CW messages the same way)
- ✅ No changes to CWTemplateEngine (still does substitution the same way)
- ✅ No changes to RadioController (still sends CW the same way)
- ✅ No changes to UI behavior (works exactly the same for users)

The only change is **where the code lives** - extracted from MainWindow into CWMessageManager.
