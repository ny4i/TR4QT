#include "NativeMapViewer.h"
#include "../ui/models/QSOTableModel.h"
#include "../utils/MapDataProvider.h"
#include "../utils/ThemeManager.h"
#include "../logging/LogMacros.h"
#include "../core/Constants.h"
#include <QVBoxLayout>

using namespace TR4QT;
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QBrush>
#include <QPen>
#include <QPalette>
#include <QShowEvent>
#include <QHideEvent>
#include <QWheelEvent>
#include <QSettings>
#include <QtMath>

using TR4QT::QSOTableModel;

NativeMapViewer::NativeMapViewer(MapType type, QSOTableModel* qsoModel, QWidget* parent)
    : QDialog(parent)
    , m_mapType(type)
    , m_qsoModel(qsoModel)
    , m_view(nullptr)
    , m_scene(nullptr)
    , m_workedLabel(nullptr)
    , m_totalLabel(nullptr)
    , m_completionLabel(nullptr)
    , m_qsoCountLabel(nullptr)
    , m_workedListWidget(nullptr)
{
    setupUI();
    loadGeoJSON();
    createPolygons();
    refreshData();

    // Connect to model changes for auto-refresh
    if (m_qsoModel) {
        connect(m_qsoModel, &QAbstractItemModel::rowsInserted,
                this, &NativeMapViewer::onModelDataChanged);
        connect(m_qsoModel, &QAbstractItemModel::dataChanged,
                this, &NativeMapViewer::onModelDataChanged);
    }
}

void NativeMapViewer::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Restore window geometry (position and size)
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization
    QString geometryKey = QString("MapViewer/%1/Geometry").arg(m_mapType == Sections ? "Sections" :
                                                                 m_mapType == States ? "States" : "DXCC");
    if (settings.contains(geometryKey)) {
        restoreGeometry(settings.value(geometryKey).toByteArray());
        LOG_DEBUG("NativeMapViewer", QString("Restored window geometry for %1").arg(geometryKey));
    }

    // Restore view state every time the window is shown
    // This ensures zoom/pan persist across hide/show cycles
    restoreViewState();
}

void NativeMapViewer::hideEvent(QHideEvent* event) {
    // Save window geometry (position and size)
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization
    QString geometryKey = QString("MapViewer/%1/Geometry").arg(m_mapType == Sections ? "Sections" :
                                                                 m_mapType == States ? "States" : "DXCC");
    settings.setValue(geometryKey, saveGeometry());
    LOG_DEBUG("NativeMapViewer", QString("Saved window geometry for %1").arg(geometryKey));

    // Save view state when hiding
    saveViewState();
    QDialog::hideEvent(event);
}

NativeMapViewer::~NativeMapViewer() {
    // Qt parent-child relationship handles cleanup
}

