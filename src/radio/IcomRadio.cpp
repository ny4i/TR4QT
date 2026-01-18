#include "IcomRadio.h"
#include "../logging/LogMacros.h"
#include <QThread>

namespace TR4QT {

IcomRadio::IcomRadio(QObject* parent)
    : RadioInterface(parent)
{
    m_network = new IcomNetwork(this);

    // Connect network signals
    QObject::connect(m_network, &IcomNetwork::connected, this, &IcomRadio::onNetworkConnected);
    QObject::connect(m_network, &IcomNetwork::disconnected, this, &IcomRadio::onNetworkDisconnected);
    QObject::connect(m_network, &IcomNetwork::civDataReceived, this, &IcomRadio::onCivDataReceived);
    QObject::connect(m_network, &IcomNetwork::civSocketReady, this, &IcomRadio::onCivSocketReady);
    QObject::connect(m_network, &IcomNetwork::connectionError, this, &IcomRadio::onNetworkError);
    QObject::connect(m_network, &IcomNetwork::authenticationFailed, this, &IcomRadio::onNetworkAuthFailed);

    // Create poll timer (for radios that don't push updates automatically)
    m_pollTimer = new QTimer(this);
    QObject::connect(m_pollTimer, &QTimer::timeout, this, &IcomRadio::pollRadio);

    m_state.isValid = false;
}

IcomRadio::~IcomRadio()
{
    disconnect();
}

void IcomRadio::debugTestSlot(int testValue)
{
    LOG_INFO("IcomRadio", QString("debugTestSlot called with value: %1").arg(testValue));
}

bool IcomRadio::connect(const RadioConfig& config)
{
    LOG_INFO("IcomRadio", QString("Connecting to Icom radio at %1").arg(config.port));

    // Parse port as "IP:PORT"
    QStringList parts = config.port.split(':');
    if (parts.size() != 2) {
        LOG_ERROR("IcomRadio", "Invalid port format. Expected IP:PORT (e.g., 192.168.1.100:50001)");
        emit errorOccurred("Invalid port format. Expected IP:PORT");
        return false;
    }

    m_civAddress = config.civAddress;

    // Configure network connection
    IcomConnectionConfig netConfig;
    netConfig.ipAddress = parts[0];
    netConfig.controlPort = parts[1].toUInt();
    netConfig.username = config.icomUsername;
    netConfig.password = config.icomPassword;
    netConfig.clientName = config.icomClientName;

    m_network->connectToRadio(netConfig);

    // Start polling timer
    if (config.pollInterval > 0) {
        m_pollTimer->start(config.pollInterval);
        LOG_DEBUG("IcomRadio", QString("Poll timer started with interval %1ms").arg(config.pollInterval));
    } else {
        LOG_WARN("IcomRadio", "Poll timer NOT started (pollInterval is 0)");
    }

    return true;
}

void IcomRadio::disconnect()
{
    m_pollTimer->stop();
    m_network->disconnectFromRadio();

    m_stateMutex.lock();
    m_state.isValid = false;
    m_stateMutex.unlock();
}

bool IcomRadio::isConnected() const
{
    return m_network->isConnected();
}

bool IcomRadio::setFrequency(freq_t freq, VFO vfo)
{
    QByteArray bcd = frequencyToBcd(freq);

    // CI-V command 0x05 = Set frequency
    // CI-V command 0x25 = Set frequency VFO B (on dual VFO radios)
    quint8 cmd = (vfo == VFO::VFO_A) ? 0x05 : 0x25;

    bool success = sendCommand(cmd, bcd);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        if (vfo == VFO::VFO_A) {
            m_state.frequencyA = freq;
            m_state.bandA = frequencyToBand(freq);
        } else {
            m_state.frequencyB = freq;
            m_state.bandB = frequencyToBand(freq);
        }
        emit frequencyChanged(freq, vfo);
    }

    return success;
}

bool IcomRadio::setBand(BandType band, VFO vfo)
{
    // Use band edge frequency
    // bandToBaseFrequency() returns kHz, but setFrequency() expects Hz
    freq_t freqKHz = bandToBaseFrequency(band);
    freq_t freqHz = freqKHz * 1000;
    LOG_DEBUG("IcomRadio", QString("setBand: %1 -> %2 kHz (%3 Hz)")
        .arg(bandToString(band)).arg(freqKHz).arg(freqHz));
    return setFrequency(freqHz, vfo);
}

