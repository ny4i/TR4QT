# Hybrid Peer-to-Peer Implementation Guide for TR4QT

**Complete Technical Specification**
**Version:** 2.0 (Simplified GUID-based Architecture)
**Date:** December 2025

This document provides complete implementation details for building TR4QT's hybrid peer-to-peer networking system from scratch.

**Architecture Summary:**
- **QSO Identity:** GUID (globally unique, no conflict possible)
- **QSO Synchronization:** Simple CRDT merge (add GUIDs we don't have)
- **Serial Allocation:** Raft consensus (separate from QSO sync)
- **Conflict Resolution:** Not needed - GUIDs ensure uniqueness
- **Duplicate Detection:** Rule-based at scoring time (contest-specific)

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Core Data Structures](#core-data-structures)
3. [CRDT Implementation](#crdt-implementation)
4. [Raft Consensus Implementation](#raft-consensus-implementation)
5. [Network Protocol](#network-protocol)
6. [Peer Connection Management](#peer-connection-management)
7. [Serial Number Allocation](#serial-number-allocation)
8. [Partition Handling](#partition-handling)
9. [Message Flow Diagrams](#message-flow-diagrams)
10. [State Machine Specifications](#state-machine-specifications)
11. [Error Handling](#error-handling)
12. [Testing Strategy](#testing-strategy)
13. [Complete Code Examples](#complete-code-examples)

---

# System Overview

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                        TR4QT Peer                            │
│                                                              │
│  ┌────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ Application    │  │  QSO Log (CRDT) │  │ Raft Serial  │ │
│  │ Layer          │  │                 │  │ Coordinator  │ │
│  │                │  │  - Add QSO      │  │              │ │
│  │ - UI Events    │  │  - Remove QSO   │  │ - Allocate   │ │
│  │ - QSO Entry    │  │  - Merge        │  │ - Replicate  │ │
│  │ - Display      │  │                 │  │ - Elect      │ │
│  └────────┬───────┘  └────────┬────────┘  └──────┬───────┘ │
│           │                   │                   │         │
│  ┌────────▼───────────────────▼───────────────────▼───────┐ │
│  │            P2P Network Layer                           │ │
│  │                                                         │ │
│  │  - Peer Discovery                                      │ │
│  │  - Connection Management                               │ │
│  │  - Message Routing                                     │ │
│  │  - Partition Detection                                 │ │
│  └─────────────────────────────────────────────────────────┘ │
│           │                   │                   │         │
└───────────┼───────────────────┼───────────────────┼─────────┘
            │                   │                   │
    ┌───────▼───────┐   ┌───────▼───────┐   ┌───────▼───────┐
    │   Peer B      │   │   Peer C      │   │   Peer D      │
    │ (Station B)   │   │ (Station C)   │   │ (Station D)   │
    └───────────────┘   └───────────────┘   └───────────────┘
```

## Component Responsibilities

### CRDT Log (Conflict-Free Replicated Data Type)
- **Purpose:** Store QSO data with automatic conflict resolution
- **Consistency:** Eventual consistency
- **Partition Behavior:** Continues operating, merges on reconnect
- **Use Cases:** QSO logging, multiplier tracking, log synchronization

### Raft Coordinator
- **Purpose:** Allocate serial numbers with strong consistency
- **Consistency:** Strong consistency (linearizable)
- **Partition Behavior:** Majority partition continues, minority stops
- **Use Cases:** Serial number contests (ARRL SS, CQ WPX, etc.)

### P2P Network Layer
- **Purpose:** Peer discovery, connection management, message routing
- **Transport:** TCP with Protocol Buffers
- **Topology:** Full mesh for ≤10 peers, gossip for >10 peers
- **Use Cases:** All inter-peer communication

---

# Core Data Structures

## QSO Record

```cpp
// src/common/qso_record.h

#pragma once
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QUuid>

namespace tr4qt {

// QSO record with GUID-based identity
struct QSORecord {
    // Identity (GUID ensures global uniqueness)
    QUuid id;                       // Universally unique identifier (generated on creation)

    // Metadata
    QString stationId;              // Which operator logged this ("A", "B", "C")
    quint64 timestamp;              // Unix milliseconds when QSO occurred
    bool deleted;                   // Tombstone for deletions

    // QSO data
    QDateTime time;                 // UTC time of QSO
    QString callsign;               // Worked station
    quint64 frequencyHz;            // Frequency in Hz
    QString band;                   // "160M", "80M", "40M", etc.
    QString mode;                   // "CW", "SSB", "FT8", etc.
    QString rstSent;                // RST sent
    QString rstRcvd;                // RST received

    // Exchange (contest-specific key-value pairs)
    QMap<QString, QString> exchangeSent;
    QMap<QString, QString> exchangeRcvd;

    // Computed fields
    QStringList multipliers;        // ["CT", "W1", "NA"]
    quint32 qsoPoints;              // Points for this QSO

    // Metadata
    QString operatorCall;           // Operator callsign
    QString comment;                // Notes

    QSORecord()
        : deleted(false), timestamp(0), frequencyHz(0), qsoPoints(0) {}

    // Create new QSO with GUID
    static QSORecord create(const QString& stationId) {
        QSORecord qso;
        qso.id = QUuid::createUuid();  // Generate GUID
        qso.stationId = stationId;
        qso.timestamp = QDateTime::currentMSecsSinceEpoch();
        qso.time = QDateTime::currentDateTimeUtc();
        return qso;
    }

    // Mark as deleted (tombstone)
    void markDeleted() {
        deleted = true;
    }
};

} // namespace tr4qt

// Hash function for QUuid (for QHash usage)
inline uint qHash(const QUuid& uuid, uint seed = 0) {
    return qHash(uuid.toString(), seed);
}
```

## Raft State

```cpp
// src/raft/raft_state.h

#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>

namespace tr4qt::raft {

// Raft node states
enum class State {
    FOLLOWER,   // Default state, follows leader
    CANDIDATE,  // Requesting votes
    LEADER      // Elected leader, manages replication
};

// Log entry for Raft consensus
struct LogEntry {
    quint64 term;           // Term when entry was received by leader
    quint32 serialNumber;   // Serial number allocated
    QString stationId;      // Which station got this serial
    QDateTime timestamp;    // When allocated

    LogEntry() : term(0), serialNumber(0) {}

    LogEntry(quint64 t, quint32 serial, const QString& station)
        : term(t), serialNumber(serial), stationId(station), timestamp(QDateTime::currentDateTimeUtc()) {}
};

// Raft persistent state (must survive crashes)
struct PersistentState {
    quint64 currentTerm;        // Latest term server has seen
    QString votedFor;           // CandidateId that received vote in current term
    QVector<LogEntry> log;      // Log entries

    PersistentState() : currentTerm(0) {}

    // Save to disk
    void save(const QString& filePath);

    // Load from disk
    static PersistentState load(const QString& filePath);
};

// Raft volatile state (OK to lose on crash)
struct VolatileState {
    quint64 commitIndex;        // Index of highest log entry known to be committed
    quint64 lastApplied;        // Index of highest log entry applied to state machine

    VolatileState() : commitIndex(0), lastApplied(0) {}
};

// Raft leader volatile state (reinitialized after election)
struct LeaderState {
    QMap<QString, quint64> nextIndex;   // For each peer, index of next log entry to send
    QMap<QString, quint64> matchIndex;  // For each peer, index of highest log entry known to be replicated

    void initialize(const QStringList& peerIds, quint64 lastLogIndex) {
        for (const QString& peerId : peerIds) {
            nextIndex[peerId] = lastLogIndex + 1;
            matchIndex[peerId] = 0;
        }
    }
};

} // namespace tr4qt::raft
```

## Network Messages

```cpp
// src/network/messages.h

#pragma once
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include "common/qso_record.h"
#include "raft/raft_state.h"

namespace tr4qt::net {

// Message types
enum class MessageType : quint16 {
    // CRDT log messages
    QSO_ADD = 1,
    QSO_EDIT = 2,
    QSO_DELETE = 3,
    LOG_SYNC_REQUEST = 4,
    LOG_SYNC_RESPONSE = 5,

    // Raft messages
    RAFT_REQUEST_VOTE = 10,
    RAFT_VOTE_REPLY = 11,
    RAFT_APPEND_ENTRIES = 12,
    RAFT_APPEND_REPLY = 13,

    // Peer management
    PEER_HELLO = 20,
    PEER_HEARTBEAT = 21,
    PEER_GOODBYE = 22,

    // Application messages
    STATION_STATUS = 30,
    DX_SPOT = 31,
    INTERCOM_MESSAGE = 32,

    // Serial allocation
    SERIAL_REQUEST = 40,
    SERIAL_REPLY = 41
};

// Base message
struct Message {
    MessageType type;
    QString senderId;           // Peer that sent this message
    quint64 timestamp;          // When message was created

    Message(MessageType t, const QString& sender)
        : type(t), senderId(sender), timestamp(QDateTime::currentMSecsSinceEpoch()) {}

    virtual ~Message() = default;

    // Serialize to bytes
    virtual QByteArray serialize() const = 0;

    // Deserialize from bytes
    static Message* deserialize(const QByteArray& data);
};

// QSO Add message
struct QSOAddMessage : public Message {
    QSORecord qso;

    QSOAddMessage(const QString& sender, const QSORecord& q)
        : Message(MessageType::QSO_ADD, sender), qso(q) {}

    QByteArray serialize() const override;
};

// Raft RequestVote RPC
struct RaftRequestVoteMessage : public Message {
    quint64 term;               // Candidate's term
    QString candidateId;        // Candidate requesting vote
    quint64 lastLogIndex;       // Index of candidate's last log entry
    quint64 lastLogTerm;        // Term of candidate's last log entry

    RaftRequestVoteMessage(const QString& sender)
        : Message(MessageType::RAFT_REQUEST_VOTE, sender)
        , term(0), lastLogIndex(0), lastLogTerm(0) {}

    QByteArray serialize() const override;
};

// Raft Vote Reply
struct RaftVoteReplyMessage : public Message {
    quint64 term;               // Current term, for candidate to update itself
    bool voteGranted;           // True means candidate received vote

    RaftVoteReplyMessage(const QString& sender)
        : Message(MessageType::RAFT_VOTE_REPLY, sender)
        , term(0), voteGranted(false) {}

    QByteArray serialize() const override;
};

// Raft AppendEntries RPC
struct RaftAppendEntriesMessage : public Message {
    quint64 term;               // Leader's term
    QString leaderId;           // So follower can redirect clients
    quint64 prevLogIndex;       // Index of log entry immediately preceding new ones
    quint64 prevLogTerm;        // Term of prevLogIndex entry
    QVector<raft::LogEntry> entries;  // Log entries to store (empty for heartbeat)
    quint64 leaderCommit;       // Leader's commitIndex

    RaftAppendEntriesMessage(const QString& sender)
        : Message(MessageType::RAFT_APPEND_ENTRIES, sender)
        , term(0), prevLogIndex(0), prevLogTerm(0), leaderCommit(0) {}

    QByteArray serialize() const override;
};

// Raft AppendEntries Reply
struct RaftAppendReplyMessage : public Message {
    quint64 term;               // Current term, for leader to update itself
    bool success;               // True if follower contained entry matching prevLogIndex and prevLogTerm
    quint64 matchIndex;         // Highest index known to match (for nextIndex optimization)

    RaftAppendReplyMessage(const QString& sender)
        : Message(MessageType::RAFT_APPEND_REPLY, sender)
        , term(0), success(false), matchIndex(0) {}

    QByteArray serialize() const override;
};

} // namespace tr4qt::net
```

---

# CRDT Implementation

## Simplified OR-Set for QSO Log

The QSO log uses a **simplified OR-Set CRDT** based on GUIDs. Since each QSO has a globally unique ID (GUID), there are no conflicts to resolve - just merge any QSOs we don't have.

**Key Insight:** In contest logging:
- Each station logs on different bands/modes (physical separation)
- QSO IDs are GUIDs (globally unique, no collision possible)
- If same GUID exists on two peers, it's the **same QSO** (already synced)
- Duplicate detection is **rule-based** at scoring time, not sync time

### Algorithm

```cpp
// src/crdt/log_crdt.h

#pragma once
#include <QMap>
#include <QSet>
#include <QReadWriteLock>
#include <QUuid>
#include "common/qso_record.h"

namespace tr4qt::crdt {

class LogCRDT {
public:
    LogCRDT(const QString& stationId);

    // Add QSO (idempotent)
    void add(const QSORecord& qso);

    // Delete QSO (tombstone)
    void remove(const QUuid& id);

    // Get QSO by ID
    QSORecord get(const QUuid& id) const;

    // Check if QSO exists and not deleted
    bool contains(const QUuid& id) const;

    // Get all QSOs (excluding deleted)
    QVector<QSORecord> getAll() const;

    // Merge from another peer (trivial: add GUIDs we don't have)
    void merge(const QMap<QUuid, QSORecord>& remoteQSOs);

    // Get QSO count
    quint32 count() const;

    // Serialize entire log
    QByteArray serialize() const;

    // Deserialize log
    static LogCRDT deserialize(const QByteArray& data, const QString& stationId);

    // Access to underlying map (for sync)
    const QMap<QUuid, QSORecord>& qsos() const { return m_qsos; }

    QString stationId() const { return m_stationId; }

private:
    QString m_stationId;
    mutable QReadWriteLock m_lock;

    // QSO storage: GUID → QSORecord
    QMap<QUuid, QSORecord> m_qsos;
};

} // namespace tr4qt::crdt
```

### Implementation

```cpp
// src/crdt/log_crdt.cpp

#include "log_crdt.h"
#include <QWriteLocker>
#include <QReadLocker>
#include <QDataStream>

namespace tr4qt::crdt {

LogCRDT::LogCRDT(const QString& stationId)
    : m_stationId(stationId)
{}

void LogCRDT::add(const QSORecord& qso) {
    QWriteLocker lock(&m_lock);

    // GUID ensures uniqueness - if we have it, it's the same QSO
    if (!m_qsos.contains(qso.id)) {
        m_qsos[qso.id] = qso;
    }
    // If GUID exists, it's already synced - no action needed
}

void LogCRDT::remove(const QUuid& id) {
    QWriteLocker lock(&m_lock);

    if (!m_qsos.contains(id))
        return;

    m_qsos[id].markDeleted();
}

QSORecord LogCRDT::get(const QUuid& id) const {
    QReadLocker lock(&m_lock);
    return m_qsos.value(id);
}

bool LogCRDT::contains(const QUuid& id) const {
    QReadLocker lock(&m_lock);
    return m_qsos.contains(id) && !m_qsos[id].deleted;
}

QVector<QSORecord> LogCRDT::getAll() const {
    QReadLocker lock(&m_lock);

    QVector<QSORecord> result;
    for (const QSORecord& qso : m_qsos) {
        if (!qso.deleted)
            result.append(qso);
    }

    // Sort by timestamp
    std::sort(result.begin(), result.end(), [](const QSORecord& a, const QSORecord& b) {
        return a.timestamp < b.timestamp;
    });

    return result;
}

void LogCRDT::merge(const QMap<QUuid, QSORecord>& remoteQSOs) {
    QWriteLocker lock(&m_lock);

    // Trivial merge: Add any GUIDs we don't have
    for (auto it = remoteQSOs.begin(); it != remoteQSOs.end(); ++it) {
        const QUuid& id = it.key();
        const QSORecord& remoteQSO = it.value();

        if (!m_qsos.contains(id)) {
            // New QSO from remote peer
            m_qsos[id] = remoteQSO;
        }
        // If we already have this GUID, it's the same QSO - skip
    }
}

quint32 LogCRDT::count() const {
    QReadLocker lock(&m_lock);

    quint32 cnt = 0;
    for (const QSORecord& qso : m_qsos) {
        if (!qso.deleted)
            cnt++;
    }
    return cnt;
}

QByteArray LogCRDT::serialize() const {
    QReadLocker lock(&m_lock);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    // Write number of QSOs
    stream << static_cast<quint32>(m_qsos.size());

    // Write each QSO (GUID + QSORecord)
    for (auto it = m_qsos.begin(); it != m_qsos.end(); ++it) {
        stream << it.key();  // QUuid
        // Serialize QSORecord fields
        const QSORecord& qso = it.value();
        stream << qso.stationId;
        stream << qso.timestamp;
        stream << qso.deleted;
        stream << qso.callsign;
        stream << qso.band;
        stream << qso.mode;
        // ... (serialize remaining fields)
    }

    return data;
}

} // namespace tr4qt::crdt
```

### Duplicate Detection (Contest Rules)

Duplicate detection is **separate** from QSO synchronization and happens at **scoring time** based on contest-specific rules:

```cpp
// src/contest/dupe_checker.h

namespace tr4qt::contest {

class DupeChecker {
public:
    enum DupeRule {
        ONE_QSO_PER_BAND,           // ARRL DX, CQ WW, etc.
        ONE_QSO_PER_MODE_PER_BAND,  // Some contests
        ONE_QSO_TOTAL               // ARRL Sweepstakes
    };

    DupeChecker(DupeRule rule);

    // Check if QSO is a duplicate
    bool isDuplicate(const QSORecord& qso, const QVector<QSORecord>& log) const;

private:
    DupeRule m_rule;
};

bool DupeChecker::isDuplicate(const QSORecord& qso, const QVector<QSORecord>& log) const {
    for (const QSORecord& existing : log) {
        if (existing.deleted || existing.id == qso.id)
            continue;

        if (existing.callsign.toUpper() != qso.callsign.toUpper())
            continue;

        switch (m_rule) {
        case ONE_QSO_PER_BAND:
            if (existing.band == qso.band)
                return true;
            break;

        case ONE_QSO_PER_MODE_PER_BAND:
            if (existing.band == qso.band && existing.mode == qso.mode)
                return true;
            break;

        case ONE_QSO_TOTAL:
            return true;
        }
    }

    return false;
}

} // namespace tr4qt::contest
```

---

# Raft Consensus Implementation

## Raft State Machine

```cpp
// src/raft/raft_node.h

#pragma once
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include "raft_state.h"
#include "network/messages.h"

namespace tr4qt::raft {

class RaftNode : public QObject {
    Q_OBJECT

public:
    explicit RaftNode(const QString& nodeId, QObject* parent = nullptr);

    // Start Raft node
    void start(const QStringList& peerIds);

    // Stop Raft node
    void stop();

    // Get current state
    State state() const { return m_state; }
    QString leaderId() const { return m_leaderId; }
    bool isLeader() const { return m_state == State::LEADER; }

    // Client request (serial allocation)
    quint32 allocateSerial(const QString& stationId);

    // Process incoming Raft messages
    void handleRequestVote(const net::RaftRequestVoteMessage& msg);
    void handleVoteReply(const net::RaftVoteReplyMessage& msg);
    void handleAppendEntries(const net::RaftAppendEntriesMessage& msg);
    void handleAppendReply(const net::RaftAppendReplyMessage& msg);

signals:
    void stateChanged(State newState);
    void leaderElected(const QString& leaderId);
    void serialAllocated(quint32 serialNumber, const QString& stationId);

    // Send message to peer
    void sendMessage(const QString& peerId, const net::Message& msg);

private slots:
    void onElectionTimeout();
    void onHeartbeatTimeout();

private:
    // Identity
    QString m_nodeId;
    QStringList m_peerIds;

    // State
    State m_state;
    QString m_leaderId;

    // Persistent state
    PersistentState m_persistent;

    // Volatile state
    VolatileState m_volatile;

    // Leader state (only used when leader)
    LeaderState m_leader;

    // Timers
    QTimer* m_electionTimer;
    QTimer* m_heartbeatTimer;

    // Election timeout (randomized)
    int electionTimeout() const;

    // Heartbeat interval
    static constexpr int HEARTBEAT_INTERVAL_MS = 100;

    // State transitions
    void becomeFollower(quint64 term);
    void becomeCandidate();
    void becomeLeader();

    // Raft algorithm
    void startElection();
    void sendHeartbeats();
    void replicateLog();
    void advanceCommitIndex();
    void applyCommittedEntries();

    // Vote tracking
    QSet<QString> m_votesReceived;

    // Next serial number to allocate
    quint32 m_nextSerial;
};

} // namespace tr4qt::raft
```

## Raft Implementation

```cpp
// src/raft/raft_node.cpp

#include "raft_node.h"
#include <QRandomGenerator>
#include <QDebug>

namespace tr4qt::raft {

RaftNode::RaftNode(const QString& nodeId, QObject* parent)
    : QObject(parent)
    , m_nodeId(nodeId)
    , m_state(State::FOLLOWER)
    , m_nextSerial(1)
{
    m_electionTimer = new QTimer(this);
    m_electionTimer->setSingleShot(true);
    connect(m_electionTimer, &QTimer::timeout, this, &RaftNode::onElectionTimeout);

    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &RaftNode::onHeartbeatTimeout);
}

void RaftNode::start(const QStringList& peerIds) {
    m_peerIds = peerIds;

    // Load persistent state from disk
    QString persistPath = QString("raft_%1.dat").arg(m_nodeId);
    m_persistent = PersistentState::load(persistPath);

    // Start as follower
    becomeFollower(m_persistent.currentTerm);

    qInfo() << "Raft node" << m_nodeId << "started with" << m_peerIds.size() << "peers";
}

void RaftNode::stop() {
    m_electionTimer->stop();
    m_heartbeatTimer->stop();

    // Save persistent state
    QString persistPath = QString("raft_%1.dat").arg(m_nodeId);
    m_persistent.save(persistPath);
}

int RaftNode::electionTimeout() const {
    // Randomized between 150-300ms (Raft paper recommends 150-300ms)
    return QRandomGenerator::global()->bounded(150, 300);
}

void RaftNode::becomeFollower(quint64 term) {
    if (m_state != State::FOLLOWER) {
        qInfo() << m_nodeId << "becoming FOLLOWER (term" << term << ")";
        m_state = State::FOLLOWER;
        emit stateChanged(State::FOLLOWER);
    }

    m_persistent.currentTerm = term;
    m_persistent.votedFor.clear();
    m_leaderId.clear();

    m_heartbeatTimer->stop();
    m_electionTimer->start(electionTimeout());
}

void RaftNode::becomeCandidate() {
    qInfo() << m_nodeId << "becoming CANDIDATE";
    m_state = State::CANDIDATE;
    emit stateChanged(State::CANDIDATE);

    startElection();
}

void RaftNode::becomeLeader() {
    qInfo() << m_nodeId << "becoming LEADER (term" << m_persistent.currentTerm << ")";
    m_state = State::LEADER;
    m_leaderId = m_nodeId;
    emit stateChanged(State::LEADER);
    emit leaderElected(m_nodeId);

    // Initialize leader state
    quint64 lastLogIndex = m_persistent.log.isEmpty() ? 0 : m_persistent.log.size();
    m_leader.initialize(m_peerIds, lastLogIndex);

    // Start sending heartbeats
    m_electionTimer->stop();
    m_heartbeatTimer->start(HEARTBEAT_INTERVAL_MS);

    sendHeartbeats();
}

void RaftNode::onElectionTimeout() {
    // Follower or Candidate didn't hear from leader
    becomeCandidate();
}

void RaftNode::onHeartbeatTimeout() {
    // Leader sends periodic heartbeats
    if (m_state == State::LEADER) {
        sendHeartbeats();
    }
}

void RaftNode::startElection() {
    // Increment term
    m_persistent.currentTerm++;

    // Vote for self
    m_persistent.votedFor = m_nodeId;
    m_votesReceived.clear();
    m_votesReceived.insert(m_nodeId);

    // Reset election timer
    m_electionTimer->start(electionTimeout());

    // Send RequestVote RPCs to all peers
    quint64 lastLogIndex = m_persistent.log.isEmpty() ? 0 : m_persistent.log.size();
    quint64 lastLogTerm = m_persistent.log.isEmpty() ? 0 : m_persistent.log.last().term;

    for (const QString& peerId : m_peerIds) {
        net::RaftRequestVoteMessage msg(m_nodeId);
        msg.term = m_persistent.currentTerm;
        msg.candidateId = m_nodeId;
        msg.lastLogIndex = lastLogIndex;
        msg.lastLogTerm = lastLogTerm;

        emit sendMessage(peerId, msg);
    }

    qDebug() << m_nodeId << "started election for term" << m_persistent.currentTerm;
}

void RaftNode::handleRequestVote(const net::RaftRequestVoteMessage& msg) {
    net::RaftVoteReplyMessage reply(m_nodeId);
    reply.term = m_persistent.currentTerm;
    reply.voteGranted = false;

    // Reply false if term < currentTerm
    if (msg.term < m_persistent.currentTerm) {
        emit sendMessage(msg.senderId, reply);
        return;
    }

    // If RPC request or response contains term T > currentTerm: set currentTerm = T, convert to follower
    if (msg.term > m_persistent.currentTerm) {
        becomeFollower(msg.term);
    }

    // Grant vote if:
    // 1. Haven't voted or already voted for this candidate
    // 2. Candidate's log is at least as up-to-date as receiver's log

    bool canVote = (m_persistent.votedFor.isEmpty() || m_persistent.votedFor == msg.candidateId);

    quint64 lastLogIndex = m_persistent.log.isEmpty() ? 0 : m_persistent.log.size();
    quint64 lastLogTerm = m_persistent.log.isEmpty() ? 0 : m_persistent.log.last().term;

    bool logUpToDate = (msg.lastLogTerm > lastLogTerm) ||
                       (msg.lastLogTerm == lastLogTerm && msg.lastLogIndex >= lastLogIndex);

    if (canVote && logUpToDate) {
        m_persistent.votedFor = msg.candidateId;
        reply.voteGranted = true;

        // Reset election timer (heard from valid candidate)
        m_electionTimer->start(electionTimeout());

        qDebug() << m_nodeId << "voted for" << msg.candidateId << "in term" << msg.term;
    }

    emit sendMessage(msg.senderId, reply);
}

void RaftNode::handleVoteReply(const net::RaftVoteReplyMessage& msg) {
    // Ignore if not candidate
    if (m_state != State::CANDIDATE)
        return;

    // Ignore stale replies
    if (msg.term < m_persistent.currentTerm)
        return;

    // If higher term, become follower
    if (msg.term > m_persistent.currentTerm) {
        becomeFollower(msg.term);
        return;
    }

    // Count vote
    if (msg.voteGranted) {
        m_votesReceived.insert(msg.senderId);

        qDebug() << m_nodeId << "received vote from" << msg.senderId
                 << "(" << m_votesReceived.size() << "/" << (m_peerIds.size() + 1) << ")";

        // Check if won election (majority of votes)
        int majority = (m_peerIds.size() + 1) / 2 + 1;
        if (m_votesReceived.size() >= majority) {
            becomeLeader();
        }
    }
}

void RaftNode::sendHeartbeats() {
    if (m_state != State::LEADER)
        return;

    for (const QString& peerId : m_peerIds) {
        quint64 prevLogIndex = m_leader.nextIndex[peerId] - 1;
        quint64 prevLogTerm = (prevLogIndex == 0) ? 0 : m_persistent.log[prevLogIndex - 1].term;

        net::RaftAppendEntriesMessage msg(m_nodeId);
        msg.term = m_persistent.currentTerm;
        msg.leaderId = m_nodeId;
        msg.prevLogIndex = prevLogIndex;
        msg.prevLogTerm = prevLogTerm;
        msg.entries.clear();  // Empty for heartbeat
        msg.leaderCommit = m_volatile.commitIndex;

        emit sendMessage(peerId, msg);
    }
}

void RaftNode::handleAppendEntries(const net::RaftAppendEntriesMessage& msg) {
    net::RaftAppendReplyMessage reply(m_nodeId);
    reply.term = m_persistent.currentTerm;
    reply.success = false;

    // Reply false if term < currentTerm
    if (msg.term < m_persistent.currentTerm) {
        emit sendMessage(msg.senderId, reply);
        return;
    }

    // If RPC contains term >= currentTerm, recognize as leader
    if (msg.term >= m_persistent.currentTerm) {
        becomeFollower(msg.term);
        m_leaderId = msg.leaderId;
    }

    // Reset election timer (heard from leader)
    m_electionTimer->start(electionTimeout());

    // Reply false if log doesn't contain entry at prevLogIndex with prevLogTerm
    if (msg.prevLogIndex > 0) {
        if (msg.prevLogIndex > m_persistent.log.size()) {
            reply.matchIndex = m_persistent.log.size();
            emit sendMessage(msg.senderId, reply);
            return;
        }

        if (m_persistent.log[msg.prevLogIndex - 1].term != msg.prevLogTerm) {
            reply.matchIndex = msg.prevLogIndex - 1;
            emit sendMessage(msg.senderId, reply);
            return;
        }
    }

    // Append new entries
    if (!msg.entries.isEmpty()) {
        // Delete conflicting entries and append new ones
        quint64 index = msg.prevLogIndex;
        for (const LogEntry& entry : msg.entries) {
            if (index < m_persistent.log.size() && m_persistent.log[index].term != entry.term) {
                // Conflict: delete this entry and all that follow
                m_persistent.log.resize(index);
            }

            if (index >= m_persistent.log.size()) {
                m_persistent.log.append(entry);
            }

            index++;
        }
    }

    // Update commit index
    if (msg.leaderCommit > m_volatile.commitIndex) {
        m_volatile.commitIndex = qMin(msg.leaderCommit, static_cast<quint64>(m_persistent.log.size()));
        applyCommittedEntries();
    }

    reply.success = true;
    reply.matchIndex = msg.prevLogIndex + msg.entries.size();
    emit sendMessage(msg.senderId, reply);
}

void RaftNode::handleAppendReply(const net::RaftAppendReplyMessage& msg) {
    if (m_state != State::LEADER)
        return;

    // Ignore stale replies
    if (msg.term < m_persistent.currentTerm)
        return;

    // If higher term, step down
    if (msg.term > m_persistent.currentTerm) {
        becomeFollower(msg.term);
        return;
    }

    if (msg.success) {
        // Update nextIndex and matchIndex
        m_leader.matchIndex[msg.senderId] = msg.matchIndex;
        m_leader.nextIndex[msg.senderId] = msg.matchIndex + 1;

        // Advance commit index if possible
        advanceCommitIndex();
    }
    else {
        // Decrement nextIndex and retry
        m_leader.nextIndex[msg.senderId] = msg.matchIndex + 1;
        replicateLog();
    }
}

void RaftNode::advanceCommitIndex() {
    // Find highest N such that:
    // 1. N > commitIndex
    // 2. A majority of matchIndex[i] >= N
    // 3. log[N].term == currentTerm

    for (quint64 n = m_volatile.commitIndex + 1; n <= m_persistent.log.size(); n++) {
        if (m_persistent.log[n - 1].term != m_persistent.currentTerm)
            continue;

        int count = 1;  // Count self
        for (const QString& peerId : m_peerIds) {
            if (m_leader.matchIndex[peerId] >= n)
                count++;
        }

        int majority = (m_peerIds.size() + 1) / 2 + 1;
        if (count >= majority) {
            m_volatile.commitIndex = n;
            applyCommittedEntries();
        }
    }
}

void RaftNode::applyCommittedEntries() {
    // Apply log entries from lastApplied to commitIndex
    while (m_volatile.lastApplied < m_volatile.commitIndex) {
        m_volatile.lastApplied++;

        const LogEntry& entry = m_persistent.log[m_volatile.lastApplied - 1];

        // Apply to state machine (serial allocation)
        m_nextSerial = qMax(m_nextSerial, entry.serialNumber + 1);

        emit serialAllocated(entry.serialNumber, entry.stationId);

        qDebug() << m_nodeId << "applied entry" << m_volatile.lastApplied
                 << "serial" << entry.serialNumber << "→" << entry.stationId;
    }
}

quint32 RaftNode::allocateSerial(const QString& stationId) {
    if (m_state != State::LEADER) {
        qWarning() << m_nodeId << "cannot allocate serial: not leader";
        return 0;  // Indicate failure
    }

    // Create log entry
    LogEntry entry(m_persistent.currentTerm, m_nextSerial, stationId);
    m_persistent.log.append(entry);

    quint32 allocated = m_nextSerial;
    m_nextSerial++;

    // Replicate to followers
    replicateLog();

    return allocated;
}

void RaftNode::replicateLog() {
    if (m_state != State::LEADER)
        return;

    for (const QString& peerId : m_peerIds) {
        quint64 nextIndex = m_leader.nextIndex[peerId];
        quint64 prevLogIndex = nextIndex - 1;
        quint64 prevLogTerm = (prevLogIndex == 0) ? 0 : m_persistent.log[prevLogIndex - 1].term;

        net::RaftAppendEntriesMessage msg(m_nodeId);
        msg.term = m_persistent.currentTerm;
        msg.leaderId = m_nodeId;
        msg.prevLogIndex = prevLogIndex;
        msg.prevLogTerm = prevLogTerm;
        msg.leaderCommit = m_volatile.commitIndex;

        // Add entries from nextIndex onwards
        for (quint64 i = nextIndex; i <= m_persistent.log.size(); i++) {
            msg.entries.append(m_persistent.log[i - 1]);
        }

        emit sendMessage(peerId, msg);
    }
}

} // namespace tr4qt::raft
```

---

# Network Protocol

## Message Serialization (Protocol Buffers)

```protobuf
// src/network/protocol/tr4qt_p2p.proto

syntax = "proto3";

package tr4qt.net;

// QSO Record (GUID-based)
message QSORecordProto {
    // Identity
    string id = 1;                      // GUID as string
    string station_id = 2;              // Which station logged this
    uint64 timestamp = 3;               // Unix milliseconds
    bool deleted = 4;

    // QSO data
    int64 time_unix_ms = 10;
    string callsign = 11;
    uint64 frequency_hz = 12;
    string band = 13;
    string mode = 14;
    string rst_sent = 15;
    string rst_rcvd = 16;

    // Exchange
    map<string, string> exchange_sent = 20;
    map<string, string> exchange_rcvd = 21;

    // Computed fields
    repeated string multipliers = 30;
    uint32 qso_points = 31;

    // Metadata
    string operator_call = 40;
    string comment = 41;
}

// CRDT Messages
message QSOAddMessageProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    QSORecordProto qso = 3;
}

message LogSyncRequestProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint64 last_known_timestamp = 3;  // Request QSOs after this time
}

message LogSyncResponseProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    repeated QSORecordProto qsos = 3;
    bool has_more = 4;
}

// Raft Messages
message RaftRequestVoteProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint64 term = 3;
    string candidate_id = 4;
    uint64 last_log_index = 5;
    uint64 last_log_term = 6;
}

message RaftVoteReplyProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint64 term = 3;
    bool vote_granted = 4;
}

message RaftLogEntryProto {
    uint64 term = 1;
    uint32 serial_number = 2;
    string station_id = 3;
    int64 timestamp_unix_ms = 4;
}

message RaftAppendEntriesProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint64 term = 3;
    string leader_id = 4;
    uint64 prev_log_index = 5;
    uint64 prev_log_term = 6;
    repeated RaftLogEntryProto entries = 7;
    uint64 leader_commit = 8;
}

message RaftAppendReplyProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint64 term = 3;
    bool success = 4;
    uint64 match_index = 5;
}

// Peer Management
message PeerHelloProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    string station_name = 3;
    string version = 4;
}

message PeerHeartbeatProto {
    string sender_id = 1;
    uint64 timestamp = 2;
}

// Serial Allocation
message SerialRequestProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    string station_id = 3;  // Who needs the serial
}

message SerialReplyProto {
    string sender_id = 1;
    uint64 timestamp = 2;
    uint32 serial_number = 3;
    bool success = 4;
}

// Message Envelope
message MessageEnvelopeProto {
    enum MessageType {
        QSO_ADD = 0;
        QSO_EDIT = 1;
        QSO_DELETE = 2;
        LOG_SYNC_REQUEST = 3;
        LOG_SYNC_RESPONSE = 4;
        RAFT_REQUEST_VOTE = 10;
        RAFT_VOTE_REPLY = 11;
        RAFT_APPEND_ENTRIES = 12;
        RAFT_APPEND_REPLY = 13;
        PEER_HELLO = 20;
        PEER_HEARTBEAT = 21;
        SERIAL_REQUEST = 30;
        SERIAL_REPLY = 31;
    }

    MessageType type = 1;

    oneof payload {
        QSOAddMessageProto qso_add = 10;
        LogSyncRequestProto log_sync_request = 11;
        LogSyncResponseProto log_sync_response = 12;
        RaftRequestVoteProto raft_request_vote = 20;
        RaftVoteReplyProto raft_vote_reply = 21;
        RaftAppendEntriesProto raft_append_entries = 22;
        RaftAppendReplyProto raft_append_reply = 23;
        PeerHelloProto peer_hello = 30;
        PeerHeartbeatProto peer_heartbeat = 31;
        SerialRequestProto serial_request = 40;
        SerialReplyProto serial_reply = 41;
    }
}
```

## Wire Format (Framing)

```
┌────────────────────────────────────────────────┐
│  4 bytes       │  N bytes                      │
│  Length (BE)   │  Protobuf Message             │
├────────────────┼───────────────────────────────┤
│  0x00000045    │  MessageEnvelopeProto (69B)   │
└────────────────┴───────────────────────────────┘

Legend:
  BE = Big Endian
  Length = size of protobuf message (does not include the 4-byte header)
```

### Serialization/Deserialization

```cpp
// src/network/message_codec.h

#pragma once
#include <QByteArray>
#include <QDataStream>
#include "tr4qt_p2p.pb.h"

namespace tr4qt::net {

class MessageCodec {
public:
    // Serialize message to wire format
    static QByteArray encode(const MessageEnvelopeProto& msg) {
        // Serialize protobuf
        std::string protobufData;
        msg.SerializeToString(&protobufData);

        // Create frame with length prefix
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);

        stream << static_cast<quint32>(protobufData.size());
        frame.append(QByteArray::fromStdString(protobufData));

        return frame;
    }

    // Deserialize message from wire format
    static bool decode(const QByteArray& data, MessageEnvelopeProto& msg) {
        if (data.size() < 4)
            return false;

        // Read length prefix
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 length;
        stream >> length;

        if (data.size() < 4 + length)
            return false;

        // Parse protobuf
        QByteArray protobufData = data.mid(4, length);
        return msg.ParseFromArray(protobufData.data(), protobufData.size());
    }
};

} // namespace tr4qt::net
```

---

# Peer Connection Management

## Peer Connection

```cpp
// src/network/peer_connection.h

#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "messages.h"
#include "tr4qt_p2p.pb.h"

namespace tr4qt::net {

class PeerConnection : public QObject {
    Q_OBJECT

public:
    PeerConnection(const QString& peerId, QObject* parent = nullptr);

    // Connect to peer
    void connectToPeer(const QString& host, quint16 port);

    // Accept incoming connection
    void acceptConnection(QTcpSocket* socket);

    // Send message
    void sendMessage(const MessageEnvelopeProto& msg);

    // Peer info
    QString peerId() const { return m_peerId; }
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    bool isConnected() const { return m_socket && m_socket->state() == QAbstractSocket::ConnectedState; }

    // Statistics
    quint64 bytesSent() const { return m_bytesSent; }
    quint64 bytesReceived() const { return m_bytesReceived; }
    QDateTime lastHeartbeat() const { return m_lastHeartbeat; }

signals:
    void connected();
    void disconnected();
    void messageReceived(const MessageEnvelopeProto& msg);
    void error(const QString& errorString);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout();

private:
    QString m_peerId;
    QString m_host;
    quint16 m_port;

    QTcpSocket* m_socket;
    QByteArray m_receiveBuffer;

    QTimer* m_heartbeatTimer;
    QDateTime m_lastHeartbeat;

    quint64 m_bytesSent;
    quint64 m_bytesReceived;

    // Process buffered data
    void processBuffer();
};

} // namespace tr4qt::net
```

## Implementation

```cpp
// src/network/peer_connection.cpp

#include "peer_connection.h"
#include "message_codec.h"
#include <QDebug>

namespace tr4qt::net {

PeerConnection::PeerConnection(const QString& peerId, QObject* parent)
    : QObject(parent)
    , m_peerId(peerId)
    , m_port(0)
    , m_socket(nullptr)
    , m_bytesSent(0)
    , m_bytesReceived(0)
{
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(5000);  // 5 second heartbeat
    connect(m_heartbeatTimer, &QTimer::timeout, this, &PeerConnection::onHeartbeatTimeout);
}

void PeerConnection::connectToPeer(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &PeerConnection::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &PeerConnection::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &PeerConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PeerConnection::onError);

    qInfo() << "Connecting to peer" << m_peerId << "at" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

void PeerConnection::acceptConnection(QTcpSocket* socket) {
    m_socket = socket;
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::disconnected, this, &PeerConnection::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &PeerConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PeerConnection::onError);

    m_host = m_socket->peerAddress().toString();
    m_port = m_socket->peerPort();

    onConnected();
}

void PeerConnection::sendMessage(const MessageEnvelopeProto& msg) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send to" << m_peerId << ": not connected";
        return;
    }

    QByteArray data = MessageCodec::encode(msg);
    qint64 written = m_socket->write(data);

    if (written != data.size()) {
        qWarning() << "Partial write to" << m_peerId << ":" << written << "/" << data.size();
    }

    m_bytesSent += written;
    m_socket->flush();
}