void NativeMapViewer::setupUI() {
    // Set window properties
    resize(UIDefaults::NATIVE_MAP_WIDTH, UIDefaults::NATIVE_MAP_HEIGHT);
    setMinimumSize(UIDefaults::NATIVE_MAP_MIN_WIDTH, UIDefaults::NATIVE_MAP_MIN_HEIGHT);  // Allow shrinking
    setSizeGripEnabled(true);   // Enable resize grip

    QString title;
    switch (m_mapType) {
        case Sections:
            title = "ARRL Sections Map";
            break;
        case States:
            title = "US States Map (WAS)";
            break;
        case DXCC:
            title = "DXCC Entities Map";
            break;
    }
    setWindowTitle(title);

    // Main layout (horizontal: map + sidebar)
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Graphics view and scene
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(ThemeManager::instance().color(ColorRole::MapBackground));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // Allow resizing in both directions
    m_view->setFrameShape(QFrame::NoFrame);  // Remove frame border
    // Ensure consistent background color for areas outside scene rect
    m_view->setBackgroundBrush(ThemeManager::instance().color(ColorRole::MapBackground));
    m_view->viewport()->setAutoFillBackground(true);
    QPalette pal = m_view->viewport()->palette();
    pal.setColor(QPalette::Window, ThemeManager::instance().color(ColorRole::MapBackground));
    m_view->viewport()->setPalette(pal);
    mainLayout->addWidget(m_view, 1);  // Stretch factor 1

    // Sidebar
    QWidget* sidebar = new QWidget(this);
    sidebar->setFixedWidth(300);
    sidebar->setStyleSheet("QWidget { background-color: #2C3E50; color: white; }");
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebar);

    // Statistics group
    QGroupBox* statsGroup = new QGroupBox("Statistics", sidebar);
    statsGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFD700; }");
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);

    QHBoxLayout* workedRow = new QHBoxLayout();
    workedRow->addWidget(new QLabel("Worked:"));
    m_workedLabel = new QLabel("0");
    m_workedLabel->setStyleSheet("font-weight: bold;");
    workedRow->addStretch();
    workedRow->addWidget(m_workedLabel);
    statsLayout->addLayout(workedRow);

    QHBoxLayout* totalRow = new QHBoxLayout();
    totalRow->addWidget(new QLabel("Total:"));
    QString totalText;
    if (m_mapType == Sections) {
        totalText = "83";
    } else if (m_mapType == States) {
        totalText = "50";
    } else {
        totalText = "---";  // Will be updated after polygons are loaded
    }
    m_totalLabel = new QLabel(totalText);
    totalRow->addStretch();
    totalRow->addWidget(m_totalLabel);
    statsLayout->addLayout(totalRow);

    QHBoxLayout* completionRow = new QHBoxLayout();
    completionRow->addWidget(new QLabel("Completion:"));
    m_completionLabel = new QLabel("0%");
    m_completionLabel->setStyleSheet("font-weight: bold; color: #2ECC71;");
    completionRow->addStretch();
    completionRow->addWidget(m_completionLabel);
    statsLayout->addLayout(completionRow);

    QHBoxLayout* qsoRow = new QHBoxLayout();
    qsoRow->addWidget(new QLabel("Total QSOs:"));
    m_qsoCountLabel = new QLabel("0");
    qsoRow->addStretch();
    qsoRow->addWidget(m_qsoCountLabel);
    statsLayout->addLayout(qsoRow);

    sidebarLayout->addWidget(statsGroup);

    // Legend group
    QGroupBox* legendGroup = new QGroupBox("QSO Count Legend", sidebar);
    legendGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFD700; }");
    QVBoxLayout* legendLayout = new QVBoxLayout(legendGroup);

    struct LegendItem { ColorRole role; QString label; };
    QVector<LegendItem> legendItems = {
        {ColorRole::MapNotWorked, "0 (Not Worked)"},
        {ColorRole::MapFirstContact, "1 QSO"},
        {ColorRole::MapSecondContact, "2 QSOs"},
        {ColorRole::MapFew, "3-9 QSOs"},
        {ColorRole::MapSome, "10-19 QSOs"},
        {ColorRole::MapMany, "20-49 QSOs"},
        {ColorRole::MapManyMore, "50-99 QSOs"},
        {ColorRole::MapHundreds, "100-199 QSOs"},
        {ColorRole::MapHundredsMore, "200-499 QSOs"},
        {ColorRole::MapThousands, "500+ QSOs"}
    };

    for (const auto& item : legendItems) {
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* colorBox = new QLabel();
        colorBox->setFixedSize(30, 20);
        QString colorName = ThemeManager::instance().colorName(item.role);
        colorBox->setStyleSheet(QString("background-color: %1; border: 1px solid white;").arg(colorName));
        row->addWidget(colorBox);
        row->addWidget(new QLabel(item.label));
        row->addStretch();
        legendLayout->addLayout(row);
    }

    sidebarLayout->addWidget(legendGroup);

    // Controls group
    QGroupBox* controlsGroup = new QGroupBox("Controls", sidebar);
    controlsGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFD700; }");
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);

    QPushButton* zoomInBtn = new QPushButton("Zoom In", sidebar);
    QPushButton* zoomOutBtn = new QPushButton("Zoom Out", sidebar);
    QPushButton* resetViewBtn = new QPushButton("Reset View", sidebar);
    QPushButton* refreshBtn = new QPushButton("Refresh Data", sidebar);

    QString btnStyle = "QPushButton { background-color: #3498DB; color: white; padding: 8px; "
                       "border: none; border-radius: 4px; } "
                       "QPushButton:hover { background-color: #2980B9; }";
    zoomInBtn->setStyleSheet(btnStyle);
    zoomOutBtn->setStyleSheet(btnStyle);
    resetViewBtn->setStyleSheet(btnStyle);
    refreshBtn->setStyleSheet(btnStyle);

    connect(zoomInBtn, &QPushButton::clicked, this, &NativeMapViewer::onZoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &NativeMapViewer::onZoomOut);
    connect(resetViewBtn, &QPushButton::clicked, this, &NativeMapViewer::onResetView);
    connect(refreshBtn, &QPushButton::clicked, this, &NativeMapViewer::refreshData);

    controlsLayout->addWidget(zoomInBtn);
    controlsLayout->addWidget(zoomOutBtn);
    controlsLayout->addWidget(resetViewBtn);
    controlsLayout->addWidget(refreshBtn);

    sidebarLayout->addWidget(controlsGroup);

    // Worked list
    QGroupBox* workedGroup = new QGroupBox("Worked List", sidebar);
    workedGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFD700; }");
    QVBoxLayout* workedLayout = new QVBoxLayout(workedGroup);

    m_workedListWidget = new QListWidget(sidebar);
    m_workedListWidget->setStyleSheet("QListWidget { background-color: #34495E; color: white; border: none; }");
    workedLayout->addWidget(m_workedListWidget);

    sidebarLayout->addWidget(workedGroup, 1);  // Stretch to fill remaining space

    mainLayout->addWidget(sidebar);
}

