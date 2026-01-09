# SettingsManager Usage Examples

This document shows how MainWindow can use SettingsManager to load and save settings.

## Loading Settings on Startup

Replace the current `loadSettings()` method in MainWindow with these calls:

```cpp
void MainWindow::loadSettings() {
    SettingsManager settingsManager;

    // Load and apply fonts
    FontConfig fonts = settingsManager.loadFontConfig();
    m_callsignEntry->setFont(fonts.entryFont);
    m_exchangeEntry->setFont(fonts.entryFont);
    m_qsoTableView->setFont(fonts.tableFont);
    m_timeLabel->setFont(fonts.miscFont);
    m_thisHrLabel->setFont(fonts.miscFont);
    m_rateLabel->setFont(fonts.miscFont);
    m_cqCountLabel->setFont(fonts.miscFont);
    m_spCountLabel->setFont(fonts.miscFont);
    m_operatorLabelStatic->setFont(fonts.miscFont);
    m_operatorLabel->setFont(fonts.miscFont);
    m_stationInfoLabel->setFont(fonts.miscFont);

    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setFontSize(fonts.gridFontSize);
    }

    m_scpMatchesLabel->setStyleSheet(
        QString("QLabel { color: #0066cc; font-size: %1pt; }").arg(fonts.scpFontSize)
    );

    // Load and restore window geometry
    WindowGeometry geometry = settingsManager.loadWindowGeometry();

    // Main window
    if (!geometry.mainWindowGeometry.isEmpty()) {
        restoreGeometry(geometry.mainWindowGeometry);
    }
    if (!geometry.mainWindowState.isEmpty()) {
        restoreState(geometry.mainWindowState);
    }

    // Connect to theme changes and apply initial theme
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::applyTheme);
    applyTheme();

    // Restore child windows
    if (geometry.dxClusterVisible) {
        LOG_DEBUG("MainWindow", "Restoring DX Cluster window");
        onShowDXCluster();
        if (!geometry.dxClusterGeometry.isEmpty()) {
            m_dxClusterWindow->restoreGeometry(geometry.dxClusterGeometry);
        }
    }

    if (geometry.bandMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Band Map window");
        onShowBandMap();
        if (!geometry.bandMapGeometry.isEmpty()) {
            m_bandMapWindow->restoreGeometry(geometry.bandMapGeometry);
        }
    }

    if (geometry.radioControlVisible) {
        LOG_DEBUG("MainWindow", "Restoring Radio Control window");
        onShowRadioControl();
        if (!geometry.radioControlGeometry.isEmpty()) {
            m_radioControlWindow->restoreGeometry(geometry.radioControlGeometry);
        }
    }

    if (geometry.multipliersVisible) {
        LOG_DEBUG("MainWindow", "Restoring Multipliers window");
        onShowMultipliers();
        if (!geometry.multipliersGeometry.isEmpty()) {
            m_multiplierWindow->restoreGeometry(geometry.multipliersGeometry);
        }
    }

    if (geometry.statisticsVisible) {
        LOG_DEBUG("MainWindow", "Restoring Statistics window");
        onShowStatistics();
        if (!geometry.statisticsGeometry.isEmpty()) {
            m_statisticsWindow->restoreGeometry(geometry.statisticsGeometry);
        }
    }

    if (geometry.sectionsMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Sections Map window");
        onShowSectionsMap();
    }

    if (geometry.statesMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring States Map window");
        onShowStatesMap();
    }

    if (geometry.graylineMapVisible) {
        LOG_DEBUG("MainWindow", "Restoring Grayline Map window");
        onShowGraylineMap();
        if (!geometry.graylineMapGeometry.isEmpty()) {
            m_graylineMapDialog->restoreGeometry(geometry.graylineMapGeometry);
        }
    }

    // Display current operator
    if (!geometry.currentOperator.isEmpty()) {
        m_operatorLabel->setText(geometry.currentOperator);
    }
}
```

## Saving Settings on Shutdown

Replace the current `saveSettings()` method with:

