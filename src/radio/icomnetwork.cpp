#include "icomnetwork.h"
#include <QHostInfo>
#include <QNetworkDatagram>
#include <QDebug>
#include <QtEndian>
#include <QRandomGenerator>
#include <cstring>

IcomNetwork::IcomNetwork(QObject *parent)
    : QObject(parent)
    , m_controlSocket(nullptr)
    , m_civSocket(nullptr)
    , m_controlLocalPort(0)
    , m_civLocalPort(0)
    , m_civRemotePort(0)
    , m_state(Disconnected)
    , m_authenticated(false)
    , m_streamOpened(false)
    , m_myId(0)
    , m_remoteId(0)
    , m_authSeq(0x30)
    , m_tokRequest(0)
    , m_token(0)
    , m_sendSeq(1)
    , m_civSendSeq(0)
    , m_pingSendSeq(0)
    , m_areYouThereTimer(nullptr)
    , m_pingTimer(nullptr)
    , m_idleTimer(nullptr)
    , m_tokenTimer(nullptr)
    , m_retransmitTimer(nullptr)
    , m_watchdogTimer(nullptr)
    , m_civStartTimer(nullptr)
    , m_packetsSent(0)
    , m_packetsLost(0)
    , m_areYouThereCounter(0)
{
    // Start monotonic timer for time tracking
    m_mono.start();
}

IcomNetwork::~IcomNetwork()
{
    disconnectFromRadio();
}

void IcomNetwork::connectToRadio(const IcomConnectionConfig& config)
{
    if (m_state != Disconnected) {
        qWarning() << "Already connected or connecting";
        return;
    }

    m_config = config;

    // Resolve IP address
    if (!m_radioIP.setAddress(config.ipAddress)) {
        QHostInfo remote = QHostInfo::fromName(config.ipAddress);
        for (const auto& addr : remote.addresses()) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                m_radioIP = addr;
                qInfo() << "Resolved" << config.ipAddress << "to" << addr.toString();
                break;
            }
        }
        if (m_radioIP.isNull()) {
            emit connectionError("Failed to resolve hostname: " + config.ipAddress);
            return;
        }
    }

    // Get local IP address
    QString localHostname = QHostInfo::localHostName();
    QList<QHostAddress> hostList = QHostInfo::fromName(localHostname).addresses();
    for (const auto& address : hostList) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
            m_localIP = QHostAddress(address.toString());
            break;
        }
    }

    if (m_localIP.isNull()) {
        m_localIP = QHostAddress::LocalHost;
    }

    qInfo() << "Local IP:" << m_localIP.toString() << "Radio IP:" << m_radioIP.toString();

    // Initialize control socket
    initControlSocket();

    // Start connection process
    setState(WaitingForHere);
    emit statusUpdate("Connecting to " + m_radioIP.toString());
}

void IcomNetwork::disconnectFromRadio()
{
    if (m_state == Disconnected) {
        return;
    }

    // Send disconnect on control socket
    if (m_controlSocket) {
        sendControlPacket(0x05, 0x00, false);
    }

    // Close CI-V stream if open
    if (m_civSocket && m_streamOpened) {
        sendCivOpenClose(true);
    }

    cleanup();
    setState(Disconnected);
    emit disconnected();
    emit statusUpdate("Disconnected");
}

void IcomNetwork::initControlSocket()
{
    m_controlSocket = new QUdpSocket(this);

    if (!m_controlSocket->bind()) {
        emit connectionError("Failed to bind control socket");
        cleanup();
        return;
    }

    m_controlLocalPort = m_controlSocket->localPort();
    m_myId = calculateMyId();

    qInfo() << "Control socket bound to port" << m_controlLocalPort << "myId:" << QString("0x%1").arg(m_myId, 8, 16, QChar('0'));

    connect(m_controlSocket, &QUdpSocket::readyRead, this, &IcomNetwork::onControlDataReceived);

    // Create timers
    m_areYouThereTimer = new QTimer(this);
    m_pingTimer = new QTimer(this);
    m_idleTimer = new QTimer(this);
    m_tokenTimer = new QTimer(this);
    m_retransmitTimer = new QTimer(this);

    connect(m_areYouThereTimer, &QTimer::timeout, this, &IcomNetwork::sendAreYouThere);
    connect(m_pingTimer, &QTimer::timeout, this, &IcomNetwork::sendPing);
    connect(m_idleTimer, &QTimer::timeout, this, &IcomNetwork::sendIdlePacket);
    connect(m_tokenTimer, &QTimer::timeout, this, &IcomNetwork::sendTokenRenewal);
    connect(m_retransmitTimer, &QTimer::timeout, this, &IcomNetwork::checkRetransmit);

    // Start retransmit checker
    m_retransmitTimer->start(RETRANSMIT_PERIOD);

    // Start "Are You There" timer
    m_areYouThereTimer->start(AREYOUTHERE_PERIOD);
}

