#ifndef ICOMNETWORK_H
#define ICOMNETWORK_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QMap>
#include <QTime>
#include <QMutex>
#include <QElapsedTimer>
#include "icompackets.h"

/**
 * @brief Configuration structure for Icom network connection
 */
struct IcomConnectionConfig {
    QString ipAddress;          // Radio IP address or hostname
    quint16 controlPort;        // Control port (usually 50001)
    QString username;           // Login username
    QString password;           // Login password
    QString clientName;         // Client identifier (max 16 chars)
};

/**
 * @brief Radio information structure
 */
struct IcomRadioInfo {
    QString name;               // Radio model name
    quint8 civAddress;          // CI-V address
    quint32 baudRate;           // CI-V baud rate
    QByteArray macAddress;      // MAC address or GUID
    bool useGuid;               // true if using GUID instead of MAC
};

/**
 * @brief Connection statistics
 */
struct IcomConnectionStats {
    quint32 packetsSent = 0;
    quint32 packetsLost = 0;
    quint16 rtt = 0;            // Round-trip time in ms
    QString statusMessage;
};

/**
 * @brief Simplified Icom network class for command and control
 *
 * This class handles network connection to Icom radios over LAN,
 * managing authentication and CI-V command transmission/reception.
 * Audio functionality is not included - only control commands.
 */
class IcomNetwork : public QObject
{
    Q_OBJECT

public:
    explicit IcomNetwork(QObject *parent = nullptr);
    ~IcomNetwork();

    // Connection management
    void connectToRadio(const IcomConnectionConfig& config);
    void disconnectFromRadio();
    bool isConnected() const { return m_streamOpened; }
    bool isAuthenticated() const { return m_authenticated; }

    // Radio information
    QList<IcomRadioInfo> availableRadios() const { return m_radios; }
    IcomRadioInfo currentRadio() const { return m_currentRadio; }

    // CI-V command interface
    void sendCivCommand(const QByteArray& command);

    // Statistics
    IcomConnectionStats statistics() const { return m_stats; }

signals:
    // Connection state signals
    void connected();
    void disconnected();
    void authenticationFailed(const QString& reason);
    void connectionError(const QString& error);

    // Radio discovery
    void radiosDiscovered(const QList<IcomRadioInfo>& radios);

    // CI-V data
    void civDataReceived(const QByteArray& data);
    void civSocketReady();  // Emitted when CI-V socket acknowledges it's ready

    // Status updates
    void statusUpdate(const QString& message);
    void statisticsUpdated(const IcomConnectionStats& stats);

public slots:
    void selectRadio(int index);

private slots:
    void onControlDataReceived();
    void onCivDataReceived();
    void sendAreYouThere();
    void sendPing();
    void sendIdlePacket();
    void sendTokenRenewal();
    void checkRetransmit();
    void watchdogTimeout();
    void checkCivSocketDiagnostic();  // DEBUG: Periodic check for CI-V data

private:
    // Connection state
    enum ConnectionState {
        Disconnected,
        WaitingForHere,
        WaitingForReady,
        WaitingForLogin,
        Authenticated,
        StreamRequested,
        Connected
    };

    // Packet buffer entry
    struct PacketBufferEntry {
        QTime timeSent;
        quint16 seqNum;
        QByteArray data;
        quint8 retransmitCount;
    };

    // Initialize sockets
    void initControlSocket();
    void initCivSocket(quint16 civPort);

    // Packet sending
    void sendControlPacket(quint8 type, quint16 seq, bool tracked = false);
    void sendLogin();
    void sendToken(quint8 magic);
    void sendRequestStream();
    void sendTrackedPacket(QUdpSocket* socket, const QByteArray& data, quint16& seqNum);
    void sendCivOpenClose(bool close);

    // Packet processing
    void processControlPacket(const QByteArray& data);
    void processCivPacket(const QByteArray& data);
    void handleRetransmitRequest(const QByteArray& data);

    // ID generation
    quint32 calculateMyId();

    // Cleanup
    void cleanup();
    void setState(ConnectionState state);

    // Configuration
    IcomConnectionConfig m_config;

    // Network
    QUdpSocket* m_controlSocket;
    QUdpSocket* m_civSocket;
    QHostAddress m_radioIP;
    QHostAddress m_localIP;
    quint16 m_controlLocalPort;
    quint16 m_civLocalPort;
    quint16 m_civRemotePort;

    // Connection state
    ConnectionState m_state;
    bool m_authenticated;
    bool m_streamOpened;
    quint32 m_myId;
    quint32 m_remoteId;         // Control socket remote ID
    quint32 m_civRemoteId;      // CI-V socket remote ID (different from control!)
    quint16 m_authSeq;
    quint16 m_tokRequest;
    quint32 m_token;

    // Sequence numbers
    quint16 m_sendSeq;           // Control socket sequence
    quint16 m_civSeq;            // CI-V socket outer packet sequence (for UDP)
    quint16 m_civInnerSeq;       // CI-V socket inner stream sequence (sendseq in packet)
    quint16 m_pingSendSeq;

    // Timers
    QTimer* m_areYouThereTimer;
    QTimer* m_pingTimer;
    QTimer* m_idleTimer;
    QTimer* m_tokenTimer;
    QTimer* m_retransmitTimer;
    QTimer* m_watchdogTimer;
    QTimer* m_civStartTimer;
    QTimer* m_civDiagnosticTimer;  // DEBUG: Diagnostic timer to check CI-V socket

    // Packet tracking
    QMap<quint16, PacketBufferEntry> m_controlTxBuf;
    QMap<quint16, PacketBufferEntry> m_civTxBuf;
    QMap<quint16, QTime> m_controlRxBuf;
    QMap<quint16, QTime> m_civRxBuf;
    QMap<quint16, int> m_controlMissing;
    QMap<quint16, int> m_civMissing;

    // Mutexes
    QMutex m_controlTxMutex;
    QMutex m_civTxMutex;
    QMutex m_controlRxMutex;
    QMutex m_civRxMutex;

    // Radio info
    QList<IcomRadioInfo> m_radios;
    IcomRadioInfo m_currentRadio;

    // Statistics
    IcomConnectionStats m_stats;
    QDateTime m_lastPingSentTime;
    QTime m_lastCivReceived;
    quint32 m_packetsSent;
    quint32 m_packetsLost;

    // Monitoring
    QElapsedTimer m_mono;
    int m_areYouThereCounter;
};

#endif // ICOMNETWORK_H
