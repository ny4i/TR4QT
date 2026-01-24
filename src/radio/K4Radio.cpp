#include "K4Radio.h"
#include "CWTiming.h"
#include "../logging/LogMacros.h"
#include "../utils/PerformanceProfiler.h"
#include <QMutexLocker>
#include <QEventLoop>
#include <QCoreApplication>
#include <QThread>

namespace TR4QT {

K4Radio::K4Radio(QObject* parent)
    : RadioInterface(parent)
{
    m_cwTimer = new QTimer(this);
    m_cwTimer->setSingleShot(true);
    QObject::connect(m_cwTimer, &QTimer::timeout, this, &K4Radio::onCWTimeout);

    m_state.radioModel = "Elecraft K4";
}

K4Radio::~K4Radio()
{
    disconnect();
}

// ============================================================================
// Connection Management
// ============================================================================

bool K4Radio::connect(const RadioConfig& config)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        LOG_WARN("K4Radio", "Already connected");
        return true;
    }

    // Parse host:port from config.port (format: "192.168.1.100:12345")
    QString portStr = config.port;
    QStringList parts = portStr.split(':');
    if (parts.size() != 2) {
        LOG_ERROR("K4Radio", QString("Invalid network address format: %1 (expected host:port)").arg(portStr));
        emit errorOccurred("Invalid network address format (expected host:port)");
        return false;
    }

    m_host = parts[0];
    m_port = parts[1].toUInt();

    if (m_port == 0) {
        LOG_ERROR("K4Radio", QString("Invalid port number: %1").arg(parts[1]));
        emit errorOccurred("Invalid port number");
        return false;
    }

    LOG_INFO("K4Radio", QString("Connecting to K4 at %1:%2").arg(m_host).arg(m_port));

    m_socket = new QTcpSocket(this);

    QObject::connect(m_socket, &QTcpSocket::connected, this, &K4Radio::onSocketConnected);
    QObject::connect(m_socket, &QTcpSocket::disconnected, this, &K4Radio::onSocketDisconnected);
    QObject::connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                     this, &K4Radio::onSocketError);
    QObject::connect(m_socket, &QTcpSocket::readyRead, this, &K4Radio::onReadyRead);

    // Connect asynchronously - DO NOT use waitForConnected() as it blocks the event loop!
    // This was causing the worker thread to stop processing events after connection,
    // preventing setFrequency/setMode/etc. slot calls from executing.
    // The onSocketConnected() slot will be called when connection succeeds.
    m_socket->connectToHost(m_host, m_port);

    LOG_DEBUG("K4Radio", "Connection initiated, waiting for onSocketConnected()");
    return true;
}

void K4Radio::disconnect()
{
    if (m_socket) {
        LOG_INFO("K4Radio", "Disconnecting from K4");
        m_socket->disconnectFromHost();
        // DO NOT use waitForDisconnected() - it blocks the event loop just like waitForConnected()
        // The socket will be cleaned up asynchronously via onSocketDisconnected() or deleteLater()
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    QMutexLocker locker(&m_stateMutex);
    m_state = RadioState();  // Reset state
    m_state.radioModel = "Elecraft K4";
    m_receiveBuffer.clear();
}

bool K4Radio::isConnected() const
{
    bool connected = m_socket && m_socket->state() == QAbstractSocket::ConnectedState;

    // Debug: Log connection state checks to diagnose why buttons are disabled
    static int callCount = 0;
    if (++callCount <= 10 || callCount % 100 == 0) {  // Log first 10 calls, then every 100th
        LOG_DEBUG("K4Radio", QString("isConnected() called: socket=%1, state=%2, returning=%3 (call #%4)")
                  .arg(m_socket ? "valid" : "NULL")
                  .arg(m_socket ? QString::number(m_socket->state()) : "N/A")
                  .arg(connected ? "true" : "false")
                  .arg(callCount));
    }

    return connected;
}

void K4Radio::onSocketConnected()
{
    LOG_INFO("K4Radio", QString("Connected to K4 at %1:%2").arg(m_host).arg(m_port));

    // Enable full async updates including S-meter
    enableAIMode(4);    // AI4 = all async updates including S-meter
    sendCommand("TM1");   // Enable temperature/power/SWR monitoring
    sendCommand("SMH1");  // Enable S-meter in dBm format

    // Query initial state
    queryInitialState();

    // Emit initial state immediately so UI knows radio model
    // Responses to queryInitialState() will update with actual radio state
    {
        QMutexLocker locker(&m_stateMutex);
        emit stateUpdated(m_state);
    }

    // Emit connected AFTER initialization commands are queued
    // This prevents race conditions where UI handlers run before radio is ready
    emit connectionStatusChanged(true);
}

void K4Radio::onSocketDisconnected()
{
    LOG_INFO("K4Radio", "Disconnected from K4");
    emit connectionStatusChanged(false);
}

void K4Radio::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorMsg = m_socket ? m_socket->errorString() : "Unknown socket error";
    LOG_ERROR("K4Radio", QString("Socket error: %1").arg(errorMsg));
    emit errorOccurred(errorMsg);
}

// ============================================================================
// Command Sending
// ============================================================================

