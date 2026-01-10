#include "GraylineMapDialog.h"
#include "../../utils/GeographicUtils.h"
#include "../../utils/ThemeManager.h"
#include "../../logging/LogMacros.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPen>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QResizeEvent>
#include <QShowEvent>
#include <QtMath>

namespace TR4QT {

GraylineMapDialog::GraylineMapDialog(QWidget* parent)
    : QDialog(parent)
    , m_view(nullptr)
    , m_scene(nullptr)
    , m_freezeButton(nullptr)
    , m_homeLat(0.0)
    , m_homeLon(0.0)
    , m_dxLat(0.0)
    , m_dxLon(0.0)
    , m_frozen(false)
    , m_resizing(false)
    , m_currentTime(QDateTime::currentDateTimeUtc())
{
    setupUi();
    setWindowTitle("Grayline Propagation Map");
    resize(UIDefaults::GRAYLINE_MAP_WIDTH, UIDefaults::GRAYLINE_MAP_HEIGHT);
}

GraylineMapDialog::~GraylineMapDialog() {
    // Cleanup handled by Qt parent-child relationship
}

void GraylineMapDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Graphics view
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, MAP_WIDTH, MAP_HEIGHT);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);

    // Make the view scale to fit the window (stretch to fill)
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mainLayout->addWidget(m_view);

    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_freezeButton = new QPushButton("Freeze Map", this);
    m_freezeButton->setCheckable(true);
    m_freezeButton->setChecked(false);
    connect(m_freezeButton, &QPushButton::toggled, this, &GraylineMapDialog::onFreezeToggled);
    buttonLayout->addWidget(m_freezeButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Initial map draw
    updateMap();

    // Ensure view is properly fitted to scene (keep aspect ratio to avoid distortion)
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void GraylineMapDialog::updateStations(const QString& homeCallsign, double homeLat, double homeLon,
                                       const QString& dxCallsign, double dxLat, double dxLon) {
    if (m_frozen) {
        return;  // Don't update if frozen
    }

    m_homeCallsign = homeCallsign;
    m_homeLat = homeLat;
    m_homeLon = homeLon;
    m_dxCallsign = dxCallsign;
    m_dxLat = dxLat;
    m_dxLon = dxLon;

    m_currentTime = QDateTime::currentDateTimeUtc();
    updateMap();
}

void GraylineMapDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);

    // Prevent recursive resize calls
    if (m_resizing) {
        return;
    }

    // Constrain window to 2:1 aspect ratio to match equirectangular projection
    const double TARGET_ASPECT_RATIO = 2.0;  // width:height for equirectangular map
    const int MIN_WIDTH = 800;   // Minimum window width
    const int MIN_HEIGHT = 400;  // Minimum window height (MIN_WIDTH / 2)

    QSize newSize = event->size();
    int width = newSize.width();
    int height = newSize.height();

    // Calculate what the height should be for this width
    int targetHeight = static_cast<int>(width / TARGET_ASPECT_RATIO);

    // If the actual height differs from target, constrain it
    if (qAbs(height - targetHeight) > 5) {  // 5 pixel tolerance to avoid resize loops
        // Determine whether user was resizing width or height by comparing with old size
        QSize oldSize = event->oldSize();
        bool widthChanged = (qAbs(width - oldSize.width()) > qAbs(height - oldSize.height()));

        if (widthChanged) {
            // Width changed more - adjust height to match
            height = qMax(MIN_HEIGHT, targetHeight);
        } else {
            // Height changed more - adjust width to match
            width = qMax(MIN_WIDTH, static_cast<int>(height * TARGET_ASPECT_RATIO));
            height = static_cast<int>(width / TARGET_ASPECT_RATIO);  // Recalculate for consistency
        }

        // Apply the constrained size (blocking recursive resize events)
        m_resizing = true;
        QRect geometry = this->geometry();
        geometry.setSize(QSize(width, height));
        setGeometry(geometry);
        m_resizing = false;
    }

    // Scale the view to fit window (now always fills since aspect ratio matches)
    if (m_view && m_scene) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void GraylineMapDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    // Ensure view is properly fitted when dialog is shown (keep aspect ratio)
    if (m_view && m_scene) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void GraylineMapDialog::setFrozen(bool frozen) {
    m_frozen = frozen;
    m_freezeButton->setChecked(frozen);
}

