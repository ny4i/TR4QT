#include "KPA1500Direct.h"
#include "../logging/LogMacros.h"

#include <QUdpSocket>
#include <QTimer>
#include <QtMath>
#include <QCoreApplication>

namespace TR4QT {

KPA1500Direct::KPA1500Direct(QObject* parent)
    : IAmplifierController(parent)
{
    initDispatchTable();

    // Default poll commands (can be customized via setPollCommands())
    m_pollCommands = {
        "^PWF;",  // Forward power
        "^PWR;",  // Reflected power
        "^SW;",   // SWR
        "^FR;",   // Frequency (kHz)
        "^OS;",   // Operating status (Operate/Standby)
        "^TM;",   // Temperature
        "^VI;",   // Input voltage
        "^FL;"    // Fault code
    };
}

KPA1500Direct::~KPA1500Direct()
{
    disconnect();
}

bool KPA1500Direct::connect(const AmplifierConfig& config)
{
    if (m_connected) {
        disconnect();
    }

    m_config = config;

    // Parse IP address and port from config.port (format: "192.168.1.100:1500")
    QStringList parts = config.port.split(":");
    if (parts.size() != 2) {
        LOG_ERROR("KPA1500Direct", QString("Invalid port format: %1 (expected IP:port)").arg(config.port));
        emit errorOccurred(QString("Invalid port format: %1 (expected IP:port)").arg(config.port));
        return false;
    }

    m_addr = QHostAddress(parts[0]);
    bool ok = false;
    m_port = parts[1].toUShort(&ok);

    if (!ok || m_port == 0) {
        LOG_ERROR("KPA1500Direct", QString("Invalid port number: %1").arg(parts[1]));
        emit errorOccurred(QString("Invalid port number: %1").arg(parts[1]));
        return false;
    }

    if (m_addr.isNull()) {
        LOG_ERROR("KPA1500Direct", QString("Invalid IP address: %1").arg(parts[0]));
        emit errorOccurred(QString("Invalid IP address: %1").arg(parts[0]));
        return false;
    }

    // Create UDP socket
    m_socket = new QUdpSocket(this);
    QObject::connect(m_socket, &QUdpSocket::readyRead, this, &KPA1500Direct::onReadyRead);

    // Create poll timer
    m_timer = new QTimer(this);
    m_timer->setInterval(m_intervalMs);
    QObject::connect(m_timer, &QTimer::timeout, this, &KPA1500Direct::doPollCycle);

    m_connected = true;
    m_currentState.connected = true;
    m_timer->start();

    LOG_INFO("KPA1500Direct", QString("Connected to KPA1500 at %1:%2").arg(parts[0]).arg(m_port));
    emit connectionStatusChanged(true);

    return true;
}

void KPA1500Direct::disconnect()
{
    if (!m_connected)
        return;

    m_connected = false;

    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }

    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    // Reset state
    m_currentState = AmplifierState{};

    emit connectionStatusChanged(false);
}

bool KPA1500Direct::isConnected() const
{
    return m_connected;
}

AmplifierState KPA1500Direct::getState() const
{
    return m_currentState;
}

void KPA1500Direct::setFrequency(freq_t freq)
{
    // KPA1500 UDP protocol doesn't support frequency setting commands
    // (frequency tracking is automatic via RF sensing)
    // Update cached state for informational purposes
    m_currentState.frequency = freq;

    LOG_DEBUG("KPA1500Direct", QString("Frequency set to %1 Hz (informational only, KPA1500 auto-tracks via RF sensing)").arg(freq));
}

void KPA1500Direct::queryStatus()
{
    if (!m_connected || m_addr.isNull()) {
        emit errorOccurred(QStringLiteral("KPA1500Direct: amplifier not connected"));
        return;
    }

    // Create temporary socket if not already running
    bool wasConnected = m_connected;
    QUdpSocket* socket = m_socket;

    if (!socket) {
        socket = new QUdpSocket(this);
        QObject::connect(socket, &QUdpSocket::readyRead, this, &KPA1500Direct::onReadyRead);
    }

    // Send poll commands
    for (const QString &cmdStr : m_pollCommands) {
        QByteArray cmd = cmdStr.toUtf8();
        if (!cmd.isEmpty() && cmd.front() == '^' && cmd.back() == ';') {
            qint64 sent = socket->writeDatagram(cmd, m_addr, m_port);
            if (sent != cmd.size()) {
                emit errorOccurred(QStringLiteral("Failed to send command '%1'").arg(cmdStr));
            }
        }
    }

    // Clean up temporary socket if we created one
    if (!wasConnected && socket != m_socket) {
        // Process any immediate responses
        QCoreApplication::processEvents();
        socket->deleteLater();
    }
}

