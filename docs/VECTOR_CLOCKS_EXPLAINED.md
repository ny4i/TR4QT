# Vector Clocks Explained

**Purpose:** Track causality in distributed systems to determine "happens-before" relationships between events.

---

## The Problem: Detecting Causality in Distributed Systems

### Scenario: Two Stations Log QSOs Concurrently

```
Time    Station A                    Station B
────────────────────────────────────────────────────────
12:00   Log QSO with W1AW            (idle)

12:01   (idle)                       Log QSO with W1AW

12:02   Receive B's QSO              Receive A's QSO
```

**Question:** Which QSO happened first?

**Problem:** We can't tell from timestamps alone!
- Station A's clock might be fast
- Station B's clock might be slow
- Network delay is variable

**We need:** A way to track **causality** (which event influenced which) without relying on synchronized clocks.

---

## What Vector Clocks Are

A **vector clock** is a data structure that assigns a logical timestamp to each event in a distributed system.

### Structure

```cpp
struct VectorClock {
    QMap<QString, quint64> clock;  // stationId → logical counter
};
```

**Example:**
```
Station A's vector clock: {"A": 5, "B": 3, "C": 2}

Meaning:
  - Station A has seen 5 events from itself
  - Station A has seen 3 events from Station B
  - Station A has seen 2 events from Station C
```

---

## How Vector Clocks Work

### Rule 1: Initialize

Each station starts with a vector clock of all zeros:

```
Station A: {"A": 0, "B": 0, "C": 0}
Station B: {"A": 0, "B": 0, "C": 0}
Station C: {"A": 0, "B": 0, "C": 0}
```

### Rule 2: Increment on Local Event

When a station performs a local event (logs a QSO), it increments **its own** counter:

```cpp
void logLocalQSO() {
    vectorClock["A"]++;  // Increment own counter

    // Now: {"A": 1, "B": 0, "C": 0}
}
```

### Rule 3: Merge on Receive

When a station receives a message from another station, it:
1. Takes the **maximum** of each counter from both clocks
2. Increments its own counter

```cpp
void receiveMessage(const VectorClock& remoteClock) {
    // Merge: take max of each counter
    for (auto it = remoteClock.clock.begin(); it != remoteClock.clock.end(); ++it) {
        QString stationId = it.key();
        quint64 remoteValue = it.value();

        vectorClock[stationId] = qMax(vectorClock[stationId], remoteValue);
    }

    // Increment own counter
    vectorClock[myStationId]++;
}
```

---

## Concrete Example: Contest Logging

### Initial State

```
Station A: {"A": 0, "B": 0, "C": 0}
Station B: {"A": 0, "B": 0, "C": 0}
Station C: {"A": 0, "B": 0, "C": 0}
```

### Event 1: Station A logs W1AW

```
Station A logs QSO:
  - Increment A's counter
  - Vector clock: {"A": 1, "B": 0, "C": 0}

Station A broadcasts: QSO(W1AW, VC={"A": 1, "B": 0, "C": 0})
```

### Event 2: Station B receives A's QSO

```
Station B receives A's message:
  - Merge clocks: max({"A": 0, "B": 0, "C": 0}, {"A": 1, "B": 0, "C": 0})
                = {"A": 1, "B": 0, "C": 0}
  - Increment own: {"A": 1, "B": 1, "C": 0}
```

### Event 3: Station B logs DL1ABC

```
Station B logs QSO:
  - Increment B's counter
  - Vector clock: {"A": 1, "B": 2, "C": 0}

Station B broadcasts: QSO(DL1ABC, VC={"A": 1, "B": 2, "C": 0})
```

### Event 4: Station A receives B's QSO

```
Station A receives B's message:
  - Merge clocks: max({"A": 1, "B": 0, "C": 0}, {"A": 1, "B": 2, "C": 0})
                = {"A": 1, "B": 2, "C": 0}
  - Increment own: {"A": 2, "B": 2, "C": 0}
```

### Event 5: Station C logs UA1ABC

```
Station C logs QSO (hasn't heard from anyone yet):
  - Increment C's counter
  - Vector clock: {"A": 0, "B": 0, "C": 1}

Station C broadcasts: QSO(UA1ABC, VC={"A": 0, "B": 0, "C": 1})
```

### Event 6: Station A receives C's QSO

```
Station A receives C's message:
  - Merge clocks: max({"A": 2, "B": 2, "C": 0}, {"A": 0, "B": 0, "C": 1})
                = {"A": 2, "B": 2, "C": 1}
  - Increment own: {"A": 3, "B": 2, "C": 1}
```

