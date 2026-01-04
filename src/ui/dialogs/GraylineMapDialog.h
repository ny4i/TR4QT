#ifndef GRAYLINEMAPDDIALOG_H
#define GRAYLINEMAPDDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include <QDateTime>

namespace TR4QT {

/**
 * Grayline Map Dialog
 *
 * Displays a world map showing:
 * - Day/night terminator (grayline)
 * - Great circle path between home and DX stations
 * - Station markers with labels
 * - Beam heading circles
 * - Maidenhead grid overlay
 * - Grayline propagation zones (±30 min)
 */
class GraylineMapDialog : public QDialog {
    Q_OBJECT

public:
    explicit GraylineMapDialog(QWidget* parent = nullptr);
    ~GraylineMapDialog() override;

    /**
     * Update map with new station information
     * @param homeCallsign Home station callsign
     * @param homeLat Home station latitude
     * @param homeLon Home station longitude
     * @param dxCallsign DX station callsign
     * @param dxLat DX station latitude
     * @param dxLon DX station longitude
     */
    void updateStations(const QString& homeCallsign, double homeLat, double homeLon,
                       const QString& dxCallsign, double dxLat, double dxLon);

    /**
     * Set whether map auto-updates or is frozen
     * @param frozen If true, map won't update when updateStations() is called
     */
    void setFrozen(bool frozen);

    /**
     * Check if map is frozen
     */
    bool isFrozen() const { return m_frozen; }

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onFreezeToggled(bool checked);
    void updateMap();

private:
    void setupUi();
    void drawWorldMap();
    void drawMaidenheadGrid();
    void drawGrayline();
    void drawGreatCircle();
    void drawStationMarkers();
    void drawBeamHeadings();
    void drawGraylineZones();

    // Convert lat/lon to map coordinates
    QPointF latLonToPoint(double lat, double lon) const;

    // Graphics
    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QPushButton* m_freezeButton;

    // Map dimensions (equirectangular projection)
    const int MAP_WIDTH = 1000;
    const int MAP_HEIGHT = 500;

    // Grid parameters
    const int MAIDENHEAD_LONGITUDE_INTERVAL = 20;  // degrees (1 field)
    const int MAIDENHEAD_LATITUDE_INTERVAL = 10;   // degrees (1 field)
    const int GRID_LINE_WIDTH = 1;                 // pixels

    // Station marker parameters
    const int STATION_MARKER_RADIUS = 5;           // pixels
    const int STATION_MARKER_PEN_WIDTH = 2;        // pixels
    const int STATION_LABEL_OFFSET_X = 8;          // pixels
    const int STATION_LABEL_OFFSET_Y = -8;         // pixels

    // Drawing parameters
    const int TERMINATOR_LINE_WIDTH = 2;           // pixels
    const int PATH_LINE_WIDTH = 2;                 // pixels
    const int BEAM_HEADING_LINE_WIDTH = 2;         // pixels
    const int BEAM_HEADING_RADIUS = 100;           // pixels (placeholder for beam pattern)

    // Time calculation constants
    const int SECONDS_PER_HOUR = 3600;
    const int SECONDS_PER_MINUTE = 60;
    const int SECONDS_PER_DAY = 86400;
    const int DEGREES_PER_ROTATION = 360;
    const int LONGITUDE_OFFSET = 180;              // Center longitude at 0°
    const int LATITUDE_OFFSET = 90;                // Center latitude at 0°

    // Map colors (visualization, not theme-dependent)
    const QColor COLOR_OCEAN = QColor(160, 192, 224);       // Theme default: #A0C0E0 (light blue ocean)
    const QColor COLOR_LAND = QColor(208, 224, 208);        // Theme default: #D0E0D0 (light green land)
    const QColor COLOR_GRID = QColor(80, 80, 80, 180);      // Theme default: #505050B4 (dark gray grid)
    const QColor COLOR_TERMINATOR = QColor(200, 100, 0, 150);  // Theme default: #C86400 (orange terminator, unused)
    const QColor COLOR_NIGHT = QColor(0, 0, 0, 120);        // Theme default: #00000078 (night overlay)
    const QColor COLOR_GREAT_CIRCLE = QColor(255, 140, 0);  // Theme default: #FF8C00 (orange path)
    const QColor COLOR_BEAM_HEADING = QColor(255, 0, 255);  // Theme default: #FF00FF (magenta beam)
    const QColor COLOR_HOME_STATION = Qt::red;              // Theme default: #FF0000 (red home marker)
    const QColor COLOR_DX_STATION = Qt::blue;               // Theme default: #0000FF (blue DX marker)

    // Station data
    QString m_homeCallsign;
    double m_homeLat;
    double m_homeLon;
    QString m_dxCallsign;
    double m_dxLat;
    double m_dxLon;

    // State
    bool m_frozen;
    bool m_resizing;  // Flag to prevent recursive resize events
    QDateTime m_currentTime;
};

} // namespace TR4QT

#endif // GRAYLINEMAPDDIALOG_H
