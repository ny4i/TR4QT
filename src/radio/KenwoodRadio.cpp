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

#include "KenwoodRadio.h"
#include "CWTiming.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>
#include <QEventLoop>
#include <QThread>

namespace TR4QT {

// TS-890 frequency field width (11 digits, zero-padded Hz)
static constexpr int KENWOOD_FREQ_DIGITS = 11;

// TS-890 CW KY command max characters per chunk
static constexpr int KENWOOD_CW_CHUNK_SIZE = 24;

// TS-890 S-meter range (0-30)
static constexpr int KENWOOD_SMETER_MAX = 30;

// TS-890 default LAN port
static constexpr quint16 KENWOOD_DEFAULT_LAN_PORT = 60000;

KenwoodRadio::KenwoodRadio(QObject* parent)
    : RadioInterface(parent)
{
    m_cwTimer = new QTimer(this);
    m_cwTimer->setSingleShot(true);
    QObject::connect(m_cwTimer, &QTimer::timeout, this, &KenwoodRadio::onCWTimeout);

    m_meterTimer = new QTimer(this);
    QObject::connect(m_meterTimer, &QTimer::timeout, this, &KenwoodRadio::onMeterPoll);

    m_state.isValid = false;
}

KenwoodRadio::~KenwoodRadio()
{
    disconnect();
}

void KenwoodRadio::debugTestSlot(int testValue)
{
    LOG_INFO("KenwoodRadio", QString("debugTestSlot called with value: %1").arg(testValue));
}

// ============================================================================
// Connection Management
// ============================================================================

bool KenwoodRadio::connect(const RadioConfig& config)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        LOG_WARN("KenwoodRadio", "Already connected");
        return true;
    }

    // Parse host:port from config.port (format: "192.168.1.100:60000")
    QStringList parts = config.port.split(':');
    if (parts.size() != 2) {
        LOG_ERROR("KenwoodRadio", QString("Invalid network address format: %1 (expected host:port)").arg(config.port));
        emit errorOccurred("Invalid network address format (expected host:port)");
        return false;
    }

    m_host = parts[0];
    m_port = parts[1].toUInt();

    if (m_port == 0) {
        LOG_ERROR("KenwoodRadio", QString("Invalid port number: %1").arg(parts[1]));
        emit errorOccurred("Invalid port number");
        return false;
    }

    // Store LAN credentials — skip auth if no credentials provided (e.g., simulator/serial)
    m_adminId = config.kenwoodAdminId;
    m_adminPassword = config.kenwoodAdminPassword;
    m_isLanConnection = !m_adminId.isEmpty();

    LOG_INFO("KenwoodRadio", QString("Connecting to Kenwood radio at %1:%2").arg(m_host).arg(m_port));

    m_socket = new QTcpSocket(this);

    QObject::connect(m_socket, &QTcpSocket::connected, this, &KenwoodRadio::onSocketConnected);
    QObject::connect(m_socket, &QTcpSocket::disconnected, this, &KenwoodRadio::onSocketDisconnected);
    QObject::connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                     this, &KenwoodRadio::onSocketError);
    QObject::connect(m_socket, &QTcpSocket::readyRead, this, &KenwoodRadio::onReadyRead);

    // Connect asynchronously - DO NOT use waitForConnected() as it blocks the event loop
    m_socket->connectToHost(m_host, m_port);

    LOG_DEBUG("KenwoodRadio", "Connection initiated, waiting for onSocketConnected()");
    return true;
}

void KenwoodRadio::disconnect()
{
    m_meterTimer->stop();
    m_cwTimer->stop();
    m_cwInProgress = false;

    if (m_socket) {
        LOG_INFO("KenwoodRadio", "Disconnecting from Kenwood radio");
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_authenticated = false;
    m_authState = AuthState::None;

    QMutexLocker lock(&m_stateMutex);
    m_state.isValid = false;
}

bool KenwoodRadio::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState && m_authenticated;
}