void IcomNetwork::initCivSocket(quint16 civPort)
{
    m_civRemotePort = civPort;

    // Find available local port
    QUdpSocket tempSocket;
    if (!tempSocket.bind()) {
        qWarning() << "Failed to find available port for CI-V";
        return;
    }
    m_civLocalPort = tempSocket.localPort();
    tempSocket.close();

    m_civSocket = new QUdpSocket(this);
    if (!m_civSocket->bind(m_civLocalPort)) {
        qWarning() << "Failed to bind CI-V socket to port" << m_civLocalPort;
        delete m_civSocket;
        m_civSocket = nullptr;
        return;
    }

    qInfo() << "CI-V socket bound to local port" << m_civLocalPort << "remote port" << m_civRemotePort;

    connect(m_civSocket, &QUdpSocket::readyRead, this, &IcomNetwork::onCivDataReceived);

    // Create CI-V timers
    m_civStartTimer = new QTimer(this);
    m_watchdogTimer = new QTimer(this);

    connect(m_watchdogTimer, &QTimer::timeout, this, &IcomNetwork::watchdogTimeout);

    m_watchdogTimer->start(WATCHDOG_PERIOD);

    // Send initial "Are You There" on CI-V socket
    control_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.type = 0x03;
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;

    m_civSocket->writeDatagram(QByteArray::fromRawData((const char*)&p, sizeof(p)), m_radioIP, m_civRemotePort);
}

quint32 IcomNetwork::calculateMyId()
{
    quint32 addr = m_localIP.toIPv4Address();
    return (addr >> 8 & 0xff) << 24 | (addr & 0xff) << 16 | (m_controlLocalPort & 0xffff);
}

void IcomNetwork::setState(ConnectionState state)
{
    m_state = state;
}

void IcomNetwork::sendAreYouThere()
{
    const int MAX_RETRIES = 10;  // Reduced from 20 since we're using exponential backoff
    const int INITIAL_INTERVAL_MS = 500;
    const int MAX_INTERVAL_MS = 5000;

    if (m_areYouThereCounter >= MAX_RETRIES) {
        emit connectionError("Radio not responding");
        disconnectFromRadio();
        return;
    }

    qDebug() << "Sending Are You There... (attempt" << (m_areYouThereCounter + 1) << "of" << MAX_RETRIES << ")";
    sendControlPacket(0x03, 0x00, false);
    m_areYouThereCounter++;

    // Exponential backoff: 500ms, 1000ms, 2000ms, 4000ms, 5000ms (capped)
    int nextInterval = INITIAL_INTERVAL_MS * (1 << m_areYouThereCounter);  // 2^counter
    if (nextInterval > MAX_INTERVAL_MS) {
        nextInterval = MAX_INTERVAL_MS;
    }

    // Restart timer with new interval
    m_areYouThereTimer->start(nextInterval);
}

void IcomNetwork::sendControlPacket(quint8 type, quint16 seq, bool tracked)
{
    control_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.type = type;
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;

    if (tracked) {
        sendTrackedPacket(m_controlSocket, QByteArray::fromRawData((const char*)&p, sizeof(p)), m_sendSeq);
    } else {
        p.seq = seq;
        m_controlSocket->writeDatagram(QByteArray::fromRawData((const char*)&p, sizeof(p)), m_radioIP, m_config.controlPort);
    }
}

void IcomNetwork::sendPing()
{
    ping_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.type = 0x07;
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.seq = m_pingSendSeq;

    QTime now = QTime::currentTime();
    p.time = (quint32)now.msecsSinceStartOfDay();
    m_lastPingSentTime = QDateTime::currentDateTime();

    m_controlSocket->writeDatagram(QByteArray::fromRawData((const char*)&p, sizeof(p)), m_radioIP, m_config.controlPort);
}

void IcomNetwork::sendIdlePacket()
{
    sendControlPacket(0x00, 0x00, true);
}