void KPA1500Direct::sendRawCommand(const QString& command)
{
    if (!m_connected || !m_socket) {
        LOG_WARN("KPA1500Direct", QString("Cannot send command '%1': not connected").arg(command));
        emit errorOccurred(QString("Cannot send command: amplifier not connected"));
        return;
    }

    QByteArray cmd = command.toUtf8();
    LOG_TRACE("KPA1500Direct", QString("Sending raw command: %1").arg(command));
    sendCommand(cmd);
}

void KPA1500Direct::setPollIntervalMs(int intervalMs)
{
    m_intervalMs = intervalMs;
    if (m_timer && m_connected) {
        m_timer->setInterval(intervalMs);
    }
}

void KPA1500Direct::setPollCommands(const QStringList& commands)
{
    m_pollCommands = commands;
}

void KPA1500Direct::initDispatchTable()
{
    // Power & SWR
    m_dispatch.insert("^PWF", [this](const QString &r){ handlePWF(r); });
    m_dispatch.insert("^PWI", [this](const QString &r){ handlePWI(r); });
    m_dispatch.insert("^PWR", [this](const QString &r){ handlePWR(r); });
    m_dispatch.insert("^SW",  [this](const QString &r){ handleSW(r);  });

    // Frequency
    m_dispatch.insert("^FR",  [this](const QString &r){ handleFR(r);  });

    // Band / ATU / antenna / fault
    m_dispatch.insert("^BN",  [this](const QString &r){ handleBN(r);  });
    m_dispatch.insert("^OS",  [this](const QString &r){ handleOS(r);  });
    m_dispatch.insert("^AI",  [this](const QString &r){ handleAI(r);  });
    m_dispatch.insert("^AM",  [this](const QString &r){ handleAM(r);  });
    m_dispatch.insert("^AN",  [this](const QString &r){ handleAN(r);  });
    m_dispatch.insert("^FL",  [this](const QString &r){ handleFL(r);  });

    // Additional status / info commands
    m_dispatch.insert("^BT",  [this](const QString &r){ handleBT(r);  }); // banner text
    m_dispatch.insert("^LQ",  [this](const QString &r){ handleLQ(r);  }); // line quality
    m_dispatch.insert("^PC",  [this](const QString &r){ handlePC(r);  }); // drive/input power
    m_dispatch.insert("^SN",  [this](const QString &r){ handleSN(r);  }); // serial number
    m_dispatch.insert("^TM",  [this](const QString &r){ handleTM(r);  }); // temperature
    m_dispatch.insert("^VI",  [this](const QString &r){ handleVI(r);  }); // input voltage
    m_dispatch.insert("^WS",  [this](const QString &r){ handleWS(r);  }); // combined fwd/ref/SWR
}

void KPA1500Direct::doPollCycle()
{
    if (!m_socket || !m_connected)
        return;

    for (const QString &cmdStr : m_pollCommands) {
        QByteArray cmd = cmdStr.toUtf8();
        if (!cmd.isEmpty() && cmd.front() == '^' && cmd.back() == ';')
            sendCommand(cmd);
        else
            emit errorOccurred(QStringLiteral("Invalid command format: %1").arg(cmdStr));
    }
}

void KPA1500Direct::sendCommand(const QByteArray &cmd)
{
    if (!m_socket)
        return;

    qint64 sent = m_socket->writeDatagram(cmd, m_addr, m_port);
    if (sent != cmd.size()) {
        emit errorOccurred(
            QStringLiteral("Failed to send command '%1'").arg(QString::fromUtf8(cmd)));
    }
}

void KPA1500Direct::onReadyRead()
{
    if (!m_socket)
        return;

    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;

        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        Q_UNUSED(sender);
        Q_UNUSED(senderPort);

        processResponse(datagram);
    }

    // Update cached state after processing responses
    updateStateFromPolling();
}