void PeerConnection::onConnected() {
    qInfo() << "Connected to peer" << m_peerId;

    m_lastHeartbeat = QDateTime::currentDateTimeUtc();
    m_heartbeatTimer->start();

    // Send HELLO message
    MessageEnvelopeProto envelope;
    envelope.set_type(MessageEnvelopeProto::PEER_HELLO);

    auto* hello = envelope.mutable_peer_hello();
    hello->set_sender_id(m_peerId.toStdString());
    hello->set_timestamp(QDateTime::currentMSecsSinceEpoch());
    hello->set_station_name("Station " + m_peerId.toStdString());
    hello->set_version("1.0.0");

    sendMessage(envelope);

    emit connected();
}

void PeerConnection::onDisconnected() {
    qInfo() << "Disconnected from peer" << m_peerId;

    m_heartbeatTimer->stop();
    emit disconnected();
}

void PeerConnection::onReadyRead() {
    m_receiveBuffer.append(m_socket->readAll());
    m_bytesReceived += m_receiveBuffer.size();

    processBuffer();
}

void PeerConnection::processBuffer() {
    while (m_receiveBuffer.size() >= 4) {
        // Read length prefix
        QDataStream stream(m_receiveBuffer);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 length;
        stream >> length;

        // Check if we have complete message
        if (m_receiveBuffer.size() < 4 + length)
            break;  // Wait for more data

        // Extract message
        QByteArray messageData = m_receiveBuffer.mid(0, 4 + length);
        m_receiveBuffer.remove(0, 4 + length);

        // Deserialize
        MessageEnvelopeProto msg;
        if (MessageCodec::decode(messageData, msg)) {
            // Update heartbeat timestamp
            if (msg.type() == MessageEnvelopeProto::PEER_HEARTBEAT) {
                m_lastHeartbeat = QDateTime::currentDateTimeUtc();
            }

            emit messageReceived(msg);
        }
        else {
            qWarning() << "Failed to decode message from" << m_peerId;
        }
    }
}

