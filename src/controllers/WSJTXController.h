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

#ifndef WSJTXCONTROLLER_H
#define WSJTXCONTROLLER_H

#include <QObject>
#include <QSet>
#include <QThread>
#include "../core/Types.h"
#include "../models/QSO.h"
#include "../network/WSJTXMessage.h"
#include "../network/WSJTXUdpClient.h"
#include "../services/WSJTXHighlightWorker.h"

namespace TR4QT {

class CountryFile;
class ContestBase;

/**
 * Orchestrator for WSJT-X integration.
 *
 * Owns:
 *   - WSJTXUdpClient (main thread) for UDP send/receive
 *   - QThread + WSJTXHighlightWorker for dupe/mult checking
 *
 * Wiring:
 *   1. Heartbeat → emit wsjtxConnected/wsjtxDisconnected
 *   2. Decode → extract callsign → worker → highlight back to WSJT-X
 *   3. QSOLogged → convert to QSO → emit qsoReady
 *   4. Status → if dxCall changed → emit dxCallChanged
 */
class WSJTXController : public QObject {
    Q_OBJECT

public:
    explicit WSJTXController(QObject* parent = nullptr);
    ~WSJTXController() override;

    /**
     * Set the country file reference for DXCC enrichment.
     * Must be called before start(). Not owned.
     */
    void setCountryFile(CountryFile* countryFile);

    /**
     * Start listening on the specified UDP port.
     */
    bool start(quint16 port, const QString& multicastGroup = QString());

    /**
     * Stop listening.
     */
    void stop();

    /**
     * Update contest context for dupe/mult highlighting.
     */
    void setContestContext(int contestDbId,
                           const QString& contestDbPath,
                           const QList<MultiplierDefinition>& multDefs,
                           DuplicateCheckingRule dupeRule);

    /**
     * Notify that a QSO was logged (updates dupe cache incrementally).
     */
    void onQSOLogged(const QSO& qso);

    /**
     * Whether WSJT-X is currently connected.
     */
    bool isConnected() const;

    /**
     * Enable/disable auto-logging of WSJT-X QSOs.
     */
    void setAutoLogEnabled(bool enabled) { m_autoLogEnabled = enabled; }
    bool autoLogEnabled() const { return m_autoLogEnabled; }

    /**
     * Enable/disable callsign highlighting.
     */
    void setHighlightEnabled(bool enabled) { m_highlightEnabled = enabled; }
    bool highlightEnabled() const { return m_highlightEnabled; }

signals:
    /**
     * A QSO is ready to be logged (from WSJT-X "Log QSO" button).
     * MainWindow connects this to its QSO logging pipeline.
     */
    void qsoReady(const TR4QT::QSO& qso);

    /**
     * WSJT-X's DX call field changed (user selected a station).
     */
    void dxCallChanged(const QString& callsign, quint64 frequencyHz);

    /**
     * WSJT-X connected (heartbeat received).
     */
    void wsjtxConnected(const QString& id, const QString& version);

    /**
     * WSJT-X disconnected (heartbeat timeout or Close message).
     */
    void wsjtxDisconnected();

private slots:
    void onHeartbeat(const WSJTXHeartbeat& msg);
    void onStatus(const WSJTXStatus& msg);
    void onDecode(const WSJTXDecode& msg);
    void onQSOLoggedFromWSJTX(const WSJTXQSOLogged& msg);
    void onConnectionLost();
    void onHighlightDecision(const WSJTXHighlightDecision& decision);

private:
    WSJTXUdpClient* m_client{nullptr};
    QThread m_workerThread;
    WSJTXHighlightWorker* m_worker{nullptr};

    CountryFile* m_countryFile{nullptr};  // Not owned

    // Track active highlights to manage WSJT-X's ~100 highlight limit
    QSet<QString> m_activeHighlights;
    static constexpr int MAX_HIGHLIGHTS = 90;  // Clear before hitting WSJT-X's 100 limit

    // Last known status for dedup
    QString m_lastDxCall;
    quint64 m_lastDialFrequency{0};
    QString m_lastMode;

    bool m_autoLogEnabled{true};
    bool m_highlightEnabled{true};
};

} // namespace TR4QT

#endif // WSJTXCONTROLLER_H
