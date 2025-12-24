#ifndef DXCLUSTERWINDOW_H
#define DXCLUSTERWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include "../../network/TelnetClient.h"

namespace TR4QT {

/**
 * DX Cluster Window
 *
 * Provides telnet interface to DX cluster servers.
 * Displays incoming spots and allows user to send commands.
 *
 * Features:
 * - Telnet client runs in separate thread (never blocks UI)
 * - Connect/Disconnect/Freeze/Clear/Commands controls
 * - Text display of cluster output
 * - Command input field
 * - Forwards spots to band map
 */
class DXClusterWindow : public QWidget {
    Q_OBJECT

public:
    explicit DXClusterWindow(QWidget* parent = nullptr);
    ~DXClusterWindow() override;

signals:
    /**
     * Emitted when a DX spot is received
     */
    void spotReceived(const QString& callsign,
                     double frequency,
                     const QString& spotter,
                     const QString& comment);

    /**
     * User wants to QSY to a frequency
     */
    void qsyRequested(double frequency);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onFreezeClicked();
    void onClearClicked();
    void onCommandsClicked();
    void onSendClicked();

    // Telnet client signals
    void onTelnetConnected();
    void onTelnetDisconnected();
    void onTelnetError(const QString& error);
    void onTelnetDataReceived(const QString& data);
    void onTelnetSpotReceived(const QString& callsign,
                             double frequency,
                             const QString& spotter,
                             const QString& comment,
                             const QString& timestamp);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateConnectionStatus(bool connected);
    void appendText(const QString& text, const QColor& color = Qt::black);

    // UI elements
    QComboBox* m_serverCombo;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QPushButton* m_freezeButton;
    QPushButton* m_clearButton;
    QPushButton* m_commandsButton;
    QPushButton* m_sendButton;
    QLineEdit* m_commandEdit;
    QTextEdit* m_textDisplay;
    QLabel* m_statusLabel;

    // Telnet client (runs in separate thread)
    TelnetThread* m_telnetThread;
    TelnetClient* m_telnetClient;

    // State
    bool m_isFrozen;
    QStringList m_recentServers;
};

} // namespace TR4QT

#endif // DXCLUSTERWINDOW_H
