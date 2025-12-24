#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>
#include "../radio/RadioInterface.h"

namespace TR4QT {

/**
 * Application settings wrapper using QSettings
 * Provides persistent storage for radio config, user preferences, etc.
 */
class AppSettings {
public:
    // Singleton access
    static AppSettings& instance();

    // Radio configuration
    void saveRadioConfig(const RadioConfig& config);
    RadioConfig loadRadioConfig() const;
    bool hasRadioConfig() const;

    // Radio auto-connect
    void setRadioAutoConnect(bool autoConnect);
    bool getRadioAutoConnect() const;

    // Station information
    void setMyCallsign(const QString& callsign);
    QString getMyCallsign() const;

    void setMyGridSquare(const QString& grid);
    QString getMyGridSquare() const;

    void setMyContinent(const QString& continent);
    QString getMyContinent() const;

    void setMyCQZone(int zone);
    int getMyCQZone() const;

    void setMyITUZone(int zone);
    int getMyITUZone() const;

    // Main window geometry
    void saveWindowGeometry(const QByteArray& geometry);
    QByteArray loadWindowGeometry() const;

    void saveWindowState(const QByteArray& state);
    QByteArray loadWindowState() const;

    // Child window geometry and visibility
    void saveDXClusterGeometry(const QByteArray& geometry);
    QByteArray loadDXClusterGeometry() const;
    void setDXClusterVisible(bool visible);
    bool getDXClusterVisible() const;

    void saveBandMapGeometry(const QByteArray& geometry);
    QByteArray loadBandMapGeometry() const;
    void setBandMapVisible(bool visible);
    bool getBandMapVisible() const;

    void saveRadioControlGeometry(const QByteArray& geometry);
    QByteArray loadRadioControlGeometry() const;
    void setRadioControlVisible(bool visible);
    bool getRadioControlVisible() const;

    void saveMultipliersGeometry(const QByteArray& geometry);
    QByteArray loadMultipliersGeometry() const;
    void setMultipliersVisible(bool visible);
    bool getMultipliersVisible() const;

    // DX Cluster settings
    void setDXClusterCallsign(const QString& callsign);
    QString getDXClusterCallsign() const;

    void setDXClusterServer(const QString& server);
    QString getDXClusterServer() const;

    void setDXClusterAutoConnect(bool autoConnect);
    bool getDXClusterAutoConnect() const;

    // Country file
    void setCountryFileVersion(int version);
    int getCountryFileVersion() const;

    void setCountryFilePath(const QString& path);
    QString getCountryFilePath() const;

    // Appearance settings
    void setEntryFontSize(int size);
    int getEntryFontSize() const;

    void setTableFontSize(int size);
    int getTableFontSize() const;

    void setGridFontSize(int size);
    int getGridFontSize() const;

private:
    AppSettings();
    ~AppSettings() = default;

    // Prevent copying
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    mutable QSettings m_settings;
};

} // namespace TR4QT

#endif // APPSETTINGS_H
