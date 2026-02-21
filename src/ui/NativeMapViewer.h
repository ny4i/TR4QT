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

#ifndef NATIVEMAPVIEWER_H
#define NATIVEMAPVIEWER_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QMap>
#include <QVector>
#include "PersistentWindow.h"

// Forward declarations
class QLabel;
class QVBoxLayout;
class QListWidget;

namespace TR4QT {
    class QSOTableModel;
}

/**
 * @brief Native Qt map viewer using QGraphicsView
 *
 * Cross-platform map viewer that uses Qt's native graphics framework
 * instead of web technologies. Works on all platforms including MinGW.
 *
 * Features:
 * - QGraphicsView-based rendering (no web engine required)
 * - GeoJSON polygon display with Mercator projection
 * - Chloropleth coloring based on QSO counts
 * - Auto-refresh when QSO model changes
 * - Zoom and pan controls
 *
 * Architecture:
 * - Self-contained dialog
 * - Independent window (not embedded)
 * - No dependencies on web technologies
 */
class NativeMapViewer : public TR4QT::PersistentWindow<QDialog> {
    Q_OBJECT

public:
    enum MapType {
        Sections,  // ARRL Sections
        States,    // US States (WAS)
        DXCC       // DXCC Entities (future)
    };

    /**
     * @brief Create a native map viewer widget
     * @param type Map type to display
     * @param qsoModel QSO table model (data source)
     * @param parent Parent widget
     */
    explicit NativeMapViewer(MapType type, TR4QT::QSOTableModel* qsoModel, QWidget* parent = nullptr);
    ~NativeMapViewer();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onModelDataChanged();
    void onZoomIn();
    void onZoomOut();
    void onResetView();
    void refreshData();

private:
    void setupUI();
    void loadGeoJSON();
    void createPolygons();
    void updatePolygonColors();
    void updateStats();
    void updateWorkedList();
    void saveViewState();
    void restoreViewState();

    // GeoJSON parsing
    struct Polygon {
        QString name;  // Section or state abbreviation
        QVector<QPolygonF> rings;  // Outer ring + holes
    };
    QVector<Polygon> parseGeoJSON(const QString& geoJsonPath);
    QPointF latLonToScene(double lat, double lon);
    QPointF applyPseudoPosition(double lat, double lon, const QString& name);

    // Color scheme
    QColor getColorForCount(int count);

    MapType m_mapType;
    TR4QT::QSOTableModel* m_qsoModel;

    // Graphics
    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QMap<QString, QList<QGraphicsPolygonItem*>> m_polygonItems;  // name -> list of polygon items

    // Data
    QMap<QString, int> m_counts;  // section/state -> QSO count
    QVector<Polygon> m_polygons;

    // UI elements
    QLabel* m_workedLabel;
    QLabel* m_totalLabel;
    QLabel* m_completionLabel;
    QLabel* m_qsoCountLabel;
    QListWidget* m_workedListWidget;
};

#endif // NATIVEMAPVIEWER_H