// ============================================================================
// Socket Event Handlers
// ============================================================================

void KenwoodRadio::onSocketConnected()
{
    LOG_INFO("KenwoodRadio", QString("TCP connected to %1:%2").arg(m_host).arg(m_port));

    if (m_isLanConnection) {
        // Start LAN authentication: send ##CN;
        m_authState = AuthState::WaitingForCN;
        sendCommand("##CN");
        LOG_DEBUG("KenwoodRadio", "LAN auth: sent ##CN;");
    } else {
        // Serial connection - no auth needed
        m_authenticated = true;
        m_authState = AuthState::Authenticated;
        onConnectedInitialize();
    }
}

void KenwoodRadio::onSocketDisconnected()
{
    LOG_INFO("KenwoodRadio", "TCP disconnected");
    m_authenticated = false;
    m_authState = AuthState::None;
    m_meterTimer->stop();

    QMutexLocker lock(&m_stateMutex);
    m_state.isValid = false;
    lock.unlock();

    emit connectionStatusChanged(false);
}

void KenwoodRadio::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorStr = m_socket ? m_socket->errorString() : "Unknown error";
    LOG_ERROR("KenwoodRadio", QString("Socket error %1: %2").arg(static_cast<int>(error)).arg(errorStr));
    emit errorOccurred(errorStr);
}

void KenwoodRadio::onReadyRead()
{
    if (!m_socket) return;

    m_receiveBuffer.append(QString::fromUtf8(m_socket->readAll()));

    // Split on semicolons (Kenwood command terminator)
    while (true) {
        int semicolonPos = m_receiveBuffer.indexOf(';');
        if (semicolonPos < 0) break;

        QString message = m_receiveBuffer.left(semicolonPos);
        m_receiveBuffer = m_receiveBuffer.mid(semicolonPos + 1);

        if (!message.isEmpty()) {
            processMessage(message);
        }
    }
}

// ============================================================================
// Command Processing
// ============================================================================

void KenwoodRadio::sendCommand(const QString& cmd)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        LOG_DEBUG("KenwoodRadio", QString("sendCommand('%1') called but not connected").arg(cmd));
        return;
    }

    QString fullCmd = cmd + ";";
    m_socket->write(fullCmd.toUtf8());
    LOG_TRACE("KenwoodRadio", QString("TX: %1").arg(fullCmd));
}

