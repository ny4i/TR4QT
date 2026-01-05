#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QTableView>
#include <QTimer>
#include <QDateTime>
#include "../radio/RadioController.h"
#include "../utils/AppSettings.h"
#include "../utils/CountryFile.h"
#include "../utils/SCPMatcher.h"
#include "models/QSOTableModel.h"
#include "widgets/BandSummaryGrid.h"
#include "widgets/NeedsDisplayWidget.h"
#include "dialogs/ContestChooserDialog.h"
#include "../contests/ContestBase.h"
#include "../contests/ContestRegistry.h"
#include "../exchanges/InitialExchangeManager.h"

class QMenuBar;
class QStatusBar;
class QGroupBox;
class QPushButton;
class NativeMapViewer;

namespace TR4QT {

/**
 * Statistics from contest rescore operation
 */
struct RescoreStats {
    int qsosUpdated = 0;
    int multsMarked = 0;
    int dupesFound = 0;
};

class DXClusterWindow;
class BandMapWidget;
class RadioControlWidget;
class MultiplierWidget;
class StatisticsWindow;
class FunctionKeysWindow;
class GraylineMapDialog;
class UdpBroadcastManager;
class WebServer;
class CountryFileDownloader;

/**
 * Main application window
 * Provides radio control, logging, and contest management
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Public method to trigger country file download
    void triggerCountryFileDownload();

signals:
    /**
     * Emitted when the current operating frequency changes
     * (from radio or manual band selection)
     * @param frequency Current frequency in Hz
     */
    void currentFrequencyChanged(freq_t frequency);

    /**
     * Emitted when the current operating band changes
     * (from radio or manual band selection)
     * @param band Current band
     */
    void currentBandChanged(BandType band);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    // Menu actions
    void onNewOpenContest();
    void onPreferences();
    void onImportADIF();
    void onExportADIF();
    void onExportCabrillo();
    void onClearLog();
    void onRadioConfigure();
    void onRadioConnect();
    void onRadioDisconnect();
    void onAbout();
    void onShowPerformanceReport();
    void onEmailLogsToSupport();
    void onExit();

    // Window menu actions
    void onShowDXCluster();
    void onShowBandMap();
    void onShowRadioControl();
    void onShowMultipliers();
    void onShowStatistics();
    void onShowSectionsMap();       // Show ARRL Sections map
    void onShowStatesMap();         // Show US States map (WAS)
    void onShowGraylineMap();       // Show Grayline propagation map
    void onSwapMultView();          // TODO: Implement swap multiplier view
    void onMissingMultsReport();    // TODO: Implement missing mults report

    // Edit menu actions (CTRL- shortcuts from TR4W)
    void onViewEditLog();           // TODO: Implement view/edit log window
    void onClearDupes();            // TODO: Implement clear dupes
    void onNote();                  // TODO: Implement add note to log
    void onRecallLast();            // TODO: Implement recall last entry

    // Tools menu actions (ALT- shortcuts from TR4W)
    void onWKMode();                // TODO: Implement WinKeyer re-initialization
    void onSendMorse();             // Send morse code dialog
    void onShowFunctionKeysRef();   // Show function keys reference window
    void onBackupLog();             // TODO: Implement backup log
    void onToggleWebServer();       // Start/stop web server
    void onDownloadCTY(bool headless = false);   // Download CTY.dat (headless=true for testing)
    void onDownloadLOTW(bool headless = false);  // Download LOTW user list (headless=true for testing)
    void onDownloadSCP(bool headless = false);   // Download MASTER.SCP (headless=true for testing)
    void onCTYUpdateAvailable(int currentVersion, int latestVersion, const QString& versionString);
    void onInitialize();            // TODO: Implement initialize
    void onResetWindowPositions();  // Reset all window positions to defaults

    // Operating menu actions (ALT- shortcuts from TR4W)
    void onAutoCQ();                // TODO: Implement auto CQ
    void onAutoCQResume();          // TODO: Implement auto CQ resume
    void onKillCW();                // TODO: Implement kill CW
    void onDupeCheck();             // TODO: Implement dupe check
    void onSearchLog();             // TODO: Implement search log
    void onDeleteLastQSO();         // TODO: Implement delete last QSO
    void onIncNumber();             // TODO: Implement increment number
    void onInitialExchange();       // TODO: Implement initial exchange
    void onCWSpeed();               // TODO: Implement CW speed
    void onToggleSidetone();        // TODO: Implement toggle sidetone
    void onToggleAutosend();        // TODO: Implement toggle autosend