void GraylineMapDialog::onFreezeToggled(bool checked) {
    m_frozen = checked;
    m_freezeButton->setText(checked ? "Unfreeze Map" : "Freeze Map");
}

void GraylineMapDialog::updateMap() {
    m_scene->clear();

    drawWorldMap();
    drawMaidenheadGrid();
    drawGrayline();

    // Only draw station-specific items if we have valid station data
    if (!m_homeCallsign.isEmpty() && !m_dxCallsign.isEmpty()) {
        drawGreatCircle();
        drawBeamHeadings();
        drawStationMarkers();
        drawGraylineZones();
    }
}

void GraylineMapDialog::drawWorldMap() {
    // Load and display NASA Blue Marble world map image
    QPixmap mapImage(":/maps/nasabluemarble.jpg");

    if (!mapImage.isNull()) {
        LOG_DEBUG("GraylineMap", QString("NASA Blue Marble map loaded successfully (size: %1x%2)")
            .arg(mapImage.width()).arg(mapImage.height()));

        // Scale the map image to fit the scene dimensions
        QPixmap scaledMap = mapImage.scaled(MAP_WIDTH, MAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // Add the map as a background pixmap item
        QGraphicsPixmapItem* mapItem = m_scene->addPixmap(scaledMap);
        mapItem->setPos(0, 0);
        mapItem->setZValue(-1);  // Place map behind all other items
    } else {
        // Fallback to ocean background if image fails to load
        // NOTE: If this occurs, verify Qt imageformats plugins are deployed (qjpeg.dll, etc.)
        LOG_WARN("GraylineMap", "Failed to load NASA Blue Marble map from resources (:/maps/nasabluemarble.jpg), using ocean background fallback - check imageformats plugins");
        QBrush oceanBrush(COLOR_OCEAN);
        m_scene->setBackgroundBrush(oceanBrush);
    }
}

void GraylineMapDialog::drawMaidenheadGrid() {
    // Draw Maidenhead grid overlay
    QPen gridPen(COLOR_GRID, GRID_LINE_WIDTH);

    // Longitude lines (1 field = MAIDENHEAD_LONGITUDE_INTERVAL degrees)
    for (int lon = -LONGITUDE_OFFSET; lon <= LONGITUDE_OFFSET; lon += MAIDENHEAD_LONGITUDE_INTERVAL) {
        double x = (lon + LONGITUDE_OFFSET) / static_cast<double>(DEGREES_PER_ROTATION) * MAP_WIDTH;
        m_scene->addLine(x, 0, x, MAP_HEIGHT, gridPen);
    }

    // Latitude lines (1 field = MAIDENHEAD_LATITUDE_INTERVAL degrees)
    for (int lat = -LATITUDE_OFFSET; lat <= LATITUDE_OFFSET; lat += MAIDENHEAD_LATITUDE_INTERVAL) {
        double y = (LATITUDE_OFFSET - lat) / static_cast<double>(LONGITUDE_OFFSET * 2) * MAP_HEIGHT;
        m_scene->addLine(0, y, MAP_WIDTH, y, gridPen);
    }

    // TODO: Add grid square labels (AA, AB, etc.)
}

void GraylineMapDialog::drawGrayline() {
    // Calculate solar position and shade the night side with curved terminator
    QDate currentDate = m_currentTime.date();
    QTime currentTime = m_currentTime.time();

    // Calculate day of year for solar declination
    int dayOfYear = currentDate.dayOfYear();
    const int SUMMER_SOLSTICE = 172;  // Approx June 21 (longest day in Northern Hemisphere)
    const int DAYS_IN_YEAR = 365;
    const double EARTH_AXIAL_TILT = 23.5;  // degrees

    // Calculate solar declination (subsolar latitude)
    double yearProgress = static_cast<double>(dayOfYear - SUMMER_SOLSTICE) / DAYS_IN_YEAR;
    double solarDeclination = EARTH_AXIAL_TILT * qCos(2.0 * M_PI * yearProgress);

    // Calculate subsolar point longitude (Earth's rotation)
    // At 12:00 UTC, the sun is over 0° longitude (Greenwich)
    // The sun appears to move west at 15° per hour as Earth rotates east
    double currentHourUTC = currentTime.hour() +
                           currentTime.minute() / 60.0 +
                           currentTime.second() / 3600.0;
    const double DEGREES_PER_HOUR = 15.0;  // Earth rotates 15° per hour
    double subsolarLon = (12.0 - currentHourUTC) * DEGREES_PER_HOUR;

    // Calculate terminator curve
    // For each longitude, calculate the latitude where solar elevation = 0
    const int TERMINATOR_POINTS = 360;  // One point per degree of longitude
    QVector<QPointF> terminatorPath;

    for (int i = 0; i <= TERMINATOR_POINTS; i++) {
        double lon = -LONGITUDE_OFFSET + (i * DEGREES_PER_ROTATION / TERMINATOR_POINTS);

        // Calculate hour angle (difference from subsolar longitude)
        double hourAngle = lon - subsolarLon;

        // Normalize hour angle to -180 to 180
        while (hourAngle > LONGITUDE_OFFSET) hourAngle -= DEGREES_PER_ROTATION;
        while (hourAngle < -LONGITUDE_OFFSET) hourAngle += DEGREES_PER_ROTATION;

        // Calculate terminator latitude at this longitude using spherical geometry
        // cos(solarZenith) = sin(lat) * sin(declination) + cos(lat) * cos(declination) * cos(hourAngle)
        // At terminator, solarZenith = 90°, so cos(90°) = 0
        // Therefore: 0 = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(H)
        // Solving for lat: tan(lat) = -cos(H) / tan(dec)

        double decRad = qDegreesToRadians(solarDeclination);
        double hourAngleRad = qDegreesToRadians(hourAngle);

        // Handle special cases
        double terminatorLat = 0.0;
        if (qAbs(solarDeclination) < 0.01) {
            // Near equinox, terminator is approximately vertical (lon ± 90° from subsolar)
            terminatorLat = 0.0;
        } else if (qAbs(qCos(hourAngleRad)) < 0.01) {
            // Near subsolar/antisolar longitude (noon/midnight line)
            if (qAbs(hourAngle) < 90.0) {
                // Subsolar side - extends to pole in summer hemisphere
                terminatorLat = (solarDeclination > 0) ? 90.0 : -90.0;
            } else {
                // Antisolar side - extends to pole in winter hemisphere
                terminatorLat = (solarDeclination > 0) ? -90.0 : 90.0;
            }
        } else {
            // Normal case: calculate terminator latitude
            double tanLat = -qCos(hourAngleRad) / qTan(decRad);
            terminatorLat = qRadiansToDegrees(qAtan(tanLat));

            // Clamp to valid latitude range
            terminatorLat = qBound(-90.0, terminatorLat, 90.0);
        }

        terminatorPath.append(latLonToPoint(terminatorLat, lon));
    }

    // Create a polygon for the night side
    // Add top edge of map (North pole)
    QVector<QPointF> nightPolygon;
    nightPolygon.append(QPointF(0, 0));
    nightPolygon.append(QPointF(MAP_WIDTH, 0));

    // Add terminator curve
    for (const QPointF& point : terminatorPath) {
        nightPolygon.append(point);
    }

    // Close the polygon
    nightPolygon.append(QPointF(MAP_WIDTH, 0));

    // Determine which side is night based on subsolar position
    // If subsolar point is in northern hemisphere, shade below terminator
    // If in southern hemisphere, shade above terminator

    // For now, create path and fill
    QPainterPath nightPath;
    nightPath.moveTo(terminatorPath.first());
    for (int i = 1; i < terminatorPath.size(); i++) {
        nightPath.lineTo(terminatorPath[i]);
    }

    // Extend to map edges to create filled region
    // Determine if we should shade above or below the terminator
    QPointF lastPoint = terminatorPath.last();
    nightPath.lineTo(MAP_WIDTH, lastPoint.y());

    // Figure out which pole to shade toward based on solar declination
    double shadeToY = (solarDeclination > 0) ? MAP_HEIGHT : 0;
    nightPath.lineTo(MAP_WIDTH, shadeToY);
    nightPath.lineTo(0, shadeToY);

    QPointF firstPoint = terminatorPath.first();
    nightPath.lineTo(0, firstPoint.y());
    nightPath.closeSubpath();

    // Draw the night region
    QBrush nightBrush(COLOR_NIGHT);
    m_scene->addPath(nightPath, QPen(Qt::NoPen), nightBrush);
}

void GraylineMapDialog::drawGreatCircle() {
    // Draw great circle path between stations with proper interpolation
    QPen pathPen(COLOR_GREAT_CIRCLE, PATH_LINE_WIDTH, Qt::DashLine);

    // Calculate intermediate points along the great circle
    const int NUM_SEGMENTS = 100;  // Number of line segments for smooth curve
    QVector<QPointF> pathPoints;

    // Convert to radians for spherical geometry calculations
    double lat1 = qDegreesToRadians(m_homeLat);
    double lon1 = qDegreesToRadians(m_homeLon);
    double lat2 = qDegreesToRadians(m_dxLat);
    double lon2 = qDegreesToRadians(m_dxLon);

    // Calculate angular distance between points
    double dLon = lon2 - lon1;
    double cosLat1 = qCos(lat1);
    double cosLat2 = qCos(lat2);
    double sinLat1 = qSin(lat1);
    double sinLat2 = qSin(lat2);

    double d = qAcos(qBound(-1.0, sinLat1 * sinLat2 + cosLat1 * cosLat2 * qCos(dLon), 1.0));

    // Handle special case: same point or antipodal points
    if (qAbs(d) < 0.0001) {
        // Points are the same, just draw a marker
        QPointF homePoint = latLonToPoint(m_homeLat, m_homeLon);
        pathPoints.append(homePoint);
    } else {
        // Interpolate points along the great circle
        for (int i = 0; i <= NUM_SEGMENTS; i++) {
            double f = static_cast<double>(i) / NUM_SEGMENTS;

            // Spherical interpolation formula (SLERP for great circles)
            double a = qSin((1.0 - f) * d) / qSin(d);
            double b = qSin(f * d) / qSin(d);

            double x = a * cosLat1 * qCos(lon1) + b * cosLat2 * qCos(lon2);
            double y = a * cosLat1 * qSin(lon1) + b * cosLat2 * qSin(lon2);
            double z = a * sinLat1 + b * sinLat2;

            // Convert back to lat/lon
            double lat = qAtan2(z, qSqrt(x * x + y * y));
            double lon = qAtan2(y, x);

            double latDeg = qRadiansToDegrees(lat);
            double lonDeg = qRadiansToDegrees(lon);

            pathPoints.append(latLonToPoint(latDeg, lonDeg));
        }
    }

    // Draw the path as connected line segments
    for (int i = 0; i < pathPoints.size() - 1; i++) {
        QPointF p1 = pathPoints[i];
        QPointF p2 = pathPoints[i + 1];

        // Handle wraparound at map edges (±180° longitude)
        double dx = p2.x() - p1.x();
        if (qAbs(dx) > MAP_WIDTH / 2) {
            // Path crosses the edge - don't draw this segment
            // (proper solution would split and draw on both sides)
            continue;
        }

        m_scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), pathPen);
    }
}