void NativeMapViewer::loadGeoJSON() {
    QString geoJsonPath;

    switch (m_mapType) {
        case Sections:
            geoJsonPath = ":/data/arrl_sections.geojson";
            break;
        case States:
            geoJsonPath = ":/data/us_states.geojson";
            break;
        case DXCC:
            geoJsonPath = ":/data/world_dxcc.geojson";
            break;
    }

    m_polygons = parseGeoJSON(geoJsonPath);
}

QVector<NativeMapViewer::Polygon> NativeMapViewer::parseGeoJSON(const QString& geoJsonPath) {
    QVector<Polygon> polygons;

    QFile file(geoJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open GeoJSON file:" << geoJsonPath;
        return polygons;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "GeoJSON is not an object";
        return polygons;
    }

    QJsonObject root = doc.object();
    QJsonArray features = root["features"].toArray();

    for (const QJsonValue& featureVal : features) {
        QJsonObject feature = featureVal.toObject();
        QJsonObject properties = feature["properties"].toObject();
        QJsonObject geometry = feature["geometry"].toObject();

        Polygon polygon;
        // Extract name based on map type
        if (m_mapType == Sections) {
            polygon.name = properties["section"].toString();
        } else if (m_mapType == States) {
            polygon.name = properties["state"].toString();
        } else if (m_mapType == DXCC) {
            polygon.name = properties["dxcc"].toString();
        }

        QString geometryType = geometry["type"].toString();
        QJsonArray coordinates = geometry["coordinates"].toArray();

        // Helper lambda to process a ring, splitting at date line crossings for DXCC maps
        // This prevents Russia's polygon from drawing a line across the entire map
        auto processRing = [&](const QJsonArray& ring) {
            QPolygonF currentPoly;
            double prevLon = 0;
            bool firstPoint = true;

            for (const QJsonValue& pointVal : ring) {
                QJsonArray point = pointVal.toArray();
                double lon = point[0].toDouble();
                double lat = point[1].toDouble();

                // For DXCC maps, split polygon at date line crossings (lon jump > 180°)
                if (m_mapType == DXCC && !firstPoint) {
                    double lonDiff = qAbs(lon - prevLon);
                    if (lonDiff > 180.0) {
                        // Date line crossing detected - save current polygon and start new one
                        if (currentPoly.size() >= 3) {
                            polygon.rings.append(currentPoly);
                        }
                        currentPoly.clear();
                    }
                }

                currentPoly << applyPseudoPosition(lat, lon, polygon.name);
                prevLon = lon;
                firstPoint = false;
            }

            if (currentPoly.size() >= 3) {
                polygon.rings.append(currentPoly);
            }
        };

        if (geometryType == "Polygon") {
            // Single polygon
            for (const QJsonValue& ringVal : coordinates) {
                QJsonArray ring = ringVal.toArray();
                processRing(ring);
            }
        } else if (geometryType == "MultiPolygon") {
            // Multiple polygons (e.g., Alaska, Hawaii)
            for (const QJsonValue& polyVal : coordinates) {
                QJsonArray polyArray = polyVal.toArray();
                for (const QJsonValue& ringVal : polyArray) {
                    QJsonArray ring = ringVal.toArray();
                    processRing(ring);
                }
            }
        }

        if (!polygon.rings.isEmpty()) {
            polygons.append(polygon);
        }
    }

    LOG_DEBUG("NativeMapViewer", QString("Loaded %1 polygons from GeoJSON").arg(polygons.size()));

    // Debug: Show first 10 polygon names
    QStringList nameList;
    for (int i = 0; i < qMin(10, polygons.size()); ++i) {
        nameList << polygons[i].name;
    }
    LOG_DEBUG("NativeMapViewer", QString("First 10 polygon names: %1").arg(nameList.join(", ")));
    return polygons;
}

