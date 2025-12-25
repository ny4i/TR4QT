#ifndef BANDMAPWIDGET_H
#define BANDMAPWIDGET_H

#include <QAbstractScrollArea>
#include <QList>
#include <QDateTime>
#include <hamlib/rig.h>
#include "../../core/Types.h"

namespace TR4QT {

/**
 * Sort modes for band map
 */
enum class BandMapSortMode {
    Frequency,   // Sort by frequency (ascending)
    Callsign     // Sort alphabetically by callsign
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

    Spot() : frequency(0), isMultiplier(false), isWorked(false), isLotwUser(false), qsx(0) {}
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
class BandMapWidget : public QAbstractScrollArea {
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
     * Get spot count
     */
    int spotCount() const { return m_spots.size(); }

    /**
     * Refresh LOTW status for all spots
     * Called when LOTW settings change (e.g., min upload months)
     */
    void refreshLotwStatus();

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
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    QList<Spot> m_spots;
    freq_t m_currentFrequency;
    int m_selectedIndex;
    int m_columnCount;       // Number of columns to display
    int m_columnWidth;       // Width of each column in pixels
    BandMapSortMode m_sortMode;  // Current sort mode
    bool m_showOnlyLotwUsers;    // Filter to show only LOTW users

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
};

} // namespace TR4QT

#endif // BANDMAPWIDGET_H