void KenwoodRadio::processMessage(const QString& message)
{
    LOG_TRACE("KenwoodRadio", QString("RX: %1").arg(message));

    // Handle LAN authentication responses
    if (message.startsWith("##CN")) {
        if (m_authState == AuthState::WaitingForCN) {
            if (message == "##CN1") {
                // Connection accepted, send credentials
                // Format: ##ID0<idLen><pwLen><adminId><password>
                QString idLen = QString("%1").arg(m_adminId.length(), 2, 10, QChar('0'));
                QString pwLen = QString("%1").arg(m_adminPassword.length(), 2, 10, QChar('0'));
                QString idCmd = QString("##ID0%1%2%3%4").arg(idLen, pwLen, m_adminId, m_adminPassword);
                m_authState = AuthState::WaitingForID;
                sendCommand(idCmd);
                LOG_DEBUG("KenwoodRadio", "LAN auth: received ##CN1, sent ##ID credentials");
            } else {
                LOG_ERROR("KenwoodRadio", QString("LAN auth: unexpected ##CN response: %1").arg(message));
                emit errorOccurred("LAN authentication failed: unexpected response from radio");
            }
        }
        return;
    }

    if (message.startsWith("##ID")) {
        if (m_authState == AuthState::WaitingForID) {
            if (message == "##ID1") {
                // Authentication successful
                m_authenticated = true;
                m_authState = AuthState::Authenticated;
                LOG_INFO("KenwoodRadio", "LAN authentication successful");
                onConnectedInitialize();
            } else {
                LOG_ERROR("KenwoodRadio", QString("LAN auth failed: %1").arg(message));
                emit errorOccurred("LAN authentication failed: invalid credentials");
            }
        }
        return;
    }

    // Skip processing if not authenticated yet
    if (!m_authenticated) {
        LOG_TRACE("KenwoodRadio", QString("Ignoring message before auth: %1").arg(message));
        return;
    }

    // Handle error responses
    if (message == "?") {
        LOG_WARN("KenwoodRadio", "Command error (?) received");
        return;
    }

    // Handle radio restart notification
    if (message == "00") {
        LOG_WARN("KenwoodRadio", "Radio restart notification received");
        return;
    }

    // Parse command prefix (first 2 characters)
    if (message.length() < 2) return;
    QString prefix = message.left(2);
    QString data = message.mid(2);

    if (prefix == "FA") {
        // VFO A frequency: FA followed by 11 digits
        if (data.length() >= KENWOOD_FREQ_DIGITS) {
            freq_t freq = data.left(KENWOOD_FREQ_DIGITS).toLongLong();
            QMutexLocker lock(&m_stateMutex);
            m_state.frequencyA = freq;
            m_state.bandA = frequencyToBand(freq);
            updateBandMemory(freq);
            lock.unlock();
            emit frequencyChanged(freq, VFO::VFO_A);
            LOG_TRACE("KenwoodRadio", QString("VFO A freq: %1 Hz").arg(freq));
        }
    } else if (prefix == "FB") {
        // VFO B frequency: FB followed by 11 digits
        if (data.length() >= KENWOOD_FREQ_DIGITS) {
            freq_t freq = data.left(KENWOOD_FREQ_DIGITS).toLongLong();
            QMutexLocker lock(&m_stateMutex);
            m_state.frequencyB = freq;
            m_state.bandB = frequencyToBand(freq);
            lock.unlock();
            emit frequencyChanged(freq, VFO::VFO_B);
            LOG_TRACE("KenwoodRadio", QString("VFO B freq: %1 Hz").arg(freq));
        }
    } else if (prefix == "OM") {
        // Mode: OM<P1><P2> where P1=VFO (0=A, 1=B), P2=mode char
        if (data.length() >= 2) {
            QString vfoStr = data.left(1);
            QString modeChar = data.mid(1, 1);
            VFO vfo = (vfoStr == "1") ? VFO::VFO_B : VFO::VFO_A;
            ModeType mode = kenwoodModeToMode(modeChar);

            QMutexLocker lock(&m_stateMutex);
            if (vfo == VFO::VFO_A) {
                m_state.modeA = mode;
            } else {
                m_state.modeB = mode;
            }
            lock.unlock();
            emit modeChanged(mode, vfo);
            LOG_TRACE("KenwoodRadio", QString("Mode VFO %1: %2 (char '%3')")
                .arg(vfoStr).arg(modeToString(mode)).arg(modeChar));
        }
    } else if (prefix == "KS") {
        // CW speed: KS followed by 3 digits (WPM)
        if (data.length() >= 3) {
            int wpm = data.left(3).toInt();
            QMutexLocker lock(&m_stateMutex);
            m_state.cwSpeed = wpm;
            LOG_TRACE("KenwoodRadio", QString("CW speed: %1 WPM").arg(wpm));
        }
    } else if (prefix == "TX") {
        // Transmitting: TX<P1> where P1: 0=SEND/PTT, 1=DATA, 2=TUNE
        QMutexLocker lock(&m_stateMutex);
        m_state.isTransmitting = true;
        lock.unlock();
        emit pttChanged(true);
        LOG_TRACE("KenwoodRadio", "TX on");
    } else if (prefix == "RX") {
        // Receiving
        QMutexLocker lock(&m_stateMutex);
        m_state.isTransmitting = false;
        lock.unlock();
        emit pttChanged(false);
        LOG_TRACE("KenwoodRadio", "TX off (RX)");
    } else if (prefix == "TB") {
        // Split toggle: TB<P1> where P1: 0=off, 1=on
        if (data.length() >= 1) {
            bool splitOn = (data.at(0) == '1');
            QMutexLocker lock(&m_stateMutex);
            m_state.isSplitEnabled = splitOn;
            lock.unlock();
            emit splitChanged(splitOn);
            LOG_TRACE("KenwoodRadio", QString("Split: %1").arg(splitOn ? "ON" : "OFF"));
        }
    } else if (prefix == "FT") {
        // TX VFO select: FT<P1> where P1: 0=A, 1=B
        if (data.length() >= 1) {
            bool txOnB = (data.at(0) == '1');
            LOG_TRACE("KenwoodRadio", QString("TX VFO: %1").arg(txOnB ? "B" : "A"));
            // If TX is on B, split is effectively enabled
            if (txOnB) {
                QMutexLocker lock(&m_stateMutex);
                m_state.isSplitEnabled = true;
                lock.unlock();
                emit splitChanged(true);
            }
        }
    } else if (prefix == "RT") {
        // RIT on/off: RT<P1> where P1: 0=off, 1=on
        if (data.length() >= 1) {
            bool ritOn = (data.at(0) == '1');
            QMutexLocker lock(&m_stateMutex);
            m_state.isRitEnabled = ritOn;
            lock.unlock();
            emit ritChanged(m_state.ritOffsetA, VFO::VFO_A);
            LOG_TRACE("KenwoodRadio", QString("RIT: %1").arg(ritOn ? "ON" : "OFF"));
        }
    } else if (prefix == "XT") {
        // XIT on/off: XT<P1> where P1: 0=off, 1=on
        if (data.length() >= 1) {
            bool xitOn = (data.at(0) == '1');
            QMutexLocker lock(&m_stateMutex);
            m_state.isXitEnabled = xitOn;
            lock.unlock();
            emit xitChanged(m_state.xitOffsetA, VFO::VFO_A);
            LOG_TRACE("KenwoodRadio", QString("XIT: %1").arg(xitOn ? "ON" : "OFF"));
        }
    } else if (prefix == "RD" || prefix == "RU") {
        // RIT/XIT offset: RD/RU<offset> (down/up direction indicator)
        // The actual offset comes from polling IF or RIT commands
        LOG_TRACE("KenwoodRadio", QString("RIT/XIT offset change: %1%2").arg(prefix, data));
    } else if (prefix == "SM") {
        // S-meter / metering: SM<P1><4-digit value>
        // P1: 0=S-meter A, 1=S-meter B, 2=power, 3=ALC, 4=SWR, 5=compression
        if (data.length() >= 5) {
            int meterType = data.left(1).toInt();
            int value = data.mid(1, 4).toInt();

            QMutexLocker lock(&m_stateMutex);
            switch (meterType) {
                case 0:  // S-meter VFO A (0-30)
                    m_state.signalStrength = value;
                    LOG_TRACE("KenwoodRadio", QString("S-meter A: %1").arg(value));
                    break;
                case 2:  // Power output (0-30 → percentage of max)
                    if (KENWOOD_SMETER_MAX > 0) {
                        int watts = (value * maxPowerWatts()) / KENWOOD_SMETER_MAX;
                        m_state.powerOutput = watts * 10;  // Store as tenths of watts
                    }
                    LOG_TRACE("KenwoodRadio", QString("Power meter: raw=%1").arg(value));
                    break;
                case 4: {  // SWR (0-30)
                    // Approximate SWR: 0=1.0, 7=1.5, 14=2.0, 21=3.0, 30=inf
                    int swrTenths = 10;  // 1.0:1 default
                    if (value <= 7) {
                        swrTenths = 10 + (value * 5) / 7;  // 1.0 to 1.5
                    } else if (value <= 14) {
                        swrTenths = 15 + ((value - 7) * 5) / 7;  // 1.5 to 2.0
                    } else if (value <= 21) {
                        swrTenths = 20 + ((value - 14) * 10) / 7;  // 2.0 to 3.0
                    } else {
                        swrTenths = 30 + ((value - 21) * 70) / 9;  // 3.0+
                    }
                    m_state.swr = swrTenths;
                    LOG_TRACE("KenwoodRadio", QString("SWR meter: raw=%1 → %2:1").arg(value).arg(swrTenths / 10.0, 0, 'f', 1));
                    break;
                }
                case 5:  // Compression
                    m_state.compressionLevel = value;
                    LOG_TRACE("KenwoodRadio", QString("Compression: %1").arg(value));
                    break;
                default:
                    LOG_TRACE("KenwoodRadio", QString("Meter type %1: %2").arg(meterType).arg(value));
                    break;
            }
        }
    } else if (prefix == "ID") {
        // Radio ID: ID<3-digit model code>
        LOG_INFO("KenwoodRadio", QString("Radio ID: %1").arg(data));
        // Verify model matches expected
        QString expectedId = radioIdString();
        if (!expectedId.isEmpty() && data != expectedId) {
            LOG_WARN("KenwoodRadio", QString("Radio ID mismatch: expected %1, got %2").arg(expectedId, data));
        }
    } else {
        // Unhandled command
        LOG_TRACE("KenwoodRadio", QString("Unhandled: %1%2").arg(prefix, data));
    }

    // Emit full state update periodically (individual signals already emitted above)
    // Full state sync happens via meter polling if enabled
}