---

## Timeline Visualization

```
Time    Event                           Station A VC              Station B VC              Station C VC
─────────────────────────────────────────────────────────────────────────────────────────────────────────
T0      (Initial)                       {"A":0,"B":0,"C":0}      {"A":0,"B":0,"C":0}      {"A":0,"B":0,"C":0}

T1      A logs W1AW                     {"A":1,"B":0,"C":0}      {"A":0,"B":0,"C":0}      {"A":0,"B":0,"C":0}
        └─> broadcasts VC

T2      B receives A's QSO              {"A":1,"B":0,"C":0}      {"A":1,"B":1,"C":0}      {"A":0,"B":0,"C":0}
        └─> merges and increments                                 ▲ merged + incremented

T3      B logs DL1ABC                   {"A":1,"B":0,"C":0}      {"A":1,"B":2,"C":0}      {"A":0,"B":0,"C":0}
        └─> broadcasts VC                                          ▲ incremented

T4      A receives B's QSO              {"A":2,"B":2,"C":0}      {"A":1,"B":2,"C":0}      {"A":0,"B":0,"C":0}
        └─> merges and increments        ▲ merged + incremented

T5      C logs UA1ABC                   {"A":2,"B":2,"C":0}      {"A":1,"B":2,"C":0}      {"A":0,"B":0,"C":1}
        └─> broadcasts VC                                                                   ▲ incremented

T6      A receives C's QSO              {"A":3,"B":2,"C":1}      {"A":1,"B":2,"C":0}      {"A":0,"B":0,"C":1}
        └─> merges and increments        ▲ merged + incremented
```

---

## Determining Causality (Happens-Before Relationship)

### Definition: VC1 "Happens Before" VC2

Vector clock VC1 happens before VC2 if:
- **All** counters in VC1 are ≤ corresponding counters in VC2
- **At least one** counter in VC1 is < corresponding counter in VC2

**Mathematical notation:** VC1 ≤ VC2

```cpp
bool VectorClock::happensBefore(const VectorClock& other) const {
    bool atLeastOneSmaller = false;

    for (auto it = clock.begin(); it != clock.end(); ++it) {
        QString stationId = it.key();
        quint64 myValue = it.value();
        quint64 otherValue = other.get(stationId);

        if (myValue > otherValue) {
            return false;  // I have a higher counter → didn't happen before
        }

        if (myValue < otherValue) {
            atLeastOneSmaller = true;
        }
    }

    return atLeastOneSmaller;
}
```

### Examples

```cpp
// Example 1: Clear causality
VectorClock vc1{"A": 1, "B": 0, "C": 0};
VectorClock vc2{"A": 1, "B": 2, "C": 0};

vc1.happensBefore(vc2) → true   // vc1 happened before vc2
vc2.happensBefore(vc1) → false  // vc2 did not happen before vc1
```

**Interpretation:** Event with vc1 **caused** event with vc2 (or vc2 happened after vc1)

```cpp
// Example 2: Concurrent events
VectorClock vc1{"A": 1, "B": 0, "C": 0};
VectorClock vc2{"A": 0, "B": 1, "C": 0};

vc1.happensBefore(vc2) → false  // vc1 has A=1 > vc2's A=0
vc2.happensBefore(vc1) → false  // vc2 has B=1 > vc1's B=0
```

**Interpretation:** Events are **concurrent** (neither caused the other)

```cpp
// Example 3: Transitivity
VectorClock vc1{"A": 1, "B": 0, "C": 0};
VectorClock vc2{"A": 2, "B": 1, "C": 0};
VectorClock vc3{"A": 3, "B": 2, "C": 1};

vc1.happensBefore(vc2) → true
vc2.happensBefore(vc3) → true
vc1.happensBefore(vc3) → true   // Transitive!
```

---

## Use Case: Conflict Resolution

### Scenario: Two Stations Edit Same QSO

```
Station A                          Station B
────────────────────────────────────────────────────────
Log QSO: W1AW, RST=599            (receives QSO)
VC: {"A": 1, "B": 0}              VC: {"A": 1, "B": 1}

(network partition)               (network partition)

Edit QSO: W1AW, RST=579           Edit QSO: W1AW, RST=589
VC: {"A": 2, "B": 0}              VC: {"A": 1, "B": 2}

(partition heals)                 (partition heals)

Now both have conflicting edits!
```

**Question:** Which edit should win?

### Conflict Detection