QPointF NativeMapViewer::latLonToScene(double lat, double lon) {
    // Simple linear projection (for now - TODO: proper Mercator)
    // Just use degrees as scene coordinates
    // Longitude: -180 to 180, Latitude: -90 to 90
    // Negate Y because scene coordinates have Y increasing downward
    return QPointF(lon, -lat);
}

QPointF NativeMapViewer::applyPseudoPosition(double lat, double lon, const QString& name) {
    // Apply pseudo-positioning for Hawaii (PAC section)
    // Position it 300 miles SW of Los Angeles to avoid overlapping the Pacific

    if (name == "PAC" || name == "HI") {
        // Hawaii: Move from ~(-157, 20) to same latitude as Puerto Rico (~18°N)
        // Position it southwest of LA at longitude ~-121°W
        // Puerto Rico is at approximately (-66, 18)
        // Shift: +36 longitude (east), -2 latitude (south to match PR)
        lon += 36.0;
        lat -= 2.0;
    }
    // Note: Alaska (AK) is NOT repositioned to avoid overlapping Mexico/Canada

    return latLonToScene(lat, lon);
}

void NativeMapViewer::createPolygons() {
    QRectF bounds;
    bool firstPolygon = true;

    for (const Polygon& polygon : m_polygons) {
        for (const QPolygonF& ring : polygon.rings) {
            QGraphicsPolygonItem* item = m_scene->addPolygon(ring);
            // For DXCC world map, don't draw borders (Russia has a horizontal edge artifact at 71°N)
            // For US maps (Sections/States), use cosmetic white borders
            if (m_mapType == DXCC) {
                item->setPen(Qt::NoPen);
            } else {
                QPen pen(Qt::white, 0);
                pen.setCosmetic(true);
                item->setPen(pen);
            }
            item->setBrush(QBrush(getColorForCount(0)));  // Initial color
            item->setToolTip(QString("%1: 0 QSOs").arg(polygon.name));

            // Store ALL items (append to list for this polygon name)
            m_polygonItems[polygon.name].append(item);

            // Calculate scene bounds
            if (firstPolygon) {
                bounds = ring.boundingRect();
                firstPolygon = false;
            } else {
                bounds = bounds.united(ring.boundingRect());
            }
        }
    }

    // Set scene rectangle to match all polygons with padding
    m_scene->setSceneRect(bounds.adjusted(-10, -10, 10, 10));

    QRectF sceneRect = m_scene->sceneRect();
    LOG_DEBUG("NativeMapViewer", QString("Scene rect: QRectF(%1,%2 %3x%4)")
             .arg(sceneRect.x()).arg(sceneRect.y()).arg(sceneRect.width()).arg(sceneRect.height()));
    LOG_DEBUG("NativeMapViewer", QString("Bounds: QRectF(%1,%2 %3x%4)")
             .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height()));

    // Note: fitInView() is called in showEvent() after widget is visible
}