void K4Radio::sendCommand(const QString& cmd, VFO vfo)
{
    if (!m_socket || !isConnected()) {
        LOG_ERROR("K4Radio", "Cannot send command: not connected");
        return;
    }

    QString fullCmd = cmd;

    // Add VFO suffix if needed (VFO B uses $ suffix)
    if (vfo == VFO::VFO_B && !cmd.contains('$')) {
        // Insert $ after command (e.g., "FA" -> "FA$", "MD3" -> "MD$3")
        if (fullCmd.length() >= 2) {
            fullCmd.insert(2, '$');
        }
    }

    // Ensure semicolon terminator
    if (!fullCmd.endsWith(';')) {
        fullCmd += ';';
    }

    LOG_TRACE("K4Radio", QString("TX: %1").arg(fullCmd.trimmed()));

    m_socket->write(fullCmd.toLatin1());
    m_socket->flush();
}

// ============================================================================
// Message Reception and Parsing
// ============================================================================

void K4Radio::onReadyRead()
{
    // Read all available data (K4 uses ; terminators, not newlines)
    if (m_socket->bytesAvailable() > 0) {
        QByteArray data = m_socket->readAll();
        QString dataStr = QString::fromLatin1(data);
        LOG_TRACE("K4Radio", QString("onReadyRead: received %1 bytes: '%2'")
                  .arg(data.size()).arg(dataStr));
        m_receiveBuffer += dataStr;

        // Process complete messages (terminated with ;)
        int messageCount = 0;
        while (m_receiveBuffer.contains(';')) {
            int idx = m_receiveBuffer.indexOf(';');
            QString message = m_receiveBuffer.left(idx);
            m_receiveBuffer = m_receiveBuffer.mid(idx + 1);

            if (!message.isEmpty()) {
                messageCount++;
                LOG_TRACE("K4Radio", QString("RX [%1]: %2;").arg(messageCount).arg(message));
                processMessage(message);
            }
        }

        if (!m_receiveBuffer.isEmpty()) {
            LOG_TRACE("K4Radio", QString("Incomplete message in buffer: '%1'").arg(m_receiveBuffer));
        }
    }
}

