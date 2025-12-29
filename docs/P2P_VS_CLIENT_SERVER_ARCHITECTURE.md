# Peer-to-Peer vs Client-Server Architecture for TR4QT

**Decision Document**
**Date:** December 2025
**Status:** Recommendation

---

## Executive Summary

TR4W uses a **hub-and-spoke (client-server)** architecture where all clients connect to a central server. This creates a **single point of failure**: if the server crashes, the entire multi-operator network goes down.

Since TR4QT does **NOT require backward compatibility** with TR4W's protocol, we have the freedom to choose a fundamentally different architecture.

**Recommendation:** **Hybrid approach** - Peer-to-peer with optional designated coordinator for serial number allocation.

---

## Architecture Comparison

### Current TR4W: Hub-and-Spoke

```
                    ┌─────────┐
                    │ SERVER  │
                    │ (Master)│
                    └────┬────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
      ┌──▼──┐         ┌──▼──┐         ┌──▼──┐
      │  A  │         │  B  │         │  C  │
      │ N6TR│         │ W1AW│         │ K3LR│
      └─────┘         └─────┘         └─────┘
```

**Data Flow:**
1. Station A logs QSO → sends to SERVER
2. SERVER validates and broadcasts to B, C
3. B and C update their logs

**Failure Mode:**
- Server crash → entire network down
- Server disk failure → log data lost
- Network split → clients can't communicate

---

### Peer-to-Peer (Full Mesh)

```
      ┌─────┐
   ┌──┤  A  ├──┐
   │  │ N6TR│  │
   │  └─────┘  │
   │           │
┌──▼──┐     ┌──▼──┐
│  B  ├─────┤  C  │
│ W1AW│     │ K3LR│
└─────┘     └─────┘
```

**Data Flow:**
1. Station A logs QSO → broadcasts to B, C directly
2. B and C validate and apply independently
3. Conflicts resolved via consensus algorithm

**Failure Mode:**
- Any single station crash → others continue operating
- Network split → partitions operate independently, merge on reconnect
- No central point of failure

---

### Hybrid: P2P with Coordinator

```
      ┌─────┐
   ┌──┤  A  ├──┐        ┌──────────────┐
   │  │ N6TR│  │        │ Coordinator  │
   │  └─────┘  │        │ (Optional)   │
   │           │        │ Serial#/Time │
┌──▼──┐     ┌──▼──┐    └──────────────┘
│  B  ├─────┤  C  │
│ W1AW│     │ K3LR│
└─────┘     └─────┘
```

**Data Flow:**
1. QSOs broadcast peer-to-peer
2. Serial numbers allocated by coordinator (or fallback algorithm)
3. If coordinator fails → automatic fallback to distributed allocation

**Failure Mode:**
- Coordinator fails → P2P serial allocation takes over
- Any peer fails → others continue
- Most resilient option

---

## Detailed Analysis

### 1. Reliability & Fault Tolerance

#### Client-Server

**Single Point of Failure:**
- ✅ Simple failure detection (server down = obvious)
- ❌ Total network failure when server crashes
- ❌ Requires dedicated server hardware
- ❌ Server recovery time = network downtime

**Real-world impact:**
During a 48-hour contest:
- Server crash at hour 36 → 12 hours lost connectivity
- Must manually sync logs post-contest
- Operators unable to see real-time multipliers
- **Lost contest time = lost score**

#### Peer-to-Peer

**No Single Point of Failure:**
- ✅ Any peer can fail without affecting others
- ✅ Automatic partition tolerance (CAP theorem)
- ✅ Graceful degradation (N-1 peers still work)
- ✅ Self-healing network topology

**Real-world impact:**
- One station's computer crashes → other 5 continue normally
- Network partition heals automatically on reconnect
- **Zero downtime for operating stations**

**Winner:** **Peer-to-Peer** (by far)

---

### 2. Consistency & Dupe Checking

#### Client-Server

**Strong Consistency:**
- ✅ Server is single source of truth
- ✅ Trivial dupe checking (server validates all QSOs)
- ✅ No conflict resolution needed
- ✅ Simple mental model

**Dupe checking:**
```cpp
// Server-side (trivial)
if (m_dupeIndex.contains(callsign_band_mode))
    return DUPLICATE;
m_dupeIndex.insert(callsign_band_mode);
```

