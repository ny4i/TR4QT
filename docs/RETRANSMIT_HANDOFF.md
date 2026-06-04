# TR4QT Retransmit/Sequence Tracking — Handoff Document

## Problem

The Icom network protocol requires both sides to track received sequence numbers, detect gaps, and request retransmission of missing packets. Without this, the remote side's send buffer eventually fills and it stops transmitting.

TR4QT has the data structures and `sendTrackedPacket` working, but the **receive-side tracking and retransmit request logic is entirely stubbed out**. The TX buffer is populated but never read. The RX buffers and missing maps are declared but never written to.

This document describes what TR4W implemented (based on the wfview model) that TR4QT should also implement.

## Current State of TR4QT (as of Feb 2026)

### What Works

| Component | File:Line | Status |
|-----------|-----------|--------|
| `sendTrackedPacket` | `icomnetwork.cpp:498-546` | Fully implemented. Patches seq bytes [6..7], stores in txBuf, increments seq, resets idle timer |
| TX buffers (`m_controlTxBuf`, `m_civTxBuf`) | Populated by sendTrackedPacket | Populated on every tracked send, cleared in cleanup. **But never read for retransmit** |
| TX mutexes (`m_controlTxMutex`, `m_civTxMutex`) | Used in sendTrackedPacket | Working |
| Idle timer | `icomnetwork.cpp:323-326` | Sends tracked idle packet every 100ms (IDLE_PERIOD). Reset on every sendTrackedPacket call |
| Watchdog timer | `icomnetwork.cpp:842-859` | Re-sends CivOpen if no CI-V data for 2 seconds |
| `sendCivOpenClose` | `icomnetwork.cpp:433-462` | **Tracked** (goes through sendTrackedPacket) |
| `sendCivCommand` | `icomnetwork.cpp:464-496` | **Tracked** (goes through sendTrackedPacket) |

### What's Stubbed/Missing

| Component | File:Line | Status |
|-----------|-----------|--------|
| `checkRetransmit()` | `icomnetwork.cpp:836-840` | **Empty stub** — timer fires every 100ms doing nothing |
| `handleRetransmitRequest()` | `icomnetwork.h:162` | **Declared only** — no implementation in .cpp, never called |
| `sendRetransmitRequest()` | — | **Does not exist** — not declared, not defined |
| RX buffers (`m_controlRxBuf`, `m_civRxBuf`) | `icomnetwork.h:213-214` | `QMap<quint16, QTime>` — **declared, cleared in cleanup, never written to** |
| RX mutexes (`m_controlRxMutex`, `m_civRxMutex`) | `icomnetwork.h:221-222` | **Declared, never locked/unlocked** |
| Missing maps (`m_controlMissing`, `m_civMissing`) | `icomnetwork.h:215-216` | `QMap<quint16, int>` — **declared, never written to, never cleared** |
| Type 0x01 handling | `icomnetwork.cpp:807` | CIV socket: treated same as 0x00 (scan for FE FE). Control socket: silently dropped |

### Key Constants (icompackets.h)

```cpp
#define IDLE_PERIOD 100        // line 27
#define RETRANSMIT_PERIOD 100  // line 29
#define WATCHDOG_PERIOD 500    // line 30
#define BUFSIZE 500            // line 31
#define MAX_MISSING 50         // line 32
```

## What Needs to Be Implemented

### 1. RX Sequence Tracking

When a type=0x00 data packet arrives on either socket with seq != 0 and isn't a ping (len != 0x15), track its sequence number:

```
trackReceivedSeq(seq, socket):
  rxBuf = socket's rxBuf map
  missList = socket's missing map

  if rxBuf is empty (first packet):
    insert seq into rxBuf
    set lastSeq = seq
    return

  if gap > MAX_MISSING:
    flush rxBuf and missList
    restart from this seq
    return

  if seq > lastSeq + 1:
    for each S from lastSeq+1 to seq-1:
      if S not in rxBuf:
        add S to missList with retryCount=0

  remove seq from missList (if present — late arrival)
  insert seq into rxBuf
  update lastSeq = max(lastSeq, seq)
  evict oldest from rxBuf if size > BUFSIZE
```

Where to hook this in: Inside `processCivPacket()` and `processControlPacket()`, after the existing type/size checks, before dispatching to specific handlers.

### 2. Implement `checkRetransmit()` (replaces empty stub)

Called every 100ms by `m_retransmitTimer`. For each socket (control and CIV):