// ============================================================================
// RadioInterface Implementation - Frequency Control
// ============================================================================

bool KenwoodRadio::setFrequency(freq_t freq, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set frequency: not connected");
        return false;
    }

    // FA/FB with 11-digit zero-padded Hz
    QString prefix = (vfo == VFO::VFO_A) ? "FA" : "FB";
    sendCommand(QString("%1%2").arg(prefix).arg(static_cast<qint64>(freq), KENWOOD_FREQ_DIGITS, 10, QChar('0')));

    return true;
}

bool KenwoodRadio::setBand(BandType band, VFO vfo)
{
    // Kenwood TS-890 doesn't have a discrete band command like K4's BN
    // Use base class band memory to recall last frequency for this band
    freq_t freqKHz = bandToBaseFrequency(band);
    freq_t fallback = freqKHz * 1000;
    freq_t targetFreq = getLastFrequencyForBand(band, fallback);

    return setFrequency(targetFreq, vfo);
}

// ============================================================================
// RadioInterface Implementation - Mode Control
// ============================================================================

bool KenwoodRadio::setMode(ModeType mode, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set mode: not connected");
        return false;
    }

    QString modeChar = modeToKenwoodMode(mode);
    if (modeChar.isEmpty()) {
        LOG_WARN("KenwoodRadio", QString("Unsupported mode: %1").arg(static_cast<int>(mode)));
        return false;
    }

    // OM<VFO><mode> where VFO is 0=A, 1=B
    QString vfoStr = (vfo == VFO::VFO_A) ? "0" : "1";
    sendCommand(QString("OM%1%2").arg(vfoStr, modeChar));

    return true;
}

