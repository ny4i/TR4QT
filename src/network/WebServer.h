#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QHttpServer>
#include <QHostAddress>
#include "../models/QSO.h"
#include "../radio/RadioInterface.h"

class QTcpServer;

namespace TR4QT {

/**
 * WebServer - HTTP server for contest information display
 *
 * Provides JSON API and HTML dashboard for viewing contest information
 * from remote browsers (phones, tablets, other PCs on network).
 *
 * Features:
 * - Configurable port and network interface
 * - JSON API endpoints for programmatic access
 * - HTML dashboard for human viewing
 * - Real-time contest statistics
 * - Recent QSO list
 * - Radio status
 * - Operator information
 *
 * Security:
 * - Runs on LAN only (no internet exposure)
 * - Default: localhost only (127.0.0.1)
 * - Optional: all interfaces (0.0.0.0) for network access
 * - No authentication (trust LAN environment)
 */
class WebServer : public QObject {
    Q_OBJECT

public:
    explicit WebServer(QObject* parent = nullptr);
    ~WebServer() override;

    // Server control
    bool start(quint16 port = 14140, const QHostAddress& address = QHostAddress::LocalHost);
    void stop();
    bool isRunning() const;

    // Configuration
    void setPort(quint16 port);
    void setAddress(const QHostAddress& address);
    quint16 port() const { return m_port; }
    QHostAddress address() const { return m_address; }
    QString url() const;

    // Data updates (called from MainWindow)
    void setContestInfo(const QString& contestName, const QString& myCall);
    void setOperator(const QString& operatorCall);
    void setRadioState(const RadioState& state);
    void addRecentQSO(const QSO& qso);
    void setScore(int qsos, int points, int multipliers);

    // Per-band score data (for detail view)
    void setBandQSOCount(BandType band, int count);
    void setBandMultCount(BandType band, int count);
    void setBandZoneCount(BandType band, int count);
    void setBandPoints(BandType band, int points);
    void clearBandData();

signals:
    void serverStarted(const QString& url);
    void serverStopped();
    void errorOccurred(const QString& error);

private:
    // HTTP route handlers
    QHttpServerResponse handleRoot();
    QHttpServerResponse handleApiStatus();
    QHttpServerResponse handleApiQsos();
    QHttpServerResponse handleApiRadio();
    QHttpServerResponse handleApiScore();
    QHttpServerResponse handleDashboard();

    // Helper methods
    QString generateDashboardHtml();
    QJsonObject radioStateToJson(const RadioState& state);
    QJsonObject qsoToJson(const QSO& qso);

    QHttpServer* m_server;
    QTcpServer* m_tcpServer{nullptr};
    quint16 m_port{14140};  // Default: 14140 (ham radio frequency joke: 14.140 MHz)
    QHostAddress m_address{QHostAddress::LocalHost};

    // Contest data (cached for serving)
    QString m_contestName;
    QString m_myCall;
    QString m_operatorCall;
    RadioState m_radioState;
    QList<QSO> m_recentQSOs;  // Last 10 QSOs
    int m_qsoCount{0};
    int m_totalPoints{0};
    int m_multiplierCount{0};

    // Per-band data (for detail view)
    QMap<BandType, int> m_bandQSOs;
    QMap<BandType, int> m_bandMults;
    QMap<BandType, int> m_bandZones;
    QMap<BandType, int> m_bandPoints;

    static constexpr int MAX_RECENT_QSOS = 10;
};

} // namespace TR4QT

#endif // WEBSERVER_H
