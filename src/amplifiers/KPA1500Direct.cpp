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
    // Based on KPA1500 Programming Reference - complete status polling
    m_pollCommands = {
        "^PWF;",  // Forward power (watts)
        "^PWI;",  // Input power (watts)
        "^PWR;",  // Reflected power (watts)
        "^SW;",   // SWR (tenths)
        "^SB;",   // SWR when ATU last bypassed (tenths)
        "^OS;",   // Operating status (Operate/Standby)
        "^ON;",   // Power state (on/off)
        "^BN;",   // Band number (00-10 for 160m-6m)
        "^FQ;",   // TX frequency counter (kHz)
        "^AN;",   // Antenna selection
        "^AI;",   // ATU inline/bypassed
        "^AM;",   // ATU mode
        "^TP;",   // Tune in progress
        "^PC;",   // Drive/input power setting
        "^TM;",   // Temperature (°C)
        "^VMH;",  // 50V supply voltage (millivolts)
        "^DS;",   // LCD display content
        "^LQ;",   // LED states (hex bitmap)
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

    // Get poll interval from config
    m_intervalMs = config.pollIntervalMs;
    LOG_INFO("KPA1500Direct", QString("Using poll interval: %1ms").arg(m_intervalMs));

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
    m_dispatch.insert("^LQ",  [this](const QString &r){ handleLQ(r);  }); // LED states (hex)
    m_dispatch.insert("^PC",  [this](const QString &r){ handlePC(r);  }); // drive/input power
    m_dispatch.insert("^SN",  [this](const QString &r){ handleSN(r);  }); // serial number
    m_dispatch.insert("^TM",  [this](const QString &r){ handleTM(r);  }); // temperature
    m_dispatch.insert("^VI",  [this](const QString &r){ handleVI(r);  }); // input voltage
    m_dispatch.insert("^WS",  [this](const QString &r){ handleWS(r);  }); // combined fwd/ref/SWR

    // New commands (KPA1500 Programming Reference)
    m_dispatch.insert("^SB",  [this](const QString &r){ handleSB(r);  }); // SWR bypass
    m_dispatch.insert("^TP",  [this](const QString &r){ handleTP(r);  }); // Tune in progress
    m_dispatch.insert("^DS",  [this](const QString &r){ handleDS(r);  }); // LCD display content
    m_dispatch.insert("^FQ",  [this](const QString &r){ handleFQ(r);  }); // TX frequency
    m_dispatch.insert("^VMH", [this](const QString &r){ handleVMH(r); }); // 50V supply voltage
    m_dispatch.insert("^ON",  [this](const QString &r){ handleON(r);  }); // Power state
}

