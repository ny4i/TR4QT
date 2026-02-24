# Kenwood TS-890 Direct Connection Flow

This document traces the complete end-to-end flow for connecting to a Kenwood TS-890S radio over the network using the Kenwood Direct interface.

## 1. User Configures the Radio

User opens **Preferences → My Radios → Add Radio** (`RadioEditDialog`) and fills in:

| Field | Source | UI Widget | Default |
|-------|--------|-----------|---------|
| Radio Name | User types | `m_radioNameEdit` | — |
| Connection | Clicks **Network** | `m_networkRadio` | — |
| Interface | Clicks **Kenwood Direct** | `m_kenwoodDirectRadio` | — |
| Model | Auto-populated | `m_radioModelCombo` | "Kenwood TS-890S" (model ID 241) |
| IP Address | User types | `m_ipAddressEdit` | — |
| Port | Auto-filled | `m_portSpin` | 60000 |
| Admin ID | From radio's LAN settings menu | `m_kenwoodAdminIdEdit` | — |
| Admin Password | From radio's LAN settings menu | `m_kenwoodAdminPasswordEdit` | — |

These get packed into a `RadioConfig` struct via `buildRadioConfigFromUI()` (`RadioEditDialog.cpp`):

```
config.port                  = "192.168.1.100:60000"
config.hamlibModelId         = 241           (TS-890S)
config.radioType             = 3             (KENWOOD_DIRECT)
config.kenwoodAdminId        = "admin"
config.kenwoodAdminPassword  = "secret"
```

The Admin ID and Password come from the TS-890S radio itself — they are configured in the radio's **Menu → LAN** settings screen.

## 2. Credentials Saved to Disk

When the user clicks **Save**, `AppSettings::saveRadioProfiles()` persists the profile:

| Data | Storage Location | Key |
|------|-----------------|-----|
| IP:Port | QSettings (plist on macOS) | `RadioProfiles/Profiles/{n}/port` |
| Model ID | QSettings | `RadioProfiles/Profiles/{n}/hamlibModelId` |
| Radio Type | QSettings | `RadioProfiles/Profiles/{n}/radioType` |
| Admin ID | QSettings | `RadioProfiles/Profiles/{n}/kenwoodAdminId` |
| Admin Password | OS Keychain (CredentialStore) | `TR4QT:KenwoodRadio/{profileName}` |

The password is stored in the OS-native credential store:
- **macOS**: Keychain
- **Windows**: Credential Manager
- **Linux**: Secret Service (libsecret)

On next launch, `AppSettings::loadRadioProfiles()` reads the admin ID from QSettings and retrieves the password from the credential store.

## 3. Radio Instance Created

When the user connects (or on auto-connect at startup), the call chain is:

```
RadioManager::connectRadio(radioIndex, config)
  → RadioController::connectToRadio(config)
    → RadioController::recreateRadio(config.radioType, config)
      → RadioFactory::createRadio(KENWOOD_DIRECT, config)
        → hamlibModelId == 241  →  return new TS890Radio()
```

The `TS890Radio` instance (which inherits from `KenwoodRadio`) is moved to a **worker thread** via `moveToThread(&m_workerThread)`. All I/O happens on the worker thread — the main UI thread never blocks.

**Class hierarchy:**
```
RadioInterface (abstract)
  └── KenwoodRadio (shared Kenwood protocol: TCP, auth, command parsing)
        └── TS890Radio (TS-890S specifics: mode mapping, CW range, init sequence)
```

## 4. TCP Connection

On the worker thread, `KenwoodRadio::connect(config)` runs:

1. Parse `"192.168.1.100:60000"` into `m_host` and `m_port`
2. Store `m_adminId` and `m_adminPassword` from config
3. Set `m_isLanConnection = true` (because adminId is not empty)
4. Create `QTcpSocket` and wire signals: `connected`, `disconnected`, `errorOccurred`, `readyRead`
5. Call `m_socket->connectToHost("192.168.1.100", 60000)` — async, non-blocking

The method returns immediately. Connection completion is handled by signals.

## 5. LAN Authentication Handshake

Once the TCP socket emits `connected()`, the 3-step Kenwood LAN authentication begins:

```
              TR4QT                          TS-890S
                |                               |
  TCP connect   |-------- TCP SYN/ACK -------->|  Port 60000
                |                               |
  Step 1        |-------- ##CN; -------------->|  "I want to connect"
                |<------- ##CN1; --------------|  "OK, send credentials"
                |                               |
  Step 2        |-- ##ID00509adminsecret; ---->|  "Here are my creds"
                |   (05=id len, 09=pw len)      |
                |<------- ##ID1; --------------|  "Authenticated!"
                |                               |
  Step 3        |  onConnectedInitialize()      |
                |-------- AI2; --------------->|  Enable auto-info push
                |-------- FA; ---------------->|  Query VFO A freq
                |-------- FB; ---------------->|  Query VFO B freq
                |-------- OM0; --------------->|  Query VFO A mode
                |-------- OM1; --------------->|  Query VFO B mode
                |-------- KS; ---------------->|  Query CW speed
                |-------- TB; ---------------->|  Query split status
                |-------- FT; ---------------->|  Query TX VFO
                |-------- RT; ---------------->|  Query RIT
                |-------- XT; ---------------->|  Query XIT
                |-------- ID; ---------------->|  Verify radio identity
                |<------- ID024; --------------|  "I'm a TS-890S"
                |                               |
                |  m_state.radioModel = "TS-890S"
                |  m_state.isValid = true
                |  emit connectionStatusChanged(true)
                |                               |
```