void GraylineMapDialog::drawStationMarkers() {
    // Draw markers for home and DX stations
    QPointF homePoint = latLonToPoint(m_homeLat, m_homeLon);
    QPointF dxPoint = latLonToPoint(m_dxLat, m_dxLon);

    const int MARKER_DIAMETER = STATION_MARKER_RADIUS * 2;

    // Home station marker
    QPen homePen(COLOR_HOME_STATION, STATION_MARKER_PEN_WIDTH);
    QBrush homeBrush(COLOR_HOME_STATION);
    m_scene->addEllipse(homePoint.x() - STATION_MARKER_RADIUS,
                       homePoint.y() - STATION_MARKER_RADIUS,
                       MARKER_DIAMETER, MARKER_DIAMETER, homePen, homeBrush);

    QGraphicsTextItem* homeLabel = m_scene->addText(m_homeCallsign);
    homeLabel->setPos(homePoint.x() + STATION_LABEL_OFFSET_X,
                     homePoint.y() + STATION_LABEL_OFFSET_Y);
    homeLabel->setDefaultTextColor(COLOR_HOME_STATION);

    // DX station marker
    QPen dxPen(COLOR_DX_STATION, STATION_MARKER_PEN_WIDTH);
    QBrush dxBrush(COLOR_DX_STATION);
    m_scene->addEllipse(dxPoint.x() - STATION_MARKER_RADIUS,
                       dxPoint.y() - STATION_MARKER_RADIUS,
                       MARKER_DIAMETER, MARKER_DIAMETER, dxPen, dxBrush);

    QGraphicsTextItem* dxLabel = m_scene->addText(m_dxCallsign);
    dxLabel->setPos(dxPoint.x() + STATION_LABEL_OFFSET_X,
                   dxPoint.y() + STATION_LABEL_OFFSET_Y);
    dxLabel->setDefaultTextColor(COLOR_DX_STATION);
}