#### Peer-to-Peer

**Eventual Consistency:**
- ⚠️ Requires distributed consensus (Raft, Paxos, or custom)
- ⚠️ Conflict resolution needed (what if two stations log same QSO?)
- ⚠️ More complex to reason about
- ✅ Still achievable with proper algorithms

**Dupe checking (distributed):**
```cpp
// Each peer maintains local log + vector clock
bool isDupe(const QSO& qso) {
    // Check local log
    if (m_localLog.contains(qso))
        return true;

    // Check if any peer has this QSO (async query)
    for (auto* peer : m_peers) {
        if (peer->hasQSO(qso))
            return true;  // Another station already logged it
    }

    return false;
}

// Conflict resolution: Last-Write-Wins with vector clocks
void resolveConflict(const QSO& qso1, const QSO& qso2) {
    if (qso1.vectorClock > qso2.vectorClock)
        keepQSO(qso1);
    else
        keepQSO(qso2);
}
```

**Winner:** **Client-Server** (simpler) but **P2P is achievable** with more effort

---

### 3. Serial Number Allocation

This is the **hardest problem** for P2P systems.

#### Client-Server

**Centralized Allocation:**
```cpp
// Server increments global counter
uint32_t nextSerial = m_serialCounter++;
sendToClient(client, nextSerial);
```

- ✅ Guaranteed no duplicates
- ✅ Trivially simple
- ❌ Requires round-trip to server per QSO
- ❌ Fails if server down

#### Peer-to-Peer

**Distributed Allocation (Option 1: Range-Based):**
```cpp
// Each station pre-allocated a range
// Station A: 1-1000
// Station B: 1001-2000
// Station C: 2001-3000

uint32_t nextSerial() {
    return m_rangeStart + m_localCounter++;
}
```

- ✅ No coordination needed
- ✅ Works offline
- ❌ Gaps in serial numbers (Station A uses 1-523, B uses 1001-1834, etc.)
- ⚠️ Contest rules may prohibit gaps

**Distributed Allocation (Option 2: Coordinator Election):**
```cpp
// One peer is elected as serial allocator
// If it fails, election happens automatically

class SerialCoordinator {
    uint32_t m_nextSerial = 1;

    uint32_t allocate() {
        return m_nextSerial++;
    }
};

// Election algorithm (Raft-based)
void electCoordinator() {
    // Peer with lowest ID becomes coordinator
    if (m_peerId == getLowestActivePeer())
        m_isCoordinator = true;
}
```

- ✅ No gaps in serials
- ✅ Automatic failover
- ⚠️ Election overhead (1-2 seconds)
- ✅ Same guarantees as client-server

**Distributed Allocation (Option 3: Hybrid):**
```cpp
// Default: Use coordinator (one of the peers)
// Fallback: If coordinator unreachable, use range-based

uint32_t allocateSerial() {
    if (m_coordinator && m_coordinator->isReachable())
        return m_coordinator->allocateSerial();
    else
        return m_rangeStart + m_localCounter++;  // Fallback
}
```

- ✅ Best of both worlds
- ✅ No single point of failure
- ⚠️ Slight complexity

**Winner:** **Hybrid P2P** (best reliability + correctness)

---

### 4. Network Bandwidth

#### Client-Server

**Bandwidth Usage:**
```
QSO logged by Station A:
  A → Server: 200 bytes
  Server → B, C, D, E, F: 200 × 5 = 1000 bytes
  Total: 1200 bytes network traffic
```

**Star topology:**
- All traffic flows through server
- Server network interface is bottleneck
- 6 stations × 200 bytes = 1200 bytes per QSO

#### Peer-to-Peer

**Bandwidth Usage (Full Mesh):**
```
QSO logged by Station A:
  A → B, C, D, E, F: 200 × 5 = 1000 bytes
  Total: 1000 bytes network traffic
```

**Full mesh topology:**
- Direct peer-to-peer connections
- No bottleneck (distributed load)
- 17% less bandwidth than client-server!