```cpp
void MainWindow::saveSettings() {
    SettingsManager settingsManager;

    WindowGeometry geometry;

    // Main window
    geometry.mainWindowGeometry = saveGeometry();
    geometry.mainWindowState = saveState();

    // Child windows
    if (m_dxClusterWindow) {
        geometry.dxClusterVisible = m_dxClusterWindow->isVisible();
        geometry.dxClusterGeometry = m_dxClusterWindow->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving DX Cluster - visible: %1")
            .arg(geometry.dxClusterVisible ? "true" : "false"));
    } else {
        geometry.dxClusterVisible = false;
    }

    if (m_bandMapWindow) {
        geometry.bandMapVisible = m_bandMapWindow->isVisible();
        geometry.bandMapGeometry = m_bandMapWindow->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving Band Map - visible: %1")
            .arg(geometry.bandMapVisible ? "true" : "false"));
    } else {
        geometry.bandMapVisible = false;
    }

    if (m_radioControlWindow) {
        geometry.radioControlVisible = m_radioControlWindow->isVisible();
        geometry.radioControlGeometry = m_radioControlWindow->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving Radio Control - visible: %1")
            .arg(geometry.radioControlVisible ? "true" : "false"));
    } else {
        geometry.radioControlVisible = false;
    }

    if (m_multiplierWindow) {
        geometry.multipliersVisible = m_multiplierWindow->isVisible();
        geometry.multipliersGeometry = m_multiplierWindow->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving Multipliers - visible: %1")
            .arg(geometry.multipliersVisible ? "true" : "false"));
    } else {
        geometry.multipliersVisible = false;
    }

    if (m_statisticsWindow) {
        geometry.statisticsVisible = m_statisticsWindow->isVisible();
        geometry.statisticsGeometry = m_statisticsWindow->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving Statistics - visible: %1")
            .arg(geometry.statisticsVisible ? "true" : "false"));
    } else {
        geometry.statisticsVisible = false;
    }

    if (m_sectionsMapViewer) {
        geometry.sectionsMapVisible = m_sectionsMapViewer->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving Sections Map - visible: %1")
            .arg(geometry.sectionsMapVisible ? "true" : "false"));
    } else {
        geometry.sectionsMapVisible = false;
    }

    if (m_statesMapViewer) {
        geometry.statesMapVisible = m_statesMapViewer->isVisible();
        LOG_DEBUG("MainWindow", QString("Saving States Map - visible: %1")
            .arg(geometry.statesMapVisible ? "true" : "false"));
    } else {
        geometry.statesMapVisible = false;
    }

    if (m_graylineMapDialog) {
        geometry.graylineMapVisible = m_graylineMapDialog->isVisible();
        geometry.graylineMapGeometry = m_graylineMapDialog->saveGeometry();
        LOG_DEBUG("MainWindow", QString("Saving Grayline Map - visible: %1")
            .arg(geometry.graylineMapVisible ? "true" : "false"));
    } else {
        geometry.graylineMapVisible = false;
    }

    settingsManager.saveWindowGeometry(geometry);
}
```

## Applying Font Settings

Replace `applyFontSettings()` with:

```cpp
void MainWindow::applyFontSettings() {
    SettingsManager settingsManager;
    FontConfig fonts = settingsManager.loadFontConfig();

    m_callsignEntry->setFont(fonts.entryFont);
    m_exchangeEntry->setFont(fonts.entryFont);
    m_qsoTableView->setFont(fonts.tableFont);
    m_timeLabel->setFont(fonts.miscFont);
    m_thisHrLabel->setFont(fonts.miscFont);
    m_rateLabel->setFont(fonts.miscFont);
    m_cqCountLabel->setFont(fonts.miscFont);
    m_spCountLabel->setFont(fonts.miscFont);
    m_operatorLabelStatic->setFont(fonts.miscFont);
    m_operatorLabel->setFont(fonts.miscFont);
    m_stationInfoLabel->setFont(fonts.miscFont);

    if (m_bandSummaryGrid) {
        m_bandSummaryGrid->setFontSize(fonts.gridFontSize);
    }

    m_scpMatchesLabel->setStyleSheet(
        QString("QLabel { color: #0066cc; font-size: %1pt; }").arg(fonts.scpFontSize)
    );
}
```

## Loading UDP Broadcast Settings

Replace `loadUdpBroadcastSettings()` with:

```cpp
void MainWindow::loadUdpBroadcastSettings() {
    SettingsManager settingsManager;
    UdpBroadcastConfig config = settingsManager.loadUdpBroadcastConfig();

    m_udpBroadcastManager->setEnabled(config.enabled);
    m_udpBroadcastManager->setRadioInfoEnabled(config.radioInfoEnabled);
    m_udpBroadcastManager->setContactInfoEnabled(config.contactInfoEnabled);
    m_udpBroadcastManager->setThrottleInterval(config.throttleInterval);
    m_udpBroadcastManager->setDestinations(config.destinations);

    LOG_DEBUG("MainWindow", QString("UDP Broadcast settings loaded: Enabled=%1 RadioInfo=%2 ContactInfo=%3 Destinations=%4")
        .arg(config.enabled ? "true" : "false")
        .arg(config.radioInfoEnabled ? "true" : "false")
        .arg(config.contactInfoEnabled ? "true" : "false")
        .arg(config.destinations.size()));
}
```

## Loading Backup Settings

Replace `loadBackupSettings()` with:

```cpp
void MainWindow::loadBackupSettings() {
    SettingsManager settingsManager;
    BackupConfig config = settingsManager.loadBackupConfig();

    BackupManager& backup = BackupManager::instance();
    backup.setAutoBackupEnabled(config.autoBackupEnabled);
    backup.setAutoBackupInterval(config.autoBackupInterval);
    backup.setBackupDirectory(config.backupDirectory);
    backup.setMaxBackups(config.maxBackups);

    LOG_DEBUG("MainWindow", QString("Backup settings loaded: Enabled=%1 Interval=%2 Directory=%3 MaxBackups=%4")
        .arg(config.autoBackupEnabled ? "true" : "false")
        .arg(config.autoBackupInterval)
        .arg(config.backupDirectory)
        .arg(config.maxBackups));
}
```

## Benefits

1. **Separation of Concerns**: Settings loading/saving logic is separated from UI widget manipulation
2. **Testability**: SettingsManager can be unit tested without MainWindow
3. **Reusability**: Other classes can use SettingsManager to load configuration
4. **Type Safety**: Config structs provide clear structure instead of individual settings calls
5. **Maintainability**: Changes to settings structure only affect SettingsManager, not MainWindow

## Notes

- SettingsManager uses AppSettings singleton internally for persistence
- MainWindow still handles window creation and geometry application
- Theme application (`applyTheme()`) remains in MainWindow (not settings-related)
- The manager follows the "load config, apply to widgets" pattern consistently
