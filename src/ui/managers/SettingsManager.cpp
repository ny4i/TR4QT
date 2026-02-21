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

    WindowGeometry geometry;

    // Main window only — child windows self-persist via PersistentWindow
    geometry.mainWindowGeometry = settings.loadWindowGeometry();
    geometry.mainWindowState = settings.loadWindowState();
    geometry.currentOperator = settings.getCurrentOperator();

    return geometry;
}

void SettingsManager::saveWindowGeometry(const WindowGeometry& geometry) {
    AppSettings& settings = AppSettings::instance();

    // Main window only — child windows self-persist via PersistentWindow
    settings.saveWindowGeometry(geometry.mainWindowGeometry);
    settings.saveWindowState(geometry.mainWindowState);
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