void PeerConnection::onError(QAbstractSocket::SocketError error) {
    QString errorStr = m_socket->errorString();
    qWarning() << "Peer" << m_peerId << "error:" << errorStr;
    emit this->error(errorStr);
}

void PeerConnection::onHeartbeatTimeout() {
    // Send heartbeat
    MessageEnvelopeProto envelope;
    envelope.set_type(MessageEnvelopeProto::PEER_HEARTBEAT);

    auto* heartbeat = envelope.mutable_peer_heartbeat();
    heartbeat->set_sender_id(m_peerId.toStdString());
    heartbeat->set_timestamp(QDateTime::currentMSecsSinceEpoch());

    sendMessage(envelope);

    // Check if peer is alive (haven't heard heartbeat in 15 seconds)
    if (m_lastHeartbeat.secsTo(QDateTime::currentDateTimeUtc()) > 15) {
        qWarning() << "Peer" << m_peerId << "heartbeat timeout";
        m_socket->disconnectFromHost();
    }
}

} // namespace tr4qt::net
```

## Peer Manager

```cpp
// src/network/peer_manager.h

#pragma once
#include <QObject>
#include <QTcpServer>
#include <QMap>
#include "peer_connection.h"

namespace tr4qt::net {

class PeerManager : public QObject {
    Q_OBJECT

public:
    PeerManager(const QString& localId, QObject* parent = nullptr);

