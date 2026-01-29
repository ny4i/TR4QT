# Threading Issues - Windows UI Freezing (Issue Found: 2026-01-29)

## Problem Summary

On Windows, the application experiences UI freezing with screen flashing and "Not Responding" window titles. The issue does not occur on macOS, suggesting platform-specific timing or I/O behavior.

**Root Cause:** Amplifier and Rotator controllers run on the main UI thread, blocking the event loop with network I/O and timer events.

## Evidence

### Log Analysis

From `WindowsFlashingNotResponding.log`:

1. Application starts normally (line 1)
2. Amplifier connects successfully (line 226): `Connected to KPA1500 at 192.168.73.109:1500`
3. Amplifier control window initializes (line 304-331)
4. **Log cuts off abruptly during LCD positioning** (line 332) - suggests UI event loop stalled

### Timestamp Anomaly (Windows-specific)

Line 288 shows suspicious timestamp:
```
29 Jan 2026 10:20:06.602 trace KPA1500Direct - Sending DS: 1769700006597ms since last
```

That's **1769700006597ms = ~20 days** - indicates timing overflow or calculation bug on Windows that doesn't occur on macOS.

## Architecture Problems

### Current State (Broken)

```
MainWindow (UI Thread)
├─> RadioController ✅ CORRECT
│   └─> Worker Thread
│       └─> RadioInterface (Hamlib I/O)
│
├─> AmplifierService ❌ WRONG
│   └─> KPA1500Direct (Main Thread!)
│       ├─> QTimer (250ms polls)
│       └─> QUdpSocket (18+ commands per cycle)
│
└─> RotatorService ❌ WRONG
    └─> HamlibRotator (Main Thread!)
        ├─> QTimer (500ms polls)
        └─> Hamlib I/O
```

### Component Comparison

| Component | Thread | Polling Interval | Commands per Cycle | UI Impact |
|-----------|--------|------------------|-------------------|-----------|
| **Radio** | ✅ Worker thread (RadioController) | Fast | Many | **None** |
| **Amplifier** | ❌ Main thread | 250ms | 18+ UDP | **Severe freezing** |
| **Rotator** | ❌ Main thread | 500ms | 1-2 | **Moderate freezing** |

## Code Evidence

### Amplifier Creation (src/ui/MainWindow.cpp:1084)

```cpp
IAmplifierController* amplifierController = AmplifierFactory::createAmplifier(type, config, this);
                                                                                              ^^^^
                                                                                    Main thread parent!
```

The amplifier is created with `this` (MainWindow) as parent, so it lives on the main UI thread.

### KPA1500Direct Polling (src/amplifiers/KPA1500Direct.cpp:19-39)

```cpp
m_pollCommands = {
    "^PWF;",  // Forward power
    "^PWI;",  // Input power
    "^PWR;",  // Reflected power
    "^SW;",   // SWR
    "^SB;",   // SWR bypass
    "^OS;",   // Operating status
    "^ON;",   // Power state
    "^BN;",   // Band number
    "^FQ;",   // TX frequency
    "^AN;",   // Antenna
    "^AI;",   // ATU inline/bypassed
    "^AM;",   // ATU mode
    "^TP;",   // Tune in progress
    "^PC;",   // Drive power
    "^TM;",   // Temperature
    "^VMH;",  // 50V supply voltage
    "^DS;",   // LCD display (throttled to 500ms)
    "^LQ;",   // LED states
    "^FL;"    // Fault code
};
```

**18 UDP commands sent every 250ms** on the main thread = UI freezing.

### Rotator Creation (src/ui/MainWindow.cpp:1133)

```cpp
IRotatorController* rotatorController = RotatorFactory::createRotator(type, config, this);
                                                                                      ^^^^
                                                                            Main thread parent!
```

Same issue - rotator created on main thread.

### HamlibRotator Polling (src/rotator/HamlibRotator.h:68-69)

```cpp
QTimer* m_pollTimer{nullptr};
int m_pollIntervalMs{500};  // 500ms polling interval
```

