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

#ifndef SPOTCOLLECTORSERVICE_H
#define SPOTCOLLECTORSERVICE_H

#include <QObject>
#include <QString>
#include <functional>
#include "DXLabPathfinder.h"

namespace TR4QT {

/**
 * SpotCollectorService - DXLab SpotCollector DDE integration (Windows only)
 *
 * Wraps the DXLabPathfinder DDE bridge and manages the state of
 * callsigns received from SpotCollector. Emits signals for MainWindow
 * to populate the callsign field and optionally QSY.
 *
 * On non-Windows platforms, this is a stub that does nothing.
 *
 * Extracted from MainWindow to keep external data interfaces in their
 * own service modules (similar to UdpBroadcastManager).
 *
 * Usage:
 *   auto* service = new SpotCollectorService(this);
 *   connect(service, &SpotCollectorService::callsignReceived,
 *           this, [](const QString& call) { m_callsignEntry->setText(call); });
 *   connect(service, &SpotCollectorService::qsyRequested,
 *           this, [](double freq) { m_radio->setFrequency(freq); });
 *   service->loadSettings();
 */
class SpotCollectorService : public QObject {
    Q_OBJECT

public:
    explicit SpotCollectorService(QObject* parent = nullptr);
    ~SpotCollectorService() override;

    /**
     * Read settings and start/stop the DDE bridge accordingly.
     * Call this on startup and after preferences change.
     */
    void loadSettings();

    /**
     * Check if a callsign was set by DDE and the user hasn't engaged yet.
     * MainWindow uses this to decide whether to overwrite the callsign field.
     */
    bool isCallsignFromDDE() const { return m_callsignFromDDE; }

    /**
     * Reset the DDE callsign flag. Call this when the user engages
     * with the contact (types in callsign or exchange, presses Enter, etc.).
     */
    void resetUserEngagement() { m_callsignFromDDE = false; }

    /**
     * Check if the DDE bridge is currently running.
     */
    bool isRunning() const;

    /**
     * Provide a callback for looking up spot frequencies by callsign.
     * Used for QSY when SpotCollector sends a callsign.
     * @param callback Returns frequency in Hz > 0 if callsign found, 0 if not
     */
    using FrequencyLookupCallback = std::function<double(const QString& callsign)>;
    void setFrequencyLookup(FrequencyLookupCallback callback);

    /**
     * Update the empty-fields state. MainWindow calls this when
     * callsign/exchange text changes so the service knows whether
     * to accept or ignore incoming DDE callsigns.
     */
    void setFieldsEmpty(bool empty) { m_fieldsEmpty = empty; }

signals:
    /**
     * A callsign was received from SpotCollector.
     * MainWindow should populate the callsign entry field.
     * Only emitted when the entry is empty or was previously set by DDE
     * (i.e., won't interrupt the user working a contact).
     *
     * @param callsign The callsign from SpotCollector
     */
    void callsignReceived(const QString& callsign);

    /**
     * QSY requested based on SpotCollector callsign + band map lookup.
     * Only emitted when QSY is enabled in settings AND the callsign was
     * found in the band map with a known frequency.
     *
     * @param frequency Frequency in Hz to QSY to
     */
    void qsyRequested(double frequency);

    /**
     * Error from the DDE bridge (informational, non-fatal).
     */
    void error(const QString& message);

private slots:
    void onPathfinderCallsign(const QString& callsign);
    void onPathfinderError(const QString& message);

private:
    DXLabPathfinder* m_pathfinder;
    bool m_callsignFromDDE{false};
    bool m_qsyEnabled{false};  // Cached from AppSettings in loadSettings()
    FrequencyLookupCallback m_frequencyLookup;

    // Tracks whether entry fields are empty (set via signal from MainWindow)
    // Used to decide if we should accept DDE callsigns
    bool m_fieldsEmpty{true};
};

} // namespace TR4QT

#endif // SPOTCOLLECTORSERVICE_H