    // Start listening for incoming connections
    bool listen(quint16 port);

    // Connect to peer
    void connectToPeer(const QString& peerId, const QString& host, quint16 port);

    // Get peer connection
    PeerConnection* peer(const QString& peerId) const;

    // Get all connected peers
    QStringList connectedPeers() const;

    // Broadcast message to all peers
    void broadcast(const MessageEnvelopeProto& msg);

    // Send message to specific peer
    void sendToPeer(const QString& peerId, const MessageEnvelopeProto& msg);

signals:
    void peerConnected(const QString& peerId);
    void peerDisconnected(const QString& peerId);
    void messageReceived(const QString& peerId, const MessageEnvelopeProto& msg);

private slots:
    void onNewConnection();
    void onPeerConnected();
    void onPeerDisconnected();
    void onPeerMessage(const MessageEnvelopeProto& msg);

private:
    QString m_localId;
    quint16 m_listenPort;
    QTcpServer* m_server;

    QMap<QString, PeerConnection*> m_peers;
};

} // namespace tr4qt::net
```

---

# Serial Number Allocation

## Allocation Strategies

### Strategy 1: Raft Coordinator (Default)

```cpp
// src/serial/serial_allocator.h

#pragma once
#include <QObject>
#include "raft/raft_node.h"

namespace tr4qt::serial {

class SerialAllocator : public QObject {
    Q_OBJECT

public:
    enum Strategy {
        RAFT_COORDINATOR,   // Use Raft consensus (strong consistency)
        RANGE_BASED,        // Pre-allocated ranges per station
        HYBRID              // Raft with range-based fallback
    };