void KPA1500Direct::processResponse(const QByteArray &datagram)
{
    QString text = QString::fromUtf8(datagram).trimmed();

    // All KPA1500 commands/responses start with '^' and end with ';'
    if (!text.startsWith('^') || !text.endsWith(';'))
        return;

    // Emit generic hook with a short key (first 4 chars)
    QString key = text.left(4);
    emit valueUpdated(key, text);

    // Determine dispatch key: for most commands first 3–4 chars are enough
    QString dispatchKey;
    if (text.size() >= 4 && text[3].isLetter())
        dispatchKey = text.left(4); // "^PWF", "^PWI", "^BNx", etc.
    else
        dispatchKey = text.left(3); // "^SW", "^AI", "^AM", "^AN", "^BT", etc.

    auto it = m_dispatch.find(dispatchKey);
    if (it != m_dispatch.end()) {
        (*it)(text);
    } else {
        // Unhandled reply - log at debug level
        LOG_DEBUG("KPA1500Direct", QString("Unhandled reply: %1").arg(text));
    }
}

void KPA1500Direct::updateStateFromPolling()
{
    // Update cached AmplifierState from last-known values
    m_currentState.connected = m_connected;
    m_currentState.isValid = true;
    m_currentState.forwardPowerWatts = m_lastForwardPower > 0 ? m_lastForwardPower : 0;
    m_currentState.reflectedPowerWatts = m_lastReflectedPower > 0 ? m_lastReflectedPower : 0;
    m_currentState.swr = m_lastSwr > 0 ? m_lastSwr : 1.0;
    m_currentState.faultDetected = (m_lastFaultCode > 0);
    m_currentState.faultCode = m_currentState.faultDetected ? QString("Fault code: %1").arg(m_lastFaultCode) : QString();
    m_currentState.temperature = static_cast<int>(m_lastTemperature);
    m_currentState.inputVoltage = m_lastInputVoltage;
    m_currentState.operateMode = m_lastOperatingStatus;

    // Emit stateUpdated signal (defined in IAmplifierController)
    emit stateUpdated(m_currentState);
}

// ------------------- Handlers -------------------

// ^PWFnnnn; Forward power in watts
void KPA1500Direct::handlePWF(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastForwardPower) {
        m_lastForwardPower = w;
        emit forwardPowerChanged(w);
    }
}

// ^PWInnnn; Input power in watts
void KPA1500Direct::handlePWI(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastInputPower) {
        m_lastInputPower = w;
        emit inputPowerChanged(w);
    }
}

// ^PWRnnnn; Reflected power in watts
void KPA1500Direct::handlePWR(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastReflectedPower) {
        m_lastReflectedPower = w;
        emit reflectedPowerChanged(w);
    }
}

// ^SWswr; SWR in tenths, ^SW123; = 12.3:1
void KPA1500Direct::handleSW(const QString &resp)
{
    bool ok = false;
    int tenths = resp.mid(3, resp.size() - 4).toInt(&ok);
    if (!ok) return;
    double swr = tenths / 10.0;
    if (!qFuzzyCompare(swr + 1.0, m_lastSwr + 1.0)) {
        m_lastSwr = swr;
        emit swrChanged(swr);
    }
}

// ^FRfffff; Frequency in kHz (e.g., ^FR14058; = 14.058 MHz)
void KPA1500Direct::handleFR(const QString &resp)
{
    bool ok = false;
    int freqKhz = resp.mid(3, resp.size() - 4).toInt(&ok);
    if (!ok) return;

    // Convert kHz to Hz (Hamlib freq_t is in Hz)
    freq_t freqHz = static_cast<freq_t>(freqKhz) * 1000;

    // Update cached state
    m_currentState.frequency = freqHz;

    LOG_TRACE("KPA1500Direct", QString("Frequency: %1 kHz (%2 Hz)").arg(freqKhz).arg(freqHz));
}

// ^BNbb; Band number 00–10 for 160–6 m
void KPA1500Direct::handleBN(const QString &resp)
{
    bool ok = false;
    int band = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (band != m_lastBandNumber) {
        m_lastBandNumber = band;
        emit bandNumberChanged(band);
    }
}

// ^OS1; Operate mode, ^OS0; Standby mode
void KPA1500Direct::handleOS(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool operateMode = (c == QLatin1Char('1'));
    if (operateMode != m_lastOperatingStatus) {
        m_lastOperatingStatus = operateMode;
        emit operatingStatusChanged(operateMode);
    }
}

// ^AI1; ATU inline, ^AI0; bypassed
void KPA1500Direct::handleAI(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool inlineMode = (c == QLatin1Char('1'));
    if (inlineMode != m_lastAtuInline) {
        m_lastAtuInline = inlineMode;
        emit atuInlineChanged(inlineMode);
    }
}