**Peer-to-Peer (Gossip Protocol):**
```
QSO logged by Station A:
  A → B, C: 200 × 2 = 400 bytes (gossip to 2 neighbors)
  B → D: 200 bytes (forward)
  C → E: 200 bytes (forward)
  E → F: 200 bytes (forward)
  Total: 1000 bytes, but spread over time
```

**Gossip topology:**
- Even more efficient for large networks (>10 stations)
- Eventual delivery (not instant)
- Good for status updates, not critical messages

**Winner:** **Peer-to-Peer** (lower bandwidth, no bottleneck)

---

### 5. Implementation Complexity

#### Client-Server

**Code Complexity: LOW**

```cpp
// Server (simple)
class TR4WServer {
    QVector<QTcpSocket*> m_clients;

    void broadcastQSO(const QSO& qso) {
        for (auto* client : m_clients)
            sendMessage(client, qso);
    }
};

// Client (simple)
class TR4WClient {
    QTcpSocket* m_socket;

    void logQSO(const QSO& qso) {
        sendMessage(m_socket, qso);
    }
};
```

**Lines of code:** ~2000-3000 LOC

#### Peer-to-Peer

**Code Complexity: MEDIUM-HIGH**

```cpp
// Peer (complex)
class TR4WPeer {
    QVector<PeerConnection*> m_peers;
    VectorClock m_clock;
    ConflictResolver m_resolver;
    CoordinatorElection m_election;

    void broadcastQSO(const QSO& qso) {
        qso.setVectorClock(m_clock.increment());
        for (auto* peer : m_peers)
            sendMessage(peer, qso);
    }

    void handleConflict(const QSO& qso1, const QSO& qso2) {
        m_resolver.resolve(qso1, qso2);
    }

    void electCoordinator() {
        // Raft election algorithm
        // ...
    }
};
```

**Lines of code:** ~5000-8000 LOC

**Winner:** **Client-Server** (much simpler)

---

### 6. Operational Simplicity

#### Client-Server

**Setup:**
1. Start server on one machine
2. Configure clients with server IP address
3. Done

**Troubleshooting:**
- Server not responding? Restart server.
- Clients can't connect? Check firewall.
- **Single point to debug.**

#### Peer-to-Peer

**Setup:**
1. Configure each station with list of peer IP addresses
2. Start all stations (any order)
3. Network topology forms automatically

**Troubleshooting:**
- One peer not visible? Check that peer's network.
- Partition detected? Check intermediate switches.
- **Multiple points to debug.**

**Winner:** **Client-Server** (simpler ops)

---

### 7. Scalability

#### Client-Server

**Scaling limits:**
- Server CPU: Can handle ~100 clients easily
- Server network: 1 Gbps = ~5000 QSOs/second (more than enough)
- Memory: Minimal (log file + connection state)

**Bottleneck:** Server network interface at ~50+ clients with high message rate.

#### Peer-to-Peer

**Scaling limits:**
- Each peer maintains N-1 connections
- 10 stations = 9 connections per peer = 90 total connections
- 26 stations = 25 connections per peer = 650 total connections

**Bottleneck:** Connection count at ~20-30 stations (TCP overhead).

**Winner for <10 stations:** **Tie** (both scale fine)
**Winner for >20 stations:** **Client-Server** (less connection overhead)

---

### 8. Real-World Contest Scenarios

#### Scenario 1: Small Multi-Op (2-3 Stations)

**Client-Server:**
- Dedicated server machine required
- 3 computers needed (2 operators + 1 server)
- Server crash = total failure

**Peer-to-Peer:**
- No extra hardware needed
- 2 computers (2 operators as peers)
- Any station crash = other continues

**Winner:** **P2P** (no wasted hardware, more resilient)

---

#### Scenario 2: Medium Multi-Op (4-6 Stations)

**Client-Server:**
- Server on dedicated hardware or one op station
- If server on op station → that station has extra load
- Manageable complexity

**Peer-to-Peer:**
- 6 peers × 5 connections = 30 total connections
- Each station maintains 5 connections
- Manageable complexity

**Winner:** **Tie** (both work well)

---

#### Scenario 3: Large Multi-Multi (10+ Stations)

**Client-Server:**
- Dedicated server essential
- Server becomes coordination point (operators ask server admin for status)
- Single administrator role needed

