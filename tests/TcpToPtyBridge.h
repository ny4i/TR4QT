#ifndef TCP_TO_PTY_BRIDGE_H
#define TCP_TO_PTY_BRIDGE_H

#include <QObject>
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSerialPort>

/**
 * @brief TCP-to-PTY Bridge for K4Radio simulator testing
 *
 * This glue class bridges between:
 * - K4Radio (QTcpSocket, expects TCP connection)
 * - Hamlib K4 simulator (pty device, serial-style)
 *
 * Architecture:
 *   K4Radio → TCP (localhost:port) → Bridge → PTY (/dev/pts/X) → Simulator
 *
 * This allows testing K4Radio's production TCP code against the pty-based simulator
 * without modifying either K4Radio or the simulator.
 */
class TcpToPtyBridge : public QObject {
    Q_OBJECT

public:
    explicit TcpToPtyBridge(const QString& ptyDevice, quint16 tcpPort, QObject* parent = nullptr)
        : QObject(parent)
        , m_ptyDevice(ptyDevice)
        , m_tcpPort(tcpPort)
        , m_server(new QTcpServer(this))
        , m_tcpClient(nullptr)
        , m_serialPort(new QSerialPort(this))
    {
    }

    /**
     * @brief Start the bridge
     * @return true if server listening and pty opened successfully
     */
    bool start() {
        // Open the pty device (serial port to simulator)
        m_serialPort->setPortName(m_ptyDevice);
        m_serialPort->setBaudRate(QSerialPort::Baud38400);
        m_serialPort->setDataBits(QSerialPort::Data8);
        m_serialPort->setParity(QSerialPort::NoParity);
        m_serialPort->setStopBits(QSerialPort::OneStop);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            qWarning() << "Failed to open pty device:" << m_ptyDevice
                      << m_serialPort->errorString();
            return false;
        }

        // Connect serial port data ready signal
        connect(m_serialPort, &QSerialPort::readyRead,
                this, &TcpToPtyBridge::onSerialDataReady);

        // Start TCP server (listen for K4Radio connection)
        connect(m_server, &QTcpServer::newConnection,
                this, &TcpToPtyBridge::onNewTcpConnection);

        if (!m_server->listen(QHostAddress::LocalHost, m_tcpPort)) {
            qWarning() << "Failed to start TCP server on port" << m_tcpPort;
            m_serialPort->close();
            return false;
        }

        qDebug() << "Bridge started: TCP" << m_tcpPort << "↔ PTY" << m_ptyDevice;
        return true;
    }

    /**
     * @brief Stop the bridge
     */
    void stop() {
        // Disconnect signals first to prevent crashes during cleanup
        if (m_tcpClient) {
            m_tcpClient->disconnect();  // Disconnect all signals/slots
            m_tcpClient->disconnectFromHost();
            m_tcpClient->deleteLater();
            m_tcpClient = nullptr;
        }

        if (m_serialPort) {
            m_serialPort->disconnect();  // Disconnect all signals/slots
            m_serialPort->close();
        }

        if (m_server) {
            m_server->disconnect();  // Disconnect all signals/slots
            m_server->close();
        }

        // Process events to let deleteLater() complete
        QCoreApplication::processEvents();

        qDebug() << "Bridge stopped";
    }

    /**
     * @brief Get the TCP port the bridge is listening on
     */
    quint16 port() const {
        return m_tcpPort;
    }

private slots:
    /**
     * @brief Handle new TCP connection from K4Radio
     */
    void onNewTcpConnection() {
        if (m_tcpClient) {
            qWarning() << "Already have a TCP client, rejecting new connection";
            QTcpSocket* rejected = m_server->nextPendingConnection();
            rejected->disconnectFromHost();
            rejected->deleteLater();
            return;
        }

        m_tcpClient = m_server->nextPendingConnection();
        connect(m_tcpClient, &QTcpSocket::readyRead,
                this, &TcpToPtyBridge::onTcpDataReady);
        connect(m_tcpClient, &QTcpSocket::disconnected,
                this, &TcpToPtyBridge::onTcpDisconnected);

        qDebug() << "K4Radio connected to bridge";
    }

    /**
     * @brief Forward data from TCP (K4Radio) to PTY (simulator)
     */
    void onTcpDataReady() {
        if (!m_tcpClient) return;

        QByteArray data = m_tcpClient->readAll();
        qint64 written = m_serialPort->write(data);

        if (written != data.size()) {
            qWarning() << "Failed to write all data to PTY";
        }
    }

    /**
     * @brief Forward data from PTY (simulator) to TCP (K4Radio)
     */
    void onSerialDataReady() {
        if (!m_tcpClient) {
            // No TCP client connected yet, just drain the buffer
            m_serialPort->readAll();
            return;
        }

        QByteArray data = m_serialPort->readAll();
        qint64 written = m_tcpClient->write(data);

        if (written != data.size()) {
            qWarning() << "Failed to write all data to TCP";
        }
    }

    /**
     * @brief Handle TCP client disconnect
     */
    void onTcpDisconnected() {
        qDebug() << "K4Radio disconnected from bridge";
        if (m_tcpClient) {
            m_tcpClient->deleteLater();
            m_tcpClient = nullptr;
        }
    }

private:
    QString m_ptyDevice;
    quint16 m_tcpPort;
    QTcpServer* m_server;
    QTcpSocket* m_tcpClient;
    QSerialPort* m_serialPort;
};

#endif // TCP_TO_PTY_BRIDGE_H