    // Band menu actions (ALT- shortcuts from TR4W)
    // onBandUp() and onBandDown() already exist
    void onToggleRigs();            // TODO: Implement toggle rigs (SO2R)
    void onEditSO2R();              // TODO: Implement edit SO2R

    // Commands menu actions (operating mode switching)
    void onCQMode();                // Switch to CQ mode (Shift+Tab)
    void onSPMode();                // Switch to S&P mode (Tab)

    // DX Cluster integration
    void onDXSpotReceived(const QString& callsign,
                          double frequency,
                          const QString& spotter,
                          const QString& comment);

    // Band switching
    void onBandClicked(BandType band);
    void onBandUp();
    void onBandDown();

    // Radio signals
    void onRadioConnected(bool connected);
    void onRadioStateUpdated(const RadioState& state);
    void onRadioError(const QString& error);
    void onRadioModelChanged(const QString& model);

    // Logging actions
    void onLogQSO();
    void onCallsignChanged(const QString& callsign);
    void onCallsignEnterPressed();  // Handle Enter key in callsign field
    void onExchangeTextChanged(const QString& text);  // Real-time validation
    void onClearEntry();
    void onEditQSO(const QModelIndex& index);
    void onQSOTableContextMenu(const QPoint& pos);

    // Timer update
    void updateTimeDisplay();

    // Data integrity checks
    void onPeriodicIntegrityCheck();  // Tier 2: Periodic check
    void onFullIntegrityCheck();      // Tier 3: On-demand full check
    void onRescoreContest();          // Rescore entire contest (points + mults)

    // UDP log rebroadcast
    void onRebroadcastLog();          // UDP command: rebroadcast entire log

private:
    void setupUI();
    void createMenuBar();
    void createStatusBar();
    void createCentralWidget();
    QWidget* createBottomPanel();
    void loadSettings();
    void saveSettings();
    void applyFontSettings();
    void applyTheme();
    void saveQSOTableColumnWidths();
    void loadQSOTableColumnWidths();
    void onQSOTableColumnResized(int logicalIndex, int oldSize, int newSize);
    void updateRadioStatusFlash();
    void loadUdpBroadcastSettings();
    void loadBackupSettings();
    void updateConnectionStatus(bool connected);
    void updateScoreDisplay();
    void recalculateAllPoints();  // Recalculate points for all QSOs in log
    void updateRadioStatusGrid();
    void raiseAllWindows();
    void setStatusMessage(const QString& message);  // Set status and log it
    void updateWindowMenuCheckmarks();  // Update checkmarks for open windows

    // Contest management
    void activateContest(const ContestInfo& contestInfo);
    void updateExchangeFieldsForContest();
    void autoPopulateExchange(const QString& callsign);
    void reopenLastContest();

    // Band switching helpers
    freq_t getFrequencyForBand(BandType band, ModeType mode) const;
    BandType getNextBand(BandType currentBand) const;
    BandType getPreviousBand(BandType currentBand) const;
    BandType getBandFromFrequency(freq_t frequency) const;

    // Duplicate checking
    bool checkForDuplicate(const QString& callsign, BandType band, ModeType mode, QString& dupeInfo) const;

    // Band needs tracking
    QList<BandType> getWorkedBandsForCallsign(const QString& callsign) const;
    QList<BandType> getWorkedBandsForMultiplier(const QString& multValue, MultiplierType type) const;
    QString getMultiplierValueForCallsign(const QString& callsign) const;
    QSet<QString> getWorkedCallsigns() const;  // Get all worked callsigns from log

    // Station info display
    void updateStationInfo(const QString& callsign);

    // Data integrity helpers
    bool quickIntegrityCheck();         // Quick count-based check
    QString fullIntegrityCheck(bool criticalOnly = false);  // Detailed check with report
    void handleIntegrityMismatch(int memoryCount, int dbCount);

    // Rescore helpers
    RescoreStats rescoreContestSilent();  // Rescore without showing dialogs

    // Operating mode helpers
    void setOperatingMode(OperatingMode mode);  // Switch operating mode and update UI
    void checkAutoSP(freq_t newFrequency);      // Check if AUTO S&P should trigger

    // Sent exchange helpers
    QString substituteSentExchangeTemplate(const QString& templateStr, int serialNumber, const QString& rst) const;

    // UI Components
    QLabel* m_statusLabel;
    QLabel* m_radioStatusLabel;