QColor NativeMapViewer::getColorForCount(int count) {
    if (count == 0) return ThemeManager::instance().color(ColorRole::MapNotWorked);
    if (count == 1) return ThemeManager::instance().color(ColorRole::MapFirstContact);
    if (count == 2) return ThemeManager::instance().color(ColorRole::MapSecondContact);
    if (count <= 9) return ThemeManager::instance().color(ColorRole::MapFew);
    if (count <= 19) return ThemeManager::instance().color(ColorRole::MapSome);
    if (count <= 49) return ThemeManager::instance().color(ColorRole::MapMany);
    if (count <= 99) return ThemeManager::instance().color(ColorRole::MapManyMore);
    if (count <= 199) return ThemeManager::instance().color(ColorRole::MapHundreds);
    if (count <= 499) return ThemeManager::instance().color(ColorRole::MapHundredsMore);
    return ThemeManager::instance().color(ColorRole::MapThousands);
}

void NativeMapViewer::updatePolygonColors() {
    // Debug: Check for mismatches
    for (auto it = m_counts.begin(); it != m_counts.end(); ++it) {
        if (!m_polygonItems.contains(it.key())) {
            LOG_WARN("NativeMapViewer", QString("Section %1 has %2 QSOs but no polygon found!").arg(it.key()).arg(it.value()));
        }
    }

    for (auto it = m_polygonItems.begin(); it != m_polygonItems.end(); ++it) {
        const QString& name = it.key();
        const QList<QGraphicsPolygonItem*>& items = it.value();

        int count = m_counts.value(name, 0);

        // Update ALL polygon items for this section (handles multi-ring polygons)
        for (QGraphicsPolygonItem* item : items) {
            item->setBrush(QBrush(getColorForCount(count)));
            item->setToolTip(QString("%1: %2 QSO%3")
                            .arg(name)
                            .arg(count)
                            .arg(count != 1 ? "s" : ""));
        }
    }
}

void NativeMapViewer::refreshData() {
    // Get QSO counts from MapDataProvider
    QJsonObject data;
    QString arrayKey;
    QString itemKey;

    if (m_mapType == Sections) {
        data = MapDataProvider::getWorkedSections(m_qsoModel);
        arrayKey = "sections";
        itemKey = "section";
    } else if (m_mapType == States) {
        data = MapDataProvider::getWorkedStates(m_qsoModel);
        arrayKey = "states";
        itemKey = "state";
    } else {
        data = MapDataProvider::getWorkedDXCCEntities(m_qsoModel);
        arrayKey = "entities";
        itemKey = "dxcc";
    }

    // Extract counts (normalize names to uppercase for matching)
    m_counts.clear();
    QJsonArray items = data[arrayKey].toArray();
    for (const QJsonValue& itemVal : items) {
        QJsonObject itemObj = itemVal.toObject();
        QString name = itemObj[itemKey].toString();
        int count = itemObj["count"].toInt();
        // Normalize to uppercase for case-insensitive matching with GeoJSON names
        m_counts[name.toUpper()] = count;
    }

    QString typeLabel;
    if (m_mapType == Sections) {
        typeLabel = "sections";
    } else if (m_mapType == States) {
        typeLabel = "states";
    } else {
        typeLabel = "DXCC entities";
    }

    LOG_DEBUG("NativeMapViewer", QString("Refreshed data, found %1 %2 with QSOs")
             .arg(m_counts.size())
             .arg(typeLabel));

    // Debug: Show what entities were found
    if (!m_counts.isEmpty()) {
        QStringList entityList;
        for (auto it = m_counts.begin(); it != m_counts.end(); ++it) {
            entityList << QString("%1(%2)").arg(it.key()).arg(it.value());
        }
        LOG_DEBUG("NativeMapViewer", QString("%1: %2").arg(typeLabel).arg(entityList.join(", ")));
    }

    // Update visuals
    updatePolygonColors();
    updateStats();
    updateWorkedList();
}

void NativeMapViewer::updateStats() {
    int worked = m_counts.size();
    int total;
    if (m_mapType == Sections) {
        total = 83;
    } else if (m_mapType == States) {
        total = 50;
    } else {
        // DXCC - use the number of polygons in the map (entities we have GeoJSON for)
        total = m_polygonItems.size();
    }
    double completion = (worked * 100.0) / total;

    int totalQsos = 0;
    for (int count : m_counts.values()) {
        totalQsos += count;
    }

    m_workedLabel->setText(QString::number(worked));
    m_totalLabel->setText(QString::number(total));
    m_completionLabel->setText(QString("%1%").arg(completion, 0, 'f', 1));
    m_qsoCountLabel->setText(QString::number(totalQsos));
}