    SerialAllocator(Strategy strategy, QObject* parent = nullptr);

    // Set Raft node (for RAFT_COORDINATOR or HYBRID)
    void setRaftNode(raft::RaftNode* raftNode);

    // Set range for this station (for RANGE_BASED or HYBRID fallback)
    void setRange(quint32 rangeStart, quint32 rangeSize);

    // Allocate next serial number
    quint32 allocateNext(const QString& stationId);

    // Release serial (QSO not completed)
    void releaseSerial(quint32 serialNumber);

    // Check if serial is available
    bool isAvailable(quint32 serialNumber) const;

signals:
    void serialAllocated(quint32 serialNumber, const QString& stationId);
    void allocationFailed(const QString& reason);

private:
    Strategy m_strategy;
    raft::RaftNode* m_raftNode;

    // Range-based state
    quint32 m_rangeStart;
    quint32 m_rangeSize;
    quint32 m_rangeCurrent;

    // Track allocated serials
    QSet<quint32> m_allocated;
};

} // namespace tr4qt::serial
```

### Implementation

```cpp
// src/serial/serial_allocator.cpp

#include "serial_allocator.h"
#include <QDebug>

namespace tr4qt::serial {

SerialAllocator::SerialAllocator(Strategy strategy, QObject* parent)
    : QObject(parent)
    , m_strategy(strategy)
    , m_raftNode(nullptr)
    , m_rangeStart(0)
    , m_rangeSize(0)
    , m_rangeCurrent(0)
{}

void SerialAllocator::setRaftNode(raft::RaftNode* raftNode) {
    m_raftNode = raftNode;

    connect(m_raftNode, &raft::RaftNode::serialAllocated,
            this, &SerialAllocator::serialAllocated);
}

void SerialAllocator::setRange(quint32 rangeStart, quint32 rangeSize) {
    m_rangeStart = rangeStart;
    m_rangeSize = rangeSize;
    m_rangeCurrent = rangeStart;

    qInfo() << "Serial range set:" << rangeStart << "-" << (rangeStart + rangeSize - 1);
}

quint32 SerialAllocator::allocateNext(const QString& stationId) {
    quint32 serial = 0;

    switch (m_strategy) {
    case RAFT_COORDINATOR:
        if (!m_raftNode || !m_raftNode->isLeader()) {
            emit allocationFailed("Not Raft leader");
            return 0;
        }

        serial = m_raftNode->allocateSerial(stationId);
        if (serial == 0) {
            emit allocationFailed("Raft allocation failed");
            return 0;
        }
        break;

    case RANGE_BASED:
        if (m_rangeCurrent >= m_rangeStart + m_rangeSize) {
            emit allocationFailed("Range exhausted");
            return 0;
        }

        serial = m_rangeCurrent++;
        emit serialAllocated(serial, stationId);
        break;

    case HYBRID:
        // Try Raft first
        if (m_raftNode && m_raftNode->isLeader()) {
            serial = m_raftNode->allocateSerial(stationId);
        }

        // Fallback to range-based
        if (serial == 0) {
            qWarning() << "Raft unavailable, using range-based fallback";

            if (m_rangeCurrent >= m_rangeStart + m_rangeSize) {
                emit allocationFailed("Range exhausted");
                return 0;
            }

            serial = m_rangeCurrent++;
            emit serialAllocated(serial, stationId);
        }
        break;
    }

    m_allocated.insert(serial);
    return serial;
}

void SerialAllocator::releaseSerial(quint32 serialNumber) {
    m_allocated.remove(serialNumber);
    // Note: Cannot reclaim serial from Raft log
    // In range-based mode, serial is wasted (acceptable)
}

bool SerialAllocator::isAvailable(quint32 serialNumber) const {
    return !m_allocated.contains(serialNumber);
}

} // namespace tr4qt::serial
```

---

# Partition Handling

## Partition Detector

```cpp
// src/network/partition_detector.h