**Peer-to-Peer:**
- 10 peers × 9 connections = 90 connections
- Potential for connection limit issues
- Gossip protocol more suitable

**Winner:** **Client-Server** (cleaner at scale)

---

#### Scenario 4: Remote/Distributed Operation

**Example:** M/M station with operators in different cities (Internet-based)

**Client-Server:**
- Server in cloud (AWS, etc.)
- NAT traversal simple (clients connect out)
- Latency: All traffic routes through server location
- Single public IP needed (server)

**Peer-to-Peer:**
- NAT traversal **hard** (each peer needs public IP or hole-punching)
- Latency: Direct peer-to-peer (lower)
- Requires STUN/TURN servers or VPN

**Winner:** **Client-Server** (much easier NAT traversal)

---

#### Scenario 5: Field Day (Outdoor, Unstable Network)

**Scenario:** WiFi mesh network, intermittent connectivity

**Client-Server:**
- Server must be reachable from all clients
- Any network partition isolates clients
- Clients can't operate independently

**Peer-to-Peer:**
- Partition tolerance built-in
- Clients continue operating during splits
- Automatic merge on reconnect
- **Much more resilient**

**Winner:** **P2P** (designed for partition tolerance)

---

## Consensus Algorithms for P2P

If we choose P2P, we need distributed consensus for:
1. QSO log consistency
2. Serial number allocation
3. Multiplier tracking

### Option 1: Raft Consensus

**Raft** is a widely-used, understandable consensus algorithm.

```cpp
class RaftPeer {
    enum State { FOLLOWER, CANDIDATE, LEADER };

    State m_state = FOLLOWER;
    uint32_t m_currentTerm = 0;
    QString m_votedFor;
    QVector<LogEntry> m_log;

    // Leader election
    void startElection() {
        m_state = CANDIDATE;
        m_currentTerm++;
        m_votedFor = m_peerId;

        // Request votes from other peers
        for (auto* peer : m_peers) {
            peer->requestVote(m_currentTerm, m_peerId);
        }
    }

    // Log replication (leader → followers)
    void appendEntries(const QVector<LogEntry>& entries) {
        if (m_state == LEADER) {
            for (auto* peer : m_peers) {
                peer->replicateLog(entries);
            }
        }
    }
};
```

**Pros:**
- ✅ Proven algorithm (used in etcd, Consul, etc.)
- ✅ Understandable (simpler than Paxos)
- ✅ Strong consistency guarantees
- ✅ Leader election automatic

**Cons:**
- ⚠️ Requires majority (3/5 peers must be online)
- ⚠️ Leader election takes 1-2 seconds
- ⚠️ Not partition-tolerant (minority partition stops)

**Good for:** Stations that stay connected (LAN-based contests)

---

### Option 2: CRDT (Conflict-Free Replicated Data Types)

**CRDTs** are data structures that automatically resolve conflicts.

```cpp
// Each QSO has a unique ID (station + timestamp + counter)
struct QSOId {
    QString stationId;      // "A"
    uint64_t timestamp;     // Unix milliseconds
    uint32_t counter;       // Disambiguate same-millisecond QSOs

    bool operator<(const QSOId& other) const {
        return std::tie(timestamp, stationId, counter) <
               std::tie(other.timestamp, other.stationId, other.counter);
    }
};

// Log is a CRDT (OR-Set: Observed-Remove Set)
class LogCRDT {
    QMap<QSOId, QSO> m_qsos;

    void add(const QSO& qso) {
        QSOId id{qso.stationId, qso.timestamp, qso.counter};
        m_qsos.insert(id, qso);  // Idempotent
    }

    void remove(const QSOId& id) {
        m_qsos.remove(id);  // Idempotent
    }

    // Merge from another peer (automatic conflict resolution)
    void merge(const LogCRDT& other) {
        for (auto it = other.m_qsos.begin(); it != other.m_qsos.end(); ++it) {
            if (!m_qsos.contains(it.key()) || m_qsos[it.key()].version < it->version)
                m_qsos[it.key()] = it.value();
        }
    }
};
```

**Pros:**
- ✅ **No coordination needed** (eventually consistent)
- ✅ **Partition tolerant** (works during network splits)
- ✅ Automatic conflict resolution
- ✅ No leader election