```cpp
bool isConflict(const VectorClock& vc1, const VectorClock& vc2) {
    return !vc1.happensBefore(vc2) && !vc2.happensBefore(vc1);
}

// In this case:
VectorClock vcA{"A": 2, "B": 0};
VectorClock vcB{"A": 1, "B": 2};

isConflict(vcA, vcB) → true  // Concurrent edits!
```

### Resolution Strategy

```cpp
QSORecord resolveConflict(const QSORecord& local, const QSORecord& remote) {
    if (local.vectorClock.happensBefore(remote.vectorClock)) {
        return remote;  // Remote happened after local → keep remote
    }

    if (remote.vectorClock.happensBefore(local.vectorClock)) {
        return local;   // Local happened after remote → keep local
    }

    // Concurrent edits → need deterministic tiebreaker
    // Option 1: Merge vector clocks, keep local data
    QSORecord merged = local;
    merged.vectorClock = local.vectorClock.merge(remote.vectorClock);
    return merged;

    // Option 2: Use station ID for deterministic choice
    if (local.id.stationId < remote.id.stationId) {
        return local;   // Alphabetically first station wins
    }
    return remote;
}
```

---

## Visual Example: Contest QSO Conflict

### Setup

```
12:00  Station A logs: W1AW, 599, VC={"A":1, "B":0, "C":0}
       Broadcasts to B and C

12:01  Station B receives, VC becomes {"A":1, "B":1, "C":0}
       Station C receives, VC becomes {"A":1, "B":0, "C":1}

12:02  Network partition! (B and C can't reach each other)

12:03  Station B edits: W1AW, 579 (bad copy), VC={"A":1, "B":2, "C":0}
       Station C edits: W1AW, 589 (also bad copy), VC={"A":1, "B":0, "C":2}

12:04  Partition heals! B and C sync...
```

### Conflict Resolution

```
Station B's QSO:
  callsign: W1AW
  rst: 579
  vectorClock: {"A": 1, "B": 2, "C": 0}

Station C's QSO:
  callsign: W1AW
  rst: 589
  vectorClock: {"A": 1, "B": 0, "C": 2}

Conflict check:
  {"A": 1, "B": 2, "C": 0} vs {"A": 1, "B": 0, "C": 2}

  B's VC happensBefore C's VC?
    - A: 1 <= 1 ✓
    - B: 2 <= 0 ✗  (B's counter is higher!)
    → false

  C's VC happensBefore B's VC?
    - A: 1 <= 1 ✓
    - B: 0 <= 2 ✓
    - C: 2 <= 0 ✗  (C's counter is higher!)
    → false

Result: CONCURRENT EDITS (conflict!)

Resolution:
  - Merge vector clocks: max({"A":1,"B":2,"C":0}, {"A":1,"B":0,"C":2})
                       = {"A":1,"B":2,"C":2}

  - Use tiebreaker: Station B < Station C (alphabetically)
  - Keep Station B's data: rst=579

  Final merged QSO:
    callsign: W1AW
    rst: 579
    vectorClock: {"A": 1, "B": 2, "C": 2}
```

---

## Properties of Vector Clocks

### 1. Causality Detection

If event E1 caused event E2, then VC(E1) < VC(E2)

```
Station A logs W1AW    →    Station B receives and logs DL1ABC
VC: {"A": 1, "B": 0}        VC: {"A": 1, "B": 2}

{"A": 1, "B": 0} < {"A": 1, "B": 2} → A's event caused B's event
```

### 2. Concurrent Event Detection

If VC(E1) ≮ VC(E2) and VC(E2) ≮ VC(E1), then E1 and E2 are concurrent

```
Station A logs W1AW              Station C logs UA1ABC
VC: {"A": 1, "B": 0, "C": 0}    VC: {"A": 0, "B": 0, "C": 1}

Neither happensBefore the other → concurrent events
```

### 3. Size

Vector clocks grow with the number of participants:
- 3 stations → 3 counters per clock
- 10 stations → 10 counters per clock
- **Space:** O(N) where N = number of stations

### 4. No Need for Synchronized Clocks

Vector clocks work even if:
- Station A's clock is in 2025
- Station B's clock is in 1970
- Station C has no clock at all

**Only logical ordering matters, not wall-clock time!**

---

## Comparison: Timestamps vs Vector Clocks

### Problem with Timestamps

```
Station A (clock is fast: 12:05)    Station B (clock is slow: 11:58)
────────────────────────────────────────────────────────────────────
Logs W1AW at 12:05                  (idle)

(network delay: 10 seconds)

                                    Logs W2XYZ at 11:59

                                    Receives W1AW (timestamp 12:05)
```

