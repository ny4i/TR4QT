# Rotator System Documentation

**Issue**: GitHub Issue #60 - Add PSTRotator support for turning the rotor

**Status**: Complete - Core implementation finished, ready for integration

**Date**: 2026-01-16

---

## Overview

The rotator system provides antenna rotator control for TR4QT. The initial implementation supports UDP control of PSTRotator software, with architecture designed for future expansion to additional rotator protocols (GS-232, Yaesu, etc.).

## Architecture

### Three-Layer Design

1. **Controller Layer** (`src/rotator/`)
   - `IRotatorController` - Abstract interface for all rotator types
   - `PSTRotatorController` - UDP implementation for PSTRotator
   - `RotatorFactory` - Creates rotator instances by type

2. **Service Layer** (`src/services/`)
   - `RotatorService` - Business logic wrapper
   - Manages connection state
   - Validates commands
   - Emits Qt signals for UI updates

3. **UI Layer** (Future - not implemented yet)
   - MainWindow integration
   - Status bar error display
   - Rotator control UI widgets

### Thread Safety

- **Background thread**: All UDP communication runs in worker thread
- **Non-blocking commands**: `setAzimuth()` and `stop()` are fire-and-forget
- **Qt signals**: Cross-thread communication (thread-safe)
- **Mutex protection**: Internal state protected with `QMutex`

## PSTRotator Protocol

### Commands Implemented

| Command | Format | Purpose |
|---------|--------|---------|
| Set Azimuth | `<PST><AZIMUTH>nnn</AZIMUTH></PST>` | Rotate to heading (0-360°) |
| Stop | `<PST><STOP>1</STOP></PST>` | Stop rotation |
| Query Azimuth | `<PST>AZ?</PST>` | Get current heading |

### UDP Ports

- **Send port**: User-configured (default 12000)
- **Response port**: Send port + 1 (e.g., 12001)
- PSTRotator listens on send port, responds on send port + 1

### Response Format

Query responses: `AZ:nnn<CR>` or `AZ:nnn.n<CR>`

Example: `AZ:90<CR>` or `AZ:90.5<CR>`

## Files Created

### Source Code
- `src/rotator/IRotatorController.h` - Base interface
- `src/rotator/PSTRotatorController.h/.cpp` - UDP implementation
- `src/rotator/RotatorFactory.h/.cpp` - Factory for creating instances
- `src/services/RotatorService.h/.cpp` - Business logic layer

### Tests
- `tests/test_rotator_service.cpp` - Unit tests with mock rotator
- All tests passing (12 tests)

### Tools
- `tools/rotator_test_manual.cpp` - Manual test program for physical hardware
- Interactive validation with actual PSTRotator

### Build System
- Updated `src/CMakeLists.txt` - Added rotator to tr4qt_core
- Updated `tests/CMakeLists.txt` - Added test target
- Created `tools/CMakeLists.txt` - Added manual test program
- Updated main `CMakeLists.txt` - Added tools subdirectory

## Usage Example

```cpp
// Create rotator service
RotatorConfig config;
config.ipAddress = "192.168.1.100";
config.port = 12000;

IRotatorController* controller = RotatorFactory::createRotator(
    RotatorFactory::RotatorType::PSTROTATOR,
    config,
    parent
);

RotatorService* service = new RotatorService(controller, this);

// Connect signals for UI updates
connect(service, &RotatorService::errorOccurred,
        this, [this](QString msg) {
    statusBar()->showMessage(msg, 5000);
});

connect(service, &RotatorService::connectionStatusChanged,
        this, &MainWindow::onRotatorConnectionChanged);

// Control rotator
service->setAzimuth(90);  // Rotate to 90° (fire-and-forget)
service->stop();          // Stop rotation

// Query position (blocks briefly with timeout)
auto azimuth = service->getCurrentAzimuth(1000);  // 1000ms timeout
if (azimuth.has_value()) {
    qDebug() << "Current azimuth:" << azimuth.value() << "°";
}
```

## Testing

### Unit Tests
```bash
cd build
ctest -R test_rotator_service --output-on-failure
```

