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

#include "KPA1500UdpPoller.h"
#include "../logging/LogMacros.h"

#include <QUdpSocket>
#include <QTimer>
#include <QtMath>
#include <QCoreApplication>

namespace TR4QT {

KPA1500UdpPoller::KPA1500UdpPoller(QObject* parent)
    : QObject(parent)
{
    initDispatchTable();
}

KPA1500UdpPoller::~KPA1500UdpPoller()
{
    stop();
}

void KPA1500UdpPoller::setAmplifierAddress(const QHostAddress &addr, quint16 port)
{
    m_addr = addr;
    m_port = port;
}

void KPA1500UdpPoller::setPollIntervalMs(int intervalMs)
{
    m_intervalMs = intervalMs;
}

void KPA1500UdpPoller::setPollCommands(const QStringList &commands)
{
    m_pollCommands = commands;
}

void KPA1500UdpPoller::start()
{
    if (m_running)
        return;

    if (m_addr.isNull()) {
        emit errorOccurred(QStringLiteral("Kpa1500UdpPoller: amplifier address not set"));
        return;
    }

    if (m_port == 0)
        m_port = 1500;

    m_socket = new QUdpSocket(this);
    // UDP server at port 1500 accepts same command set; one command per packet. [file:124]
    connect(m_socket, &QUdpSocket::readyRead,
            this, &KPA1500UdpPoller::onReadyRead);

    m_timer = new QTimer(this);
    m_timer->setInterval(m_intervalMs);
    connect(m_timer, &QTimer::timeout, this, &KPA1500UdpPoller::doPollCycle);

    m_running = true;
    m_timer->start();
}

void KPA1500UdpPoller::stop()
{
    if (!m_running)
        return;

    m_running = false;

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
}

void KPA1500UdpPoller::queryNow()
{
    if (m_addr.isNull()) {
        emit errorOccurred(QStringLiteral("Kpa1500UdpPoller: amplifier address not set"));
        return;
    }

    // Create temporary socket if not already running
    bool wasRunning = m_running;
    QUdpSocket* socket = m_socket;

    if (!socket) {
        socket = new QUdpSocket(this);
        connect(socket, &QUdpSocket::readyRead, this, &KPA1500UdpPoller::onReadyRead);
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
    if (!wasRunning && socket != m_socket) {
        // Process any immediate responses
        QCoreApplication::processEvents();
        socket->deleteLater();
    }
}

void KPA1500UdpPoller::initDispatchTable()
{
    // Power & SWR [file:124]
    m_dispatch.insert("^PWF", [this](const QString &r){ handlePWF(r); });
    m_dispatch.insert("^PWI", [this](const QString &r){ handlePWI(r); });
    m_dispatch.insert("^PWR", [this](const QString &r){ handlePWR(r); });
    m_dispatch.insert("^SW",  [this](const QString &r){ handleSW(r);  });

    // Band / ATU / antenna / fault [file:124]
    m_dispatch.insert("^BN",  [this](const QString &r){ handleBN(r);  });
    m_dispatch.insert("^OS",  [this](const QString &r){ handleOS(r);  });
    m_dispatch.insert("^AI",  [this](const QString &r){ handleAI(r);  });
    m_dispatch.insert("^AM",  [this](const QString &r){ handleAM(r);  });
    m_dispatch.insert("^AN",  [this](const QString &r){ handleAN(r);  });
    m_dispatch.insert("^FL",  [this](const QString &r){ handleFL(r);  });

    // Additional status / info commands [file:124]
    m_dispatch.insert("^BT",  [this](const QString &r){ handleBT(r);  }); // banner text
    m_dispatch.insert("^LQ",  [this](const QString &r){ handleLQ(r);  }); // line quality
    m_dispatch.insert("^PC",  [this](const QString &r){ handlePC(r);  }); // drive/input power
    m_dispatch.insert("^SN",  [this](const QString &r){ handleSN(r);  }); // serial number
    m_dispatch.insert("^TM",  [this](const QString &r){ handleTM(r);  }); // temperature
    m_dispatch.insert("^VI",  [this](const QString &r){ handleVI(r);  }); // input voltage
    m_dispatch.insert("^WS",  [this](const QString &r){ handleWS(r);  }); // combined fwd/ref/SWR

    // Add more with m_dispatch.insert("^XX", lambda) as needed.
}

void KPA1500UdpPoller::doPollCycle()
{
    if (!m_socket || !m_running)
        return;

    for (const QString &cmdStr : m_pollCommands) {
        QByteArray cmd = cmdStr.toUtf8();
        if (!cmd.isEmpty() && cmd.front() == '^' && cmd.back() == ';')
            sendCommand(cmd);
        else
            emit errorOccurred(QStringLiteral("Invalid command format: %1").arg(cmdStr));
    }
}

void KPA1500UdpPoller::sendCommand(const QByteArray &cmd)
{
    if (!m_socket)
        return;

    qint64 sent = m_socket->writeDatagram(cmd, m_addr, m_port);
    if (sent != cmd.size()) {
        emit errorOccurred(
            QStringLiteral("Failed to send command '%1'").arg(QString::fromUtf8(cmd)));
    }
}

void KPA1500UdpPoller::onReadyRead()
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
}

void KPA1500UdpPoller::processResponse(const QByteArray &datagram)
{
    QString text = QString::fromUtf8(datagram).trimmed();

    // All KPA1500 commands/responses start with '^' and end with ';' [file:124]
    if (!text.startsWith('^') || !text.endsWith(';'))
        return;

    // Emit generic hook with a short key (first 4 chars)
    QString key = text.left(4);
    emit valueUpdated(key, text);

    // Determine dispatch key: for most commands first 3–4 chars are enough. [file:124]
    QString dispatchKey;
    if (text.size() >= 4 && text[3].isLetter())
        dispatchKey = text.left(4); // "^PWF", "^PWI", "^BNx", etc.
    else
        dispatchKey = text.left(3); // "^SW", "^AI", "^AM", "^AN", "^BT", etc.

    auto it = m_dispatch.find(dispatchKey);
    if (it != m_dispatch.end()) {
        (*it)(text);
    } else {
        // Unhandled reply - log at debug level to flag for investigation
        LOG_DEBUG("KPA1500UdpPoller", QString("Unhandled reply: %1").arg(text));
    }
}

// ------------------- Handlers -------------------

// ^PWFnnnn; Forward power in watts [file:124]
void KPA1500UdpPoller::handlePWF(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastForwardPower) {
        m_lastForwardPower = w;
        emit forwardPowerChanged(w);
    }
}