void IcomNetwork::sendLogin()
{
    qInfo() << "Sending login packet - Username:" << m_config.username
            << "Password length:" << m_config.password.length() << "chars";

    m_tokRequest = static_cast<quint16>(QRandomGenerator::global()->generate());

    QByteArray usernameEncoded;
    QByteArray passwordEncoded;
    passcode(m_config.username, usernameEncoded);
    passcode(m_config.password, passwordEncoded);

    login_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.payloadsize = qToBigEndian((quint32)(sizeof(p) - 0x10));
    p.requesttype = 0x00;
    p.requestreply = 0x01;
    p.innerseq = qToBigEndian(m_authSeq++);
    p.tokrequest = m_tokRequest;

    memcpy(p.username, usernameEncoded.constData(), qMin(usernameEncoded.length(), 16));
    memcpy(p.password, passwordEncoded.constData(), qMin(passwordEncoded.length(), 16));

    QString clientName = m_config.clientName.left(16);
    memcpy(p.name, clientName.toLocal8Bit().constData(), clientName.length());

    sendTrackedPacket(m_controlSocket, QByteArray::fromRawData((const char*)&p, sizeof(p)), m_sendSeq);
    setState(WaitingForLogin);
}

void IcomNetwork::sendToken(quint8 magic)
{
    qDebug() << "Sending token request, magic:" << magic;

    token_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.payloadsize = qToBigEndian((quint32)(sizeof(p) - 0x10));
    p.requesttype = magic;
    p.requestreply = 0x01;
    p.innerseq = qToBigEndian(m_authSeq++);
    p.tokrequest = m_tokRequest;
    p.resetcap = qToBigEndian((quint16)0x0798);
    p.token = m_token;

    sendTrackedPacket(m_controlSocket, QByteArray::fromRawData((const char*)&p, sizeof(p)), m_sendSeq);
}

void IcomNetwork::sendTokenRenewal()
{
    sendToken(0x05);
}

void IcomNetwork::sendRequestStream()
{
    qInfo() << "Requesting CI-V stream for radio:" << m_currentRadio.name;

    QByteArray usernameEncoded;
    passcode(m_config.username, usernameEncoded);

    conninfo_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.payloadsize = qToBigEndian((quint32)(sizeof(p) - 0x10));
    p.requesttype = 0x03;
    p.requestreply = 0x01;
    p.innerseq = qToBigEndian(m_authSeq++);
    p.tokrequest = m_tokRequest;
    p.token = m_token;

    // Set MAC or GUID
    if (m_currentRadio.useGuid) {
        memcpy(&p.guid, m_currentRadio.macAddress.constData(), GUIDLEN);
    } else {
        p.commoncap = 0x8010;
        memcpy(&p.macaddress, m_currentRadio.macAddress.constData(), 6);
    }

    // Set radio name and username
    memcpy(&p.name, m_currentRadio.name.toLocal8Bit().constData(), qMin(m_currentRadio.name.length(), 32));
    memcpy(&p.username, usernameEncoded.constData(), qMin(usernameEncoded.length(), 16));

    // Enable RX only (no TX/audio)
    p.rxenable = 1;
    p.txenable = 0;
    p.rxcodec = 0x04;  // LPCM
    p.txcodec = 0;
    p.rxsample = qToBigEndian((quint32)8000);
    p.txsample = 0;
    p.civport = qToBigEndian((quint32)m_civLocalPort);
    p.audioport = 0;
    p.txbuffer = 0;
    p.convert = 1;

    sendTrackedPacket(m_controlSocket, QByteArray::fromRawData((const char*)&p, sizeof(p)), m_sendSeq);
    setState(StreamRequested);
}

void IcomNetwork::sendCivOpenClose(bool close)
{
    if (!m_civSocket) return;

    quint8 magic = close ? 0x00 : 0x04;

    openclose_packet p;
    memset(&p, 0, sizeof(p));
    p.len = sizeof(p);
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.data = 0x01c0;
    p.sendseq = qToBigEndian(m_civSendSeq);
    p.magic = magic;

    m_civSendSeq++;

    sendTrackedPacket(m_civSocket, QByteArray::fromRawData((const char*)&p, sizeof(p)), m_civSendSeq);
}

void IcomNetwork::sendCivCommand(const QByteArray& command)
{
    if (!m_civSocket || !m_streamOpened) {
        qWarning() << "Cannot send CI-V command: not connected";
        return;
    }

    data_packet p;
    memset(&p, 0, sizeof(p));
    p.len = (quint32)sizeof(p) + command.length();
    p.sentid = m_myId;
    p.rcvdid = m_remoteId;
    p.reply = (char)0xc1;
    p.datalen = command.length();
    p.sendseq = qToBigEndian(m_civSendSeq);

    QByteArray packet = QByteArray::fromRawData((const char*)&p, sizeof(p));
    packet.append(command);

    sendTrackedPacket(m_civSocket, packet, m_civSendSeq);
    m_civSendSeq++;
}