void GraylineMapDialog::drawBeamHeadings() {
    // Draw beam heading lines from each station
    QPen beamPen(COLOR_BEAM_HEADING, BEAM_HEADING_LINE_WIDTH);

    QPointF homePoint = latLonToPoint(m_homeLat, m_homeLon);
    QPointF dxPoint = latLonToPoint(m_dxLat, m_dxLon);

    // Calculate beam heading from home to DX
    double homeBearing = GeographicUtils::calculateBearing(m_homeLat, m_homeLon, m_dxLat, m_dxLon);
    double dxBearing = GeographicUtils::calculateBearing(m_dxLat, m_dxLon, m_homeLat, m_homeLon);

    // Draw beam heading lines showing antenna pointing direction
    const int BEAM_LINE_LENGTH = 150;  // pixels

    // Convert bearing to direction vector
    // Bearing: 0° = North, 90° = East, 180° = South, 270° = West
    // On map: x increases east, y increases south
    double homeBearingRad = qDegreesToRadians(homeBearing);
    double dxBearingRad = qDegreesToRadians(dxBearing);

    // Calculate line endpoints
    double homeDx = qSin(homeBearingRad) * BEAM_LINE_LENGTH;
    double homeDy = -qCos(homeBearingRad) * BEAM_LINE_LENGTH;  // Negative because y increases downward
    QPointF homeEnd(homePoint.x() + homeDx, homePoint.y() + homeDy);

    double dxDx = qSin(dxBearingRad) * BEAM_LINE_LENGTH;
    double dxDy = -qCos(dxBearingRad) * BEAM_LINE_LENGTH;
    QPointF dxEnd(dxPoint.x() + dxDx, dxPoint.y() + dxDy);

    // Draw the beam heading lines
    m_scene->addLine(homePoint.x(), homePoint.y(), homeEnd.x(), homeEnd.y(), beamPen);
    m_scene->addLine(dxPoint.x(), dxPoint.y(), dxEnd.x(), dxEnd.y(), beamPen);
}

void GraylineMapDialog::drawGraylineZones() {
    // Draw grayline propagation zones (±30 minutes around terminator)
    // TODO: Implement shaded zones around grayline
    // For now, this is a placeholder
}

QPointF GraylineMapDialog::latLonToPoint(double lat, double lon) const {
    // Equirectangular projection (simple but distorted near poles)
    double x = (lon + LONGITUDE_OFFSET) / DEGREES_PER_ROTATION * MAP_WIDTH;
    double y = (LATITUDE_OFFSET - lat) / (LATITUDE_OFFSET * 2) * MAP_HEIGHT;
    return QPointF(x, y);
}

} // namespace TR4QT