// ^PWInnnn; Input power in watts [file:124]
void KPA1500UdpPoller::handlePWI(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastInputPower) {
        m_lastInputPower = w;
        emit inputPowerChanged(w);
    }
}

// ^PWRnnnn; Reflected power in watts [file:124]
void KPA1500UdpPoller::handlePWR(const QString &resp)
{
    bool ok = false;
    int w = resp.mid(4, resp.size() - 5).toInt(&ok);
    if (!ok) return;
    if (w != m_lastReflectedPower) {
        m_lastReflectedPower = w;
        emit reflectedPowerChanged(w);
    }
}

// ^SWswr; SWR in tenths, ^SW123; = 12.3:1 [file:124]
void KPA1500UdpPoller::handleSW(const QString &resp)
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

// ^BNbb; Band number 00–10 for 160–6 m [file:124]
void KPA1500UdpPoller::handleBN(const QString &resp)
{
    bool ok = false;
    int band = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (band != m_lastBandNumber) {
        m_lastBandNumber = band;
        emit bandNumberChanged(band);
    }
}

// ^OS1; Operate mode, ^OS0; Standby mode [file:124]
void KPA1500UdpPoller::handleOS(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool operateMode = (c == QLatin1Char('1'));
    if (operateMode != m_lastOperatingStatus) {
        m_lastOperatingStatus = operateMode;
        emit operatingStatusChanged(operateMode);
    }
}

// ^AI1; ATU inline, ^AI0; bypassed [file:124]
void KPA1500UdpPoller::handleAI(const QString &resp)
{
    if (resp.size() < 5) return;
    QChar c = resp[3];
    bool inlineMode = (c == QLatin1Char('1'));
    if (inlineMode != m_lastAtuInline) {
        m_lastAtuInline = inlineMode;
        emit atuInlineChanged(inlineMode);
    }
}

// ^AM...; ATU Mode Inline/Bypassed variants [file:124]
void KPA1500UdpPoller::handleAM(const QString &resp)
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

// ^ANa; or ^ANaa; Antenna select [file:124]
void KPA1500UdpPoller::handleAN(const QString &resp)
{
    bool ok = false;
    int ant = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (ant != m_lastAntenna) {
        m_lastAntenna = ant;
        emit antennaChanged(ant);
    }
}

// ^FLnnn; Fault code [file:124]
void KPA1500UdpPoller::handleFL(const QString &resp)
{
    bool ok = false;
    int code = resp.mid(3, resp.size() - 4).toInt(&ok, 10);
    if (!ok) return;
    if (code != m_lastFaultCode) {
        m_lastFaultCode = code;
        emit faultCodeChanged(code);
    }
}

// ^BTtext; Banner text shown on front panel [file:124]
void KPA1500UdpPoller::handleBT(const QString &resp)
{
    QString text = resp.mid(3, resp.size() - 4); // between ^BT and ;
    if (text != m_lastBannerText) {
        m_lastBannerText = text;
        emit bannerTextChanged(text);
    }
}

// ^LQnn; Line quality percent [file:124]
void KPA1500UdpPoller::handleLQ(const QString &resp)
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

// ^PCnnn; Drive/input power setting in watts [file:124]
void KPA1500UdpPoller::handlePC(const QString &resp)
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

// ^SNxxxxx; Serial number ASCII [file:124]
void KPA1500UdpPoller::handleSN(const QString &resp)
{
    QString serial = resp.mid(3, resp.size() - 4);
    if (serial != m_lastSerialNumber) {
        m_lastSerialNumber = serial;
        emit serialNumberChanged(serial);
    }
}

// ^TMxxx; Temperature in degrees C (NOT tenths - per KPA1500 protocol docs)
void KPA1500UdpPoller::handleTM(const QString &resp)
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

// ^VInnnn; Input DC voltage in tenths of a volt [file:124]
void KPA1500UdpPoller::handleVI(const QString &resp)
{
    bool ok = false;
    QString valStr = resp.mid(3, resp.size() - 4);
    int tenths = valStr.toInt(&ok);
    if (!ok) return;

    double volts = tenths / 10.0;
    if (!qFuzzyCompare(volts + 1.0, m_lastInputVoltage + 1.0)) {
        m_lastInputVoltage = volts;
        emit inputVoltageChanged(volts);
    }
}

// ^WS...; Combined fwd/ref/SWR status (exact format per manual) [file:124]
void KPA1500UdpPoller::handleWS(const QString &resp)
{
    // Example: assume ^WSffffrrrrsss; (4 digits fwd, 4 digits ref, 3 digits swr tenths).
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