void IcomNetwork::sendTrackedPacket(QUdpSocket* socket, const QByteArray& data, quint16& seqNum)
{
    QByteArray packet = data;

    // Set sequence number
    packet[6] = seqNum & 0xff;
    packet[7] = (seqNum >> 8) & 0xff;

    // Store in buffer for retransmission
    PacketBufferEntry entry;
    entry.seqNum = seqNum;
    entry.timeSent = QTime::currentTime();
    entry.retransmitCount = 0;
    entry.data = packet;

    QMap<quint16, PacketBufferEntry>* txBuf;
    QMutex* mutex;

    if (socket == m_controlSocket) {
        txBuf = &m_controlTxBuf;
        mutex = &m_controlTxMutex;
    } else {
        txBuf = &m_civTxBuf;
        mutex = &m_civTxMutex;
    }

    mutex->lock();
    if (seqNum == 0) {
        txBuf->clear();
    }
    if (txBuf->size() > BUFSIZE) {
        txBuf->erase(txBuf->begin());
    }
    txBuf->insert(seqNum, entry);
    mutex->unlock();

    seqNum++;

    // Send packet
    quint16 port = (socket == m_controlSocket) ? m_config.controlPort : m_civRemotePort;
    socket->writeDatagram(packet, m_radioIP, port);

    // Reset idle timer if active
    if (m_idleTimer && m_idleTimer->isActive()) {
        m_idleTimer->start(IDLE_PERIOD);
    }

    m_packetsSent++;
}

void IcomNetwork::onControlDataReceived()
{
    while (m_controlSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_controlSocket->receiveDatagram();
        processControlPacket(datagram.data());
    }
}

void IcomNetwork::onCivDataReceived()
{
    if (!m_civSocket) return;

    while (m_civSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_civSocket->receiveDatagram();
        qDebug() << "IcomNetwork: Received CI-V datagram:" << datagram.data().size() << "bytes"
                 << "Data:" << datagram.data().toHex(' ');
        processCivPacket(datagram.data());
    }
}