#pragma once
#include <QObject>
#include <QTimer>
#include <QSet>

namespace tr4qt::net {

class PartitionDetector : public QObject {
    Q_OBJECT

public:
    PartitionDetector(QObject* parent = nullptr);

    // Set expected peers
    void setExpectedPeers(const QStringList& peerIds);

    // Peer became reachable
    void peerReachable(const QString& peerId);

    // Peer became unreachable
    void peerUnreachable(const QString& peerId);

    // Get current partition (peers we can reach)
    QStringList currentPartition() const;

    // Check if partitioned
    bool isPartitioned() const;

    // Get majority size
    int majoritySize() const;

    // Check if in majority partition
    bool isInMajority() const;

signals:
    void partitionDetected(const QStringList& reachablePeers);
    void partitionHealed();

private slots:
    void checkPartitionStatus();

private:
    QTimer* m_checkTimer;
    QStringList m_expectedPeers;
    QSet<QString> m_reachablePeers;
    bool m_wasPartitioned;
};

} // namespace tr4qt::net
```

## Implementation

```cpp
// src/network/partition_detector.cpp

#include "partition_detector.h"
#include <QDebug>

namespace tr4qt::net {

PartitionDetector::PartitionDetector(QObject* parent)
    : QObject(parent)
    , m_wasPartitioned(false)
{
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(2000);  // Check every 2 seconds
    connect(m_checkTimer, &QTimer::timeout, this, &PartitionDetector::checkPartitionStatus);
    m_checkTimer->start();
}

void PartitionDetector::setExpectedPeers(const QStringList& peerIds) {
    m_expectedPeers = peerIds;
}

void PartitionDetector::peerReachable(const QString& peerId) {
    if (!m_reachablePeers.contains(peerId)) {
        m_reachablePeers.insert(peerId);
        qDebug() << "Peer" << peerId << "is now reachable";
    }
}

void PartitionDetector::peerUnreachable(const QString& peerId) {
    if (m_reachablePeers.contains(peerId)) {
        m_reachablePeers.remove(peerId);
        qDebug() << "Peer" << peerId << "is now unreachable";
    }
}

QStringList PartitionDetector::currentPartition() const {
    return m_reachablePeers.values();
}

bool PartitionDetector::isPartitioned() const {
    return m_reachablePeers.size() < m_expectedPeers.size();
}

int PartitionDetector::majoritySize() const {
    return (m_expectedPeers.size() + 1) / 2 + 1;  // +1 for self
}

bool PartitionDetector::isInMajority() const {
    return (m_reachablePeers.size() + 1) >= majoritySize();  // +1 for self
}

void PartitionDetector::checkPartitionStatus() {
    bool partitioned = isPartitioned();

    if (partitioned && !m_wasPartitioned) {
        // Partition detected
        qWarning() << "Network partition detected!"
                   << "Reachable:" << m_reachablePeers.size() << "/" << m_expectedPeers.size();

        emit partitionDetected(m_reachablePeers.values());
        m_wasPartitioned = true;
    }
    else if (!partitioned && m_wasPartitioned) {
        // Partition healed
        qInfo() << "Network partition healed";
        emit partitionHealed();
        m_wasPartitioned = false;
    }
}

} // namespace tr4qt::net
```

## Partition Merge Strategy (Simplified)

```cpp
// src/crdt/partition_merger.h

#pragma once
#include <QObject>
#include "log_crdt.h"

namespace tr4qt::crdt {

class PartitionMerger : public QObject {
    Q_OBJECT

public:
    PartitionMerger(LogCRDT* localLog, QObject* parent = nullptr);

    // Merge logs after partition heals
    void mergeAfterPartition(const QString& peerId, const QMap<QUuid, QSORecord>& remoteQSOs);

signals:
    void mergeCompleted(int newQSOsAdded);
    void mergeProgress(int qsosProcessed, int totalQSOs);

private:
    LogCRDT* m_localLog;
};

} // namespace tr4qt::crdt
```

```cpp
// src/crdt/partition_merger.cpp

#include "partition_merger.h"
#include <QDebug>

namespace tr4qt::crdt {

PartitionMerger::PartitionMerger(LogCRDT* localLog, QObject* parent)
    : QObject(parent)
    , m_localLog(localLog)
{}

void PartitionMerger::mergeAfterPartition(const QString& peerId,
                                          const QMap<QUuid, QSORecord>& remoteQSOs) {
    qInfo() << "Merging log from peer" << peerId << "after partition";

    int newQSOs = 0;
    int processed = 0;

    for (auto it = remoteQSOs.begin(); it != remoteQSOs.end(); ++it) {
        const QUuid& guid = it.key();
        const QSORecord& remoteQSO = it.value();

        // Check if this is a new GUID
        if (!m_localLog->contains(guid)) {
            m_localLog->add(remoteQSO);
            newQSOs++;
        }

        processed++;
        if (processed % 100 == 0) {
            emit mergeProgress(processed, remoteQSOs.size());
        }
    }

    qInfo() << "Merge completed:" << processed << "QSOs processed,"
            << newQSOs << "new QSOs added";

    emit mergeCompleted(newQSOs);
}

} // namespace tr4qt::crdt
```

**Key simplification:** No conflict resolution needed - just count new GUIDs added.

---

# Message Flow Diagrams

## QSO Logging Flow

```
Station A          Peers (B, C)         Raft Leader      CRDT Log
    |                   |                     |              |
    │                   │                     │              │
