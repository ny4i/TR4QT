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

#ifndef MAPDATAPROVIDER_H
#define MAPDATAPROVIDER_H

#include <QJsonObject>

// Forward declaration
namespace TR4QT {
    class QSOTableModel;
}

/**
 * @brief Pure utility class for converting QSO data to map-ready JSON
 *
 * This class transforms QSO data from QSOTableModel into JSON format
 * suitable for consumption by JavaScript map visualizations.
 *
 * Design: Static-only class (no state, no instances)
 * Purpose: Data transformation layer between C++ and JavaScript
 */
class MapDataProvider {
public:
    /**
     * @brief Get worked ARRL sections with QSO counts
     * @param model QSO table model containing contest log
     * @return JSON object: { sections: [{section, count}], totalSections, totalQsos, lastUpdate }
     */
    static QJsonObject getWorkedSections(TR4QT::QSOTableModel* model);

    /**
     * @brief Get worked US states with QSO counts
     * @param model QSO table model containing contest log
     * @return JSON object: { states: [{state, count}], totalStates, totalQsos, lastUpdate }
     */
    static QJsonObject getWorkedStates(TR4QT::QSOTableModel* model);

    /**
     * @brief Get worked DXCC entities with QSO counts
     * @param model QSO table model containing contest log
     * @return JSON object: { entities: [{dxcc, count}], totalEntities, totalQsos, lastUpdate }
     */
    static QJsonObject getWorkedDXCCEntities(TR4QT::QSOTableModel* model);

private:
    // Private constructor - this is a static-only utility class
    MapDataProvider() = delete;
    ~MapDataProvider() = delete;
    MapDataProvider(const MapDataProvider&) = delete;
    MapDataProvider& operator=(const MapDataProvider&) = delete;
};

#endif // MAPDATAPROVIDER_H
