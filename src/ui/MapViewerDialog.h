#ifndef MAPVIEWERDIALOG_H
#define MAPVIEWERDIALOG_H

#include <QDialog>
#include <QString>

// Forward declarations
class QWebEngineView;
class QWebChannel;
class MapViewerBridge;

namespace TR4QT {
    class QSOTableModel;
}

/**
 * @brief Self-contained dialog for viewing geographic maps in-app
 *
 * This dialog embeds interactive maps (sections, states, DXCC) using
 * QWebEngineView and passes QSO data via QWebChannel.
 *
 * Design goals:
 * - No web server dependency (works offline via qrc:// resources)
 * - Self-contained (owns all map logic, doesn't pollute MainWindow)
 * - Reusable (same dialog for sections, states, DXCC maps)
 *
 * Usage:
 *   auto* dialog = new MapViewerDialog(MapType::Sections, qsoModel, this);
 *   dialog->show();
 */
class MapViewerDialog : public QDialog {
    Q_OBJECT

public:
    enum MapType {
        Sections,  // ARRL Sections
        States,    // US States (WAS)
        DXCC       // DXCC Entities (future)
    };

    /**
     * @brief Create a map viewer dialog
     * @param type Map type to display
     * @param qsoModel QSO table model (data source)
     * @param parent Parent widget
     */
    explicit MapViewerDialog(MapType type, TR4QT::QSOTableModel* qsoModel, QWidget* parent = nullptr);
    ~MapViewerDialog();

private:
    void setupUi();
    void loadMap();

    MapType m_mapType;
    TR4QT::QSOTableModel* m_qsoModel;
    QWebEngineView* m_webView;
    QWebChannel* m_channel;
    MapViewerBridge* m_bridge;
};

/**
 * @brief QWebChannel bridge object exposed to JavaScript
 *
 * This object is exposed via QWebChannel as "bridge" and provides:
 * - Properties: callsign, operatorName, contestName
 * - Methods: requestSectionsData(), requestStatesData()
 * - Signals: sectionsDataChanged(), statesDataChanged()
 *
 * JavaScript accesses this via:
 *   bridge.callsign
 *   bridge.requestSectionsData()
 *   bridge.sectionsDataChanged.connect(callback)
 */
class MapViewerBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString callsign READ callsign CONSTANT)
    Q_PROPERTY(QString operatorName READ operatorName CONSTANT)
    Q_PROPERTY(QString contestName READ contestName CONSTANT)

public:
    explicit MapViewerBridge(TR4QT::QSOTableModel* qsoModel, MapViewerDialog::MapType mapType, QObject* parent = nullptr);

    QString callsign() const { return m_callsign; }
    QString operatorName() const { return m_operatorName; }
    QString contestName() const { return m_contestName; }

public slots:
    /**
     * @brief JavaScript calls this to request sections data
     * Emits sectionsDataChanged() with JSON data
     */
    void requestSectionsData();

    /**
     * @brief JavaScript calls this to request states data
     * Emits statesDataChanged() with JSON data
     */
    void requestStatesData();

    /**
     * @brief Get ARRL sections GeoJSON content
     * @return GeoJSON string for sections map
     */
    QString getSectionsGeoJSON();

    /**
     * @brief Get US states GeoJSON content
     * @return GeoJSON string for states map
     */
    QString getStatesGeoJSON();

private slots:
    /**
     * @brief Auto-refresh when QSO model changes
     */
    void onModelDataChanged();

signals:
    /**
     * @brief Emitted when sections data is ready
     * @param jsonData JSON string with sections data
     */
    void sectionsDataChanged(const QString& jsonData);

    /**
     * @brief Emitted when states data is ready
     * @param jsonData JSON string with states data
     */
    void statesDataChanged(const QString& jsonData);

private:
    TR4QT::QSOTableModel* m_qsoModel;
    MapViewerDialog::MapType m_mapType;
    QString m_callsign;
    QString m_operatorName;
    QString m_contestName;
};

#endif // MAPVIEWERDIALOG_H
