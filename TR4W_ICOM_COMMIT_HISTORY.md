# TR4W IcomNetwork Branch — Icom & Radio Factory Commit History

> Extracted from the `IcomNetwork` branch of [github.com/n4af/TR4W](https://github.com/n4af/TR4W)  
> Purpose: Reference for porting battle-tested fixes to TR4QT and QK4  
> Generated: 2026-04-20

---

## IC-7760 Specific

### 2024-12-26 — `3cc3fcaa` — Added IC-7760
> "Added Icom IC-7760 and also fixed uSuperCheckPartialFileUpload.dcu version again."

First introduction of IC-7760 support. Key discoveries documented elsewhere:
- Controller address **must be `$E1`** (not the standard `$E0`)
- VFO B commands `$25`/`$26` require sub-command byte `$01`
- VFO B CI-V frame parsing requires override of `ProcessCIVFrame`

---

## Icom Network Protocol — Major Commits

### 2026-03-16 — `9d735135` — Full Icom Network Protocol Support (merged to IcomNetwork)
> "Add Icom network protocol support and Radio/CAT dialog fixes"

- Full CI-V over Ethernet/WiFi for: IC-705, IC-7100, IC-7300, IC-7300MK2, IC-7600, IC-7610, IC-7760, IC-7850, IC-7905, IC-9700
- **IC-705 ToggleBand fix**: skip `rb4m` (70 MHz) — the radio rejects that frequency and leaves the display out of sync
- Radio/CAT dialog: dynamically shifts controls when USERNAME/PASSWORD fields are inserted for TCP/IP Icom mode
- New units: `uIcomCIV`, `uRadioBand`, `uRadioIcom7100`
- CI-V BCD unit test suite (56 tests)
- Docs added: Icom network protocol guide, new radio implementation guide

### 2026-02-26 — `164eec4c` — Icom UDP Protocol Stack Implementation
> "Add Icom network protocol support for controlling radios over Ethernet/WiFi"

Core UDP protocol implementation:
- `uIcomNetworkTypes` — Protocol packet types and constants
- `uIcomNetworkTransport` — Full connection lifecycle: handshake, auth, CI-V framing, keepalives, retransmit
- `uIcomNetworkDiscovery` — Broadcast-based radio discovery
- New radio models: IC-705, IC-7300MK2, IC-7600, IC-7760, IC-7850, IC-905
- **Key fix in `uRadioIcomBase`**: Fixed `RadioAddress` property shadowing (CI-V Byte vs IP string) — this was a subtle but fatal bug where the CI-V address byte was being overwritten by the IP address string
- Verified compiles clean with Delphi 7

### 2026-01-08 — `4d0528fc` — Add IC-7300, IC-7610, IC-9700 with CI-V Protocol
> "Add Icom radio support (IC-7300, IC-7610, IC-9700) with CI-V protocol"

- `TIcomRadioBase` with full CI-V protocol implementation
- IC-7300 (`$94`), IC-7610 (`$98`), IC-9700 (`$A2`) radio classes
- Factory pattern integration for network and serial

---

## Radio-Specific Timing & Polling Fixes

### 2026-01-07 — `d9dca9d6` — Fix IC-9700 Startup Delay
> "Fix IC-9700 startup delay - send immediate poll on connection"

**Problem:** IC-9700 frequency display took **17 seconds** to appear on startup.  
**Root cause:** Icom radios don't send data until queried (unlike K4 with AI5 auto-info). Initial poll only happened after a delay in the polling cycle.  
**Fix:** Send `PollRadioState()` immediately on `wasConnected` transition.  
**Result:** Startup reduced from 17s to ~1.6s at 38400 baud; essentially immediate at 115200.  
**Tested:** IC-9700 on COM7 at both 38400 and 115200 baud.

> **Back-port status for TR4QT:** Verify `IcomRadio.connect()` calls an immediate poll or seeds state on connection.

### 2026-01-06 — `8e2220d9` — Fix Serial Radio Reconnection After Power Cycle
> "Fix serial radio reconnection after power cycle"

**Problem:** Serial port was closed/reopened every second during reconnection, creating a race condition. Icom radios don't send unsolicited messages on power-up.  
**Fix 1:** Keep serial port open during reconnection — skip close/reopen if port already open with reading thread.  
**Fix 2:** Send poll queries even when radio is marked disconnected — wakes up Icom radios that wait for queries before responding.  
**Result:** IC-9700 reconnects ~16 seconds after power cycle automatically.

> **Back-port status for TR4QT:** Verify reconnection logic doesn't tear down the port on each retry.

---

## Factory Pattern & Architecture

### 2026-02-23 — `f40ff310` — Fix Reset Radio Ports
> "Fix Reset Radio Ports: clean thread teardown and K4 reconnect state"

- Serial Disconnect now terminates reading thread **before** `Free`, releasing COM port so new radio object can open it without error 5
- `PollingStopRequested` flag stops polling thread cleanly on reset; `WaitForSingleObject` ensures exit before old radio freed
- K4 `firstProcessMessage` moved from unit-global to instance variable — each new radio object calls `Initialize` on first message
- `ResetRadioPorts` simplified to call `CheckAndInitializePorts`

### 2026-01-05 — `516e3a15` — Add Serial Port Support to K4 Factory Pattern
Full serial port support through the factory:
- `uSerialPort.pas` — Win32 serial wrapper (CreateFile/ReadFile/WriteFile, non-blocking 10ms timeouts)
- `uNetRadioBase` extended for both serial and network connections
- Factory `CreateRadioSerial()` passes baud rate, data bits, parity, stop bits

### 2026-01-05 — `6f89fd80` — Refactor Radio Classes, Eliminate K4-Suffixed Methods
Major architectural cleanup:
- Added default VFO parameters (`= nrVFOA`) to base class abstract methods — backward compatible
- Eliminated `SetModeK4`, `SetBandK4` etc. naming hacks
- `uRadioInterfaces.pas` created with capability-based interfaces (IRadioBasic, IRadioFrequency, IRadioMode, IRadioDualVFO, IRadioRIT, IRadioXIT, IRadioSplit, IRadioCW, IRadioFilter, IRadioBand) — not yet wired in but available

### 2026-01-05 — `46ba55e6` — Fix Abstract Error in K4Radio
> "Fix Abstract Error in K4Radio by implementing missing base class methods"

**Root cause:** K4Radio methods had different signatures than `TNetRadioBase` abstract methods (e.g., `SetMode` with vs without VFO parameter), causing `EAbstractError` on polymorphic calls.  
**Fix:** Private K4-specific methods + public overrides matching base class signatures.

### 2025-12-29 — `ff069fd7` — Fix Radio Crash on Disconnect + Auto Reconnection
> "Fix radio crash on disconnect and implement automatic reconnection"

- Prevent crash by removing blocking locks from I/O operations
- `Disconnecting` state flag to coordinate threads during disconnect
- Polling thread stays alive during disconnection, resumes on reconnect
- **Auth timing fix:** `GetIsConnected` must return `True` during `WaitingForLogin` to prevent polling thread from hammering `Connect()` every second and aborting the handshake
- Send ID command after reconnection to wake radio communication

### 2025-12-29 — `4e94f924` — Exponential Backoff Reconnection
- Initial delay: 1s, max: 30s, multiplier: 2x
- `wasConnected` flag, `consecutiveFailures` counter
- Thread never exits on transient failures
- Graceful shutdown even during reconnection

### 2025-12-29 — `d119e5d0` — Initial Radio Factory Pattern
First factory implementation: `uRadioFactory.pas`, `uRadioManager.pas`, `TestRadioFactory.pas`

---

## Key Radio-Specific Values (Quick Reference)

| Radio | CI-V Address | Controller Address | Transceive Menu Bytes | Notes |
|---|---|---|---|---|
| IC-7300 | `$94` | `$E0` | `$01 $50` (default) | Standard |
| IC-7610 | `$98` | `$E0` | `$01 $50` (default) | Standard |
| IC-7760 | `$B2` | **`$E1`** | `$01 $50` (default) | **Non-standard controller addr; VFO B needs `$01` sub-cmd** |
| IC-9700 | `$A2` | `$E0` | **`$01 $27`** | Different transceive menu |
| IC-705  | `$A4` | `$E0` | **`$01 $31`** | No 4m band — skip `rb4m` in ToggleBand |
| IC-7850 | (TBD) | `$E0` | `$01 $50` | |
| IC-905  | (TBD) | `$E0` | `$01 $50` | |
| IC-7100 | (TBD) | `$E0` | (TBD) | |

---

## Checklist: TR4QT Back-Port Verification

- [ ] IC-7760 controller address is `$E1` (not `$E0`)
- [ ] IC-7760 VFO B `QueryVFOBFrequency` sends `$25 $01`
- [ ] IC-7760 `ProcessCIVFrame` handles extended VFO B response format
- [ ] IC-9700 `FTransceiveMenuBytes` = `$01 $27`
- [ ] IC-705 `FTransceiveMenuBytes` = `$01 $31`
- [ ] IC-705 `ToggleBand` skips `rb4m` (70 MHz)
- [ ] `GetIsConnected` returns `True` during `WaitingForLogin` state
- [ ] Initial freq/mode queries delayed 250ms after connect (`FInitialQueryPending`)
- [ ] `PollRadioState()` called immediately on `wasConnected` transition (not just at first poll interval)
- [ ] Serial reconnection keeps port open during retry (no close/reopen cycle)
- [ ] `PollingStopRequested` flag + `WaitForSingleObject` before freeing radio object on reset
- [ ] CW speed stale echo suppression (`FLastSetCWSpeedTick`)
- [ ] `RadioAddress` property doesn't shadow CI-V byte with IP string
- [ ] Poll drops to 1000ms in network mode (transceive push handles freq/mode)