**Cons:**
- ⚠️ Eventual consistency (not immediate)
- ⚠️ Can't enforce unique serial numbers (need Raft for that)
- ⚠️ More complex mental model

**Good for:** Stations with unstable connectivity (Field Day, portable ops)

---

### Option 3: Hybrid (Raft for Serials, CRDT for Log)

**Best of both worlds:**

```cpp
class HybridPeer {
    RaftCoordinator m_serialAllocator;  // Raft for serial numbers
    LogCRDT m_log;                      // CRDT for QSO log

    uint32_t allocateSerial() {
        if (m_serialAllocator.isLeaderReachable())
            return m_serialAllocator.allocate();  // Use Raft
        else
            return m_rangeStart + m_localCounter++;  // Fallback
    }

    void logQSO(QSO qso) {
        qso.serial = allocateSerial();
        m_log.add(qso);  // CRDT (always works)

        // Broadcast to peers
        for (auto* peer : m_peers)
            peer->receiveQSO(qso);
    }
};
```

**Pros:**
- ✅ Strong consistency for serials (when possible)
- ✅ Eventual consistency for log (always works)
- ✅ Graceful degradation

**Cons:**
- ⚠️ Most complex implementation

---

## Recommendation: Hybrid P2P Architecture

### Architecture Design

```
┌─────────────────────────────────────────────┐
│          Station A (Coordinator)            │
│  ┌────────────┐  ┌──────────────────────┐  │
│  │Raft Leader │  │  CRDT Log            │  │
│  │Serial: 1-∞ │  │  QSOs: {...}         │  │
│  └────────────┘  └──────────────────────┘  │
└──────┬──────────────────┬───────────────────┘
       │                  │
   ┌───▼───┐          ┌───▼───┐
   │Station│          │Station│
   │   B   │◄────────►│   C   │
   │(Peer) │          │(Peer) │
   └───────┘          └───────┘
```

**Components:**

1. **Raft Coordinator (for serial numbers)**
   - One station elected as serial allocator
   - Automatic failover if coordinator crashes
   - Fallback to range-based allocation if network partitioned

2. **CRDT Log (for QSO data)**
   - Each station maintains local log using CRDT
   - Automatic conflict-free merging
   - Works even during network partitions

3. **Direct P2P connections**
   - Full mesh for small ops (2-6 stations)
   - Gossip protocol for large ops (10+ stations)

### Implementation Plan

#### Phase 1: Basic P2P (No Serials)

```cpp
class TR4QTPeer : public QObject {
    Q_OBJECT

public:
    TR4QTPeer(const QString& stationId);

    void connectToPeer(const QString& host, quint16 port);
    void logQSO(const QSO& qso);

signals:
    void qsoReceived(const QSO& qso);
    void peerConnected(const QString& peerId);
    void peerDisconnected(const QString& peerId);

private:
    QString m_stationId;
    QVector<PeerConnection*> m_peers;
    LogCRDT m_log;

    void broadcastQSO(const QSO& qso);
    void mergeLog(const LogCRDT& otherLog);
};
```

**Timeline:** 2-3 months

#### Phase 2: Raft Serial Coordinator

```cpp
class RaftSerialCoordinator : public QObject {
    Q_OBJECT

public:
    uint32_t allocateSerial();

    void startElection();
    bool isLeader() const;

signals:
    void leaderElected(const QString& leaderId);
    void leaderLost();

private:
    enum State { FOLLOWER, CANDIDATE, LEADER };
    State m_state = FOLLOWER;
    uint32_t m_currentTerm = 0;
    uint32_t m_nextSerial = 1;
};
```

**Timeline:** 3-4 months

#### Phase 3: Partition Handling

```cpp
class PartitionDetector : public QObject {
    Q_OBJECT

public:
    bool isPartitioned() const;
    QVector<QString> getVisiblePeers() const;

signals:
    void partitionDetected(const QVector<QString>& visiblePeers);
    void partitionHealed();

private:
    void pingPeers();
    void detectSplit();
};
```

**Timeline:** 2 months

---

### Configuration