void NativeMapViewer::updateWorkedList() {
    m_workedListWidget->clear();

    if (m_counts.isEmpty()) {
        m_workedListWidget->addItem("No QSOs yet");
        return;
    }

    // Sort by count descending
    QVector<QPair<QString, int>> sorted;
    for (auto it = m_counts.begin(); it != m_counts.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });

    for (const auto& pair : sorted) {
        QString text = QString("%1: %2 QSO%3")
                      .arg(pair.first)
                      .arg(pair.second)
                      .arg(pair.second != 1 ? "s" : "");
        m_workedListWidget->addItem(text);
    }
}

void NativeMapViewer::onModelDataChanged() {
    LOG_DEBUG("NativeMapViewer", "Model data changed, refreshing map");
    refreshData();
}

void NativeMapViewer::onZoomIn() {
    m_view->scale(1.2, 1.2);
}

void NativeMapViewer::onZoomOut() {
    m_view->scale(1.0 / 1.2, 1.0 / 1.2);
}

void NativeMapViewer::onResetView() {
    m_view->resetTransform();
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void NativeMapViewer::saveViewState() {
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization
    QString key = QString("MapViewer/%1/Transform").arg(m_mapType == Sections ? "Sections" :
                                                         m_mapType == States ? "States" : "DXCC");

    // Save the transform matrix elements
    QTransform transform = m_view->transform();
    QList<QVariant> matrixValues;
    matrixValues << transform.m11() << transform.m12() << transform.m13()
                 << transform.m21() << transform.m22() << transform.m23()
                 << transform.m31() << transform.m32() << transform.m33();
    settings.setValue(key, matrixValues);

    LOG_DEBUG("NativeMapViewer", QString("Saved view state for %1").arg(key));
}

void NativeMapViewer::restoreViewState() {
    QSettings settings(APP_ORG, APP_NAME);  // Must match AppSettings initialization
    QString key = QString("MapViewer/%1/Transform").arg(m_mapType == Sections ? "Sections" :
                                                         m_mapType == States ? "States" : "DXCC");

    // Always start with a valid view first
    if (m_mapType == Sections || m_mapType == States) {
        // Center on CONUS
        QPointF conusCenter = latLonToScene(39.8, -98.6);
        m_view->centerOn(conusCenter);

        // Zoom to show CONUS nicely
        double scaleX = m_view->width() / 70.0;   // ~50 degrees lon + padding
        double scaleY = m_view->height() / 40.0;  // ~25 degrees lat + padding
        double scale = qMin(scaleX, scaleY);
        m_view->scale(scale, scale);
    } else if (m_mapType == DXCC) {
        // World map - center on Atlantic, show full world including Greenland/Arctic
        QPointF worldCenter = latLonToScene(35.0, -20.0);  // Center higher to show northern regions
        m_view->centerOn(worldCenter);

        // Scale to show most of the world (360 degrees lon, ~150 degrees lat visible)
        double scaleX = m_view->width() / 380.0;   // ~360 degrees + padding
        double scaleY = m_view->height() / 160.0;  // ~150 degrees lat visible
        double scale = qMin(scaleX, scaleY);
        m_view->scale(scale, scale);
    } else {
        // Fallback: fit entire scene
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        m_view->scale(2.0, 2.0);
    }

    // Then try to restore saved state on top of that
    if (settings.contains(key)) {
        QList<QVariant> matrixValues = settings.value(key).toList();
        if (matrixValues.size() == 9) {
            QTransform transform(
                matrixValues[0].toDouble(), matrixValues[1].toDouble(), matrixValues[2].toDouble(),
                matrixValues[3].toDouble(), matrixValues[4].toDouble(), matrixValues[5].toDouble(),
                matrixValues[6].toDouble(), matrixValues[7].toDouble(), matrixValues[8].toDouble()
            );
            m_view->setTransform(transform);
            LOG_DEBUG("NativeMapViewer", QString("Restored view state for %1").arg(key));
        }
    } else {
        LOG_DEBUG("NativeMapViewer", QString("No saved state, using default CONUS view"));
    }
}