bool IcomRadio::setMode(ModeType mode, VFO vfo)
{
    quint8 icomMode = modeToIcom(mode);
    if (icomMode == 0xFF) {
        LOG_WARN("IcomRadio", QString("Unsupported mode: %1").arg(static_cast<int>(mode)));
        return false;
    }

    QByteArray data;
    data.append(icomMode);
    data.append(0x01);  // Filter setting (01 = FIL1, normal default)

    // CI-V command 0x06 = Set mode
    // CI-V command 0x26 = Set mode VFO B
    quint8 cmd = (vfo == VFO::VFO_A) ? 0x06 : 0x26;

    bool success = sendCommand(cmd, data);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        if (vfo == VFO::VFO_A) {
            m_state.modeA = mode;
        } else {
            m_state.modeB = mode;
        }
        emit modeChanged(mode, vfo);
    }

    return success;
}

bool IcomRadio::setPTT(bool transmit)
{
    QByteArray data;
    data.append(transmit ? 0x01 : 0x00);

    // CI-V command 0x1C 0x00 = Set PTT
    QByteArray cmdData;
    cmdData.append(static_cast<char>(0x00));
    cmdData.append(data);

    bool success = sendCommand(0x1C, cmdData);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        m_state.isTransmitting = transmit;
        emit pttChanged(transmit);
    }

    return success;
}

bool IcomRadio::sendCW(const QString& text)
{
    // CI-V command 0x17 = Send CW message
    QByteArray data;
    data.append(text.toUtf8());

    return sendCommand(0x17, data);
}

bool IcomRadio::setCWSpeed(int wpm)
{
    // CI-V command 0x14 0x0C = Set CW speed
    // IC-7760 uses 0-255 value range encoded as 2 BCD bytes
    wpm = qBound(6, wpm, 48);  // Typical Icom range: 6-48 WPM

    // Convert WPM to 0-255 value
    int value = ((wpm - 6) * 255) / 42;
    value = qBound(0, value, 255);

    // Convert value to 2 BCD bytes (big-endian)
    // Example: value=108 → high=01, low=08
    int hundreds = value / 100;
    int tens = (value % 100) / 10;
    int ones = value % 10;

    quint8 bcdHigh = ((hundreds / 10) << 4) | (hundreds % 10);  // Hundreds/thousands
    quint8 bcdLow = (tens << 4) | ones;  // Tens/ones

    QByteArray data;
    data.append(static_cast<char>(0x0C));  // Sub-command for CW speed
    data.append(static_cast<char>(bcdHigh));
    data.append(static_cast<char>(bcdLow));

    LOG_DEBUG("IcomRadio", QString("setCWSpeed: wpm=%1 -> value=%2 -> BCD 0x%3 0x%4")
        .arg(wpm).arg(value)
        .arg(bcdHigh, 2, 16, QChar('0'))
        .arg(bcdLow, 2, 16, QChar('0')));

    bool success = sendCommand(0x14, data);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        m_state.cwSpeed = wpm;
    }

    return success;
}

bool IcomRadio::stopCW()
{
    // Send empty CW message to abort
    return sendCommand(0x17, QByteArray());
}

bool IcomRadio::waitForMorseComplete()
{
    // Poll TX status until not transmitting
    // This is a simplified implementation
    while (getPTT()) {
        QThread::msleep(10);
    }
    return true;
}

bool IcomRadio::setRIT(int offset_hz, VFO vfo)
{
    // Convert Hz to radio units (typically 10 Hz steps)
    qint16 offset = offset_hz / 10;

    QByteArray data;
    data.append(offset & 0xFF);
    data.append((offset >> 8) & 0xFF);

    // CI-V command 0x21 = Set RIT offset
    bool success = sendCommand(0x21, data);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        if (vfo == VFO::VFO_A) {
            m_state.ritOffsetA = offset_hz;
        } else {
            m_state.ritOffsetB = offset_hz;
        }
        emit ritChanged(offset_hz, vfo);
    }

    return success;
}

bool IcomRadio::setXIT(int offset_hz, VFO vfo)
{
    // XIT uses same command as RIT but with different subcommand
    qint16 offset = offset_hz / 10;

    QByteArray data;
    data.append(offset & 0xFF);
    data.append((offset >> 8) & 0xFF);

    // CI-V command 0x21 = Set XIT offset (same as RIT on most Icom radios)
    bool success = sendCommand(0x21, data);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        if (vfo == VFO::VFO_A) {
            m_state.xitOffsetA = offset_hz;
        } else {
            m_state.xitOffsetB = offset_hz;
        }
        emit xitChanged(offset_hz, vfo);
    }

    return success;
}