All tests pass:
- Connection management
- Azimuth validation (0-360°)
- Command execution
- Error signal propagation
- Factory creation

### Manual Testing with Physical Rotator

```bash
cd build/tools
./rotator_test_manual 192.168.1.100 12000
```

Interactive test sequence:
1. Connect to PSTRotator
2. Query current azimuth
3. Rotate to 90°
4. Verify position
5. Rotate to 180°
6. Stop rotation
7. Query final position

Developer manually verifies physical rotator movement.

## Missing Items / Future Work

### 1. MainWindow Integration (Not Implemented)

**What's needed**:
- Add `RotatorService* m_rotatorService` member to MainWindow
- Create rotator configuration dialog (IP address, port)
- Add menu item: "Rotator → Connect"
- Add status indicator (connected/disconnected)
- Connect error signals to status bar

**Why not implemented now**:
- Per user request: "this can be done in parallel to other work"
- Architecture is complete, integration is straightforward
- Avoids MainWindow bloat (keep it under 3,000 line limit)

**Estimated effort**: 2-3 hours
- Dialog: 30 minutes
- Menu integration: 30 minutes
- Status indicator: 30 minutes
- Signal connections: 30 minutes
- Testing: 30 minutes

### 2. UI for Rotator Control (Not Implemented)

**What's needed**:
- Widget showing current azimuth
- Buttons for manual rotation (←90°, 180°→, etc.)
- Input field for custom heading
- Visual indicator (compass rose or numeric display)

**Why not implemented now**:
- User requested focus on core functionality first
- UI design decisions need user input

**Estimated effort**: 4-6 hours

### 3. Configuration Persistence (Not Implemented)

**What's needed**:
- Save rotator config to `AppSettings`
- Auto-connect on startup option
- Remember last IP/port

**Where to add**:
- `AppSettings::saveRotatorConfig()`
- `AppSettings::loadRotatorConfig()`

**Estimated effort**: 1 hour

### 4. Additional Rotator Protocols (Not Implemented)

**Future protocols to support**:
- **GS-232**: Serial protocol (Yaesu rotators)
- **Yaesu GS-232A/B**: Extended commands
- **DCU-1**: M2 Antenna Systems
- **SPID Rot2Prog**: Serial/network

**Architecture ready**: Just implement new controller classes:
```cpp
class GS232RotatorController : public IRotatorController {
    // Serial communication instead of UDP
};

// Add to factory
enum class RotatorType {
    PSTROTATOR,
    GS232,      // ← New
    YAESU_GS232A,
    // ...
};
```

**Estimated effort per protocol**: 4-8 hours

### 5. Elevation Support (Not Implemented)

**What's needed**:
- `setElevation(int degrees)` implemented
- Azimuth/Elevation (az/el) rotators
- Query elevation command for PSTRotator

**Why not implemented now**:
- PSTRotator elevation support unclear from documentation
- Most TR4QT users have azimuth-only rotators
- Easy to add later if needed

**Estimated effort**: 2-3 hours

### 6. Rotator Position Tracking (Not Implemented)

**What's needed**:
- Periodic polling of azimuth
- Emit `azimuthChanged` signal on updates
- Display live position in UI

**Why not implemented now**:
- "Fire-and-forget" approach per user request
- Don't care about state during rotation

**If needed later**:
```cpp
// Add to PSTRotatorController
QTimer* m_pollTimer{nullptr};
void startPolling(int intervalMs = 1000);
void stopPolling();

private slots:
    void pollAzimuth();
```

**Estimated effort**: 2 hours

### 7. Rotator Presets (Not Implemented)

**What's needed**:
- Save favorite headings (e.g., "EU: 45°", "JA: 315°")
- Quick-access buttons
- Stored in AppSettings

**Estimated effort**: 3-4 hours

### 8. Automatic Beam Heading (Not Implemented)

**What's needed**:
- Calculate beam heading from logged callsign
- Use CountryFile DXCC data + Great Circle
- Auto-rotate to calculated heading

**Integration points**:
- `MainWindow::onLogQSO()` - Calculate heading after lookup
- `RotatorService::setAzimuth()` - Command rotation

**Estimated effort**: 4-6 hours