void IcomNetwork::processControlPacket(const QByteArray& data)
{
    if (data.length() < CONTROL_SIZE) {
        return;
    }

    const control_packet* ctrl = reinterpret_cast<const control_packet*>(data.constData());

    // Handle different packet types based on length
    switch (data.length()) {
        case CONTROL_SIZE: {
            if (ctrl->type == 0x04) {
                // "I am here" response
                qInfo() << "Received 'I am here' from radio";
                m_remoteId = ctrl->sentid;
                m_areYouThereCounter = 0;

                if (m_areYouThereTimer->isActive()) {
                    m_areYouThereTimer->stop();
                    m_pingTimer->start(PING_PERIOD);
                    m_idleTimer->start(IDLE_PERIOD);
                }

                // Send "Are you ready"
                sendControlPacket(0x06, 0x01, false);
                setState(WaitingForReady);

            } else if (ctrl->type == 0x06) {
                // "I am ready" response
                qInfo() << "Received 'I am ready'";
                sendLogin();
            }
            break;
        }

        case PING_SIZE: {
            const ping_packet* ping = reinterpret_cast<const ping_packet*>(data.constData());
            if (ping->type == 0x07) {
                if (ping->reply == 0x00) {
                    // Ping request from radio - respond
                    ping_packet response;
                    memcpy(&response, ping, sizeof(response));
                    response.sentid = m_myId;
                    response.rcvdid = m_remoteId;
                    response.reply = 0x01;
                    m_controlSocket->writeDatagram(QByteArray::fromRawData((const char*)&response, sizeof(response)),
                                                   m_radioIP, m_config.controlPort);
                } else if (ping->reply == 0x01 && ping->seq == m_pingSendSeq) {
                    // Ping response to our request
                    m_pingSendSeq++;
                    m_stats.rtt = m_lastPingSentTime.msecsTo(QDateTime::currentDateTime());
                }
            }
            break;
        }

        case TOKEN_SIZE: {
            const token_packet* token = reinterpret_cast<const token_packet*>(data.constData());
            if (token->requesttype == 0x05 && token->requestreply == 0x02) {
                // Token renewal response
                if (token->response == 0x0000) {
                    qDebug() << "Token renewal successful";
                    m_tokenTimer->start(TOKEN_RENEWAL);
                } else {
                    qWarning() << "Token renewal failed";
                }
            }
            break;
        }

        case STATUS_SIZE: {
            const status_packet* status = reinterpret_cast<const status_packet*>(data.constData());
            if (status->type != 0x01) {
                if (status->error == 0xffffffff) {
                    emit connectionError("Connection failed - try rebooting the radio");
                } else if (status->error == 0x00000000 && status->disc == 0x01) {
                    emit statusUpdate("Radio disconnected");
                    disconnectFromRadio();
                } else {
                    // Got stream connection info
                    quint16 civPort = qFromBigEndian(status->civport);
                    qInfo() << "Got CI-V port:" << civPort;

                    // Initialize CI-V socket
                    initCivSocket(civPort);
                    sendCivOpenClose(false);

                    m_streamOpened = true;
                    setState(Connected);
                    emit connected();
                    emit statusUpdate("Connected to " + m_currentRadio.name);
                }
            }
            break;
        }

        case LOGIN_RESPONSE_SIZE: {
            const login_response_packet* login = reinterpret_cast<const login_response_packet*>(data.constData());
            if (login->type != 0x01) {
                if (login->error == 0xfeffffff) {
                    emit authenticationFailed("Invalid username/password");
                    disconnectFromRadio();
                } else if (login->tokrequest == m_tokRequest) {
                    qInfo() << "Login successful, token received";
                    m_token = login->token;
                    sendToken(0x02);
                    m_tokenTimer->start(TOKEN_RENEWAL);
                    m_authenticated = true;
                    setState(Authenticated);
                    emit statusUpdate("Authenticated");
                }
            }
            break;
        }

        default: {
            // Check if it's a capabilities packet
            if ((data.length() - CAPABILITIES_SIZE) % RADIO_CAP_SIZE == 0) {
                const capabilities_packet* cap = reinterpret_cast<const capabilities_packet*>(data.constData());
                quint16 numRadios = qFromBigEndian(cap->numradios);

                m_radios.clear();

                for (int i = 0; i < numRadios; i++) {
                    int offset = CAPABILITIES_SIZE + (i * RADIO_CAP_SIZE);
                    const radio_cap_packet* radio = reinterpret_cast<const radio_cap_packet*>(data.constData() + offset);

                    IcomRadioInfo info;
                    // Stop at first NUL instead of reading all 32 bytes (prevents NUL chars in status)
                    info.name = QString::fromLatin1(radio->name).trimmed();
                    info.civAddress = radio->civ;
                    info.baudRate = qFromBigEndian(radio->baudrate);

                    if (radio->commoncap == 0x8010) {
                        info.useGuid = false;
                        info.macAddress = QByteArray((const char*)radio->macaddress, 6);
                    } else {
                        info.useGuid = true;
                        info.macAddress = QByteArray((const char*)radio->guid, GUIDLEN);
                    }

                    m_radios.append(info);

                    qInfo() << "Radio" << i << ":" << info.name << "CIV:"
                            << QString("0x%1").arg(info.civAddress, 2, 16, QChar('0'));
                }

                emit radiosDiscovered(m_radios);

                // Auto-select if only one radio
                if (m_radios.size() == 1) {
                    selectRadio(0);
                }
            }
            break;
        }
    }
}

