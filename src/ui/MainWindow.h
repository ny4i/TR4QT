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
#include "models/QSOTableModel.h"
#include "widgets/BandSummaryGrid.h"

class QMenuBar;
class QStatusBar;
class QGroupBox;
class QPushButton;

namespace TR4QT {

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
    void onRadioConfigure();
    void onRadioConnect();
    void onRadioDisconnect();
    void onAbout();
    void onExit();

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
    void loadSettings();
    void saveSettings();
    void updateConnectionStatus(bool connected);
    void updateScoreDisplay();

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

    // Radio control
    RadioController* m_radio;
    RadioState m_currentState;
    bool m_radioConnected;

    // Menus
    QAction* m_connectAction;
    QAction* m_disconnectAction;

    // Time tracking
    QTimer* m_updateTimer;
    QDateTime m_lastQSOTime;
    int m_qsosThisHour;
};

} // namespace TR4QT

#endif // MAINWINDOW_H
