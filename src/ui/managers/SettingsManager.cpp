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

#include "SettingsManager.h"
#include "../../utils/AppSettings.h"
#include "../../network/UdpBroadcaster.h"
#include "../../logging/LogMacros.h"
#include "../../core/Constants.h"
#include "../../utils/FontManager.h"
#include <QSettings>

namespace TR4QT {

FontConfig SettingsManager::loadFontConfig() const {
    AppSettings& settings = AppSettings::instance();

    FontConfig config;

    // Entry field font (callsign, exchange)
    int entryFontSize = settings.getEntryFontSize();
    config.entryFont = FontManager::instance().monospaceFont(entryFontSize);

    // QSO table font
    int tableFontSize = settings.getTableFontSize();
    config.tableFont = FontManager::instance().monospaceFont(tableFontSize);

    // Misc display font (stats panel: This Hr, Rate, Op, etc.)
    int miscFontSize = settings.getMiscDisplayFontSize();
    config.miscFont = FontManager::instance().monospaceFont(miscFontSize);

    // Band summary grid font size
    config.gridFontSize = settings.getGridFontSize();

    // SCP matches font size
    config.scpFontSize = settings.getSCPFontSize();

    return config;
}

WindowGeometry SettingsManager::loadWindowGeometry() const {
    AppSettings& settings = AppSettings::instance();
    QSettings qsettings(APP_ORG, APP_NAME);  // Must match AppSettings initialization

    WindowGeometry geometry;

    // Main window
    geometry.mainWindowGeometry = settings.loadWindowGeometry();
    geometry.mainWindowState = settings.loadWindowState();

    // DX Cluster window
    geometry.dxClusterVisible = settings.getDXClusterVisible();
    geometry.dxClusterGeometry = settings.loadDXClusterGeometry();

    // Band Map window
    geometry.bandMapVisible = settings.getBandMapVisible();
    geometry.bandMapGeometry = settings.loadBandMapGeometry();

    // Radio Control window
    geometry.radioControlVisible = settings.getRadioControlVisible();
    geometry.radioControlGeometry = settings.loadRadioControlGeometry();

    // Radio 2 Control window (SO2R)
    geometry.radio2ControlVisible = settings.getRadio2ControlVisible();
    geometry.radio2ControlGeometry = settings.loadRadio2ControlGeometry();

    // Multipliers window
    geometry.multipliersVisible = settings.getMultipliersVisible();
    geometry.multipliersGeometry = settings.loadMultipliersGeometry();

    // Statistics window
    geometry.statisticsVisible = qsettings.value("Windows/Statistics/Visible", false).toBool();
    geometry.statisticsGeometry = qsettings.value("Windows/Statistics/Geometry").toByteArray();

    // Map viewers (visibility only, geometry saved by viewers)
    geometry.sectionsMapVisible = qsettings.value("MapViewer/Sections/Visible", false).toBool();
    geometry.statesMapVisible = qsettings.value("MapViewer/States/Visible", false).toBool();
    geometry.worldMapVisible = qsettings.value("MapViewer/DXCC/Visible", false).toBool();

    // Grayline Map
    geometry.graylineMapVisible = settings.getGraylineMapVisible();
    geometry.graylineMapGeometry = settings.loadGraylineMapGeometry();

    // Amplifier Control window
    geometry.amplifierControlVisible = settings.getAmplifierControlVisible();
    geometry.amplifierControlGeometry = settings.loadAmplifierControlGeometry();
    LOG_DEBUG("SettingsManager", QString("Loaded amplifier control visibility: %1").arg(geometry.amplifierControlVisible));

    // Current operator
    geometry.currentOperator = settings.getCurrentOperator();

    return geometry;
}

void SettingsManager::saveWindowGeometry(const WindowGeometry& geometry) {
    AppSettings& settings = AppSettings::instance();
    QSettings qsettings(APP_ORG, APP_NAME);  // Must match AppSettings initialization

    // Main window
    settings.saveWindowGeometry(geometry.mainWindowGeometry);
    settings.saveWindowState(geometry.mainWindowState);

    // DX Cluster window
    settings.saveDXClusterGeometry(geometry.dxClusterGeometry);
    settings.setDXClusterVisible(geometry.dxClusterVisible);

    // Band Map window
    settings.saveBandMapGeometry(geometry.bandMapGeometry);
    settings.setBandMapVisible(geometry.bandMapVisible);

    // Radio Control window
    settings.saveRadioControlGeometry(geometry.radioControlGeometry);
    settings.setRadioControlVisible(geometry.radioControlVisible);

    // Radio 2 Control window (SO2R)
    settings.saveRadio2ControlGeometry(geometry.radio2ControlGeometry);
    settings.setRadio2ControlVisible(geometry.radio2ControlVisible);

    // Multipliers window
    settings.saveMultipliersGeometry(geometry.multipliersGeometry);
    settings.setMultipliersVisible(geometry.multipliersVisible);

    // Statistics window
    qsettings.setValue("Windows/Statistics/Geometry", geometry.statisticsGeometry);
    qsettings.setValue("Windows/Statistics/Visible", geometry.statisticsVisible);

    // Map viewers (visibility only, geometry saved by viewers)
    qsettings.setValue("MapViewer/Sections/Visible", geometry.sectionsMapVisible);
    qsettings.setValue("MapViewer/States/Visible", geometry.statesMapVisible);
    qsettings.setValue("MapViewer/DXCC/Visible", geometry.worldMapVisible);

    // Grayline Map
    settings.saveGraylineMapGeometry(geometry.graylineMapGeometry);
    settings.setGraylineMapVisible(geometry.graylineMapVisible);

    // Amplifier Control window
    LOG_DEBUG("SettingsManager", QString("Saving amplifier control visibility: %1").arg(geometry.amplifierControlVisible));
    settings.saveAmplifierControlGeometry(geometry.amplifierControlGeometry);
    settings.setAmplifierControlVisible(geometry.amplifierControlVisible);

    // Ensure local qsettings are written to disk (important on Windows)
    qsettings.sync();
}

UdpBroadcastConfig SettingsManager::loadUdpBroadcastConfig() const {
    AppSettings& settings = AppSettings::instance();

    UdpBroadcastConfig config;
    config.enabled = settings.getUDPBroadcastEnabled();
    config.radioInfoEnabled = settings.getUDPRadioInfoEnabled();
    config.contactInfoEnabled = settings.getUDPContactInfoEnabled();
    config.throttleInterval = settings.getUDPThrottleInterval();
    config.destinations = settings.getUDPDestinations();

    return config;
}

BackupConfig SettingsManager::loadBackupConfig() const {
    AppSettings& settings = AppSettings::instance();

    BackupConfig config;
    config.autoBackupEnabled = settings.getAutoBackupEnabled();
    config.autoBackupInterval = settings.getAutoBackupInterval();
    config.backupDirectory = settings.getBackupDirectory();
    config.maxBackups = settings.getMaxBackups();

    return config;
}

QList<int> SettingsManager::loadQSOTableColumnWidths(const QString& contestId) const {
    AppSettings& settings = AppSettings::instance();
    return settings.loadQSOTableColumnWidths(contestId);
}

void SettingsManager::saveQSOTableColumnWidths(const QString& contestId, const QList<int>& widths) {
    AppSettings& settings = AppSettings::instance();
    settings.saveQSOTableColumnWidths(contestId, widths);
}

} // namespace TR4QT