void KPA1500Direct::doPollCycle()
{
    if (!m_socket || !m_connected)
        return;

    for (const QString &cmdStr : m_pollCommands) {
        // DS command has longer interval to reduce load on amplifier
        if (cmdStr == "^DS;") {
            // Use QElapsedTimer for proper elapsed time measurement
            // Timer is invalid until first start(), so first call always proceeds
            if (m_dsSendTimer.isValid() && m_dsSendTimer.elapsed() < DS_POLL_INTERVAL_MS) {
                LOG_TRACE("KPA1500Direct", QString("Skipping DS: %1ms elapsed (need %2ms)")
                    .arg(m_dsSendTimer.elapsed()).arg(DS_POLL_INTERVAL_MS));
                continue;  // Skip DS this cycle
            }
            if (m_dsSendTimer.isValid()) {
                LOG_TRACE("KPA1500Direct", QString("Sending DS: %1ms since last").arg(m_dsSendTimer.elapsed()));
            } else {
                LOG_TRACE("KPA1500Direct", "Sending DS: first poll");
            }
            m_dsSendTimer.start();  // Restart timer
        }

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

    // Determine dispatch key: try 4-char first (^PWF, ^PWI, ^VMH), fallback to 3-char (^SW, ^AM, ^AI)
    // Some commands like ^AM have letter values (^AMI, ^AMB) that look like 4-char commands
    QString dispatchKey;
    auto it = m_dispatch.end();

    // Try 4-char key first if 4th char is a letter
    if (text.size() >= 4 && text[3].isLetter()) {
        dispatchKey = text.left(4);
        it = m_dispatch.find(dispatchKey);
    }

    // Fallback to 3-char key if 4-char not found
    if (it == m_dispatch.end()) {
        dispatchKey = text.left(3);
        it = m_dispatch.find(dispatchKey);
    }

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
    m_currentState.lcdLine1 = m_lastDisplayLine1;
    m_currentState.lcdLine2 = m_lastDisplayLine2;

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

// ^LQppppppppssssmm; LED states as hex digits
// pppppppp = power bar bitmap (32-bit), ssss = SWR bar bitmap (16-bit), mm = status LEDs (8-bit)
// Status LED bits: x80=FAULT, x40=OVR, x20=ANT2, x10=ANT1, x08=ATU IN, x04=ATU BYP, x02=OPER, x01=TX
void KPA1500Direct::handleLQ(const QString &resp)
{
    // Format: ^LQppppppppssssmm; (16 hex chars between ^LQ and ;)
    QString payload = resp.mid(3, resp.size() - 4);
    if (payload.size() < 16) return;

    bool ok1 = false, ok2 = false, ok3 = false;
    quint32 powerBar = payload.mid(0, 8).toUInt(&ok1, 16);
    quint16 swrBar = payload.mid(8, 4).toUShort(&ok2, 16);
    quint8 statusLeds = static_cast<quint8>(payload.mid(12, 2).toUShort(&ok3, 16));

    if (!ok1 || !ok2 || !ok3) return;

    bool changed = false;
    if (powerBar != m_lastLedPowerBar) { m_lastLedPowerBar = powerBar; changed = true; }
    if (swrBar != m_lastLedSwrBar) { m_lastLedSwrBar = swrBar; changed = true; }
    if (statusLeds != m_lastLedStatus) { m_lastLedStatus = statusLeds; changed = true; }

    if (changed) {
        emit ledStateChanged(powerBar, swrBar, statusLeds);
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

// ^TMxxx; Temperature in degrees C (NOT tenths - per KPA1500 protocol docs)
void KPA1500Direct::handleTM(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int celsius = valStr.toInt(&ok);
    if (!ok) return;

    if (celsius != static_cast<int>(m_lastTemperature)) {
        m_lastTemperature = celsius;
        emit temperatureChanged(celsius);
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

// ^SBswr; SWR when ATU last bypassed (tenths, 123 = 12.3:1)
void KPA1500Direct::handleSB(const QString &resp)
{
    bool ok = false;
    int tenths = resp.mid(3, resp.size() - 4).toInt(&ok);
    if (!ok) return;

    double swr = tenths / 10.0;
    if (!qFuzzyCompare(swr + 1.0, m_lastSwrBypass + 1.0)) {
        m_lastSwrBypass = swr;
        emit swrBypassChanged(swr);
    }
}

// ^TP0; ATU not tuning, ^TP1; ATU tune in progress
void KPA1500Direct::handleTP(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool tuning = (c == QLatin1Char('1'));
    if (tuning != m_lastTuneInProgress) {
        m_lastTuneInProgress = tuning;
        emit tuneInProgressChanged(tuning);
    }
}

// ^DSFirstLCD Line\nSecond LCD line; LCD display content
void KPA1500Direct::handleDS(const QString &resp)
{
    QString payload = resp.mid(3, resp.size() - 4); // between ^DS and ;

    // Split on newline (0x0A)
    int nlPos = payload.indexOf(QLatin1Char('\n'));
    QString line1, line2;
    if (nlPos >= 0) {
        line1 = payload.left(nlPos);
        line2 = payload.mid(nlPos + 1);
    } else {
        line1 = payload;
        line2.clear();
    }

    LOG_TRACE("KPA1500Direct", QString("^DS response: line1='%1' line2='%2'").arg(line1, line2));

    if (line1 != m_lastDisplayLine1 || line2 != m_lastDisplayLine2) {
        m_lastDisplayLine1 = line1;
        m_lastDisplayLine2 = line2;

        // Update cached state for UI consumption
        m_currentState.lcdLine1 = line1;
        m_currentState.lcdLine2 = line2;

        emit displayContentChanged(line1, line2);
    }
}

// ^FQfffff; TX frequency counter in kHz (8 kHz increments)
void KPA1500Direct::handleFQ(const QString &resp)
{
    bool ok = false;
    int freqKhz = resp.mid(3, resp.size() - 4).toInt(&ok);
    if (!ok) return;

    if (freqKhz != m_lastTxFrequency) {
        m_lastTxFrequency = freqKhz;
        emit txFrequencyChanged(freqKhz);

        // Also update cached state frequency (convert kHz to Hz)
        m_currentState.frequency = static_cast<freq_t>(freqKhz) * 1000;
    }
}

// ^VMH nnnnn; 50V supply voltage in millivolts (e.g., ^VMH 52749; = 52.749V)
void KPA1500Direct::handleVMH(const QString &resp)
{
    QString payload = resp.mid(4, resp.size() - 5).trimmed(); // ^VMH has space before value
    bool ok = false;
    int millivolts = payload.toInt(&ok);
    if (!ok) return;

    double volts = millivolts / 1000.0;
    if (!qFuzzyCompare(volts + 1.0, m_lastVoltage50V + 1.0)) {
        m_lastVoltage50V = volts;
        emit voltage50VChanged(volts);
    }
}

// ^ON0; power off, ^ON1; power on
void KPA1500Direct::handleON(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool powerOn = (c == QLatin1Char('1'));
    if (powerOn != m_lastPowerState) {
        m_lastPowerState = powerOn;
        emit powerStateChanged(powerOn);
    }
}

} // namespace TR4QT