// TODO: Change all data packet debug messages to TRACE level
// The qDebug() calls in this function and related CI-V processing functions
// (onCivDataReceived, pollRadio, sendCommand) should be changed to TRACE level
// to reduce log verbosity during normal operation. Keep them as DEBUG for now
// during initial CI-V debugging.
void IcomNetwork::processCivPacket(const QByteArray& data)
{
    qDebug() << "IcomNetwork::processCivPacket: Processing packet of" << data.length() << "bytes";

    if (data.length() < CONTROL_SIZE) {
        qDebug() << "IcomNetwork::processCivPacket: Packet too small, ignoring";
        return;
    }

    const control_packet* ctrl = reinterpret_cast<const control_packet*>(data.constData());

    if (data.length() == CONTROL_SIZE) {
        qDebug() << "IcomNetwork::processCivPacket: Control packet (type" << ctrl->type << ")";
        if (ctrl->type == 0x04) {
            // CI-V socket "I am here" - radio is ready to receive CI-V commands
            qDebug() << "CI-V socket ready - emitting civSocketReady signal";
            emit civSocketReady();
        } else if (ctrl->type == 0x06) {
            // "I am ready" - start sending CI-V data requests
            m_remoteId = ctrl->sentid;
            sendCivOpenClose(false);
            if (m_civStartTimer) {
                m_civStartTimer->start(100);
            }
        }
    } else if (data.length() > CIV_SIZE) {
        // CI-V data packet
        const data_packet* dp = reinterpret_cast<const data_packet*>(data.constData());
        qDebug() << "IcomNetwork::processCivPacket: Data packet (type" << dp->type << "datalen" << dp->datalen << "len" << dp->len << ")";
        qDebug() << "IcomNetwork::processCivPacket: Validation check: type!= 0x01?" << (dp->type != 0x01)
                 << "length match?" << (quint16(dp->datalen + 0x15) == (quint16)dp->len)
                 << "(" << (quint16)(dp->datalen + 0x15) << "==" << (quint16)dp->len << ")";
        if (dp->type != 0x01 && quint16(dp->datalen + 0x15) == (quint16)dp->len) {
            // Stop requesting CI-V data - we're getting it
            if (m_civStartTimer && m_civStartTimer->isActive()) {
                m_civStartTimer->stop();
            }

            m_lastCivReceived = QTime::currentTime();

            // Extract CI-V data (skip 0x15 byte header)
            QByteArray civData = data.mid(0x15);
            qDebug() << "IcomNetwork::processCivPacket: Emitting civDataReceived with" << civData.size() << "bytes";
            emit civDataReceived(civData);
        } else {
            qDebug() << "IcomNetwork::processCivPacket: Data packet validation failed";
        }
    } else {
        qDebug() << "IcomNetwork::processCivPacket: Unknown packet type/length";
    }
}

void IcomNetwork::checkRetransmit()
{
    // Check for missing packets and request retransmission
    // This is a simplified version - full implementation would track missing sequences
}

void IcomNetwork::watchdogTimeout()
{
    if (!m_streamOpened) return;

    if (m_lastCivReceived.msecsTo(QTime::currentTime()) > 2000) {
        qWarning() << "No CI-V data received for 2s, requesting restart";
        if (m_civStartTimer) {
            m_civStartTimer->start(100);
        }
        sendCivOpenClose(false);
    }
}

void IcomNetwork::selectRadio(int index)
{
    if (index < 0 || index >= m_radios.size()) {
        qWarning() << "Invalid radio index:" << index;
        return;
    }

    m_currentRadio = m_radios[index];

    // Find available local port for CI-V
    QUdpSocket tempSocket;
    if (tempSocket.bind()) {
        m_civLocalPort = tempSocket.localPort();
        tempSocket.close();
    }

    qInfo() << "Selected radio:" << m_currentRadio.name << "CI-V local port:" << m_civLocalPort;

    sendRequestStream();
}

void IcomNetwork::cleanup()
{
    // Stop all timers
    if (m_areYouThereTimer) m_areYouThereTimer->stop();
    if (m_pingTimer) m_pingTimer->stop();
    if (m_idleTimer) m_idleTimer->stop();
    if (m_tokenTimer) m_tokenTimer->stop();
    if (m_retransmitTimer) m_retransmitTimer->stop();
    if (m_watchdogTimer) m_watchdogTimer->stop();
    if (m_civStartTimer) m_civStartTimer->stop();

    // Close sockets
    if (m_controlSocket) {
        disconnect(m_controlSocket, nullptr, this, nullptr);  // Disconnect all signals before deletion
        m_controlSocket->close();
        m_controlSocket->deleteLater();
        m_controlSocket = nullptr;
    }

    if (m_civSocket) {
        disconnect(m_civSocket, nullptr, this, nullptr);  // Disconnect all signals before deletion
        m_civSocket->close();
        m_civSocket->deleteLater();
        m_civSocket = nullptr;
    }

    // Clear buffers
    m_controlTxBuf.clear();
    m_civTxBuf.clear();
    m_controlRxBuf.clear();
    m_civRxBuf.clear();

    // Reset state
    m_authenticated = false;
    m_streamOpened = false;
    m_remoteId = 0;
    m_areYouThereCounter = 0;

    // CRITICAL: Reset connection state to Disconnected
    // Without this, failed connection attempts leave state in intermediate state
    // (e.g., WaitingForHere, WaitingForLogin) preventing subsequent connection attempts
    setState(Disconnected);
}
