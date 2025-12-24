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

    bool isEnabled() const { return m_enabled; }
    bool isRadioInfoEnabled() const { return m_radioInfoEnabled; }
    bool isContactInfoEnabled() const { return m_contactInfoEnabled; }
    int throttleInterval() const { return m_throttleInterval; }

    // Get broadcaster for direct access (e.g., for multicast configuration)
    UdpBroadcaster* broadcaster() { return m_broadcaster; }

public slots:
    /**
     * Called from MainWindow when radio state changes
     * Queues state for throttled broadcast
     */
    void onRadioStateChanged(const RadioState& state, const QString& stationCall);

    /**
     * Called from MainWindow when new QSO is logged
     * Sends immediately (no throttling)
     */
    void onQSOLogged(const QSO& qso, const QString& stationCall, const QString& contestName);

signals:
    void errorOccurred(const QString& error);
    void messageSent(const QString& messageType, int destinationCount);

private slots:
    void onThrottleTimeout();

private:
    UdpBroadcaster* m_broadcaster;

    // Throttling for RadioInfo messages
    QTimer* m_throttleTimer;
    RadioState m_pendingRadioState;
    QString m_pendingStationCall;
    bool m_hasPendingRadioState{false};
    RadioState m_lastSentRadioState;

    // Enable/disable flags
    bool m_enabled{false};
    bool m_radioInfoEnabled{true};
    bool m_contactInfoEnabled{true};
    int m_throttleInterval{500};  // ms

    // Helper methods

    /**
     * Create RadioInfo message from RadioState
     */
    RadioInfo createRadioInfo(const RadioState& state, const QString& stationCall);

    /**
     * Create ContactInfo message from QSO
     */
    ContactInfo createContactInfo(const QSO& qso, const QString& stationCall,
                                 const QString& contestName);

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
