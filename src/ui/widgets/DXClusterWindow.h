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

#ifndef DXCLUSTERWINDOW_H
#define DXCLUSTERWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include "../../network/TelnetClient.h"
#include "../../utils/ReconnectionManager.h"

namespace TR4QT {

// Forward declarations
class ContestBase;
class CountryFile;

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

    /**
     * Set the active contest for dupe/multiplier checking
     * @param contest Pointer to active contest (nullptr if no contest)
     * @param contestDbId Database ID of the active contest
     */
    void setActiveContest(ContestBase* contest, int contestDbId);

    /**
     * Set the country file for DXCC/zone lookup
     * @param countryFile Pointer to CountryFile (from MainWindow)
     */
    void setCountryFile(CountryFile* countryFile);

signals:
    /**
     * Emitted when a DX spot is received
     */
    void spotReceived(const QString& callsign,
                     double frequency,
                     const QString& spotter,
                     const QString& comment);

    /**
     * User wants to QSY to a frequency (simplex)
     */
    void qsyRequested(double frequency);

    /**
     * User wants to QSY with split operation
     * @param txFrequency - Transmit frequency (where we transmit)
     * @param rxFrequency - Receive frequency (where we listen to DX)
     */
    void splitQsyRequested(double txFrequency, double rxFrequency);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

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
    /**
     * Format range for character-based text formatting
     * Used to apply colors and bold to specific character positions
     */
    struct FormatRange {
        int start;      // Starting character position
        int length;     // Number of characters
        QColor color;   // Text color
        bool bold;      // Bold weight
    };

    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateConnectionStatus(bool connected);
    void appendText(const QString& text, const QColor& color = Qt::black);
    void appendRichText(const QString& text, const QList<FormatRange>& formats, bool isSplit = false);
    void applyTheme();

    /**
     * Determine color for DX spot based on dupe/multiplier status
     * @param callsign Spotted callsign
     * @param frequency Spotted frequency in Hz
     * @return Color to use for the spot (dupe/mult/normal)
     */
    QColor getSpotColor(const QString& callsign, double frequency) const;

    /**
     * Parse split operation info from comment
     * Returns listening frequency in Hz, or 0 if not a split spot
     * Handles: "QSX 14205", "UP 10", "DOWN 5", "UP10", etc.
     */
    double parseSplitInfo(const QString& comment, double spotFrequency);

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
    bool m_autoReconnect;
    ReconnectionManager* m_reconnectManager;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    int m_spotRowCount;  // For alternating row backgrounds

    // Contest context (for dupe/multiplier checking)
    ContestBase* m_activeContest;
    int m_contestDbId;

    // Country file (for DXCC/zone lookup)
    CountryFile* m_countryFile;

    // Split operation tracking
    // Maps displayed line text -> struct with split info
    struct SplitSpotInfo {
        double spotFrequency;      // DX transmit frequency (Hz)
        double listenFrequency;    // DX listening frequency (Hz)
        QString callsign;
    };
    QMap<QString, SplitSpotInfo> m_splitSpots;
};

} // namespace TR4QT

#endif // DXCLUSTERWINDOW_H
