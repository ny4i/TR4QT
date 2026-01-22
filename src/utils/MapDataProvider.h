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
