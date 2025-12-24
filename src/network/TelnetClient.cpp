#include "TelnetClient.h"
#include <QRegularExpression>
#include <QDateTime>
#include <hamlib/rig.h>

namespace TR4QT {

TelnetClient::TelnetClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_port(0)
    , m_isConnected(false)
{
    connect(m_socket, &QTcpSocket::connected,
            this, &TelnetClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TelnetClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TelnetClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &TelnetClient::onError);
}

TelnetClient::~TelnetClient() {
    if (m_isConnected) {
        disconnectFromServer();
    }
}

void TelnetClient::connectToServer(const QString& server, int port) {
    if (m_isConnected) {
        disconnectFromServer();
    }

    m_server = server;
    m_port = port;
    m_dataBuffer.clear();

    m_socket->connectToHost(server, port);
}

void TelnetClient::disconnectFromServer() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_isConnected = false;
}

void TelnetClient::sendCommand(const QString& command) {
    if (!m_isConnected) {
        return;
    }

    QString cmd = command.trimmed() + "\r\n";
    m_socket->write(cmd.toUtf8());
    m_socket->flush();
}

void TelnetClient::onConnected() {
    m_isConnected = true;
    emit connected();
}

void TelnetClient::onDisconnected() {
    m_isConnected = false;
    emit disconnected();
}

void TelnetClient::onReadyRead() {
    // Read all available data
    QByteArray data = m_socket->readAll();
    QString text = QString::fromUtf8(data);

    // Add to buffer
    m_dataBuffer += text;

    // Process complete lines
    QStringList lines = m_dataBuffer.split('\n');

    // Keep last incomplete line in buffer
    m_dataBuffer = lines.takeLast();

    // Process each complete line
    for (const QString& line : lines) {
        QString cleanLine = line.trimmed();
        if (!cleanLine.isEmpty()) {
            // Emit raw data for display
            emit dataReceived(cleanLine);

            // Try to parse as DX spot
            parseSpotLine(cleanLine);
        }
    }
}

void TelnetClient::onError(QAbstractSocket::SocketError error) {
    QString errorMsg;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorMsg = "Connection refused";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorMsg = "Remote host closed connection";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorMsg = "Host not found";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorMsg = "Connection timeout";
        break;
    case QAbstractSocket::NetworkError:
        errorMsg = "Network error";
        break;
    default:
        errorMsg = m_socket->errorString();
        break;
    }

    emit connectionError(errorMsg);
    m_isConnected = false;
}

bool TelnetClient::parseSpotLine(const QString& line) {
    // DX Cluster spot format:
    // DX de SPOTTER:   FREQUENCY  CALLSIGN  COMMENT                 TIME Z
    // Example:
    // DX de W1AW:     14025.0  DL1ABC        CQ                      2345 Z

    // Regex pattern for DX spots
    static QRegularExpression spotRegex(
        R"(^DX\s+de\s+([A-Z0-9/]+):\s+([0-9.]+)\s+([A-Z0-9/]+)\s+(.+?)\s+(\d{4})Z?$)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = spotRegex.match(line);
    if (match.hasMatch()) {
        QString spotter = match.captured(1);
        double frequency = match.captured(2).toDouble();
        QString callsign = match.captured(3);
        QString comment = match.captured(4).trimmed();
        QString time = match.captured(5);

        // Convert frequency to Hz (input is in kHz)
        freq_t freqHz = static_cast<freq_t>(frequency * 1000);

        emit spotReceived(callsign, frequency, spotter, comment, time);
        return true;
    }

    return false;
}

// TelnetThread implementation

TelnetThread::TelnetThread(QObject* parent)
    : QThread(parent)
    , m_client(nullptr)
{
}

TelnetThread::~TelnetThread() {
    if (isRunning()) {
        quit();
        wait();
    }
}

void TelnetThread::run() {
    // Create client in this thread
    m_client = new TelnetClient();

    // Run event loop
    exec();

    // Cleanup
    delete m_client;
    m_client = nullptr;
}

} // namespace TR4QT
