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
    freq_t freq = bandToBaseFrequency(band);
    return setFrequency(freq, vfo);
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
    QByteArray data;
    data.append(0x0C);

    // Convert WPM to BCD (0-255)
    quint8 tens = wpm / 10;
    quint8 ones = wpm % 10;
    data.append((tens << 4) | ones);

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

    emit connectionStatusChanged(true);

    // Request initial state
    pollRadio();
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
    parseCivResponse(data);
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
        return;
    }

    // Request frequency VFO A (command 0x03)
    sendCommand(0x03, QByteArray());

    // Request mode VFO A (command 0x04)
    sendCommand(0x04, QByteArray());

    // Request PTT status (command 0x1C 0x00)
    QByteArray pttCmd;
    pttCmd.append(static_cast<char>(0x00));
    sendCommand(0x1C, pttCmd);
}

QByteArray IcomRadio::buildCivCommand(quint8 command, const QByteArray& data)
{
    QByteArray cmd;
    cmd.append(0xFE);  // Preamble
    cmd.append(0xFE);  // Preamble
    cmd.append(m_civAddress);  // Radio address
    cmd.append(0xE0);  // Controller address
    cmd.append(command);
    cmd.append(data);
    cmd.append(0xFD);  // End marker

    return cmd;
}

bool IcomRadio::sendCommand(quint8 command, const QByteArray& data)
{
    if (!isConnected()) {
        return false;
    }

    QByteArray cmd = buildCivCommand(command, data);
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
        return;
    }

    // Check for valid CI-V response: FE FE E0 <radio> <cmd> <data> FD
    if (data[0] != (char)0xFE || data[1] != (char)0xFE) {
        return;
    }

    quint8 cmd = (quint8)data[4];
    QByteArray responseData = data.mid(5, data.length() - 6);  // Strip preamble and FD

    switch (cmd) {
        case 0x03:  // Frequency response
            parseFrequencyResponse(responseData, VFO::VFO_A);
            break;

        case 0x04:  // Mode response
            parseModeResponse(responseData, VFO::VFO_A);
            break;

        case 0x1C:  // PTT/TX status
            if (responseData.length() >= 2 && responseData[0] == 0x00) {
                parsePTTResponse(responseData.mid(1));
            }
            break;

        default:
            // Unknown or unhandled response
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

} // namespace TR4QT
