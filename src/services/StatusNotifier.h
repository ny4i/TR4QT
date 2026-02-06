/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef STATUSNOTIFIER_H
#define STATUSNOTIFIER_H

#include <QObject>
#include <QString>
#include <QVariant>

namespace TR4QT {

/**
 * Status event types for centralized messaging.
 * Use-cases and services emit these events; UI subscribes to display them.
 */
enum class StatusEvent {
    // General
    Ready,
    Error,
    Warning,

    // Radio
    RadioConnecting,
    RadioConnected,
    RadioDisconnected,
    RadioConfigSaved,
    RadioNotConfigured,
    CWSpeedChanged,
    CWAborted,
    CWAutoSendOff,

    // Contest
    ContestCreated,
    ContestResumed,
    ContestReopened,
    NoActiveContest,

    // QSO Logging
    QSOLogged,
    QSODuplicate,
    QSOError,

    // Export/Import
    ExportComplete,
    ImportComplete,
    BackupCreated,
    LogCleared,

    // Maintenance
    RescoreStarted,
    RescoreComplete,
    IntegrityCheckStarted,
    IntegrityCheckComplete,
    ExchangeUpdateStarted,
    ExchangeUpdateComplete,
    PointsRecalculated,

    // Data Downloads
    CTYUpdateComplete,
    CTYUpdateFailed,
    LOTWDownloadComplete,
    LOTWDownloadFailed,
    SCPDownloadComplete,
    SCPDownloadFailed,

    // Web Server
    WebServerStarted,
    WebServerStopped,
    WebServerFailed,

    // UDP Broadcast
    UDPBroadcastStarted,
    UDPBroadcastProgress,
    UDPBroadcastComplete,
    UDPBroadcastDisabled,

    // SO2R
    SO2RNotEnabled,
    SO2REnabled,

    // Operator
    OperatorChanged,
    OperatorChangeCancelled,

    // Frequency/Band
    FrequencyError,
    FrequencySet,
    BandManuallySet,

    // Search
    SearchCancelled,
    SearchComplete,

    // Preferences
    PreferencesSaved,
    WindowPositionsReset
};

/**
 * Style hint for status message display
 */
enum class StatusStyle {
    Normal,     // Default style
    Warning,    // Orange/yellow, bold
    Error,      // Red, bold
    Success,    // Green
    Highlight   // Bold, theme color
};

/**
 * StatusNotifier - Centralized service for user-facing status messages.
 *
 * Extracted from MainWindow as part of Issue #75.
 *
 * Benefits:
 * - Centralized message formatting
 * - Domain code doesn't know about UI
 * - Consistent message styling
 * - Single place for status bar policy
 * - Easier to add notification features (sounds, popups)
 *
 * Usage:
 *   // In service/controller:
 *   m_statusNotifier->notify(StatusEvent::QSOLogged, qso.callsign);
 *
 *   // In MainWindow:
 *   connect(m_statusNotifier, &StatusNotifier::statusChanged,
 *           this, &MainWindow::onStatusChanged);
 */
class StatusNotifier : public QObject {
    Q_OBJECT

public:
    static StatusNotifier& instance();

    /**
     * Notify a status event with optional context data
     * @param event The status event type
     * @param data Optional context (callsign, count, error message, etc.)
     */
    void notify(StatusEvent event, const QVariant& data = QVariant());

    /**
     * Notify with a custom message (for cases not covered by events)
     * @param message The status message
     * @param style Optional style hint
     */
    void notifyCustom(const QString& message, StatusStyle style = StatusStyle::Normal);

    /**
     * Format a status event into a display message
     * @param event The event type
     * @param data Optional context data
     * @return Formatted message string
     */
    static QString formatMessage(StatusEvent event, const QVariant& data = QVariant());

    /**
     * Get the style for a status event
     * @param event The event type
     * @return Style hint for display
     */
    static StatusStyle styleForEvent(StatusEvent event);

signals:
    /**
     * Emitted when a status update occurs
     * @param message The formatted message to display
     * @param style Style hint for display
     */
    void statusChanged(const QString& message, StatusStyle style);

private:
    explicit StatusNotifier(QObject* parent = nullptr);
    ~StatusNotifier() override = default;

    // Singleton - prevent copying
    StatusNotifier(const StatusNotifier&) = delete;
    StatusNotifier& operator=(const StatusNotifier&) = delete;
};

} // namespace TR4QT

#endif // STATUSNOTIFIER_H