// ============================================================================
// RadioInterface Implementation - PTT Control
// ============================================================================

bool KenwoodRadio::setPTT(bool transmit)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set PTT: not connected");
        return false;
    }

    if (transmit) {
        sendCommand("TX0");  // TX via SEND/PTT
    } else {
        sendCommand("RX");
    }

    return true;
}

// ============================================================================
// RadioInterface Implementation - CW Control
// ============================================================================

bool KenwoodRadio::sendCW(const QString& text)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot send CW: not connected");
        return false;
    }

    if (text.isEmpty()) {
        return true;
    }

    // Kenwood KY command: KY <text>; (space then up to 24 characters)
    // For longer text, we send in chunks
    QString remaining = text;
    while (!remaining.isEmpty()) {
        QString chunk = remaining.left(KENWOOD_CW_CHUNK_SIZE);
        remaining = remaining.mid(KENWOOD_CW_CHUNK_SIZE);
        sendCommand(QString("KY %1").arg(chunk));
    }

    // Calculate CW duration for the full message (with radio processing buffer)
    int estimatedMs = CWTiming::estimatedDuration(text, m_state.cwSpeed);
    m_cwInProgress = true;
    m_cwTimer->start(estimatedMs);

    LOG_DEBUG("KenwoodRadio", QString("Sending CW: '%1' (estimated: %2ms)")
              .arg(text).arg(estimatedMs));

    return true;
}

