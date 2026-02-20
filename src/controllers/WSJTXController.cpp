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

#include "WSJTXController.h"
#include "../services/WSJTXService.h"
#include "../utils/CountryFile.h"
#include "../logging/LogMacros.h"

static const char* LOG_TAG = "WSJTXController";

namespace TR4QT {

WSJTXController::WSJTXController(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<QSO>("QSO");
    qRegisterMetaType<WSJTXHighlightDecision>("WSJTXHighlightDecision");
    qRegisterMetaType<WSJTXHeartbeat>("WSJTXHeartbeat");
    qRegisterMetaType<WSJTXStatus>("WSJTXStatus");
    qRegisterMetaType<WSJTXDecode>("WSJTXDecode");
    qRegisterMetaType<WSJTXQSOLogged>("WSJTXQSOLogged");

    // Create worker for highlight checking (lives in worker thread)
    m_worker = new WSJTXHighlightWorker();
    m_worker->moveToThread(&m_workerThread);

    // Worker cleanup on thread finish
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // Worker → controller (cross-thread signal)
    connect(m_worker, &WSJTXHighlightWorker::highlightDecision,
            this, &WSJTXController::onHighlightDecision);

    m_workerThread.start();
}

WSJTXController::~WSJTXController()
{
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

void WSJTXController::setCountryFile(CountryFile* countryFile)
{
    m_countryFile = countryFile;
}

bool WSJTXController::start(quint16 port, const QString& multicastGroup)
{
    stop();

    m_client = new WSJTXUdpClient(this);

    // Connect client signals
    connect(m_client, &WSJTXUdpClient::heartbeatReceived,
            this, &WSJTXController::onHeartbeat);
    connect(m_client, &WSJTXUdpClient::statusReceived,
            this, &WSJTXController::onStatus);
    connect(m_client, &WSJTXUdpClient::decodeReceived,
            this, &WSJTXController::onDecode);
    connect(m_client, &WSJTXUdpClient::qsoLoggedReceived,
            this, &WSJTXController::onQSOLoggedFromWSJTX);
    connect(m_client, &WSJTXUdpClient::connectionEstablished,
            this, &WSJTXController::wsjtxConnected);
    connect(m_client, &WSJTXUdpClient::connectionLost,
            this, &WSJTXController::onConnectionLost);

    if (!m_client->bind(port, multicastGroup)) {
        delete m_client;
        m_client = nullptr;
        return false;
    }

    LOG_INFO(LOG_TAG, QString("Started WSJT-X integration on port %1").arg(port));
    return true;
}

void WSJTXController::stop()
{
    if (m_client) {
        m_client->stop();
        delete m_client;
        m_client = nullptr;
    }
    m_activeHighlights.clear();
    m_lastDxCall.clear();
    m_lastDialFrequency = 0;
}

void WSJTXController::setContestContext(int contestDbId,
                                          const QString& contestDbPath,
                                          const QList<MultiplierDefinition>& multDefs,
                                          DuplicateCheckingRule dupeRule)
{
    // Initialize worker database from worker thread
    QMetaObject::invokeMethod(m_worker, [this, contestDbPath]() {
        m_worker->initDatabase(contestDbPath);
    }, Qt::QueuedConnection);

    // Update contest context in worker
    QMetaObject::invokeMethod(m_worker, [this, contestDbId, multDefs, dupeRule]() {
        m_worker->setContestContext(contestDbId, multDefs, dupeRule);
    }, Qt::QueuedConnection);

    // Clear existing highlights in WSJT-X
    if (m_client && m_client->isConnected()) {
        m_client->sendClearHighlights();
        m_activeHighlights.clear();
    }
}

void WSJTXController::onQSOLogged(const QSO& qso)
{
    // Incrementally update the worker's dupe cache
    QString band = bandToString(qso.band);
    QString mode = modeToString(qso.mode);
    QMetaObject::invokeMethod(m_worker, [this, callsign = qso.callsign, band, mode]() {
        m_worker->addWorkedCallsign(callsign, band, mode);
    }, Qt::QueuedConnection);
}

bool WSJTXController::isConnected() const
{
    return m_client && m_client->isConnected();
}

// ─── Private slots ──────────────────────────────────────────────────────────

void WSJTXController::onHeartbeat(const WSJTXHeartbeat& msg)
{
    Q_UNUSED(msg)
    // Connection tracking handled by WSJTXUdpClient
}

void WSJTXController::onStatus(const WSJTXStatus& msg)
{
    // Track dial frequency and mode for decode context
    m_lastDialFrequency = msg.dialFrequency;
    m_lastMode = msg.mode;

    // Emit dxCallChanged if the DX call field changed
    if (!msg.dxCall.isEmpty() && msg.dxCall != m_lastDxCall) {
        m_lastDxCall = msg.dxCall;
        emit dxCallChanged(msg.dxCall, msg.dialFrequency);
    } else if (msg.dxCall.isEmpty() && !m_lastDxCall.isEmpty()) {
        m_lastDxCall.clear();
    }
}

void WSJTXController::onDecode(const WSJTXDecode& msg)
{
    if (!m_highlightEnabled || !m_client)
        return;

    // Extract callsign from decoded message
    auto info = WSJTXService::extractCallsign(msg.message);
    if (!info.valid)
        return;

    // Send to worker thread for dupe/mult checking
    quint64 freq = m_lastDialFrequency;
    QString mode = m_lastMode;
    QMetaObject::invokeMethod(m_worker, [this, callsign = info.callsign, freq, mode]() {
        m_worker->checkCallsign(callsign, freq, mode);
    }, Qt::QueuedConnection);
}

void WSJTXController::onQSOLoggedFromWSJTX(const WSJTXQSOLogged& msg)
{
    if (!m_autoLogEnabled)
        return;

    QSO qso = WSJTXService::convertToQSO(msg, m_countryFile);

    LOG_INFO(LOG_TAG, QString("Auto-logging WSJT-X QSO: %1 %2 %3")
             .arg(qso.callsign, modeToString(qso.mode), bandToString(qso.band)));

    emit qsoReady(qso);
}

void WSJTXController::onConnectionLost()
{
    m_activeHighlights.clear();
    m_lastDxCall.clear();
    m_lastDialFrequency = 0;
    emit wsjtxDisconnected();
}

void WSJTXController::onHighlightDecision(const WSJTXHighlightDecision& decision)
{
    if (!m_client || !m_client->isConnected())
        return;

    if (!decision.isDupe && !decision.isMultiplier)
        return;

    // Clear highlights if approaching WSJT-X's limit
    if (m_activeHighlights.size() >= MAX_HIGHLIGHTS) {
        m_client->sendClearHighlights();
        m_activeHighlights.clear();
    }

    m_client->sendHighlightCallsign(decision.callsign,
                                      decision.bgColor,
                                      decision.fgColor);
    m_activeHighlights.insert(decision.callsign);
}

} // namespace TR4QT
