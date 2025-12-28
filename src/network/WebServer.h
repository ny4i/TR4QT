#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QHttpServer>
#include <QHostAddress>
#include "../models/QSO.h"
#include "../radio/RadioInterface.h"

class QTcpServer;

namespace TR4QT {

// Forward declarations
class QSOTableModel;
class RadioController;
class AppSettings;

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
    explicit WebServer(QSOTableModel* qsoModel,
                      RadioController* radioController,
                      QObject* parent = nullptr);
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

    // Contest state (set when contest changes)
    void setContestName(const QString& contestName);

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
    QHttpServerResponse handleApiWorkedSections();
    QHttpServerResponse handleDashboard();
    QHttpServerResponse handleSectionsMap();
    QHttpServerResponse handleFavicon();
    QHttpServerResponse handleAppleTouchIcon();

    // Helper methods
    QString generateDashboardHtml();
    QString generateSectionsMapHtml();
    QJsonObject radioStateToJson(const RadioState& state);
    QJsonObject qsoToJson(const QSO& qso);

    QHttpServer* m_server;
    QTcpServer* m_tcpServer{nullptr};
    quint16 m_port{14140};  // Default: 14140 (ham radio frequency joke: 14.140 MHz)
    QHostAddress m_address{QHostAddress::LocalHost};

    // Data sources (not owned - pull model)
    QSOTableModel* m_qsoModel;
    RadioController* m_radioController;

    // Contest state (minimal - only what can't be pulled)
    QString m_contestName;

    static constexpr int MAX_RECENT_QSOS = 10;
};

} // namespace TR4QT

#endif // WEBSERVER_H
