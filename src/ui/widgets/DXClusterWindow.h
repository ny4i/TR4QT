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
#include <QPointer>
#include <optional>
#include "../../network/TelnetClient.h"
#include "../../utils/ReconnectionManager.h"
#include "../../services/SpotProcessorWorker.h"

namespace TR4QT {

// Forward declarations
class ContestBase;

/**
 * DX Cluster Window
 *
 * Provides telnet interface to DX cluster servers.
 * Displays incoming spots and allows user to send commands.
 *
 * Features:
 * - Telnet client runs in separate thread (never blocks UI)
 * - Spot processing runs in separate worker thread (DB queries off UI)
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
     * Get the spot processor worker (for MainWindow to connect signals)
     */
    SpotProcessorWorker* spotProcessor() const { return m_spotWorker; }

    /**
     * Update spot processor configuration (colors, settings).
     * Call this after theme changes or settings changes.
     */
    void updateSpotProcessorConfig();

signals:
    /**
     * Emitted when a processed spot is ready (forwarded from worker)
     */
    void spotProcessed(const TR4QT::ProcessedSpot& spot);

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

    /**
     * Receive processed spot from worker thread and display it
     */
    void onSpotProcessed(const TR4QT::ProcessedSpot& result);

    /**
     * Called when telnet thread has created its client object
     */
    void onTelnetClientReady(TelnetClient* client);

    /**
     * Called (via queued connection) after worker's initDatabase() completes.
     * Drains any pending contest context.
     */
    void onWorkerInitialized();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateConnectionStatus(bool connected);
    void appendText(const QString& text, const QColor& color = Qt::black);
    void appendRichText(const QString& text, const QList<SpotFormatRange>& formats, bool isSplit = false);
    void applyTheme();
    void pruneSplitSpots();

    /**
     * Build a SpotProcessorConfig from current ThemeManager + AppSettings.
     * Called on the main thread, result is passed to worker via queued connection.
     */
    static SpotProcessorConfig buildConfig();

    /**
     * Parsed result from a spot display line (used by event filter click handlers)
     */
    struct ParsedSpotLine {
        bool valid{false};
        bool isSplit{false};
        double freqKHz{0};
        QString callsign;
    };

    /**
     * Parse a spot display line to extract split flag, frequency, and callsign.
     */
    static ParsedSpotLine parseSpotLine(const QString& line);

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
    bool m_isConnected{false};
    QStringList m_recentServers;
    bool m_autoReconnect;
    ReconnectionManager* m_reconnectManager;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
    static constexpr int RECONNECT_INTERVAL_MS = 10000;    // 10 seconds between reconnect attempts
    static constexpr int AUTO_CONNECT_DELAY_MS = 500;      // Delay to allow UI initialization before auto-connect
    int m_spotRowCount;  // For alternating row backgrounds

    // Spot processor (runs in worker thread)
    QThread* m_spotWorkerThread;
    SpotProcessorWorker* m_spotWorker;
    bool m_workerInitialized{false};

    // Pending contest context (queued if setActiveContest called before worker init)
    struct ContestContext {
        int contestDbId{-1};
        QList<MultiplierDefinition> multDefs;
    };
    std::optional<ContestContext> m_pendingContestContext;

    // Cached config for alternating row colors (read from config on theme change)
    QColor m_evenRowBackground{Qt::white};
    QColor m_oddRowBackground{Qt::white};

    // Split operation tracking
    // Maps displayed line text -> struct with split info
    struct SplitSpotInfo {
        double spotFrequency;      // DX transmit frequency (Hz)
        double listenFrequency;    // DX listening frequency (Hz)
        QString callsign;
    };
    QMap<QString, SplitSpotInfo> m_splitSpots;
    static constexpr int MAX_SPLIT_SPOTS_CACHED = 500;
};

} // namespace TR4QT

#endif // DXCLUSTERWINDOW_H