**tr4qt.conf:**
```json
{
    "network": {
        "mode": "peer-to-peer",
        "station_id": "A",
        "station_name": "N6TR-Alpha",

        "peers": [
            {"host": "192.168.1.101", "port": 7300, "station_id": "B"},
            {"host": "192.168.1.102", "port": 7300, "station_id": "C"},
            {"host": "192.168.1.103", "port": 7300, "station_id": "D"}
        ],

        "coordinator": {
            "enabled": true,
            "preferred_coordinator": "A",   // Default coordinator
            "election_timeout_ms": 5000,
            "heartbeat_interval_ms": 1000
        },

        "serial_allocation": {
            "mode": "coordinator",           // or "range-based"
            "range_start": 1,                // For range-based fallback
            "range_size": 1000
        },

        "partition_tolerance": {
            "enabled": true,
            "auto_merge": true,
            "merge_strategy": "crdt"         // or "manual"
        }
    }
}
```

---

## Decision Matrix

| Criterion | Client-Server | P2P (Full) | **Hybrid P2P** | Weight |
|-----------|---------------|------------|----------------|--------|
| **Reliability** | ⭐⭐ (SPOF) | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 5x |
| **Simplicity** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | 3x |
| **Partition Tolerance** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 4x |
| **Serial Number Consistency** | ⭐⭐⭐⭐⭐ | ⭐⭐ (gaps) | ⭐⭐⭐⭐ | 4x |
| **Scalability (>10 sta)** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | 2x |
| **NAT Traversal** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ | 3x |
| **Bandwidth Efficiency** | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 2x |
| **Operational Simplicity** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | 3x |
| **TOTAL (weighted)** | **79** | **85** | **96** | |

**Winner:** **Hybrid P2P** (best balance of reliability and practicality)

---

## Final Recommendation

### For TR4QT: Implement Hybrid Peer-to-Peer

**Architecture:**
1. **CRDT-based log replication** (always works, partition-tolerant)
2. **Raft-based serial allocation** (strong consistency when possible)
3. **Range-based fallback** (when network partitioned)
4. **No dedicated server required** (any station can be coordinator)

**Benefits:**
- ✅ No single point of failure
- ✅ Continues operating during network issues
- ✅ Automatic failover and partition tolerance
- ✅ Strong consistency for serial numbers (when achievable)
- ✅ Eventual consistency for QSO log (always achievable)
- ✅ More resilient for Field Day / portable operations

**Tradeoffs:**
- ⚠️ More complex implementation (~2x code)
- ⚠️ Harder to debug distributed issues
- ⚠️ NAT traversal more complex (but solvable with STUN/TURN)

**Recommended for:**
- Small to medium operations (2-10 stations)
- Field Day and portable ops
- Stations with unreliable connectivity
- Future-proof architecture

**Avoid if:**
- Very large ops (>20 stations) → use client-server
- Internet-based distributed ops → use client-server (NAT issues)
- Simplicity is paramount → use client-server

---

## Implementation Roadmap

### Milestone 1: Basic P2P (6 months)
- Direct peer connections
- CRDT log replication
- No serial numbers yet (contests without serials)
- **Deliverable:** Working P2P for non-serial contests

### Milestone 2: Raft Coordinator (9 months)
- Implement Raft consensus
- Serial number allocation via coordinator
- Leader election
- **Deliverable:** Full functionality with serial contests

### Milestone 3: Partition Tolerance (12 months)
- Partition detection
- Automatic fallback to range-based serials
- Merge after partition heals
- **Deliverable:** Production-ready P2P system

### Milestone 4: Scale Optimizations (15 months)
- Gossip protocol for >10 stations
- Connection pooling
- Bandwidth optimization
- **Deliverable:** Scalable to 20+ stations

---

## Conclusion

**Recommendation: Build TR4QT with Hybrid P2P architecture.**

The single point of failure in TR4W's client-server model is a significant liability. With no backward compatibility requirement, TR4QT has the opportunity to build a more resilient, modern architecture.

The hybrid approach (Raft + CRDT) provides:
- **Reliability** of P2P (no SPOF)
- **Consistency** of client-server (for serials)
- **Partition tolerance** for real-world network issues

This is the right architectural foundation for TR4QT.

---

**Document Version:** 1.0
**Author:** Architecture Team
**Status:** Recommendation for Review