### Authentication State Machine

`KenwoodRadio` tracks auth progress via `m_authState`:

| State | Trigger | Action |
|-------|---------|--------|
| `None` | `connect()` called | — |
| `WaitingForCN` | TCP `connected()` signal | Send `##CN;` |
| `WaitingForID` | Receive `##CN1;` | Send `##ID0{idLen}{pwLen}{id}{pw};` |
| `Authenticated` | Receive `##ID1;` | Set `m_authenticated = true`, call `onConnectedInitialize()` |

### Credential Format

The `##ID` command format is:
```
##ID0{idLength:2digits}{pwLength:2digits}{adminId}{adminPassword};
```

Example with admin ID `"admin"` (5 chars) and password `"secret123"` (9 chars):
```
##ID00509adminsecret123;
```

### Auth Failure

If the radio responds with anything other than `##ID1;`, the connection emits `errorOccurred("LAN authentication failed: invalid credentials")` and no commands are processed.

All messages received before authentication completes are silently ignored.

## 6. Post-Authentication Initialization

`TS890Radio::onConnectedInitialize()` sends the startup command sequence:

| Command | Purpose |
|---------|---------|
| `AI2;` | Enable Auto-Information mode 2 (radio pushes freq/mode/split changes) |
| `FA;` | Query VFO A frequency (11-digit Hz response) |
| `FB;` | Query VFO B frequency |
| `OM0;` | Query VFO A operating mode (hex-encoded) |
| `OM1;` | Query VFO B operating mode |
| `KS;` | Query CW keyer speed (3-digit WPM) |
| `TB;` | Query split on/off |
| `FT;` | Query TX VFO select (0=A, 1=B) |
| `RT;` | Query RIT on/off |
| `XT;` | Query XIT on/off |
| `ID;` | Query radio identification (expects `ID024;` for TS-890S) |

After sending queries, the code sets `m_state.radioModel = "TS-890S"`, `m_state.isValid = true`, and emits `connectionStatusChanged(true)`.

## 7. Signal Propagation to UI

The connected signal propagates from the worker thread to the main thread:

```
TS890Radio::connectionStatusChanged(true)        [worker thread]
  --> RadioController::connectionStatusChanged(true)  [cross-thread Qt::QueuedConnection]
    --> RadioManager::connectionStatusChanged(true)    [main thread]
      --> MainWindow updates UI: frequency, mode, connection indicator
```

Qt handles the cross-thread signal delivery automatically.

## 8. Where Each Piece Comes From

| Data | User enters in | Stored in | Read by |
|------|---------------|-----------|---------|
| IP Address | `m_ipAddressEdit` | QSettings `port` as `"IP:port"` | `KenwoodRadio::connect()` splits on `:` |
| Port | `m_portSpin` (default 60000) | QSettings `port` as `"IP:port"` | Same split |
| Admin ID | `m_kenwoodAdminIdEdit` | QSettings `kenwoodAdminId` | `config.kenwoodAdminId` → `m_adminId` |
| Password | `m_kenwoodAdminPasswordEdit` | OS Keychain `TR4QT:KenwoodRadio/{name}` | `config.kenwoodAdminPassword` → `m_adminPassword` |
| Radio model | Auto-selected when Kenwood Direct chosen | QSettings `hamlibModelId = 241` | `RadioFactory` routes to `TS890Radio` |

## 9. Auth Skipped When No Credentials

If `kenwoodAdminId` is empty (e.g., simulator testing or future serial support), `m_isLanConnection` is set to `false` and the code skips the `##CN`/`##ID` handshake entirely — jumping straight to `onConnectedInitialize()` after TCP connects.

This is how the simulator integration tests work: they connect via a TCP-to-PTY bridge without authentication.

## Key Files

| File | Role |
|------|------|
| `src/ui/dialogs/RadioEditDialog.cpp` | UI: IP, port, credentials input |
| `src/utils/AppSettings.cpp` | Save/load radio profiles + credentials |
| `src/utils/CredentialStore.h` | OS-native secure password storage |
| `src/radio/RadioFactory.cpp` | Routes `KENWOOD_DIRECT` + model 241 → `TS890Radio` |
| `src/controllers/RadioController.cpp` | Worker thread management, queues `connect()` |
| `src/radio/KenwoodRadio.cpp` | TCP socket, auth handshake, command protocol |
| `src/radio/TS890Radio.cpp` | Model-specific init sequence and mode mapping |
| `src/radio/RadioInterface.h` | `RadioConfig` struct with `kenwoodAdminId`/`kenwoodAdminPassword` |