**Question:** Did W1AW happen before W2XYZ?

**Using timestamps:** 12:05 > 11:59 → W1AW happened after (WRONG!)
**Using vector clocks:** VC(W1AW)={"A":1,"B":0}, VC(W2XYZ)={"A":0,"B":1} → concurrent ✓

### Vector Clocks Fix This

```
Station A                           Station B
────────────────────────────────────────────────────────
Logs W1AW                          (idle)
VC: {"A": 1, "B": 0}

                                   Receives W1AW
                                   VC: {"A": 1, "B": 1}

                                   Logs W2XYZ
                                   VC: {"A": 1, "B": 2}
```

**Using vector clocks:** VC(W1AW)={"A":1,"B":0} < VC(W2XYZ)={"A":1,"B":2}
**Correct interpretation:** W1AW happened before W2XYZ ✓

---

## Implementation in TR4QT

### QSO Record with Vector Clock

```cpp
struct QSORecord {
    QSOId id;
    VectorClock vectorClock;  // ← Tracks causality
    quint32 version;           // ← Increments on edit

    QString callsign;
    QString rstSent;
    // ... other fields
};
```

### When Vector Clock is Updated

```cpp
// 1. Log new QSO
QSORecord logQSO(const QString& callsign) {
    QSORecord qso;
    qso.id = generateQSOId();
    qso.callsign = callsign;

    // Increment own counter
    qso.vectorClock.increment(myStationId);

    return qso;
}

// 2. Edit existing QSO
void editQSO(QSORecord& qso, const QString& newRst) {
    qso.rstSent = newRst;
    qso.version++;

    // Increment own counter
    qso.vectorClock.increment(myStationId);
}

// 3. Receive QSO from peer
void receiveQSO(const QSORecord& remoteQSO) {
    // Merge vector clocks
    m_vectorClock = m_vectorClock.merge(remoteQSO.vectorClock);

    // Increment own counter
    m_vectorClock.increment(myStationId);

    // Add to log
    m_log.add(remoteQSO);
}
```

### CRDT Merge with Vector Clocks

```cpp
QSORecord LogCRDT::resolveConflict(const QSORecord& local,
                                    const QSORecord& remote) const {
    // Check if one happened before the other
    if (local.vectorClock.happensBefore(remote.vectorClock)) {
        return remote;  // Remote is newer
    }

    if (remote.vectorClock.happensBefore(local.vectorClock)) {
        return local;   // Local is newer
    }

    // Concurrent edits → merge
    QSORecord merged = local;
    merged.vectorClock = local.vectorClock.merge(remote.vectorClock);
    merged.version = qMax(local.version, remote.version);

    return merged;
}
```

---

## Alternative: Lamport Clocks (Simpler but Less Powerful)

**Lamport Clock:** Single counter instead of vector

```cpp
struct LamportClock {
    quint64 counter;

    void increment() { counter++; }

    void merge(const LamportClock& other) {
        counter = qMax(counter, other.counter) + 1;
    }
};
```

**Comparison:**

| Feature | Lamport Clock | Vector Clock |
|---------|---------------|--------------|
| **Size** | O(1) | O(N) |
| **Can detect causality** | ❌ No | ✅ Yes |
| **Can detect concurrency** | ❌ No | ✅ Yes |
| **Simple** | ✅ Very | ⚠️ Moderate |

**Why TR4QT uses Vector Clocks:**
- Need to detect concurrent edits
- Need accurate conflict resolution
- Small number of stations (N ≤ 26) → size not a concern

---

## Summary

### What Vector Clocks Do

✅ **Track causality** without synchronized clocks
✅ **Detect concurrent events** for conflict resolution
✅ **Enable CRDTs** to automatically merge logs after partition

### How They Work

1. Each station maintains counters for all stations
2. Increment own counter on local events
3. Merge clocks (take max) when receiving messages
4. Compare clocks to determine happens-before relationships

### Why TR4QT Needs Them

- **Distributed logging** across multiple stations
- **Network partitions** possible (Field Day, WiFi issues)
- **Automatic conflict resolution** when partition heals
- **No central server** to arbitrate conflicts

### Key Insight

**Vector clocks provide a mathematical proof of causality** without requiring:
- Synchronized time
- Central coordinator
- Atomic broadcasts

This makes them perfect for TR4QT's peer-to-peer architecture!

---

**Document Version:** 1.0
**Last Updated:** December 28, 2025