```
checkRetransmit():
  for each socket in [control, civ]:
    missList = socket's missing map
    if missList is empty: continue
    if missList.size() > MAX_MISSING: flush everything, continue

    remove entries with retryCount >= 4 (give up)
    if missList is empty: continue

    increment retryCount for each remaining entry

    if missList.size() == 1:
      send 16-byte control packet: type=0x01, seq=missing_seq, sentId=myId, rcvdId=remoteId
    else:
      send 16-byte header (type=0x01) + list of 2-byte LE seq numbers

    send directly via socket->writeDatagram (NOT through sendTrackedPacket)
```

### 3. Handle Incoming Retransmit Requests (type=0x01)

When the radio sends type=0x01 to us, it's asking us to resend packets it missed.

```
handleRetransmitRequest(data, socket):
  txBuf = socket's txBuf

  if len == 0x10 (16 bytes):
    // Single retransmit: seq field = the seq they want
    reqSeq = bytes[6..7] as LE uint16
    if reqSeq in txBuf:
      resend txBuf[reqSeq].data directly (untracked)

  else if len > 0x10:
    // Multi retransmit: seq list follows 16-byte header
    for offset from 0x10 to end, step 2:
      reqSeq = bytes[offset..offset+1] as LE uint16
      if reqSeq in txBuf:
        resend txBuf[reqSeq].data directly (untracked)
```

Where to hook this in:
- In `processCivPacket()` at line 807: change `if (dp->type == 0x00 || dp->type == 0x01)` to check type 0x01 separately and call `handleRetransmitRequest` instead of scanning for CI-V data
- In `processControlPacket()`: add handling for type 0x01 (currently silently dropped)

### 4. Fix cleanup() to Clear Missing Maps

At `icomnetwork.cpp` ~line 942, add:
```cpp
m_controlMissing.clear();
m_civMissing.clear();
```

## Protocol Detail: Type 0x01 Packet Format

Type 0x01 serves double duty in the Icom protocol:
- **During auth handshake**: It's `ICOM_PKT_AUTH` — login response, capabilities, etc.
- **When connected**: It's a retransmit request

The distinction is contextual. During handshake states, treat 0x01 as auth. When connected, treat 0x01 as retransmit.

### Single retransmit request (16 bytes)
```
Offset  Size  Field
0x00    4     Len = 0x10
0x04    2     PktType = 0x0001
0x06    2     Seq = the missing sequence number (LE)
0x08    4     SentID = sender's ID
0x0C    4     RcvdID = receiver's ID
```

### Multi retransmit request (16 + N*2 bytes)
```
Offset  Size  Field
0x00    4     Len = 0x10 + N*2
0x04    2     PktType = 0x0001
0x06    2     Seq = 0
0x08    4     SentID
0x0C    4     RcvdID
0x10    2     Missing seq #1 (LE uint16)
0x12    2     Missing seq #2 (LE uint16)
...
```

## TR4W Reference Implementation

The complete working implementation is in:
- `tr4w/src/uIcomNetworkTransport.pas` — `SendTrackedPacket`, `TrackReceivedSeq`, `SendRetransmitRequest`, `HandleRetransmitRequest` and all buffer management methods
- `tr4w/src/uIcomNetworkTypes.pas` — `TSeqBufEntry`, `TRxSeqEntry`, `TMissingEntry` record types

Key design decisions in TR4W:
- Uses `TList` with pointer records (Delphi 7 has no generics) — TR4QT already has `QMap` which is cleaner
- Per-socket buffers and tracking (control + CIV, independently)
- RX tracking needs per-socket "empty" flag and "lastSeq" high-water mark
- Retransmit requests sent untracked (direct writeDatagram, not through sendTrackedPacket)
- Re-sent packets from TX buffer also sent untracked
- Idle timer is killed and restarted in sendTrackedPacket (KillTimer + SetTimer in TR4W, `m_idleTimer->start()` in TR4QT — already done)

## Verification Plan

1. **Compile and run**: Connect to an Icom radio over network
2. **Stability test**: Let it run for 5+ minutes. Previously CI-V data would stop after ~2 minutes (290 packets) due to the radio's send buffer filling
3. **Check logs**: Look for retransmit request activity (both sent and received). Some retransmits are normal on lossy links
4. **Watchdog test**: Briefly disconnect/reconnect network — should recover via CivOpen retry
5. **Edge case**: Rapid polling (lower the interval) to stress-test the retransmit system
