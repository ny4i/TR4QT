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

#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <QObject>
#include <QMenuBar>
#include <QAction>
#include <functional>

namespace TR4QT {

/**
 * MenuManager
 *
 * Handles creation and configuration of the main menu bar.
 * Extracts menu creation logic from MainWindow to improve maintainability.
 */
class MenuManager : public QObject {
    Q_OBJECT

public:
    /**
     * Configuration structure for menu action callbacks.
     * Contains all slot connections needed by menu items.
     */
    struct Config {
        // File menu
        std::function<void()> onNewOpenContest;
        std::function<void()> onClearLog;
        std::function<void()> onImportADIF;
        std::function<void()> onExportADIF;
        std::function<void()> onExportCabrillo;
        std::function<void()> onPreferences;
        std::function<void()> onExit;

        // Radio menu
        std::function<void()> onRadioConfigure;
        std::function<void()> onRadioConnect;
        std::function<void()> onRadioDisconnect;
        std::function<void(bool)> onAutoSendCWToggled;
        std::function<void()> onUpdateRadioStatusGrid;

        // Edit menu
        std::function<void()> onViewEditLog;
        std::function<void()> onClearDupes;
        std::function<void()> onNote;
        std::function<void()> onRecallLast;

        // Tools menu
        std::function<void()> onWKMode;
        std::function<void()> onBackupLog;
        std::function<void()> onDownloadCTY;
        std::function<void()> onDownloadLOTW;
        std::function<void()> onDownloadSCP;
        std::function<void()> onInitialize;
        std::function<void()> onRescoreContest;
        std::function<void()> onEditContestSettings;
        std::function<void()> onFullIntegrityCheck;
        std::function<void()> onToggleWebServer;
        std::function<void()> onResetWindowPositions;

        // Operating menu
        std::function<void()> onAutoCQ;
        std::function<void()> onAutoCQResume;
        std::function<void()> onKillCW;
        std::function<void()> onDupeCheck;
        std::function<void()> onSearchLog;
        std::function<void()> onDeleteLastQSO;
        std::function<void()> onIncNumber;
        std::function<void()> onInitialExchange;
        std::function<void()> onToggleSidetone;
        std::function<void()> onToggleAutosend;

        // Commands menu
        std::function<void()> onCQMode;
        std::function<void()> onSPMode;

        // Automation menu
        std::function<void(bool)> onAutoSPEnableToggled;
        std::function<void()> onAutoSPSensitivity;

        // Window menu
        std::function<void()> onShowBandMap;
        std::function<void()> onShowDXCluster;
        std::function<void()> onShowRadioControl;
        std::function<void()> onShowRadio2Control;
        std::function<void()> onSendMorse;
        std::function<void()> onEditCWMessages;
        std::function<void()> onShowFunctionKeysRef;
        std::function<void()> onShowMultipliers;
        std::function<void()> onShowStatistics;
        std::function<void()> onShowSectionsMap;
        std::function<void()> onShowStatesMap;
        std::function<void()> onShowWorldMap;
        std::function<void()> onShowGraylineMap;
        std::function<void()> onShowAmplifierControl;
        std::function<void()> onShowPanadapter;
        std::function<void()> onSwapMultView;
        std::function<void()> onMissingMultsReport;

        // Band menu
        std::function<void()> onBandUp;
        std::function<void()> onBandDown;
        std::function<void()> onToggleRigs;
        std::function<void()> onEditSO2R;

        // Help menu
        std::function<void()> onAbout;
#ifdef ENABLE_PERFORMANCE_PROFILING
        std::function<void()> onShowPerformanceReport;
#endif
        std::function<void()> onEmailLogsToSupport;
    };

    explicit MenuManager(QWidget* parent = nullptr);
    ~MenuManager() override = default;

    /**
     * Creates and returns the complete menu bar.
     * @param config Configuration with all menu action callbacks
     * @return Pointer to configured QMenuBar (ownership transferred to caller)
     */
    QMenuBar* createMenuBar(const Config& config);

    // Accessor methods for actions that MainWindow needs to reference
    QAction* connectAction() const { return m_connectAction; }
    QAction* disconnectAction() const { return m_disconnectAction; }
    QAction* autoSendCWAction() const { return m_autoSendCWAction; }
    QAction* webServerAction() const { return m_webServerAction; }
    QAction* bandMapAction() const { return m_bandMapAction; }
    QAction* dxClusterAction() const { return m_dxClusterAction; }
    QAction* radioControlAction() const { return m_radioControlAction; }
    QAction* radio2ControlAction() const { return m_radio2ControlAction; }
    QAction* multipliersAction() const { return m_multipliersAction; }
    QAction* statisticsAction() const { return m_statisticsAction; }
    QAction* sectionsMapAction() const { return m_sectionsMapAction; }
    QAction* statesMapAction() const { return m_statesMapAction; }
    QAction* worldMapAction() const { return m_worldMapAction; }
    QAction* graylineMapAction() const { return m_graylineMapAction; }
    QAction* amplifierControlAction() const { return m_amplifierControlAction; }
    QAction* panadapterAction() const { return m_panadapterAction; }

private:
    void createFileMenu(QMenuBar* menuBar, const Config& config);
    void createRadioMenu(QMenuBar* menuBar, const Config& config);
    void createEditMenu(QMenuBar* menuBar, const Config& config);
    void createToolsMenu(QMenuBar* menuBar, const Config& config);
    void createOperatingMenu(QMenuBar* menuBar, const Config& config);
    void createCommandsMenu(QMenuBar* menuBar, const Config& config);
    void createAutomationMenu(QMenuBar* menuBar, const Config& config);
    void createWindowMenu(QMenuBar* menuBar, const Config& config);
    void createBandMenu(QMenuBar* menuBar, const Config& config);
    void createHelpMenu(QMenuBar* menuBar, const Config& config);

    QWidget* m_parent;

    // Actions that MainWindow needs to reference
    QAction* m_connectAction;
    QAction* m_disconnectAction;
    QAction* m_autoSendCWAction;
    QAction* m_webServerAction;
    QAction* m_bandMapAction;
    QAction* m_dxClusterAction;
    QAction* m_radioControlAction;
    QAction* m_radio2ControlAction;
    QAction* m_multipliersAction;
    QAction* m_statisticsAction;
    QAction* m_sectionsMapAction;
    QAction* m_statesMapAction;
    QAction* m_worldMapAction;
    QAction* m_graylineMapAction;
    QAction* m_amplifierControlAction;
    QAction* m_panadapterAction;
};

} // namespace TR4QT

#endif // MENUMANAGER_H