1.  │ Log QSO          │                     │              │
    │ "W1AW"           │                     │              │
    │─────────────────>│                     │              │
    │                   │                     │              │
2.  │ Generate QSO ID  │                     │              │
    │ QSOId(A,ts,0)    │                     │              │
    │                   │                     │              │
3.  │                   │                     │   Add to     │
    │                   │                     │   CRDT───────>│
    │                   │                     │              │
4.  │                   │   Broadcast         │              │
    │ ──────────────────>────QSOAddMsg───────>│              │
    │                   │                     │              │
5.  │                   │   Receive           │              │
    │                   │<──QSOAddMsg─────────│              │
    │                   │                     │              │
6.  │                   │   Add to CRDT       │              │
    │                   │──────────────────────────────────>│
    │                   │                     │              │
```

## Serial Number Allocation Flow (Raft)

```
Client A        Raft Leader       Raft Followers      State Machine
    |                |                   |                   |
    │                │                   │                   │
1.  │ Request Serial│                   │                   │
    │──────────────>│                   │                   │
    │                │                   │                   │
2.  │                │ Append Log Entry │                   │
    │                │ LogEntry(term=1, │                   │
    │                │  serial=123)     │                   │
    │                │                   │                   │
3.  │                │ Replicate────────>│                   │
    │                │   AppendEntries  │                   │
    │                │                   │                   │
4.  │                │<─────────────────│                   │
    │                │   Success        │                   │
    │                │                   │                   │
5.  │                │ Update CommitIdx │                   │
    │                │                   │                   │
6.  │                │ Apply to State   │                   │
    │                │   Machine────────────────────────────>│
    │                │                   │   Allocate 123   │
    │                │                   │                   │
7.  │<───────────────│                   │                   │
    │   Serial=123  │                   │                   │
    │                │                   │                   │
```

## Partition and Heal Flow

```
Time    Station A         Station B         Station C
T0      Connected─────────Connected─────────Connected
         (Full mesh)

T1      │                 │
        │                 │ Network partition!
        │                 X (B-C link fails)
        │                 │

T2      A logs W1AW       │                 C logs W2XYZ
        CRDT: [W1AW]      │                 CRDT: [W2XYZ]

T3      │                 │                 │
        │                 │ Partition heals
        │                 │<────────────────│

T4      │ LogSyncRequest  │                 │
        │────────────────>│                 │
        │                 │ LogSyncRequest  │
        │                 │────────────────>│

T5      │ LogSyncResponse │                 │
        │<────────────────│                 │
        │ QSOs: [W2XYZ]   │ LogSyncResponse │
        │                 │<────────────────│
        │                 │ QSOs: [W1AW]    │

T6      CRDT Merge        CRDT Merge        CRDT Merge
        Final: [W1AW,     Final: [W1AW,     Final: [W1AW,
                W2XYZ]            W2XYZ]            W2XYZ]
```

---

# State Machine Specifications

## Raft State Transitions

```
┌──────────────────────────────────────────────────────────┐
│                    Raft Node States                      │
└──────────────────────────────────────────────────────────┘

         ┌───────────┐
         │           │
         │ FOLLOWER  │◄──────────────────────┐
         │           │                       │
         └─────┬─────┘                       │
               │                             │
               │ Election timeout            │ Discover leader
               │                             │ or higher term
               ▼                             │
         ┌───────────┐                       │
         │           │                       │
         │ CANDIDATE │───────────────────────┘
         │           │
         └─────┬─────┘
               │
               │ Receive majority
               │ of votes
               ▼
         ┌───────────┐
         │           │
         │  LEADER   │
         │           │
         └───────────┘

State: FOLLOWER
─────────────────
Triggers:
  - Election timeout → become CANDIDATE
  - Receive AppendEntries from valid leader → reset timer
  - Receive RequestVote → grant vote if log up-to-date

State: CANDIDATE
────────────────
Triggers:
  - Start election (increment term, request votes)
  - Receive majority votes → become LEADER
  - Receive AppendEntries from leader → become FOLLOWER
  - Election timeout → start new election

State: LEADER
─────────────
Triggers:
  - Send periodic heartbeats (AppendEntries)
  - Receive client request → append to log, replicate
  - Discover higher term → become FOLLOWER
```

## CRDT Merge State Machine (Simplified)

```
┌──────────────────────────────────────────────────────────┐
│            CRDT Merge Algorithm (GUID-based)             │
└──────────────────────────────────────────────────────────┘

Input: Map of remote QSOs (GUID → QSORecord)

         ┌─────────────────┐
         │ For each remote │
         │ GUID            │
         └──────┬──────────┘
                │
        ┌───────▼────────────┐
        │ Do we have this    │
        │ GUID locally?      │
        └───────┬────────────┘
                │
         ┌──────┴──────┐
         │             │
        No            Yes
         │             │
         ▼             ▼
    ┌────────┐    ┌──────────┐
    │  Add   │    │   Skip   │
    │ to log │    │ (already │
    │        │    │  synced) │
    └────────┘    └──────────┘

No conflict resolution needed!
GUIDs are globally unique.
```

**Why this works:**
- Each QSO gets a GUID when created
- Same GUID = same QSO (already synced)
- Different GUID = different QSO (add it)
- No version tracking needed
- No causality tracking needed
- No conflict resolution needed

---

# Error Handling

## Network Errors

```cpp
// src/network/error_handler.h

#pragma once
#include <QString>
#include <QAbstractSocket>

namespace tr4qt::net {

class ErrorHandler {
public:
    enum ErrorSeverity {
        WARNING,        // Recoverable, log and continue
        ERROR,          // Serious, retry or fail gracefully
        FATAL           // Unrecoverable, abort operation
    };

    static void handleSocketError(QAbstractSocket::SocketError error,
                                   const QString& context);

    static void handleRaftError(const QString& error,
                                 const QString& context);

    static void handleCRDTError(const QString& error,
                                 const QString& context);

    static ErrorSeverity classifyError(QAbstractSocket::SocketError error);
};

} // namespace tr4qt::net
```

## Retry Logic

```cpp
// src/common/retry_policy.h

#pragma once
#include <functional>
#include <QTimer>

namespace tr4qt {

class RetryPolicy {
public:
    using RetryFunction = std::function<bool()>;

    static void executeWithRetry(RetryFunction func,
                                  int maxAttempts,
                                  int initialDelayMs,
                                  double backoffMultiplier = 2.0);

private:
    static void scheduleRetry(RetryFunction func,
                              int attemptsLeft,
                              int delayMs,
                              double backoffMultiplier);
};

} // namespace tr4qt
```

---

# Testing Strategy

## Unit Tests

### CRDT Tests

```cpp
// tests/test_crdt.cpp

#include <QtTest>
#include "crdt/log_crdt.h"

class CRDTTest : public QObject {
    Q_OBJECT

private slots:
    void testAddQSO();
    void testDeleteQSO();
    void testPartitionMerge();
    void testGUIDUniqueness();
};

void CRDTTest::testAddQSO() {
    LogCRDT log("A");

    // Create QSO with GUID
    QSORecord qso = QSORecord::create("A");
    qso.callsign = "W1AW";
    qso.band = "20M";
    qso.mode = "CW";

    log.add(qso);

    QVERIFY(log.contains(qso.id));
    QCOMPARE(log.count(), 1u);
}

void CRDTTest::testPartitionMerge() {
    // Station A and B work different stations during partition
    LogCRDT logA("A");
    LogCRDT logB("B");

    // A works W1AW
    QSORecord qsoA = QSORecord::create("A");
    qsoA.callsign = "W1AW";
    qsoA.band = "20M";
    logA.add(qsoA);

    // B works W2XYZ
    QSORecord qsoB = QSORecord::create("B");
    qsoB.callsign = "W2XYZ";
    qsoB.band = "40M";
    logB.add(qsoB);

    // Merge after partition heals
    logA.merge(logB.qsos());
    logB.merge(logA.qsos());

    // Both should have both QSOs
    QCOMPARE(logA.count(), 2u);
    QCOMPARE(logB.count(), 2u);

    QVERIFY(logA.contains(qsoA.id));
    QVERIFY(logA.contains(qsoB.id));
    QVERIFY(logB.contains(qsoA.id));
    QVERIFY(logB.contains(qsoB.id));
}

void CRDTTest::testGUIDUniqueness() {
    LogCRDT log("A");

    // Add same QSO twice (simulate duplicate broadcast)
    QSORecord qso = QSORecord::create("A");
    qso.callsign = "W1AW";

    log.add(qso);
    log.add(qso);  // Add again

    // Should only have one QSO (GUID prevents duplicates)
    QCOMPARE(log.count(), 1u);
}
```

### Raft Tests

```cpp
// tests/test_raft.cpp

#include <QtTest>
#include "raft/raft_node.h"

class RaftTest : public QObject {
    Q_OBJECT

private slots:
    void testLeaderElection();
    void testLogReplication();
    void testPartitionRecovery();
};