    // Band summary grid (top)
    BandSummaryGrid* m_bandSummaryGrid;

    // Needs display widget (upper right)
    NeedsDisplayWidget* m_needsDisplayWidget;

    // Logging UI
    QLineEdit* m_callsignEntry;
    QLineEdit* m_exchangeEntry;
    QTableView* m_qsoTableView;
    QSOTableModel* m_qsoTableModel;
    QLabel* m_scpMatchesLabel;  // Display SCP callsign matches (Column 3)
    QLabel* m_countryNameLabel;  // Display country name
    QLabel* m_stationInfoLabel;  // Display station info (prefix, bearing, distance)

    // Stats panel (bottom right)
    QLabel* m_rateLabel;
    QLabel* m_timeLabel;
    QLabel* m_thisHrLabel;
    QLabel* m_cqCountLabel;
    QLabel* m_spCountLabel;
    QLabel* m_operatorLabelStatic;  // "Op:" label
    QLabel* m_operatorLabel;

    // Radio status grid (bottom)
    QLabel* m_radioFreqBandLabel;  // Shows "15SSB"
    QLabel* m_radioFreqLabel;      // Shows frequency
    QLabel* m_radioWpmLabel;       // Shows CW speed (WPM)
    QLabel* m_radioDateLabel;      // Shows current date
    QLabel* m_radioTimeLabel;      // Shows current time

    // Radio control
    RadioController* m_radio;
    RadioState m_currentState;
    bool m_radioConnected;
    bool m_radioAutoReconnect;
    QTimer* m_radioReconnectTimer;
    RadioConfig m_lastRadioConfig;  // Store config for reconnection attempts
    int m_radioReconnectAttempts;
    static constexpr int MAX_RADIO_RECONNECT_ATTEMPTS = 10;
    QTimer* m_radioFlashTimer;      // Timer for flashing red indicator
    bool m_radioFlashState;          // Current flash state (on/off)

    // Menus
    QAction* m_connectAction;
    QAction* m_disconnectAction;
    QAction* m_autoSendCWAction;
    QAction* m_webServerAction;

    // Window menu actions (for checkmarks)
    QAction* m_bandMapAction;
    QAction* m_dxClusterAction;
    QAction* m_radioControlAction;
    QAction* m_multipliersAction;
    QAction* m_statisticsAction;
    QAction* m_sectionsMapAction;
    QAction* m_statesMapAction;
    QAction* m_graylineMapAction;

    // Window widgets
    DXClusterWindow* m_dxClusterWindow;
    BandMapWidget* m_bandMapWindow;
    RadioControlWidget* m_radioControlWindow;
    MultiplierWidget* m_multiplierWindow;
    StatisticsWindow* m_statisticsWindow;
    FunctionKeysWindow* m_functionKeysWindow;
    NativeMapViewer* m_sectionsMapViewer;
    NativeMapViewer* m_statesMapViewer;
    GraylineMapDialog* m_graylineMapDialog;

    // Time tracking
    QTimer* m_updateTimer;
    QDateTime m_lastQSOTime;
    int m_qsosThisHour;

    // Data integrity tracking (Tier 2)
    QTimer* m_integrityCheckTimer;
    int m_qsosSinceLastIntegrityCheck;

    // Contest information
    ContestInfo m_currentContest;
    bool m_hasActiveContest;
    ContestBase* m_activeContest;
    int m_currentContestDbId;  // Database primary key for current contest
    int m_nextSerialNumber;

    // Country file for lookups
    CountryFile m_countryFile;
    CountryFileDownloader* m_countryFileDownloader;  // For version checking
    int m_latestCTYVersion;  // Latest CTY version from update check

    // Super Check Partial matcher
    SCPMatcher* m_scpMatcher;

    // UDP Broadcast manager
    UdpBroadcastManager* m_udpBroadcastManager;

    // Web server for remote contest viewing
    WebServer* m_webServer;

    // Guard flag to prevent infinite recursion in raiseAllWindows
    bool m_inRaiseAllWindows;

    // Exchange auto-population tracking
    bool m_initialExchangePopulated;

    // Operating mode (CQ vs S&P)
    OperatingMode m_operatingMode;
    QLabel* m_operatingModeLabel;  // Visual indicator

    // AUTO S&P VFO tracking
    freq_t m_lastFrequency;
    QDateTime m_lastFrequencyTime;
};

} // namespace TR4QT

#endif // MAINWINDOW_H