bool KenwoodRadio::setCWSpeed(int wpm)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set CW speed: not connected");
        return false;
    }

    int minWpm = 0, maxWpm = 0;
    getCWSpeedRange(minWpm, maxWpm);
    if (wpm < minWpm || wpm > maxWpm) {
        LOG_ERROR("KenwoodRadio", QString("CW speed out of range: %1 (must be %2-%3 WPM)")
            .arg(wpm).arg(minWpm).arg(maxWpm));
        return false;
    }

    // KS<3-digit WPM>
    sendCommand(QString("KS%1").arg(wpm, 3, 10, QChar('0')));
    // Query back to confirm
    sendCommand("KS");

    return true;
}

bool KenwoodRadio::stopCW()
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot stop CW: not connected");
        return false;
    }

    // Kenwood uses KY0; to abort CW message
    // Followed by RX to ensure we're back in receive
    sendCommand("KY0");
    sendCommand("RX");
    m_cwInProgress = false;
    m_cwTimer->stop();

    return true;
}

bool KenwoodRadio::waitForMorseComplete()
{
    if (!m_cwInProgress) {
        return true;
    }

    // Wait for CW timer to expire (with timeout)
    QEventLoop loop;
    QObject::connect(m_cwTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Safety timeout (max 30 seconds)
    const int CW_SAFETY_TIMEOUT_MS = 30000;
    QTimer::singleShot(CW_SAFETY_TIMEOUT_MS, &loop, &QEventLoop::quit);

    loop.exec();

    return !m_cwInProgress;
}

void KenwoodRadio::onCWTimeout()
{
    m_cwInProgress = false;
    LOG_DEBUG("KenwoodRadio", "CW transmission estimated complete (timer expired)");
}

// ============================================================================
// RadioInterface Implementation - RIT/XIT Control
// ============================================================================

bool KenwoodRadio::setRIT(int offset_hz, VFO /*vfo*/)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set RIT: not connected");
        return false;
    }

    // Clear RIT first, then set offset by sending RU (up) or RD (down) commands
    sendCommand("RC");  // Clear RIT offset to 0

    if (offset_hz != 0) {
        // RU/RD commands change offset by specified amount
        if (offset_hz > 0) {
            sendCommand(QString("RU%1").arg(offset_hz, 5, 10, QChar('0')));
        } else {
            sendCommand(QString("RD%1").arg(qAbs(offset_hz), 5, 10, QChar('0')));
        }
    }

    return true;
}

bool KenwoodRadio::setXIT(int offset_hz, VFO vfo)
{
    // TS-890 shares RIT/XIT offset mechanism
    return setRIT(offset_hz, vfo);
}

bool KenwoodRadio::clearRIT(VFO /*vfo*/)
{
    if (!isConnected()) return false;
    sendCommand("RC");  // RIT Clear
    return true;
}

bool KenwoodRadio::clearXIT(VFO vfo)
{
    return clearRIT(vfo);
}

bool KenwoodRadio::enableRIT(bool enable, VFO /*vfo*/)
{
    if (!isConnected()) return false;
    sendCommand(enable ? "RT1" : "RT0");
    return true;
}

bool KenwoodRadio::enableXIT(bool enable, VFO /*vfo*/)
{
    if (!isConnected()) return false;
    sendCommand(enable ? "XT1" : "XT0");
    return true;
}