void RaftTest::testLeaderElection() {
    // Create 3 nodes
    RaftNode nodeA("A");
    RaftNode nodeB("B");
    RaftNode nodeC("C");

    nodeA.start({"B", "C"});
    nodeB.start({"A", "C"});
    nodeC.start({"A", "B"});

    // Connect nodes
    // (test helper to connect signals/slots)

    // Wait for election
    QSignalSpy spyA(&nodeA, &RaftNode::stateChanged);
    QSignalSpy spyB(&nodeB, &RaftNode::stateChanged);
    QSignalSpy spyC(&nodeC, &RaftNode::stateChanged);

    QVERIFY(spyA.wait(5000) || spyB.wait(5000) || spyC.wait(5000));

    // Verify exactly one leader
    int leaders = 0;
    if (nodeA.isLeader()) leaders++;
    if (nodeB.isLeader()) leaders++;
    if (nodeC.isLeader()) leaders++;

    QCOMPARE(leaders, 1);
}
```

## Integration Tests

```cpp
// tests/test_integration.cpp

class IntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void testFullQSOFlow();
    void testPartitionAndMerge();
    void testSerialAllocation();
};

void IntegrationTest::testFullQSOFlow() {
    // Setup: 3 peers
    // 1. Peer A logs QSO
    // 2. Verify B and C receive it
    // 3. Verify logs converge
}
```

## Chaos Testing

```cpp
// tests/chaos/test_network_chaos.cpp

class ChaosTest : public QObject {
    Q_OBJECT

private slots:
    void testRandomDisconnects();
    void testRandomPartitions();
    void testHighLatency();
    void testPacketLoss();
};
```

---

# Complete Code Examples

## Example: Simple P2P Peer

```cpp
// examples/simple_peer.cpp

#include <QCoreApplication>
#include "network/peer_manager.h"
#include "crdt/log_crdt.h"
#include "raft/raft_node.h"

using namespace tr4qt;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // Parse args: station ID and peers
    QString stationId = "A";
    QStringList peers = {"B:localhost:7301", "C:localhost:7302"};

    // Create components
    net::PeerManager peerMgr(stationId);
    crdt::LogCRDT log(stationId);
    raft::RaftNode raft(stationId);

    // Start listening
    peerMgr.listen(7300);

    // Connect to peers
    for (const QString& peer : peers) {
        QStringList parts = peer.split(':');
        QString peerId = parts[0];
        QString host = parts[1];
        quint16 port = parts[2].toUInt();

        peerMgr.connectToPeer(peerId, host, port);
    }

    // Start Raft
    QStringList peerIds = {"B", "C"};
    raft.start(peerIds);

    // Handle incoming messages
    QObject::connect(&peerMgr, &net::PeerManager::messageReceived,
                     [&](const QString& peerId, const net::MessageEnvelopeProto& msg) {
        if (msg.type() == net::MessageEnvelopeProto::QSO_ADD) {
            // Extract QSO
            // Add to CRDT log
            // Update UI
        }
    });

    return app.exec();
}
```

## Example: QSO Logging (Simplified)

```cpp
// examples/log_qso.cpp

void logQSO(const QString& callsign,
            crdt::LogCRDT& log,
            net::PeerManager& peers,
            serial::SerialAllocator& serials) {

    // Create QSO with GUID (automatic)
    QSORecord qso = QSORecord::create(log.stationId());
    qso.callsign = callsign;
    qso.frequencyHz = 14025000;
    qso.band = "20M";
    qso.mode = "CW";
    qso.rstSent = "599";
    qso.rstRcvd = "599";

    // Allocate serial if needed (via Raft - separate concern)
    quint32 serial = serials.allocateNext(log.stationId());
    qso.exchangeSent["serial"] = QString::number(serial);

    // Add to local log (GUID ensures uniqueness)
    log.add(qso);

    // Broadcast to peers
    net::MessageEnvelopeProto envelope;
    envelope.set_type(net::MessageEnvelopeProto::QSO_ADD);

    auto* qsoAdd = envelope.mutable_qso_add();
    qsoAdd->set_sender_id(log.stationId().toStdString());
    qsoAdd->set_timestamp(QDateTime::currentMSecsSinceEpoch());

    // Convert QSORecord to protobuf
    auto* qsoProto = qsoAdd->mutable_qso();
    qsoProto->set_id(qso.id.toString(QUuid::WithoutBraces).toStdString());
    qsoProto->set_station_id(qso.stationId.toStdString());
    qsoProto->set_timestamp(qso.timestamp);
    qsoProto->set_callsign(qso.callsign.toStdString());
    qsoProto->set_band(qso.band.toStdString());
    qsoProto->set_mode(qso.mode.toStdString());
    // ... (set remaining fields)

    peers.broadcast(envelope);

    qInfo() << "Logged QSO:" << callsign << "GUID:" << qso.id.toString()
            << "serial:" << serial;
}
```

---

## Configuration File

```json
// config/tr4qt.json

{
    "station": {
        "id": "A",
        "name": "N6TR-Alpha",
        "operator": "Larry N6TR",
        "contest": "CQ WW CW"
    },

    "network": {
        "mode": "peer-to-peer",
        "listen_port": 7300,

        "peers": [
            {"id": "B", "host": "192.168.1.101", "port": 7301},
            {"id": "C", "host": "192.168.1.102", "port": 7302},
            {"id": "D", "host": "192.168.1.103", "port": 7303}
        ],

        "heartbeat_interval_ms": 5000,
        "connection_timeout_ms": 15000
    },

    "raft": {
        "enabled": true,
        "election_timeout_min_ms": 150,
        "election_timeout_max_ms": 300,
        "heartbeat_interval_ms": 100,
        "snapshot_interval": 1000,
        "log_dir": "raft_data/"
    },

    "serial": {
        "strategy": "hybrid",
        "range_start": 1,
        "range_size": 1000,
        "preferred_coordinator": "A"
    },

    "crdt": {
        "sync_interval_ms": 10000,
        "auto_merge": true
    },

    "partition": {
        "detection_enabled": true,
        "check_interval_ms": 2000,
        "auto_heal": true
    }
}
```

---

# Build System

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(TR4QT VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

# Protocol Buffers
find_package(Protobuf REQUIRED)

# Generate protobuf
set(PROTO_FILES
    src/network/protocol/tr4qt_p2p.proto
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Source files
set(SOURCES
    src/main.cpp
    src/common/qso_record.cpp
    src/crdt/log_crdt.cpp
    src/crdt/partition_merger.cpp
    src/raft/raft_node.cpp
    src/raft/raft_state.cpp
    src/network/peer_connection.cpp
    src/network/peer_manager.cpp
    src/network/partition_detector.cpp
    src/network/message_codec.cpp
    src/serial/serial_allocator.cpp
    ${PROTO_SRCS}
)

# Executable
add_executable(tr4qt ${SOURCES})

target_link_libraries(tr4qt
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    protobuf::libprotobuf
)

target_include_directories(tr4qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated protobuf headers
)

# Tests
enable_testing()
add_subdirectory(tests)
```

---

# Summary

This implementation guide provides:

1. **Simplified data structures** - GUID-based QSO records (no vector clocks, no version tracking)
2. **Trivial CRDT merge** - Just add GUIDs we don't have (no conflict resolution)
3. **Raft consensus** - For serial number allocation only (separate concern)
4. **Network protocol** - Protocol Buffers with simplified schema
5. **Concrete implementations** - Full C++/Qt source code
6. **State machines** - Raft consensus, simplified CRDT merge
7. **Testing approach** - Unit, integration, and chaos tests
8. **Build configuration** - CMake with Qt6 and Protocol Buffers

## Key Architectural Decisions

### Two Separate Problems

1. **QSO Synchronization:**
   - GUID-based identity (QUuid::createUuid())
   - Trivial CRDT merge: `if (!have(guid)) add(qso)`
   - No conflict resolution (GUIDs ensure uniqueness)
   - Duplicate detection at scoring time (contest rules)

2. **Serial Number Allocation:**
   - Raft consensus for strong consistency
   - Leader allocates serials sequentially
   - Replicated log ensures no duplicates
   - Completely orthogonal to QSO sync

### What We Removed

- ❌ Vector clocks (unnecessary - GUIDs solve identity)
- ❌ Version tracking (unnecessary - no concurrent edits of same QSO)
- ❌ Conflict resolution (unnecessary - GUIDs prevent conflicts)
- ❌ Happens-before analysis (unnecessary - different QSOs = different GUIDs)

### What We Kept

- ✅ CRDT semantics (eventual consistency, partition tolerance)
- ✅ Raft consensus (serial allocation with strong consistency)
- ✅ Partition detection and automatic merge
- ✅ Tombstone deletions (soft delete with `deleted` flag)

**This document contains everything needed to implement TR4QT's simplified hybrid P2P networking from scratch.**

## Implementation Steps

1. Implement Protocol Buffers schema (simplified - no VectorClockProto)
2. Build LogCRDT class (trivial merge - ~50 lines of code)
3. Build DupeChecker (contest rules - separate from sync)
4. Build Raft consensus module (serial allocation)
5. Integrate Qt networking (PeerConnection, PeerManager)
6. Add UI layer
7. Test extensively

**Estimated Implementation Time:**
- Phase 1 (Basic P2P + CRDT): **1-2 months** (simplified!)
- Phase 2 (Raft): 3-4 months
- Phase 3 (Partition handling): 1 month
- **Total: 5-7 months** (reduced from 7-9 months due to simplification)

**Document Version:** 2.0 (Simplified GUID-based Architecture)
**Last Updated:** December 29, 2025
**Maintained By:** TR4QT Architecture Team
