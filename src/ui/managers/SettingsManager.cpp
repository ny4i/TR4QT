#include "SettingsManager.h"
#include "../../utils/AppSettings.h"
#include "../../network/UdpBroadcaster.h"
#include "../../logging/LogMacros.h"
#include <QSettings>

namespace TR4QT {

FontConfig SettingsManager::loadFontConfig() const {
    AppSettings& settings = AppSettings::instance();

    FontConfig config;

    // Entry field font (callsign, exchange)
    int entryFontSize = settings.getEntryFontSize();
    config.entryFont = QFont("Monospace", entryFontSize);

    // QSO table font
    int tableFontSize = settings.getTableFontSize();
    config.tableFont = QFont("Monospace", tableFontSize);

    // Misc display font (stats panel: This Hr, Rate, Op, etc.)
    int miscFontSize = settings.getMiscDisplayFontSize();
    config.miscFont = QFont("Monospace", miscFontSize);

    // Band summary grid font size
    config.gridFontSize = settings.getGridFontSize();

    // SCP matches font size
    config.scpFontSize = settings.getSCPFontSize();

    return config;
}

WindowGeometry SettingsManager::loadWindowGeometry() const {
    AppSettings& settings = AppSettings::instance();
    QSettings qsettings("TR4QT", "TR4QT");

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

    // Multipliers window
    geometry.multipliersVisible = settings.getMultipliersVisible();
    geometry.multipliersGeometry = settings.loadMultipliersGeometry();

    // Statistics window
    geometry.statisticsVisible = qsettings.value("Windows/Statistics/Visible", false).toBool();
    geometry.statisticsGeometry = qsettings.value("Windows/Statistics/Geometry").toByteArray();

    // Map viewers (visibility only, geometry saved by viewers)
    geometry.sectionsMapVisible = qsettings.value("MapViewer/Sections/Visible", false).toBool();
    geometry.statesMapVisible = qsettings.value("MapViewer/States/Visible", false).toBool();

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
    QSettings qsettings("TR4QT", "TR4QT");

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

    // Multipliers window
    settings.saveMultipliersGeometry(geometry.multipliersGeometry);
    settings.setMultipliersVisible(geometry.multipliersVisible);

    // Statistics window
    qsettings.setValue("Windows/Statistics/Geometry", geometry.statisticsGeometry);
    qsettings.setValue("Windows/Statistics/Visible", geometry.statisticsVisible);

    // Map viewers (visibility only, geometry saved by viewers)
    qsettings.setValue("MapViewer/Sections/Visible", geometry.sectionsMapVisible);
    qsettings.setValue("MapViewer/States/Visible", geometry.statesMapVisible);

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