bool IcomRadio::clearRIT(VFO vfo)
{
    return setRIT(0, vfo);
}

bool IcomRadio::clearXIT(VFO vfo)
{
    return setXIT(0, vfo);
}

bool IcomRadio::enableRIT(bool enable, VFO vfo)
{
    QByteArray data;
    data.append(enable ? 0x01 : 0x00);

    // CI-V command 0x21 0x01 = RIT on/off
    QByteArray cmdData;
    cmdData.append(0x01);
    cmdData.append(data);

    bool success = sendCommand(0x21, cmdData);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        m_state.isRitEnabled = enable;
    }

    return success;
}

bool IcomRadio::enableXIT(bool enable, VFO vfo)
{
    QByteArray data;
    data.append(enable ? 0x01 : 0x00);

    // CI-V command 0x21 0x02 = XIT on/off
    QByteArray cmdData;
    cmdData.append(0x02);
    cmdData.append(data);

    bool success = sendCommand(0x21, cmdData);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        m_state.isXitEnabled = enable;
    }

    return success;
}

bool IcomRadio::setSplit(bool enable, VFO txVfo)
{
    QByteArray data;
    data.append(enable ? 0x01 : 0x00);

    // CI-V command 0x0F = Set split
    bool success = sendCommand(0x0F, data);

    if (success) {
        QMutexLocker lock(&m_stateMutex);
        m_state.isSplitEnabled = enable;
        emit splitChanged(enable);
    }

    return success;
}

bool IcomRadio::vfoBumpUp(VFO vfo)
{
    // CI-V command 0x0F = Tuning dial step up
    return sendCommand(0x0F, QByteArray());
}

bool IcomRadio::vfoBumpDown(VFO vfo)
{
    // CI-V command 0x0E = Tuning dial step down
    return sendCommand(0x0E, QByteArray());
}

bool IcomRadio::setFilterWidth(int width_hz)
{
    // Filter width is mode-dependent on Icom radios
    // This is a simplified implementation
    LOG_WARN("IcomRadio", "setFilterWidth not fully implemented for Icom radios");
    return false;
}