### 9. Error Recovery (Partial Implementation)

**What's implemented**:
- UDP timeout detection (1 second default)
- Error logging
- Error signals to UI (via `errorOccurred`)

**What's missing**:
- Auto-reconnect on connection loss
- Retry failed commands
- Health monitoring (periodic keep-alive)

**Estimated effort**: 2-3 hours

### 10. Rotator Calibration (Not Implemented)

**What's needed**:
- Offset correction (if rotator shows wrong heading)
- Store calibration in AppSettings
- Apply offset to all commands/queries

**Example**:
```cpp
// User's rotator shows 10° when pointed at 0° true north
m_calibrationOffset = -10;  // Subtract 10° from all headings

// When sending commands:
int correctedAzimuth = (requestedAzimuth + m_calibrationOffset + 360) % 360;
```

**Estimated effort**: 1-2 hours

## Known Limitations

### 1. Const Method Signal Emission

**Issue**: Qt signals cannot be emitted from const methods.

**Impact**: `getAzimuth()` and `parseAzimuthResponse()` log errors but cannot emit `errorOccurred` signal.

**Workaround**: Errors are logged with `LOG_ERROR`. UI won't receive signal for query failures.

**Proper fix** (future):
```cpp
// Option A: Make getAzimuth() non-const
std::optional<int> getAzimuth(int timeoutMs = 1000);  // Remove const

// Option B: Use mutable for signal emission (requires Qt meta-object changes)
// Option C: Return error info instead of emitting signal
struct AzimuthResult {
    std::optional<int> azimuth;
    QString error;
};
AzimuthResult getAzimuth(int timeoutMs = 1000) const;
```

**Estimated effort**: 30 minutes

### 2. UDP Binding on Startup

**Issue**: PSTRotatorController binds to response port (e.g., 12001) on connection.

**Impact**: Only one TR4QT instance can connect to same PSTRotator port.

**Workaround**: Use different ports for multiple instances.

**Proper fix** (if needed):
- Bind response socket only during queries
- Unbind after response received
- Allows multiple clients to same rotator

**Estimated effort**: 1 hour

### 3. No Position Interpolation

**Issue**: Rotator position is only known when queried.

**Impact**: UI cannot show smooth rotation progress.

**Workaround**: Query periodically (see "Position Tracking" above).

**Proper fix**:
- Track last commanded heading
- Estimate current position based on rotation speed
- Query periodically to correct estimate

**Estimated effort**: 3-4 hours

## Integration Checklist (For Future PR)

Before integrating rotator system into MainWindow:

- [ ] Add rotator config dialog
- [ ] Add menu item: "Rotator → Connect"
- [ ] Connect `RotatorService::errorOccurred` to status bar
- [ ] Add UI indicator for connection status
- [ ] Save/load rotator config from AppSettings
- [ ] Document user-facing features in CHANGELOG.md
- [ ] Test with physical rotator (if available)
- [ ] Update user documentation

## Credits

**Implementation**: Claude Sonnet 4.5 + User (toms)

**Date**: 2026-01-16

**GitHub Issue**: #60

**Commit Message Template**:
```
Add PSTRotator UDP control support - Issue #60

Implements antenna rotator control with PSTRotator UDP protocol.
Architecture designed for future expansion to GS-232, Yaesu, etc.

Core Components:
- IRotatorController abstract interface
- PSTRotatorController UDP implementation
- RotatorService business logic layer
- RotatorFactory for creating instances
- Background thread for non-blocking commands
- Qt signals for UI error notifications

Testing:
- 12 unit tests with mock rotator (all passing)
- Manual test program for physical hardware validation

Not included (future work):
- MainWindow integration
- UI widgets for rotator control
- Configuration persistence
- Additional rotator protocols

Files added:
- src/rotator/IRotatorController.h
- src/rotator/PSTRotatorController.{h,cpp}
- src/rotator/RotatorFactory.{h,cpp}
- src/services/RotatorService.{h,cpp}
- tests/test_rotator_service.cpp
- tools/rotator_test_manual.cpp
- docs/ROTATOR_SYSTEM.md

🤖 Generated with Claude Code
Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```
