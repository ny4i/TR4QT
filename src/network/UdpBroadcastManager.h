#ifndef UDPBROADCASTMANAGER_H
#define UDPBROADCASTMANAGER_H

#include <QObject>
#include <QTimer>
#include "UdpBroadcaster.h"
#include "../radio/RadioInterface.h"
#include "../models/QSO.h"

namespace TR4QT {

/**
 * UdpBroadcastManager - High-level coordinator for UDP broadcasts
 *
 * Manages N1MM+ compatible UDP broadcasts with intelligent throttling:
 * - RadioInfo messages: Throttled to avoid flooding (default 500ms)
 * - ContactInfo messages: Sent immediately when QSO logged
 *
 * Integrates with MainWindow to automatically broadcast:
 * - Radio state changes (frequency, mode, PTT, split, etc.)
 * - New QSO entries
 *
 * Features:
 * - Configurable throttle interval
 * - Individual enable/disable for RadioInfo and ContactInfo
 * - Global enable/disable toggle
 * - Automatic state change detection
 * - Support for multiple UDP destinations
 */
class UdpBroadcastManager : public QObject {
    Q_OBJECT

public:
    explicit UdpBroadcastManager(QObject* parent = nullptr);
    ~UdpBroadcastManager() override;

    // Configuration
    void setEnabled(bool enabled);
    void setRadioInfoEnabled(bool enabled);
    void setContactInfoEnabled(bool enabled);
    void setThrottleInterval(int milliseconds);  // Default 500ms
    void setDestinations(const QList<UdpDestination>& destinations);
    void setOperatingMode(bool isRunMode);  // CQ/Run mode (true) vs S&P mode (false)

    bool isEnabled() const { return m_enabled; }
    bool isRadioInfoEnabled() const { return m_radioInfoEnabled; }
    bool isContactInfoEnabled() const { return m_contactInfoEnabled; }
    int throttleInterval() const { return m_throttleInterval; }

    // Get broadcaster for direct access (e.g., for multicast configuration)
    UdpBroadcaster* broadcaster() { return m_broadcaster; }

    /**
     * Set active radio index for SO2R (affects focusRadioNr/activeRadioNr in messages)
     * @param index Active radio index (0 or 1)
     */
    void setActiveRadioIndex(int index) { m_activeRadioIndex = (index >= 0 && index < 2) ? index : 0; }

public slots:
    /**
     * Called from MainWindow when radio state changes (legacy, single radio)
     * Queues state for throttled broadcast
     */
    void onRadioStateChanged(const RadioState& state, const QString& stationCall);

    /**
     * Called when a specific radio's state changes (SO2R support)
     * Queues state for throttled broadcast with correct radio number
     * @param radioIndex Radio index (0 or 1)
     * @param state New radio state
     * @param stationCall Station callsign
     */
    void onRadioStateChangedIndexed(int radioIndex, const RadioState& state, const QString& stationCall);

    /**
     * Called from MainWindow when new QSO is logged
     * Sends immediately (no throttling)
     * @param qso The logged QSO
     * @param stationCall Station callsign
     * @param adifContestId ADIF Contest-ID (e.g., "CQ-WW-CW")
     * @param wa7bnmContestId WA7BNM Contest Calendar ID (e.g., 4 for CQ WW CW)
     */
    void onQSOLogged(const QSO& qso, const QString& stationCall,
                     const QString& adifContestId, int wa7bnmContestId);

signals:
    void errorOccurred(const QString& error);
    void messageSent(const QString& messageType, int destinationCount);

private slots:
    void onThrottleTimeout();
    void onHeartbeatTimeout();

private:
    static constexpr int MAX_RADIOS = 2;  // SO2R support
    static constexpr int HEARTBEAT_INTERVAL_MS = 10000;  // 10 second heartbeat

    UdpBroadcaster* m_broadcaster;

    // Throttling for RadioInfo messages (per-radio for SO2R)
    QTimer* m_throttleTimer;
    QTimer* m_heartbeatTimer;  // Periodic heartbeat (every 10 seconds)
    RadioState m_pendingRadioState[MAX_RADIOS];
    QString m_pendingStationCall;
    bool m_hasPendingRadioState[MAX_RADIOS]{false, false};
    RadioState m_lastSentRadioState[MAX_RADIOS];
    qint64 m_lastSendTime{0};  // Track when last message was sent

    // Enable/disable flags
    bool m_enabled{false};
    bool m_radioInfoEnabled{true};
    bool m_contactInfoEnabled{true};
    int m_throttleInterval{500};  // ms
    bool m_isRunMode{false};  // CQ/Run mode (true) vs S&P mode (false)
    int m_activeRadioIndex{0};  // Active radio for SO2R (0 or 1)

    // Helper methods

    /**
     * Create RadioInfo message from RadioState
     * @param state Radio state
     * @param stationCall Station callsign
     * @param radioIndex Radio index (0 or 1) for radioNr field
     */
    RadioInfo createRadioInfo(const RadioState& state, const QString& stationCall, int radioIndex = 0);

    /**
     * Create ContactInfo message from QSO
     */
    ContactInfo createContactInfo(const QSO& qso, const QString& stationCall,
                                 const QString& adifContestId, int wa7bnmContestId);

    /**
     * Check if radio state has changed significantly
     * (ignores minor changes that don't warrant a broadcast)
     */
    bool hasRadioStateChanged(const RadioState& a, const RadioState& b);

    /**
     * Get N1MM+ compatible mode string from ModeType
     * Maps TR4QT modes to N1MM+ expected values
     */
    QString getModeString(ModeType mode);
};

} // namespace TR4QT

#endif // UDPBROADCASTMANAGER_H