Hamlib I/O every 500ms on main thread contributes to freezing.

## Solution: Worker Thread Architecture

### Pattern (from RadioController)

RadioController successfully isolates radio I/O to a worker thread:

1. **Create QThread** for worker
2. **Move controller to worker thread** using `moveToThread()`
3. **Connect signals across threads** (Qt handles automatically with QueuedConnection)
4. **Start worker thread**
5. **Use signals to communicate** between main thread and worker

### Implementation Plan

#### 1. Create AmplifierController (like RadioController)

```cpp
class AmplifierController : public QObject {
    Q_OBJECT
public:
    explicit AmplifierController(QObject* parent = nullptr);
    ~AmplifierController() override;

public slots:
    void connectToAmplifier(const AmplifierConfig& config);
    void disconnectFromAmplifier();
    void sendCommand(const QString& command);

signals:
    void connectionStatusChanged(bool connected);
    void stateUpdated(const AmplifierState& state);
    // ... other signals forwarded from IAmplifierController

private:
    QThread m_workerThread;
    IAmplifierController* m_amplifier;  // Lives in worker thread
    QMutex m_stateMutex;
    AmplifierState m_lastState;
};
```

**Key change in MainWindow:**
```cpp
// OLD (broken):
IAmplifierController* amp = AmplifierFactory::createAmplifier(type, config, this);
m_amplifierService = new AmplifierService(amp, this);

// NEW (fixed):
m_amplifierController = new AmplifierController(this);  // Controller on main thread
m_amplifierController->connectToAmplifier(config);      // Amplifier moved to worker thread
m_amplifierService = new AmplifierService(m_amplifierController, this);
```

#### 2. Create RotatorController (rename existing if needed)

Similar pattern - move rotator to worker thread:

```cpp
class RotatorController : public QObject {
    // Same pattern as AmplifierController
private:
    QThread m_workerThread;
    IRotatorController* m_rotator;  // Lives in worker thread
};
```

## Why This Works on macOS but Fails on Windows

Possible explanations for platform differences:

1. **macOS event loop is more tolerant** of blocking I/O on main thread
2. **Windows UDP I/O may block longer** than macOS (different kernel behavior)
3. **Timer coalescing differs** between platforms
4. **The timestamp overflow** (1769700006597ms) suggests Windows-specific timing bug in `QDateTime::currentMSecsSinceEpoch()` usage

## Files to Modify

### New Files
- `src/controllers/AmplifierController.h`
- `src/controllers/AmplifierController.cpp`
- `src/controllers/RotatorController.h` (or rename existing)
- `src/controllers/RotatorController.cpp`

### Modified Files
- `src/ui/MainWindow.h` - Add `AmplifierController*` and `RotatorController*` members
- `src/ui/MainWindow.cpp` - Use controllers instead of creating devices directly
- `src/services/AmplifierService.h` - Accept controller instead of IAmplifierController
- `src/services/AmplifierService.cpp` - Work with controller
- `src/services/RotatorService.h` - Accept controller instead of IRotatorController
- `src/services/RotatorService.cpp` - Work with controller

### CMakeLists.txt
- Add new controller source files to build

## Testing Plan

1. **Verify on Windows** that UI no longer freezes with amplifier connected
2. **Verify on macOS** that nothing broke (regression test)
3. **Test rotator** on both platforms
4. **Test rapid commands** (button mashing in amplifier control)
5. **Test shutdown** - ensure worker threads clean up properly

## References

- Working example: `src/radio/RadioController.h` and `.cpp`
- Log file: `C:\Users\toms\AppData\Local\TR4QT\logs\WindowsFlashingNotResponding.log`
- Date discovered: 2026-01-29
- Platforms affected: Windows (primarily), possibly Linux (untested)
- Platform unaffected: macOS (masks the issue due to different I/O characteristics)

## Priority

**HIGH** - This is a critical usability bug on Windows. The application becomes unusable when the amplifier is connected.