// ^AM...; ATU Mode Inline/Bypassed variants
void KPA1500Direct::handleAM(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar modeChar = resp[resp.size() - 2]; // last before ';'
    QString mode;
    if (modeChar == QLatin1Char('I'))
        mode = QStringLiteral("Inline");
    else if (modeChar == QLatin1Char('B'))
        mode = QStringLiteral("Bypassed");
    else
        return;

    if (mode != m_lastAtuMode) {
        m_lastAtuMode = mode;
        emit atuModeChanged(mode);
    }
}

// ^ANa; or ^ANaa; Antenna select
void KPA1500Direct::handleAN(const QString &resp)
{
    bool ok = false;
    int ant = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (ant != m_lastAntenna) {
        m_lastAntenna = ant;
        emit antennaChanged(ant);
    }
}

// ^FLnnn; Fault code
void KPA1500Direct::handleFL(const QString &resp)
{
    bool ok = false;
    int code = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (code != m_lastFaultCode) {
        m_lastFaultCode = code;
        if (code > 0) {
            QString faultMsg = QString("Fault code: %1").arg(code);
            emit faultDetected(faultMsg);
        }
    }
}

// ^BTtext; Banner text shown on front panel
void KPA1500Direct::handleBT(const QString &resp)
{
    QString text = resp.mid(3, resp.size() - 4); // between ^BT and ;
    if (text != m_lastBannerText) {
        m_lastBannerText = text;
        emit bannerTextChanged(text);
    }
}

// ^LQnn; Line quality percent
void KPA1500Direct::handleLQ(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int percent = valStr.toInt(&ok);
    if (!ok) return;

    if (percent != m_lastLineQuality) {
        m_lastLineQuality = percent;
        emit lineQualityChanged(percent);
    }
}

// ^PCnnn; Drive/input power setting in watts
void KPA1500Direct::handlePC(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int watts = valStr.toInt(&ok);
    if (!ok) return;

    if (watts != m_lastDrivePower) {
        m_lastDrivePower = watts;
        emit drivePowerChanged(watts);
    }
}

// ^SNxxxxx; Serial number ASCII
void KPA1500Direct::handleSN(const QString &resp)
{
    QString serial = resp.mid(3, resp.size() - 4);
    if (serial != m_lastSerialNumber) {
        m_lastSerialNumber = serial;
        emit serialNumberChanged(serial);
    }
}

// ^TMnnn; Temperature in tenths of a degree C
void KPA1500Direct::handleTM(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int tenths = valStr.toInt(&ok);
    if (!ok) return;

    double celsius = tenths / 10.0;
    if (!qFuzzyCompare(celsius + 1.0, m_lastTemperature + 1.0)) {
        m_lastTemperature = celsius;
        emit temperatureChanged(static_cast<int>(celsius));
    }
}

// ^VInnnn; Input DC voltage in tenths of a volt
void KPA1500Direct::handleVI(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int tenths = valStr.toInt(&ok);
    if (!ok) return;

    double volts = tenths / 10.0;
    if (!qFuzzyCompare(volts + 1.0, m_lastInputVoltage + 1.0)) {
        m_lastInputVoltage = volts;
        // No specific signal for input voltage in IAmplifierController
        // (it's in AmplifierState which is emitted via stateUpdated())
    }
}

// ^WS...; Combined fwd/ref/SWR status
void KPA1500Direct::handleWS(const QString &resp)
{
    // Example: assume ^WSffffrrrrsss; (4 digits fwd, 4 digits ref, 3 digits swr tenths)
    QString payload = resp.mid(3, resp.size() - 4);
    if (payload.size() < 11)
        return;

    bool ok1 = false, ok2 = false, ok3 = false;
    int fwd  = payload.mid(0, 4).toInt(&ok1);
    int ref  = payload.mid(4, 4).toInt(&ok2);
    int swrt = payload.mid(8).toInt(&ok3);

    if (!ok1 || !ok2 || !ok3)
        return;

    double swr = swrt / 10.0;

    bool changed = false;
    if (fwd != m_lastWsFwd) { m_lastWsFwd = fwd; changed = true; }
    if (ref != m_lastWsRef) { m_lastWsRef = ref; changed = true; }
    if (!qFuzzyCompare(swr + 1.0, m_lastWsSwr + 1.0)) {
        m_lastWsSwr = swr;
        changed = true;
    }

    if (changed)
        emit wsStatusChanged(fwd, ref, swr);
}

} // namespace TR4QT
