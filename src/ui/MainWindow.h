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
#include "models/QSOTableModel.h"
#include "widgets/BandSummaryGrid.h"
#include "dialogs/ContestChooserDialog.h"
#include "../contests/ContestBase.h"
#include "../contests/CQWWContest.h"
#include "../contests/CQWPXContest.h"
#include "../contests/WinterFieldDayContest.h"

class QMenuBar;
class QStatusBar;
class QGroupBox;
class QPushButton;

namespace TR4QT {

class DXClusterWindow;
class BandMapWidget;
class RadioControlWidget;
class MultiplierWidget;
class UdpBroadcastManager;

/**
 * Main application window
 * Provides radio control, logging, and contest management
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Menu actions
    void onNewOpenContest();
    void onPreferences();
    void onExportADIF();
    void onExportCabrillo();
    void onClearLog();
    void onRadioConfigure();
    void onRadioConnect();
    void onRadioDisconnect();
    void onAbout();
    void onExit();

    // Window menu actions
    void onShowDXCluster();
    void onShowBandMap();
    void onShowRadioControl();
    void onShowMultipliers();
    void onSwapMultView();          // TODO: Implement swap multiplier view
    void onMissingMultsReport();    // TODO: Implement missing mults report

    // Edit menu actions (CTRL- shortcuts from TR4W)
    void onViewEditLog();           // TODO: Implement view/edit log window
    void onClearDupes();            // TODO: Implement clear dupes
    void onNote();                  // TODO: Implement add note to log
    void onRecallLast();            // TODO: Implement recall last entry

    // Tools menu actions (ALT- shortcuts from TR4W)
    void onWKMode();                // TODO: Implement WinKeyer re-initialization
    void onBackupLog();             // TODO: Implement backup log
    void onDownloadCTY();           // TODO: Implement download CTY.dat
    void onSetDateTime();           // TODO: Implement set system date/time
    void onInitialize();            // TODO: Implement initialize

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
    void onClearEntry();

    // Timer update
    void updateTimeDisplay();

private:
    void setupUI();
    void createMenuBar();
    void createStatusBar();
    void createCentralWidget();
    QWidget* createBottomPanel();
    QWidget* createRadioStatusGrid();
    void loadSettings();
    void saveSettings();
    void applyFontSettings();
    void applyTheme();
    void loadUdpBroadcastSettings();
    void loadBackupSettings();
    void updateConnectionStatus(bool connected);
    void updateScoreDisplay();
    void updateRadioStatusGrid();

    // Contest management
    void activateContest(const ContestInfo& contestInfo);
    void updateExchangeFieldsForContest();
    void autoPopulateExchange(const QString& callsign);

    // Band switching helpers
    freq_t getFrequencyForBand(BandType band, ModeType mode) const;
    BandType getNextBand(BandType currentBand) const;
    BandType getPreviousBand(BandType currentBand) const;
    BandType getBandFromFrequency(freq_t frequency) const;

    // UI Components
    QLabel* m_statusLabel;
    QLabel* m_radioStatusLabel;

    // Band summary grid (top)
    BandSummaryGrid* m_bandSummaryGrid;

    // Logging UI
    QLineEdit* m_callsignEntry;
    QLineEdit* m_exchangeEntry;
    QPushButton* m_logButton;
    QTableView* m_qsoTableView;
    QSOTableModel* m_qsoTableModel;

    // Stats panel (bottom right)
    QLabel* m_rateLabel;
    QLabel* m_timeLabel;
    QLabel* m_thisHrLabel;
    QLabel* m_cqCountLabel;
    QLabel* m_spCountLabel;
    QLabel* m_operatorLabel;

    // Radio status grid (bottom)
    QLabel* m_radioFreqBandLabel;  // Shows "15SSB"
    QLabel* m_radioFreqLabel;      // Shows frequency
    QLabel* m_radioDateTimeLabel;  // Shows current date/time

    // Radio control
    RadioController* m_radio;
    RadioState m_currentState;
    bool m_radioConnected;

    // Menus
    QAction* m_connectAction;
    QAction* m_disconnectAction;

    // Window widgets
    DXClusterWindow* m_dxClusterWindow;
    BandMapWidget* m_bandMapWindow;
    RadioControlWidget* m_radioControlWindow;
    MultiplierWidget* m_multiplierWindow;

    // Time tracking
    QTimer* m_updateTimer;
    QDateTime m_lastQSOTime;
    int m_qsosThisHour;

    // Contest information
    ContestInfo m_currentContest;
    bool m_hasActiveContest;
    ContestBase* m_activeContest;
    int m_currentContestDbId;  // Database primary key for current contest
    int m_nextSerialNumber;

    // Country file for lookups
    CountryFile m_countryFile;

    // UDP Broadcast manager
    UdpBroadcastManager* m_udpBroadcastManager;
};

} // namespace TR4QT

#endif // MAINWINDOW_H