int KenwoodRadio::getRIT(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.ritOffsetA : m_state.ritOffsetB;
}

int KenwoodRadio::getXIT(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.xitOffsetA : m_state.xitOffsetB;
}

// ============================================================================
// RadioInterface Implementation - Split Control
// ============================================================================

bool KenwoodRadio::setSplit(bool enable, VFO /*txVfo*/)
{
    if (!isConnected()) {
        LOG_ERROR("KenwoodRadio", "Cannot set split: not connected");
        return false;
    }

    if (enable) {
        // Enable split and set TX to VFO B
        sendCommand("TB1");  // Split ON
        sendCommand("FT1");  // TX VFO B
    } else {
        sendCommand("TB0");  // Split OFF
    }

    return true;
}

bool KenwoodRadio::getSplit() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.isSplitEnabled;
}

// ============================================================================
// RadioInterface Implementation - VFO Tuning
// ============================================================================

bool KenwoodRadio::vfoBumpUp(VFO vfo)
{
    if (!isConnected()) return false;
    // UD<VFO><direction> where VFO=0/1, direction: 0=up
    const QString vfoStr = (vfo == VFO::VFO_A) ? "0" : "1";
    sendCommand("UD" + vfoStr + "0");
    return true;
}

bool KenwoodRadio::vfoBumpDown(VFO vfo)
{
    if (!isConnected()) return false;
    // UD<VFO><direction> where VFO=0/1, direction: 1=down
    const QString vfoStr = (vfo == VFO::VFO_A) ? "0" : "1";
    sendCommand("UD" + vfoStr + "1");
    return true;
}

// ============================================================================
// RadioInterface Implementation - Filter
// ============================================================================

bool KenwoodRadio::setFilterWidth(int /*width_hz*/)
{
    // Filter width requires mode-dependent SH/SL lookup tables
    // Defer to subclass or approximate
    LOG_WARN("KenwoodRadio", "setFilterWidth not fully implemented for Kenwood radios");
    return false;
}

int KenwoodRadio::getFilterWidth() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.filterWidth;
}

// ============================================================================
// RadioInterface Implementation - Metering
// ============================================================================

void KenwoodRadio::setDetailedRigInfoEnabled(bool enabled)
{
    m_collectDetailedRigInfo = enabled;
    if (enabled && isConnected()) {
        m_meterTimer->start(METER_POLL_INTERVAL_MS);
        LOG_DEBUG("KenwoodRadio", "Meter polling started");
    } else {
        m_meterTimer->stop();
        LOG_DEBUG("KenwoodRadio", "Meter polling stopped");
    }
}

void KenwoodRadio::onMeterPoll()
{
    if (!isConnected()) return;

    // Poll S-meter, power, and SWR
    sendCommand("SM0");  // S-meter VFO A
    sendCommand("SM2");  // Power output
    sendCommand("SM4");  // SWR

    // Emit full state update
    emit stateUpdated(getCurrentState());
}

// ============================================================================
// Query Methods (const)
// ============================================================================

freq_t KenwoodRadio::getFrequency(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.frequencyA : m_state.frequencyB;
}

ModeType KenwoodRadio::getMode(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.modeA : m_state.modeB;
}

bool KenwoodRadio::getPTT() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.isTransmitting;
}

int KenwoodRadio::getCWSpeed() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.cwSpeed;
}

RadioState KenwoodRadio::getCurrentState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

bool KenwoodRadio::supportsDiscreteBandCommand() const
{
    // Kenwood radios don't have a discrete band command like K4's BN
    return false;
}

QList<ModeType> KenwoodRadio::getSupportedModes() const
{
    return {
        ModeType::LSB,
        ModeType::USB,
        ModeType::CW,
        ModeType::FM,
        ModeType::AM,
        ModeType::RTTY,
        ModeType::PSK,
        ModeType::DATA
    };
}

bool KenwoodRadio::supportsCWSending() const
{
    return true;
}

} // namespace TR4QT