freq_t IcomRadio::getFrequency(VFO vfo) const
{
    QMutexLocker lock(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.frequencyA : m_state.frequencyB;
}

ModeType IcomRadio::getMode(VFO vfo) const
{
    QMutexLocker lock(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.modeA : m_state.modeB;
}

bool IcomRadio::getPTT() const
{
    QMutexLocker lock(&m_stateMutex);
    return m_state.isTransmitting;
}

int IcomRadio::getCWSpeed() const
{
    QMutexLocker lock(&m_stateMutex);
    return m_state.cwSpeed;
}

int IcomRadio::getRIT(VFO vfo) const
{
    QMutexLocker lock(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.ritOffsetA : m_state.ritOffsetB;
}

int IcomRadio::getXIT(VFO vfo) const
{
    QMutexLocker lock(&m_stateMutex);
    return (vfo == VFO::VFO_A) ? m_state.xitOffsetA : m_state.xitOffsetB;
}

bool IcomRadio::getSplit() const
{
    QMutexLocker lock(&m_stateMutex);
    return m_state.isSplitEnabled;
}

int IcomRadio::getFilterWidth() const
{
    QMutexLocker lock(&m_stateMutex);
    return m_state.filterWidth;
}

RadioState IcomRadio::getCurrentState() const
{
    QMutexLocker lock(&m_stateMutex);
    return m_state;
}

QList<ModeType> IcomRadio::getSupportedModes() const
{
    // All Icom radios support these basic modes
    return {
        ModeType::LSB,
        ModeType::USB,
        ModeType::CW,
        ModeType::RTTY,
        ModeType::AM,
        ModeType::FM
    };
}

bool IcomRadio::supportsCWSending() const
{
    return true;  // All Icom network radios support CW sending
}

void IcomRadio::onNetworkConnected()
{
    LOG_INFO("IcomRadio", "Network connected, initializing radio state");

    m_stateMutex.lock();
    m_state.isValid = true;
    m_state.radioModel = m_network->currentRadio().name;
    m_stateMutex.unlock();

    // If CI-V address is 0 (auto/default), use the address discovered from radio
    if (m_civAddress == 0) {
        m_civAddress = m_network->currentRadio().civAddress;
        LOG_INFO("IcomRadio", QString("Using discovered CI-V address: 0x%1")
            .arg(m_civAddress, 2, 16, QChar('0')));
    }

    emit connectionStatusChanged(true);

    // Emit initial state with radio model name
    // This allows RadioManager to update status: "Radio: IC-7760"
    emit stateUpdated(getCurrentState());

    // Don't poll yet - wait for civSocketReady signal from network
    // The radio sends a "CI-V socket ready" control packet when it's ready
}

void IcomRadio::onNetworkDisconnected()
{
    LOG_INFO("IcomRadio", "Network disconnected");

    m_stateMutex.lock();
    m_state.isValid = false;
    m_stateMutex.unlock();

    emit connectionStatusChanged(false);
}

void IcomRadio::onCivDataReceived(const QByteArray& data)
{
    LOG_DEBUG("IcomRadio", QString("Received CI-V data: %1 bytes").arg(data.size()));
    parseCivResponse(data);
}

void IcomRadio::onCivSocketReady()
{
    LOG_INFO("IcomRadio", "CI-V socket ready - waiting 200ms for radio to process open before sending commands");

    // CRITICAL: wfview waits for radio acknowledgment after CI-V open before sending commands.
    // The radio needs time to process the open packet. Without this delay, commands sent
    // immediately after open are ignored (radio only sends ACKs, no CI-V data).
    const int CI_V_OPEN_DELAY_MS = 200;

    QTimer::singleShot(CI_V_OPEN_DELAY_MS, this, [this]() {
        LOG_INFO("IcomRadio", "Delay complete - now sending transceiver ID query (like wfview)");

        // wfview sends a transceiver ID query (0x19 0x00) to broadcast address 0x00 first
        // This might be required to initialize the CI-V interface
        QByteArray initCmd;
        initCmd.append(static_cast<char>(0xFE));
        initCmd.append(static_cast<char>(0xFE));
        initCmd.append(static_cast<char>(0x00));  // Broadcast address (like wfview)
        initCmd.append(static_cast<char>(0xE1));  // Controller address
        initCmd.append(static_cast<char>(0x19));  // Get transceiver ID command
        initCmd.append(static_cast<char>(0x00));  // Sub-command
        initCmd.append(static_cast<char>(0xFD));

        LOG_DEBUG("IcomRadio", QString("Sending transceiver ID query: %1").arg(QString(initCmd.toHex(' '))));
        m_network->sendCivCommand(initCmd);

        // Now start normal polling
        pollRadio();
    });
}

void IcomRadio::onNetworkError(const QString& error)
{
    LOG_ERROR("IcomRadio", QString("Network error: %1").arg(error));
    emit errorOccurred(error);
}

void IcomRadio::onNetworkAuthFailed(const QString& reason)
{
    LOG_ERROR("IcomRadio", QString("Authentication failed: %1").arg(reason));
    emit errorOccurred("Authentication failed: " + reason);
}

void IcomRadio::pollRadio()
{
    if (!isConnected()) {
        LOG_DEBUG("IcomRadio", "pollRadio called but not connected, skipping");
        return;
    }

    LOG_DEBUG("IcomRadio", QString("pollRadio: Sending CI-V commands (address=0x%1)")
        .arg(m_civAddress, 2, 16, QChar('0')));

    // Request frequency VFO A (command 0x03)
    sendCommand(0x03, QByteArray());

    // Request mode VFO A (command 0x04)
    sendCommand(0x04, QByteArray());

    // Request frequency VFO B (command 0x25 - supported on dual-receiver radios like IC-7610, IC-7760)
    sendCommand(0x25, QByteArray());

    // Request mode VFO B (command 0x26)
    sendCommand(0x26, QByteArray());

    // Request PTT status (command 0x1C 0x00)
    QByteArray pttCmd;
    pttCmd.append(static_cast<char>(0x00));
    sendCommand(0x1C, pttCmd);

    // Request Split status (command 0x0F)
    sendCommand(0x0F, QByteArray());

    // Request shared RIT/XIT offset (command 0x21 0x00)
    // IC-7760 shares one offset value for both RIT and XIT (like K4)
    QByteArray offsetCmd;
    offsetCmd.append(static_cast<char>(0x00));
    sendCommand(0x21, offsetCmd);

    // Request RIT on/off status (command 0x21 0x01)
    QByteArray ritOnOffCmd;
    ritOnOffCmd.append(static_cast<char>(0x01));
    sendCommand(0x21, ritOnOffCmd);

    // Request XIT on/off status (command 0x21 0x02)
    QByteArray xitOnOffCmd;
    xitOnOffCmd.append(static_cast<char>(0x02));
    sendCommand(0x21, xitOnOffCmd);

    // Request CW speed (command 0x14 0x0C)
    QByteArray cwSpeedCmd;
    cwSpeedCmd.append(static_cast<char>(0x0C));
    sendCommand(0x14, cwSpeedCmd);
}

QByteArray IcomRadio::buildCivCommand(quint8 command, const QByteArray& data)
{
    QByteArray cmd;
    cmd.append(0xFE);  // Preamble
    cmd.append(0xFE);  // Preamble
    cmd.append(m_civAddress);  // Radio address
    cmd.append(0xE1);  // Controller address (0xE1 like wfview, not 0xE0)
    cmd.append(command);
    cmd.append(data);
    cmd.append(0xFD);  // End marker

    return cmd;
}

bool IcomRadio::sendCommand(quint8 command, const QByteArray& data)
{
    if (!isConnected()) {
        LOG_DEBUG("IcomRadio", QString("sendCommand(0x%1) called but not connected")
            .arg(command, 2, 16, QChar('0')));
        return false;
    }

    QByteArray cmd = buildCivCommand(command, data);
    LOG_DEBUG("IcomRadio", QString("Sending CI-V command 0x%1 to address 0x%2 (%3 bytes)")
        .arg(command, 2, 16, QChar('0'))
        .arg(m_civAddress, 2, 16, QChar('0'))
        .arg(cmd.size()));
    m_network->sendCivCommand(cmd);

    return true;
}

QByteArray IcomRadio::frequencyToBcd(freq_t freq)
{
    // Convert frequency to 10-digit BCD, LSB first
    // freq_t is double, so convert to unsigned long long for integer operations
    quint64 freqInt = static_cast<quint64>(freq);
    QByteArray bcd;
    for (int i = 0; i < 5; i++) {
        quint8 lo = freqInt % 10;
        freqInt /= 10;
        quint8 hi = freqInt % 10;
        freqInt /= 10;
        bcd.append(static_cast<char>((hi << 4) | lo));
    }
    return bcd;
}

freq_t IcomRadio::bcdToFrequency(const QByteArray& bcd)
{
    freq_t freq = 0;
    freq_t multiplier = 1;

    for (int i = 0; i < bcd.length() && i < 5; i++) {
        quint8 byte = bcd[i];
        freq += (byte & 0x0F) * multiplier;
        multiplier *= 10;
        freq += ((byte >> 4) & 0x0F) * multiplier;
        multiplier *= 10;
    }

    return freq;
}

quint8 IcomRadio::modeToIcom(ModeType mode)
{
    switch (mode) {
        case ModeType::LSB:   return 0x00;
        case ModeType::USB:   return 0x01;
        case ModeType::AM:    return 0x02;
        case ModeType::CW:    return 0x03;
        case ModeType::RTTY:  return 0x04;
        case ModeType::FM:    return 0x05;
        case ModeType::CWR:   return 0x07;
        case ModeType::RTTYR: return 0x08;
        default:              return 0xFF;
    }
}

ModeType IcomRadio::icomToMode(quint8 icomMode)
{
    switch (icomMode) {
        case 0x00: return ModeType::LSB;
        case 0x01: return ModeType::USB;
        case 0x02: return ModeType::AM;
        case 0x03: return ModeType::CW;
        case 0x04: return ModeType::RTTY;
        case 0x05: return ModeType::FM;
        case 0x07: return ModeType::CWR;
        case 0x08: return ModeType::RTTYR;
        default:   return ModeType::None;
    }
}

void IcomRadio::parseCivResponse(const QByteArray& data)
{
    if (data.length() < 6) {
        LOG_DEBUG("IcomRadio", QString("parseCivResponse: packet too short (%1 bytes)").arg(data.length()));
        return;
    }

    // Check for valid CI-V response: FE FE E0 <radio> <cmd> <data> FD
    if (data[0] != (char)0xFE || data[1] != (char)0xFE) {
        LOG_DEBUG("IcomRadio", QString("parseCivResponse: invalid preamble (got %1 %2)")
            .arg((quint8)data[0], 2, 16, QChar('0'))
            .arg((quint8)data[1], 2, 16, QChar('0')));
        return;
    }

    quint8 cmd = (quint8)data[4];
    QByteArray responseData = data.mid(5, data.length() - 6);  // Strip preamble and FD

    LOG_DEBUG("IcomRadio", QString("parseCivResponse: command=0x%1 dataLen=%2")
        .arg(cmd, 2, 16, QChar('0'))
        .arg(responseData.length()));

    switch (cmd) {
        case 0x03:  // Frequency response VFO A
            parseFrequencyResponse(responseData, VFO::VFO_A);
            break;

        case 0x04:  // Mode response VFO A
            parseModeResponse(responseData, VFO::VFO_A);
            break;

        case 0x25:  // Frequency response VFO B
            parseFrequencyResponse(responseData, VFO::VFO_B);
            break;

        case 0x26:  // Mode response VFO B
            parseModeResponse(responseData, VFO::VFO_B);
            break;

        case 0x0F:  // Split status response
            if (responseData.length() >= 1) {
                QMutexLocker lock(&m_stateMutex);
                m_state.isSplitEnabled = (responseData[0] == 0x01);
                LOG_DEBUG("IcomRadio", QString("Split status: %1").arg(m_state.isSplitEnabled ? "ON" : "OFF"));
            }
            break;

        case 0x21:  // RIT/XIT status and offset responses
            if (responseData.length() >= 1) {
                quint8 subCmd = (quint8)responseData[0];
                LOG_DEBUG("IcomRadio", QString("0x21 response: subCmd=0x%1 len=%2 data=%3")
                    .arg(subCmd, 2, 16, QChar('0'))
                    .arg(responseData.length())
                    .arg(QString(responseData.toHex(' '))));

                if (subCmd == 0x01) {
                    // RIT response - format may vary by radio
                    // IC-7760 on/off status: 0x01 <on/off> (2 bytes total)
                    // IC-7760 offset:        0x01 <on/off> <offset-bcd-3bytes> <sign> (6 bytes)
                    if (responseData.length() == 2) {
                        // On/off status only (push update from front panel)
                        bool enabled = (responseData[1] == 0x01);
                        QMutexLocker lock(&m_stateMutex);
                        m_state.isRitEnabled = enabled;
                        LOG_DEBUG("IcomRadio", QString("RIT status: %1").arg(enabled ? "ON" : "OFF"));
                        emit ritChanged(m_state.ritOffsetA, VFO::VFO_A);
                    } else if (responseData.length() >= 5) {
                        // Check if byte 1 looks like on/off (0x00 or 0x01) vs BCD
                        quint8 byte1 = (quint8)responseData[1];
                        if (byte1 <= 0x01 && responseData.length() >= 6) {
                            // Format: subCmd + on/off + 3-byte BCD + sign (6 bytes total)
                            bool enabled = (byte1 == 0x01);
                            int offset = 0;
                            offset += (responseData[2] & 0x0F);
                            offset += ((responseData[2] >> 4) & 0x0F) * 10;
                            offset += (responseData[3] & 0x0F) * 100;
                            offset += ((responseData[3] >> 4) & 0x0F) * 1000;
                            if (responseData[5] != 0x00) offset = -offset;
                            QMutexLocker lock(&m_stateMutex);
                            m_state.isRitEnabled = enabled;
                            m_state.ritOffsetA = offset;
                            LOG_DEBUG("IcomRadio", QString("RIT: %1, offset: %2 Hz")
                                .arg(enabled ? "ON" : "OFF").arg(offset));
                            emit ritChanged(offset, VFO::VFO_A);
                        } else {
                            // Format: subCmd + 3-byte BCD + sign (5 bytes total)
                            int offset = 0;
                            offset += (responseData[1] & 0x0F);
                            offset += ((responseData[1] >> 4) & 0x0F) * 10;
                            offset += (responseData[2] & 0x0F) * 100;
                            offset += ((responseData[2] >> 4) & 0x0F) * 1000;
                            if (responseData[4] != 0x00) offset = -offset;
                            QMutexLocker lock(&m_stateMutex);
                            m_state.ritOffsetA = offset;
                            LOG_DEBUG("IcomRadio", QString("RIT offset: %1 Hz (no on/off in response)").arg(offset));
                        }
                    }
                } else if (subCmd == 0x02) {
                    // XIT response - same format variations as RIT
                    if (responseData.length() == 2) {
                        // On/off status only (push update from front panel)
                        bool enabled = (responseData[1] == 0x01);
                        QMutexLocker lock(&m_stateMutex);
                        m_state.isXitEnabled = enabled;
                        LOG_DEBUG("IcomRadio", QString("XIT status: %1").arg(enabled ? "ON" : "OFF"));
                        emit xitChanged(m_state.xitOffsetA, VFO::VFO_A);
                    } else if (responseData.length() >= 5) {
                        quint8 byte1 = (quint8)responseData[1];
                        if (byte1 <= 0x01 && responseData.length() >= 6) {
                            // Format: subCmd + on/off + 3-byte BCD + sign
                            bool enabled = (byte1 == 0x01);
                            int offset = 0;
                            offset += (responseData[2] & 0x0F);
                            offset += ((responseData[2] >> 4) & 0x0F) * 10;
                            offset += (responseData[3] & 0x0F) * 100;
                            offset += ((responseData[3] >> 4) & 0x0F) * 1000;
                            if (responseData[5] != 0x00) offset = -offset;
                            QMutexLocker lock(&m_stateMutex);
                            m_state.isXitEnabled = enabled;
                            m_state.xitOffsetA = offset;
                            LOG_DEBUG("IcomRadio", QString("XIT: %1, offset: %2 Hz")
                                .arg(enabled ? "ON" : "OFF").arg(offset));
                            emit xitChanged(offset, VFO::VFO_A);
                        } else {
                            // Format: subCmd + 3-byte BCD + sign
                            int offset = 0;
                            offset += (responseData[1] & 0x0F);
                            offset += ((responseData[1] >> 4) & 0x0F) * 10;
                            offset += (responseData[2] & 0x0F) * 100;
                            offset += ((responseData[2] >> 4) & 0x0F) * 1000;
                            if (responseData[4] != 0x00) offset = -offset;
                            QMutexLocker lock(&m_stateMutex);
                            m_state.xitOffsetA = offset;
                            LOG_DEBUG("IcomRadio", QString("XIT offset: %1 Hz (no on/off in response)").arg(offset));
                        }
                    }
                } else if (subCmd == 0x00 && responseData.length() >= 4) {
                    // Shared RIT/XIT offset (IC-7760, K4 behavior)
                    // Format: 0x00 <bcd-high> <bcd-low> <sign>
                    // BCD is big-endian: high byte = thousands/hundreds, low byte = tens/ones
                    // Example: 0x00 0x48 0x00 = "0048" = 48 Hz, sign 0x00 = positive
                    quint8 bcdHigh = (quint8)responseData[1];
                    quint8 bcdLow = (quint8)responseData[2];
                    quint8 sign = (quint8)responseData[3];

                    // Convert BCD to decimal (big-endian)
                    // High byte 0x00 = "00" (thousands and hundreds)
                    // Low byte 0x48 = "48" (tens and ones)
                    // Result: 0*1000 + 0*100 + 4*10 + 8*1 = 48 Hz
                    int offset = 0;
                    offset += ((bcdHigh >> 4) & 0x0F) * 1000;  // Thousands digit
                    offset += (bcdHigh & 0x0F) * 100;          // Hundreds digit
                    offset += ((bcdLow >> 4) & 0x0F) * 10;     // Tens digit
                    offset += (bcdLow & 0x0F);                 // Ones digit

                    if (sign != 0x00) offset = -offset;

                    QMutexLocker lock(&m_stateMutex);
                    // IC-7760 shares offset for both RIT and XIT (like K4)
                    m_state.ritOffsetA = offset;
                    m_state.xitOffsetA = offset;
                    LOG_DEBUG("IcomRadio", QString("Shared RIT/XIT offset: %1 Hz (BCD 0x%2 0x%3, sign=%4)")
                        .arg(offset)
                        .arg(bcdHigh, 2, 16, QChar('0'))
                        .arg(bcdLow, 2, 16, QChar('0'))
                        .arg(sign));
                } else {
                    LOG_DEBUG("IcomRadio", QString("0x21 unknown subCmd=0x%1 len=%2").arg(subCmd, 2, 16, QChar('0')).arg(responseData.length()));
                }
            }
            break;

        case 0x14:  // Various levels including CW speed
            if (responseData.length() >= 2) {
                quint8 subCmd = (quint8)responseData[0];
                LOG_DEBUG("IcomRadio", QString("0x14 response: subCmd=0x%1 len=%2 data=%3")
                    .arg(subCmd, 2, 16, QChar('0'))
                    .arg(responseData.length())
                    .arg(QString(responseData.toHex(' '))));
                if (subCmd == 0x0C && responseData.length() >= 3) {
                    // CW speed: IC-7760 returns 2 BCD bytes encoding 0-255 value
                    // Format: 0x0C <bcd-high> <bcd-low> (e.g., 0x01 0x08 = 108)
                    // Each byte is BCD (0x01 = decimal 01, 0x08 = decimal 08)
                    // Big-endian: high byte * 100 + low byte
                    quint8 bcdHigh = (quint8)responseData[1];
                    quint8 bcdLow = (quint8)responseData[2];

                    // Convert each BCD byte to decimal
                    int highDecimal = ((bcdHigh >> 4) * 10) + (bcdHigh & 0x0F);
                    int lowDecimal = ((bcdLow >> 4) * 10) + (bcdLow & 0x0F);

                    // Combine: value = high*100 + low (big-endian)
                    int value = highDecimal * 100 + lowDecimal;

                    // Convert 0-255 value to WPM (6-48 range)
                    // Use proper rounding: add half divisor before dividing
                    int wpm = 6 + (value * 42 + 127) / 255;

                    QMutexLocker lock(&m_stateMutex);
                    m_state.cwSpeed = wpm;
                    LOG_DEBUG("IcomRadio", QString("CW speed: %1 WPM (BCD 0x%2 0x%3 = value %4)")
                        .arg(wpm)
                        .arg(bcdHigh, 2, 16, QChar('0'))
                        .arg(bcdLow, 2, 16, QChar('0'))
                        .arg(value));
                }
            }
            break;

        case 0x1C:  // PTT/TX status
            if (responseData.length() >= 2 && responseData[0] == 0x00) {
                parsePTTResponse(responseData.mid(1));
            }
            break;

        case 0x27:  // Scope/transceive data (IC-7610/IC-7760 push updates)
            parseScopeData(responseData);
            break;

        case 0xFB:  // OK response (command acknowledged)
            LOG_DEBUG("IcomRadio", "Command acknowledged (OK)");
            break;

        case 0xFA:  // NG response (command failed)
            LOG_WARN("IcomRadio", "Command failed (NG)");
            break;

        default:
            // Unknown or unhandled response
            LOG_DEBUG("IcomRadio", QString("parseCivResponse: unhandled command 0x%1 (ignoring %2 bytes)")
                .arg(cmd, 2, 16, QChar('0'))
                .arg(responseData.length()));
            break;
    }

    // Emit state update
    emit stateUpdated(getCurrentState());
}

void IcomRadio::parseFrequencyResponse(const QByteArray& data, VFO vfo)
{
    if (data.length() < 5) {
        return;
    }

    freq_t freq = bcdToFrequency(data);

    QMutexLocker lock(&m_stateMutex);
    if (vfo == VFO::VFO_A) {
        m_state.frequencyA = freq;
        m_state.bandA = frequencyToBand(freq);
    } else {
        m_state.frequencyB = freq;
        m_state.bandB = frequencyToBand(freq);
    }

    emit frequencyChanged(freq, vfo);
}

void IcomRadio::parseModeResponse(const QByteArray& data, VFO vfo)
{
    if (data.length() < 1) {
        return;
    }

    ModeType mode = icomToMode((quint8)data[0]);

    QMutexLocker lock(&m_stateMutex);
    if (vfo == VFO::VFO_A) {
        m_state.modeA = mode;
    } else {
        m_state.modeB = mode;
    }

    emit modeChanged(mode, vfo);
}

void IcomRadio::parsePTTResponse(const QByteArray& data)
{
    if (data.length() < 1) {
        return;
    }

    bool transmitting = (data[0] == 0x01);

    QMutexLocker lock(&m_stateMutex);
    m_state.isTransmitting = transmitting;

    emit pttChanged(transmitting);
}

void IcomRadio::parseScopeData(const QByteArray& data)
{
    // Command 0x27 scope/transceive data from IC-7610/IC-7760
    // Structure (after command byte):
    //   Bytes 0-1: Sub-command (0x00 0x00)
    //   Bytes 2-3: Fixed/Edge indicator (0x01 0x01)
    //   Byte 4: Unknown (0x00)
    //   Bytes 5-9: Frequency (5 bytes BCD, little-endian)
    //   Byte 10: Mode
    //   Byte 11+: Filter and scope data

    if (data.length() < 11) {
        LOG_DEBUG("IcomRadio", QString("parseScopeData: packet too short (%1 bytes, need 11+)")
            .arg(data.length()));
        return;
    }

    // Extract frequency (bytes 5-9)
    QByteArray freqData = data.mid(5, 5);
    freq_t freq = bcdToFrequency(freqData);

    // Extract mode (byte 10)
    ModeType mode = icomToMode((quint8)data[10]);

    LOG_DEBUG("IcomRadio", QString("parseScopeData: freq=%1 Hz, mode=%2")
        .arg(freq)
        .arg(modeToString(mode)));

    // Update state
    QMutexLocker lock(&m_stateMutex);
    m_state.frequencyA = freq;
    m_state.bandA = frequencyToBand(freq);
    m_state.modeA = mode;

    // Emit signals
    emit frequencyChanged(freq, VFO::VFO_A);
    emit modeChanged(mode, VFO::VFO_A);
}

} // namespace TR4QT