void K4Radio::processMessage(const QString& message)
{
    if (message.length() < 2) return;

    // Skip display commands (start with #) - these are panadapter/spectrum settings we don't track
    // Examples: #AR, #AVG, #CAL$, #DSM, #FPS, #REF$, #SPN$, #VFA, #WFC$, etc.
    if (message[0] == '#') {
        LOG_TRACE("K4Radio", QString("Skipping display command: %1").arg(message));
        return;
    }

    QString command = message.left(2);
    bool isVFOB = (message.length() > 2 && message[2] == '$');
    QString data = isVFOB ? message.mid(3) : message.mid(2);
    VFO vfo = isVFOB ? VFO::VFO_B : VFO::VFO_A;

    QMutexLocker locker(&m_stateMutex);

    // DEBUG: Log all processed messages to diagnose state update issues
    LOG_TRACE("K4Radio", QString("Processing: cmd=%1 vfo=%2 data=%3")
              .arg(command)
              .arg(vfo == VFO::VFO_A ? "A" : "B")
              .arg(data));

    if (command == "AI") {
        // Auto Information mode confirmation
        LOG_INFO("K4Radio", QString("AI mode set to %1").arg(data));
    }
    else if (command == "FA") {
        // Frequency VFO A
        bool ok;
        freq_t freq = data.toLongLong(&ok);
        if (ok && freq != m_state.frequencyA) {
            m_state.frequencyA = freq;
            m_state.bandA = frequencyToBand(freq);
            emit frequencyChanged(freq, VFO::VFO_A);
            emit stateUpdated(m_state);
        }
    }
    else if (command == "FB") {
        // Frequency VFO B
        bool ok;
        freq_t freq = data.toLongLong(&ok);
        if (ok && freq != m_state.frequencyB) {
            m_state.frequencyB = freq;
            m_state.bandB = frequencyToBand(freq);
            emit frequencyChanged(freq, VFO::VFO_B);
            emit stateUpdated(m_state);
        }
    }
    else if (command == "MD") {
        // Mode
        ModeType mode = modeStringToMode(data, "0");
        LOG_DEBUG("K4Radio", QString("MD command: raw=%1 -> ModeType=%2 (%3)")
            .arg(data).arg(static_cast<int>(mode)).arg(modeToString(mode)));
        ModeType& currentMode = (vfo == VFO::VFO_A) ? m_state.modeA : m_state.modeB;
        if (mode != currentMode) {
            currentMode = mode;
            emit modeChanged(mode, vfo);
            emit stateUpdated(m_state);
        }
    }
    else if (command == "DT") {
        // Data sub-mode (for DATA modes)
        // Store for mode parsing when combined with MD
        LOG_DEBUG("K4Radio", QString("Data sub-mode: %1 for VFO %2").arg(data).arg(vfo == VFO::VFO_A ? "A" : "B"));
    }
    else if (command == "IF") {
        // Comprehensive transceiver information
        parseIFCommand(message, vfo);
    }
    else if (command == "FT") {
        // Split on/off
        bool splitEnabled = (data.left(1) == "1");
        if (splitEnabled != m_state.isSplitEnabled) {
            m_state.isSplitEnabled = splitEnabled;
            emit splitChanged(splitEnabled);
            emit stateUpdated(m_state);
        }
    }
    else if (command == "RT") {
        // RIT on/off
        bool ritEnabled = (data.left(1) == "1");
        if (ritEnabled != m_state.isRitEnabled) {
            m_state.isRitEnabled = ritEnabled;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "XT") {
        // XIT on/off
        bool xitEnabled = (data.left(1) == "1");
        if (xitEnabled != m_state.isXitEnabled) {
            m_state.isXitEnabled = xitEnabled;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "RO") {
        // RIT/XIT offset (-9999 to +9999 Hz)
        int sign = (data.left(1) == "-") ? -1 : 1;
        bool ok;
        int offset = data.mid(1, 4).toInt(&ok) * sign;
        if (ok) {
            int& currentRIT = (vfo == VFO::VFO_A) ? m_state.ritOffsetA : m_state.ritOffsetB;
            int& currentXIT = (vfo == VFO::VFO_A) ? m_state.xitOffsetA : m_state.xitOffsetB;
            if (offset != currentRIT) {
                currentRIT = offset;
                currentXIT = offset;  // K4 uses same offset for RIT and XIT
                emit ritChanged(offset, vfo);
                emit xitChanged(offset, vfo);
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "KS") {
        // CW speed
        bool ok;
        int speed = data.left(3).toInt(&ok);
        LOG_DEBUG("K4Radio", QString("KS response: data='%1' parsed=%2 ok=%3 current=%4")
            .arg(data).arg(speed).arg(ok).arg(m_state.cwSpeed));
        if (ok && speed != m_state.cwSpeed) {
            m_state.cwSpeed = speed;
            LOG_INFO("K4Radio", QString("CW speed changed: %1 WPM").arg(speed));
            emit stateUpdated(m_state);
        }
    }
    else if (command == "TX") {
        // Transmit
        if (!m_state.isTransmitting) {
            m_state.isTransmitting = true;
            emit pttChanged(true);
            emit stateUpdated(m_state);
        }
    }
    else if (command == "RX") {
        // Receive - radio stopped transmitting
        if (m_state.isTransmitting) {
            m_state.isTransmitting = false;
            emit pttChanged(false);
            emit stateUpdated(m_state);
        }

        // If we were waiting for CW to complete, stop the timer
        // Radio has confirmed it's back to receive, so CW transmission is done
        if (m_cwInProgress) {
            m_cwInProgress = false;
            m_cwTimer->stop();
            LOG_DEBUG("K4Radio", "CW transmission completed (radio confirmed with RX)");
        }
    }
    else if (command == "BN") {
        // Band number
        bool ok;
        int bandNum = data.toInt(&ok);
        if (ok) {
            BandType band = bandNumberToBand(bandNum);
            BandType& currentBand = (vfo == VFO::VFO_A) ? m_state.bandA : m_state.bandB;
            currentBand = band;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "FP") {
        // Filter preset
        LOG_DEBUG("K4Radio", QString("Filter preset: %1 for VFO %2").arg(data).arg(vfo == VFO::VFO_A ? "A" : "B"));
    }
    else if (command == "ID") {
        // Radio ID
        LOG_INFO("K4Radio", QString("Radio ID: %1").arg(data));
    }
    else if (command == "PO") {
        // Power output (tenths of watts)
        bool ok;
        int power = data.toInt(&ok);
        if (ok && power != m_state.powerOutput) {
            m_state.powerOutput = power;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "TM") {
        // Power/ALC/SWR monitoring (format: TMaaabbbcccddd;)
        // aaa = ALC (bars 0-7), bbb = CMP (dB), ccc = FWD power (watts), ddd = SWR (tenths)
        if (data.length() >= 12) {
            bool ok;
            int alc = data.mid(0, 3).toInt(&ok);
            if (ok) m_state.alcLevel = alc;

            int cmp = data.mid(3, 3).toInt(&ok);
            if (ok) m_state.compressionLevel = cmp;

            int power = data.mid(6, 3).toInt(&ok);
            if (ok) {
                // Power is in watts (QRO mode) or tenths of watts (QRP mode)
                // Store as tenths of watts for consistency with PO command
                // TODO: Detect QRP mode and multiply by 10 if needed
                m_state.powerOutput = power * 10;  // Assume QRO mode (watts → tenths)
            }

            int swr = data.mid(9, 3).toInt(&ok);
            if (ok) m_state.swr = swr;

            emit stateUpdated(m_state);
            LOG_DEBUG("K4Radio", QString("TM: ALC=%1 bars, CMP=%2 dB, FWD=%3W, SWR=%4")
                .arg(alc).arg(cmp).arg(power).arg(swr / 10.0, 0, 'f', 1));
        }
    }
    else if (command == "CW") {
        // CW pitch (sidetone frequency)
        bool ok;
        int pitch = data.toInt(&ok);
        if (ok && pitch != m_state.cwPitch) {
            m_state.cwPitch = pitch;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "AG") {
        // AF Gain
        bool ok;
        int gain = data.toInt(&ok);
        if (ok) {
            int& currentGain = (vfo == VFO::VFO_A) ? m_state.afGainA : m_state.afGainB;
            if (gain != currentGain) {
                currentGain = gain;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "RG") {
        // RF Gain
        bool ok;
        int gain = data.toInt(&ok);
        if (ok) {
            int& currentGain = (vfo == VFO::VFO_A) ? m_state.rfGainA : m_state.rfGainB;
            if (gain != currentGain) {
                currentGain = gain;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "MG") {
        // Microphone gain
        bool ok;
        int gain = data.toInt(&ok);
        if (ok && gain != m_state.micGain) {
            m_state.micGain = gain;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "CP") {
        // Speech compression
        bool ok;
        int comp = data.toInt(&ok);
        if (ok && comp != m_state.speechCompression) {
            m_state.speechCompression = comp;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "SQ") {
        // Squelch
        bool ok;
        int squelch = data.toInt(&ok);
        if (ok) {
            int& currentSq = (vfo == VFO::VFO_A) ? m_state.squelchA : m_state.squelchB;
            if (squelch != currentSq) {
                currentSq = squelch;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "GT") {
        // AGC mode
        bool ok;
        int agc = data.toInt(&ok);
        if (ok) {
            int& currentAgc = (vfo == VFO::VFO_A) ? m_state.agcModeA : m_state.agcModeB;
            if (agc != currentAgc) {
                currentAgc = agc;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "PA") {
        // Preamp
        bool ok;
        int preamp = data.toInt(&ok);
        if (ok) {
            int& currentPreamp = (vfo == VFO::VFO_A) ? m_state.preampA : m_state.preampB;
            if (preamp != currentPreamp) {
                currentPreamp = preamp;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "RA") {
        // RX Attenuator
        bool ok;
        int atten = data.toInt(&ok);
        if (ok) {
            int& currentAtten = (vfo == VFO::VFO_A) ? m_state.attenuatorA : m_state.attenuatorB;
            if (atten != currentAtten) {
                currentAtten = atten;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "BW") {
        // Receiver filter bandwidth
        bool ok;
        int bw = data.toInt(&ok);
        if (ok && bw != m_state.filterWidth) {
            m_state.filterWidth = bw;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "NB") {
        // Noise blanker
        bool ok;
        int nb = data.toInt(&ok);
        if (ok) {
            int& currentNB = (vfo == VFO::VFO_A) ? m_state.noiseBlankerA : m_state.noiseBlankerB;
            if (nb != currentNB) {
                currentNB = nb;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "AN") {
        // TX Antenna
        bool ok;
        int ant = data.toInt(&ok);
        if (ok && ant != m_state.txAntenna) {
            m_state.txAntenna = ant;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "AR") {
        // RX Antenna
        bool ok;
        int ant = data.toInt(&ok);
        if (ok) {
            int& currentAnt = (vfo == VFO::VFO_A) ? m_state.rxAntennaA : m_state.rxAntennaB;
            if (ant != currentAnt) {
                currentAnt = ant;
                emit stateUpdated(m_state);
            }
        }
    }
    else if (command == "AT") {
        // ATU mode
        bool atuOn = (data.left(1) == "1");
        if (atuOn != m_state.atuEnabled) {
            m_state.atuEnabled = atuOn;
            emit stateUpdated(m_state);
        }
    }
    else if (command == "SM") {
        // S-Meter reading - handle both segment (SM) and dBm (SMH) formats
        if (data.startsWith("H")) {
            // SMH response: dBm format (e.g., "H-073" = -73 dBm)
            // Format: H followed by sign and 3-digit dBm value
            QString dbmStr = data.mid(1);  // Skip the 'H'
            bool ok;
            int dbm = dbmStr.toInt(&ok);
            if (ok && dbm != m_state.signalStrength) {
                m_state.signalStrength = dbm;
                emit stateUpdated(m_state);
                LOG_TRACE("K4Radio", QString("S-meter: %1 dBm").arg(dbm));
            }
        } else {
            // SM response: segment format (0000-0030)
            // This shouldn't happen if SMH1; was sent, but handle it anyway
            bool ok;
            int segments = data.toInt(&ok);
            if (ok) {
                // Convert segments to approximate dBm
                // S9 = -73 dBm, each S-unit below S9 is 6 dB
                // 18 segments = S9 = -73 dBm
                int dbm = -127 + (segments * 3);  // Rough approximation
                if (dbm != m_state.signalStrength) {
                    m_state.signalStrength = dbm;
                    emit stateUpdated(m_state);
                    LOG_TRACE("K4Radio", QString("S-meter: %1 segments ≈ %2 dBm").arg(segments).arg(dbm));
                }
            }
        }
    }
    else if (command == "SB") {
        // Sub receiver enable
        bool subRx = (data.left(1) == "1");
        if (subRx != m_state.subRxEnabled) {
            m_state.subRxEnabled = subRx;
            emit stateUpdated(m_state);
        }
    }
    else {
        // Unknown command - trace for debugging
        LOG_TRACE("K4Radio", QString("Unhandled command: %1 data: %2").arg(command).arg(data));
    }
}

bool K4Radio::parseIFCommand(const QString& response, VFO vfo)
{
    // IF command format: IF[f]*****+yyyyrx*00tmvspbd1*;
    // Example: IF00014200000     +0000001001000301;
    QString data = response;
    if (data.startsWith("IF")) {
        data = data.mid(2);  // Remove "IF" prefix
    }
    if (data.startsWith("$")) {
        data = data.mid(1);  // Remove "$" for VFO B
    }

    if (data.length() < 34) {
        LOG_ERROR("K4Radio", QString("IF command too short: %1 bytes (expected 34+)").arg(data.length()));
        return false;
    }

    int pos = 0;

    // [f] Frequency (11 digits)
    bool ok;
    freq_t freq = data.mid(pos, 11).toLongLong(&ok);
    if (!ok) {
        LOG_ERROR("K4Radio", QString("IF command invalid frequency: %1").arg(data.mid(pos, 11)));
        return false;
    }
    pos += 11;

    // ***** (5 spaces)
    pos += 5;

    // + or - sign for RIT/XIT offset
    int sign = (data[pos] == '-') ? -1 : 1;
    pos++;

    // yyyy RIT/XIT offset (4 digits)
    int ritOffset = data.mid(pos, 4).toInt(&ok) * sign;
    if (!ok) {
        LOG_ERROR("K4Radio", QString("IF command invalid RIT offset: %1").arg(data.mid(pos, 4)));
        return false;
    }
    pos += 4;

    // r RIT on/off
    bool ritEnabled = (data[pos] == '1');
    pos++;

    // x XIT on/off
    bool xitEnabled = (data[pos] == '1');
    pos++;

    // * (space)
    pos++;

    // 00 (always 00)
    pos += 2;

    // t TX/RX
    bool transmitting = (data[pos] == '1');
    pos++;

    // m Mode
    QString modeStr = data.mid(pos, 1);
    pos++;

    // v Active VFO (0=A, 1=B)
    bool activeVFOB = (data[pos] == '1');
    pos++;

    // s Scan (skip)
    pos++;

    // p Split
    bool splitEnabled = (data[pos] == '1');
    pos++;

    // b Band change flag (skip)
    pos++;

    // d Data sub-mode
    QString dataModeStr = data.mid(pos, 1);

    // Update state
    // NOTE: Mutex is already locked by processMessage caller - do NOT lock again!
    if (vfo == VFO::VFO_A) {
        m_state.frequencyA = freq;
        m_state.bandA = frequencyToBand(freq);
        ModeType mode = modeStringToMode(modeStr, dataModeStr);
        LOG_DEBUG("K4Radio", QString("IF command VFO-A: modeStr=%1 dataModeStr=%2 -> ModeType=%3 (%4)")
            .arg(modeStr).arg(dataModeStr).arg(static_cast<int>(mode)).arg(modeToString(mode)));
        m_state.modeA = mode;
        m_state.ritOffsetA = ritOffset;
        m_state.xitOffsetA = ritOffset;  // K4 uses same offset
    } else {
        m_state.frequencyB = freq;
        m_state.bandB = frequencyToBand(freq);
        m_state.modeB = modeStringToMode(modeStr, dataModeStr);
        m_state.ritOffsetB = ritOffset;
        m_state.xitOffsetB = ritOffset;
    }

    m_state.isRitEnabled = ritEnabled;
    m_state.isXitEnabled = xitEnabled;
    m_state.isTransmitting = transmitting;
    m_state.isSplitEnabled = splitEnabled;

    emit stateUpdated(m_state);
    return true;
}

// ============================================================================
// Mode Conversion
// ============================================================================

ModeType K4Radio::modeStringToMode(const QString& modeStr, const QString& dataModeStr)
{
    bool ok;
    int mode = modeStr.toInt(&ok);
    if (!ok) {
        LOG_ERROR("K4Radio", QString("Invalid mode string: %1").arg(modeStr));
        return ModeType::None;
    }

    // K4 mode numbers:
    // 0=None, 1=LSB, 2=USB, 3=CW, 4=FM, 5=AM, 6=Data, 7=CW-Rev, 9=Data-Rev
    switch (mode) {
        case 0: return ModeType::None;
        case 1: return ModeType::LSB;
        case 2: return ModeType::USB;
        case 3: return ModeType::CW;
        case 4: return ModeType::FM;
        case 5: return ModeType::AM;
        case 6: {
            // Data mode - check sub-mode
            int dataMode = dataModeStr.toInt(&ok);
            if (!ok) dataMode = 0;
            switch (dataMode) {
                case 0: return ModeType::DATA;      // DATA A
                case 1: return ModeType::RTTY;      // AFSK A (RTTY)
                case 2: return ModeType::RTTY;      // FSK D (RTTY)
                case 3: return ModeType::PSK;       // PSK D
                default: return ModeType::DATA;
            }
        }
        case 7: return ModeType::CW;  // CW-Rev (treat as CW)
        case 9: return ModeType::DATA;  // Data-Rev
        default:
            LOG_WARN("K4Radio", QString("Unknown mode number: %1").arg(mode));
            return ModeType::None;
    }
}

QString K4Radio::modeToModeString(ModeType mode, int& dataModeInt)
{
    dataModeInt = -1;  // No data sub-mode by default

    switch (mode) {
        case ModeType::None:   return "0";
        case ModeType::CW:     return "3";
        case ModeType::LSB:    return "1";
        case ModeType::USB:    return "2";
        case ModeType::FM:     return "4";
        case ModeType::AM:     return "5";
        case ModeType::DATA:
            dataModeInt = 0;  // DATA A
            return "6";
        case ModeType::RTTY:
            dataModeInt = 1;  // AFSK A
            return "6";
        case ModeType::PSK:
            dataModeInt = 3;  // PSK D
            return "6";
        default:
            LOG_WARN("K4Radio", QString("Unsupported mode: %1").arg(static_cast<int>(mode)));
            return "0";
    }
}

// ============================================================================
// Band Conversion
// ============================================================================

BandType K4Radio::bandNumberToBand(int bandNum)
{
    // K4 band numbers: 00=160m, 01=80m, 02=60m, 03=40m, 04=30m,
    //                  05=20m, 06=17m, 07=15m, 08=12m, 09=10m, 10=6m
    switch (bandNum) {
        case 0:  return BandType::Band160M;
        case 1:  return BandType::Band80M;
        case 2:  return BandType::Band60M;
        case 3:  return BandType::Band40M;
        case 4:  return BandType::Band30M;
        case 5:  return BandType::Band20M;
        case 6:  return BandType::Band17M;
        case 7:  return BandType::Band15M;
        case 8:  return BandType::Band12M;
        case 9:  return BandType::Band10M;
        case 10: return BandType::Band6M;
        default:
            LOG_WARN("K4Radio", QString("Unknown band number: %1").arg(bandNum));
            return BandType::None;
    }
}

int K4Radio::bandToBandNumber(BandType band)
{
    switch (band) {
        case BandType::Band160M: return 0;
        case BandType::Band80M:  return 1;
        case BandType::Band60M:  return 2;
        case BandType::Band40M:  return 3;
        case BandType::Band30M:  return 4;
        case BandType::Band20M:  return 5;
        case BandType::Band17M:  return 6;
        case BandType::Band15M:  return 7;
        case BandType::Band12M:  return 8;
        case BandType::Band10M:  return 9;
        case BandType::Band6M:   return 10;
        default:
            LOG_WARN("K4Radio", QString("Unsupported band: %1").arg(static_cast<int>(band)));
            return -1;
    }
}

// ============================================================================
// Initialization
// ============================================================================

void K4Radio::enableAIMode(int level)
{
    if (level < 0 || level > 5) {
        LOG_WARN("K4Radio", QString("Invalid AI mode level: %1 (must be 0-5)").arg(level));
        return;
    }

    sendCommand(QString("AI%1").arg(level));
    LOG_INFO("K4Radio", QString("Enabled AI mode %1 (automatic status updates)").arg(level));
}

void K4Radio::setDetailedRigInfoEnabled(bool enabled)
{
    if (m_collectDetailedRigInfo == enabled) {
        return;  // Already in desired state
    }

    m_collectDetailedRigInfo = enabled;

    if (enabled) {
        // Enable detailed rig info: S-meter, temperature, power
        LOG_INFO("K4Radio", "Enabling detailed rig info (TM1, SMH1)");
        sendCommand("TM1");   // Enable temperature/power/SWR monitoring
        sendCommand("SMH1");  // Enable S-meter in dBm format
    } else {
        // Disable detailed rig info (keep AI4 for consistent behavior)
        LOG_INFO("K4Radio", "Disabling detailed rig info (TM0, SMH0)");
        sendCommand("TM0");   // Disable temperature/power/SWR monitoring
        sendCommand("SMH0");  // Disable S-meter
    }
}

void K4Radio::queryInitialState()
{
    LOG_DEBUG("K4Radio", "Querying initial K4 state");

    // Query VFO A state
    sendCommand("FA");    // Frequency A
    sendCommand("MD");    // Mode A
    sendCommand("DT");    // Data sub-mode A
    sendCommand("RT");    // RIT state
    sendCommand("XT");    // XIT state
    sendCommand("RO");    // RIT/XIT offset
    sendCommand("FT");    // Split state
    sendCommand("KS");    // CW speed
    sendCommand("BN");    // Band number
    sendCommand("IF");    // Comprehensive status

    // Query VFO B state
    // Note: K4 uses FB (not FA$) for VFO B frequency
    sendCommand("FB");    // Frequency B
    sendCommand("MD", VFO::VFO_B);  // MD$ for VFO B mode (uses $ suffix)
    sendCommand("DT", VFO::VFO_B);  // DT$ for VFO B data sub-mode (uses $ suffix)
    // Note: IF command only works for current VFO, no VFO B version

    // Query radio ID to confirm K4
    sendCommand("ID");
}

QString K4Radio::vfoSuffix(VFO vfo) const
{
    return (vfo == VFO::VFO_B) ? "$" : "";
}

// ============================================================================
// RadioInterface Implementation - Frequency Control
// ============================================================================

// DEBUG: Test slot to verify signal/slot mechanism works
void K4Radio::debugTestSlot(int testValue)
{
    LOG_ERROR("K4Radio", QString("***** DEBUG TEST SLOT CALLED WITH VALUE: %1 *****").arg(testValue));
    LOG_ERROR("K4Radio", QString("***** This proves worker thread is processing slot calls *****"));
}

bool K4Radio::setFrequency(freq_t freq, VFO vfo)
{
    PROFILE_FUNCTION("K4Direct");

    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set frequency: not connected");
        return false;
    }

    // Format frequency as 11-digit zero-padded string (e.g., "00007000000")
    QString freqStr = QString::number(static_cast<qulonglong>(freq)).rightJustified(11, '0');

    // K4 uses FA for VFO A, FB for VFO B (not FA$)
    QString command = (vfo == VFO::VFO_A) ? QString("FA%1").arg(freqStr)
                                           : QString("FB%1").arg(freqStr);

    double freqKHz = freq / 1000.0;
    LOG_DEBUG("K4Radio", QString("setFrequency: freq=%1 kHz, VFO %2, command='%3'")
              .arg(freqKHz, 0, 'f', 1)
              .arg(vfo == VFO::VFO_A ? "A" : "B")
              .arg(command));

    // Send command without VFO suffix (FA/FB already encodes the VFO)
    sendCommand(command);

    return true;
}

freq_t K4Radio::getFrequency(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.frequencyA : m_state.frequencyB;
}

// ============================================================================
// RadioInterface Implementation - Mode Control
// ============================================================================

bool K4Radio::setMode(ModeType mode, VFO vfo)
{
    PROFILE_FUNCTION("K4Direct");

    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set mode: not connected");
        return false;
    }

    int dataModeInt;
    QString modeStr = modeToModeString(mode, dataModeInt);

    sendCommand(QString("MD%1").arg(modeStr), vfo);

    if (dataModeInt >= 0) {
        sendCommand(QString("DT%1").arg(dataModeInt), vfo);
    }

    return true;
}

ModeType K4Radio::getMode(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.modeA : m_state.modeB;
}

// ============================================================================
// RadioInterface Implementation - PTT Control
// ============================================================================

bool K4Radio::setPTT(bool transmit)
{
    PROFILE_FUNCTION("K4Direct");

    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set PTT: not connected");
        return false;
    }

    sendCommand(transmit ? "TX" : "RX");
    return true;
}

bool K4Radio::getPTT() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.isTransmitting;
}

// ============================================================================
// RadioInterface Implementation - CW Functions
// ============================================================================

bool K4Radio::sendCW(const QString& text)
{
    PROFILE_FUNCTION("K4Direct");

    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot send CW: not connected");
        return false;
    }

    if (text.isEmpty()) {
        return true;
    }

    // Send CW via KY command
    sendCommand(QString("KY %1").arg(text));

    // Calculate accurate transmission duration using Morse code timing units
    // Add 10% buffer for radio processing and any extra spacing
    int accurateMs = CWTiming::calculateDuration(text, m_state.cwSpeed);
    int estimatedMs = static_cast<int>(accurateMs * 1.1);
    m_cwInProgress = true;
    m_cwTimer->start(estimatedMs);

    LOG_DEBUG("K4Radio", QString("Sending CW: '%1' (accurate: %2ms, with buffer: %3ms)")
              .arg(text).arg(accurateMs).arg(estimatedMs));

    return true;
}

bool K4Radio::setCWSpeed(int wpm)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set CW speed: not connected");
        return false;
    }

    if (wpm < MIN_CW_SPEED_WPM || wpm > MAX_CW_SPEED_WPM) {
        LOG_ERROR("K4Radio", QString("CW speed out of range: %1 (must be %2-%3 WPM)")
            .arg(wpm).arg(MIN_CW_SPEED_WPM).arg(MAX_CW_SPEED_WPM));
        return false;
    }

    sendCommand(QString("KS%1").arg(wpm, 3, 10, QChar('0')));
    // AI4 doesn't echo command confirmations, so query to get updated value
    sendCommand("KS");
    return true;
}

int K4Radio::getCWSpeed() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.cwSpeed;
}

void K4Radio::getCWSpeedRange(int& minWpm, int& maxWpm) const
{
    // K4 radios support 8-100 WPM
    minWpm = 8;
    maxWpm = 100;
}

bool K4Radio::stopCW()
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot stop CW: not connected");
        return false;
    }

    // Ctrl-D stops CW on K4
    sendCommand(QString(QChar(0x04)) + ";RX");
    m_cwInProgress = false;
    m_cwTimer->stop();

    return true;
}

bool K4Radio::waitForMorseComplete()
{
    if (!m_cwInProgress) {
        return true;
    }

    // Wait for CW timer to expire (with timeout)
    QEventLoop loop;
    QObject::connect(m_cwTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Safety timeout (max 30 seconds)
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);

    loop.exec();

    return !m_cwInProgress;
}

void K4Radio::onCWTimeout()
{
    // Timer expired - this is a fallback if we didn't receive RX from radio
    m_cwInProgress = false;
    LOG_DEBUG("K4Radio", "CW transmission estimated complete (timer expired, RX not received yet)");
}

// ============================================================================
// RadioInterface Implementation - RIT/XIT Control
// ============================================================================

bool K4Radio::setRIT(int offset_hz, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set RIT: not connected");
        return false;
    }

    if (offset_hz < -9999 || offset_hz > 9999) {
        LOG_ERROR("K4Radio", QString("RIT offset out of range: %1 (must be -9999 to +9999)").arg(offset_hz));
        return false;
    }

    QString sign = (offset_hz >= 0) ? "+" : "-";
    sendCommand(QString("RO%1%2").arg(sign).arg(qAbs(offset_hz), 4, 10, QChar('0')), vfo);

    return true;
}

bool K4Radio::setXIT(int offset_hz, VFO vfo)
{
    // K4 uses same offset for RIT and XIT
    return setRIT(offset_hz, vfo);
}

int K4Radio::getRIT(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.ritOffsetA : m_state.ritOffsetB;
}

int K4Radio::getXIT(VFO vfo) const
{
    QMutexLocker locker(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.xitOffsetA : m_state.xitOffsetB;
}

bool K4Radio::clearRIT(VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot clear RIT: not connected");
        return false;
    }

    sendCommand("RC", vfo);  // RIT Clear
    return true;
}

bool K4Radio::clearXIT(VFO vfo)
{
    // K4 uses RC command for both RIT and XIT
    return clearRIT(vfo);
}

bool K4Radio::enableRIT(bool enable, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot enable/disable RIT: not connected");
        return false;
    }

    sendCommand(QString("RT%1").arg(enable ? "1" : "0"), vfo);
    return true;
}

bool K4Radio::enableXIT(bool enable, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot enable/disable XIT: not connected");
        return false;
    }

    sendCommand(QString("XT%1").arg(enable ? "1" : "0"), vfo);
    return true;
}

// ============================================================================
// RadioInterface Implementation - Split Operation
// ============================================================================

bool K4Radio::setSplit(bool enable, VFO txVfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set split: not connected");
        return false;
    }

    sendCommand(QString("FT%1").arg(enable ? "1" : "0"));
    return true;
}

bool K4Radio::getSplit() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.isSplitEnabled;
}

// ============================================================================
// RadioInterface Implementation - VFO Tuning
// ============================================================================

bool K4Radio::vfoBumpUp(VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot bump VFO: not connected");
        return false;
    }

    sendCommand((vfo == VFO::VFO_A) ? "UP" : "UPB");
    return true;
}

bool K4Radio::vfoBumpDown(VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot bump VFO: not connected");
        return false;
    }

    sendCommand((vfo == VFO::VFO_A) ? "DN" : "DNB");
    return true;
}

// ============================================================================
// RadioInterface Implementation - Filter Control
// ============================================================================

bool K4Radio::setFilterWidth(int width_hz)
{
    // K4 uses filter presets (1-5), not direct Hz setting
    // This is a limitation - RadioInterface expects Hz
    LOG_WARN("K4Radio", "setFilterWidth(Hz) not supported on K4 - use setFilterPreset(1-5) instead");
    return false;
}

int K4Radio::getFilterWidth() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state.filterWidth;
}

// ============================================================================
// RadioInterface Implementation - State
// ============================================================================

RadioState K4Radio::getCurrentState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_state;
}

QList<ModeType> K4Radio::getSupportedModes() const
{
    // K4 supports all standard modes
    return {
        ModeType::CW,
        ModeType::CWR,
        ModeType::USB,
        ModeType::LSB,
        ModeType::FM,
        ModeType::AM,
        ModeType::DATA,
        ModeType::DATAR,
        ModeType::RTTY,
        ModeType::RTTYR
    };
}

bool K4Radio::supportsCWSending() const
{
    // K4 fully supports CW sending via KY command
    return true;
}

bool K4Radio::supportsDiscreteBandCommand() const
{
    // K4 supports discrete band selection via BN command
    // Radio maintains its own band memory (last frequency per band)
    return true;
}

// ============================================================================
// K4-Specific Features
// ============================================================================

bool K4Radio::setFilterPreset(int preset, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set filter preset: not connected");
        return false;
    }

    if (preset < 1 || preset > 5) {
        LOG_ERROR("K4Radio", QString("Filter preset out of range: %1 (must be 1-5)").arg(preset));
        return false;
    }

    sendCommand(QString("FP%1").arg(preset), vfo);
    LOG_INFO("K4Radio", QString("Set filter preset %1 for VFO %2").arg(preset).arg(vfo == VFO::VFO_A ? "A" : "B"));

    return true;
}

bool K4Radio::setBand(BandType band, VFO vfo)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot set band: not connected");
        return false;
    }

    int bandNum = bandToBandNumber(band);
    if (bandNum < 0) {
        return false;
    }

    sendCommand(QString("BN%1").arg(bandNum, 2, 10, QChar('0')), vfo);
    return true;
}

bool K4Radio::queryOptionModules(QStringList& modules)
{
    // TODO: Implement OM command parsing
    // This requires a request/response pattern, not just fire-and-forget commands
    LOG_WARN("K4Radio", "queryOptionModules() not yet implemented");
    return false;
}

bool K4Radio::playDVKMessage(int message)
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot play DVK message: not connected");
        return false;
    }

    if (message < 1 || message > 8) {
        LOG_ERROR("K4Radio", QString("DVK message out of range: %1 (must be 1-8)").arg(message));
        return false;
    }

    sendCommand(QString("DAMP%1%2").arg(message).arg("00000"));  // DAMPmnnnnn (no repeat)
    LOG_INFO("K4Radio", QString("Playing DVK message %1").arg(message));

    return true;
}

bool K4Radio::stopDVK()
{
    if (!isConnected()) {
        LOG_ERROR("K4Radio", "Cannot stop DVK: not connected");
        return false;
    }

    sendCommand("DA0");  // DA0 stops all DVK activity
    return true;
}

} // namespace TR4QT
