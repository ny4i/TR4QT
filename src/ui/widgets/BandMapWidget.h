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

#ifndef BANDMAPWIDGET_H
#define BANDMAPWIDGET_H

#include <QAbstractScrollArea>
#include <QList>
#include <QDateTime>
#include <hamlib/rig.h>
#include "../../core/Types.h"
#include "../../utils/CountryFile.h"
#include "../PersistentWindow.h"

namespace TR4QT {

/**
 * Sort modes for band map
 */
enum class BandMapSortMode {
    Frequency,   // Sort by frequency (ascending)
    Callsign,    // Sort alphabetically by callsign
    Azimuth      // Sort by azimuth/bearing (ascending 0-360°)
};

/**
 * Frequency spot entry for band map
 */
struct Spot {
    freq_t frequency;       // Frequency in Hz (transmit frequency)
    QString callsign;       // Callsign spotted
    QDateTime timestamp;    // When spotted
    bool isMultiplier;      // Is this a needed multiplier?
    bool isWorked;          // Already worked this station?
    bool isLotwUser;        // Is this a LOTW user?
    QString comment;        // Comment from DX cluster spot
    freq_t qsx;             // Split receive frequency in Hz (0 if not split, for VFO B)
    QString source;         // Source of spot (DX Cluster, manual, etc.)
    double azimuth;         // Bearing from user's location (degrees, 0-360, -1 if unknown)
    double distance;        // Distance from user's location (km, -1 if unknown)

    Spot() : frequency(0), isMultiplier(false), isWorked(false), isLotwUser(false), qsx(0), azimuth(-1.0), distance(-1.0) {}
};

/**
 * Band map widget - displays frequency spots
 *
 * Shows a list of spotted stations with frequencies, similar to TR4W's band map.
 * Features:
 * - Color coding (multipliers in blue, worked stations grayed out)
 * - Click to QSY radio to frequency
 * - Automatic aging/removal of old spots
 * - Manual spot entry
 * - Integration with DX Cluster
 * - Scrollable when content exceeds viewport
 */
class BandMapWidget : public PersistentWindow<QAbstractScrollArea> {
    Q_OBJECT

public:
    explicit BandMapWidget(QWidget* parent = nullptr);
    ~BandMapWidget() override = default;

    /**
     * Add a spot to the band map
     */
    void addSpot(const Spot& spot);

    /**
     * Remove a spot by callsign
     */
    void removeSpot(const QString& callsign);

    /**
     * Clear all spots
     */
    void clearSpots();

    /**
     * Set current radio frequency (for highlighting)
     */
    void setCurrentFrequency(freq_t freq);

    /**
     * Set current band (for filtering when radio not connected)
     */
    void setCurrentBand(BandType band);

    /**
     * Get spot count (all spots, not just displayed)
     */
    int spotCount() const { return m_allSpots.size(); }

    /**
     * Find a spot by callsign and return its frequency (0 if not found)
     */
    freq_t findFrequencyByCallsign(const QString& callsign) const;

    /**
     * Refresh LOTW status for all spots
     * Called when LOTW settings change (e.g., min upload months)
     */
    void refreshLotwStatus();

    /**
     * Load spots from database on startup
     */
    void loadSpotsFromDatabase();

    /**
     * Save spots to database on shutdown
     */
    void saveSpotsToDatabase();

    /**
     * Update spot status after QSO logging
     */
    void updateSpotStatus(const QString& callsign, bool isWorked, bool isMultiplier);

signals:
    /**
     * User clicked on a spot - request QSY to this frequency
     */
    void qsyRequested(freq_t frequency);

    /**
     * User double-clicked on a callsign - populate call entry
     */
    void callsignSelected(const QString& callsign);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private slots:
    /**
     * Periodic refresh timer - cleanup expired spots and update aging colors
     */
    void onRefreshTimer();

    /**
     * Mouse activity timeout - resume refreshes after mouse stops moving
     */
    void onMouseActivityTimeout();

private:
    // Storage - master list and filtered display list
    QList<Spot> m_allSpots;      // All spots in memory (master list)
    QList<Spot> m_displaySpots;  // Filtered spots for display (cache)
    bool m_spotsDirty{false};    // Spots added while hidden, need geography calc
    freq_t m_currentFrequency;
    BandType m_currentBand;      // Current band (for filtering when radio not connected)
    int m_selectedIndex;
    QString m_selectedCallsign;  // Track selected spot by callsign (survives sort/filter)
    int m_columnCount;       // Number of columns to display
    int m_columnWidth;       // Width of each column in pixels
    BandMapSortMode m_sortMode;  // Current sort mode
    bool m_showOnlyLotwUsers;    // Filter to show only LOTW users
    bool m_showAllBands;         // Show all bands or only current band

    QTimer* m_refreshTimer;         // Periodic refresh for aging and cleanup
    QTimer* m_mouseActivityTimer;   // Tracks mouse movement to pause refreshes
    bool m_mouseActive;             // True when mouse is moving inside widget

    /**
     * Apply current theme colors
     */
    void applyTheme();

    /**
     * Sort spots according to current sort mode
     */
    void sortSpots();

    /**
     * Calculate optimal column layout based on widget size
     */
    void calculateColumnLayout();

    /**
     * Find spot at mouse position (supports multi-column layout)
     */
    int findSpotAtPosition(const QPoint& pos);

    /**
     * Calculate row height
     */
    int rowHeight() const;

    /**
     * Format frequency for display (e.g., "7050.0")
     */
    QString formatFrequency(freq_t freq) const;

    /**
     * Update scrollbar ranges based on content size
     */
    void updateScrollBars();

    /**
     * Get band name from frequency (e.g., "160m", "80m", "40m")
     * Returns empty string if not a ham band
     */
    QString getBandFromFrequency(freq_t freq) const;

    /**
     * Rebuild m_displaySpots from m_allSpots with current filters
     * Applies band filter, LOTW filter, and expiry filter
     */
    void rebuildDisplayList();

    /**
     * Remove expired spots from m_allSpots
     * Called periodically by refresh timer
     */
    void removeExpiredSpots();

    /**
     * Spot age category for color coding
     */
    enum class SpotAge {
        New,      // < 60 seconds
        Normal,   // 60s - 8 minutes
        Aging,    // Last 2 minutes before expiry
        Expired   // > expiry time
    };

    /**
     * Get spot age category based on timestamp
     */
    SpotAge getSpotAge(const Spot& spot) const;

    /**
     * Get text color based on spot properties and age
     */
    QColor getSpotTextColor(const Spot& spot) const;

    /**
     * Get background color based on spot age
     */
    QColor getSpotBackgroundColor(const Spot& spot) const;

    /**
     * Calculate and update azimuth/distance for a spot
     * Uses user's grid square from settings and callsign country lookup
     */
    void calculateSpotGeography(Spot& spot);

    // Country file for callsign lookups
    CountryFile m_countryFile;
};

} // namespace TR4QT

#endif // BANDMAPWIDGET_H
