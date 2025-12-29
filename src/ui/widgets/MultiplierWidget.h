#ifndef MULTIPLIERWIDGET_H
#define MULTIPLIERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QStringList>
#include "../../core/Types.h"

namespace TR4QT {

/**
 * Multiplier status (worked or needed)
 */
enum class MultiplierStatus {
    Worked,      // Already worked on this band
    Needed,      // Not yet worked
    Confirmed    // Worked on all bands (for all-band multipliers)
};

/**
 * Multiplier widget - displays worked and needed multipliers
 *
 * Shows DXCC prefixes, zones, states, or other contest-specific multipliers
 * in a grid layout similar to TR4W.
 *
 * Features:
 * - Grid display of multipliers (e.g., DXCC prefixes)
 * - Color coding (worked vs needed)
 * - Click to add to bandmap or populate callsign
 * - Band-specific or all-band views
 */
class MultiplierWidget : public QWidget {
    Q_OBJECT

public:
    explicit MultiplierWidget(QWidget* parent = nullptr);
    ~MultiplierWidget() override = default;

    /**
     * Set multiplier type to display
     */
    void setMultiplierType(MultiplierType type);

    /**
     * Update multiplier status
     */
    void setMultiplierWorked(const QString& value, BandType band = BandType::None);
    void setMultiplierNeeded(const QString& value);

    /**
     * Clear all multipliers
     */
    void clear();

    /**
     * Get multiplier status
     */
    MultiplierStatus getStatus(const QString& value, BandType band = BandType::None) const;

signals:
    /**
     * User clicked on a multiplier
     */
    void multiplierSelected(const QString& value);

private slots:
    void onContextMenuRequested(const QPoint& pos);
    void onToggleHideWorked();

private:
    void setupUI();
    void loadMultiplierList();
    void updateDisplay();
    QColor getColorForStatus(MultiplierStatus status) const;
    void applyTheme();

    QTableWidget* m_table;
    MultiplierType m_type;
    BandType m_currentBand;  // For band-specific views

    // Multiplier status tracking
    // Key: multiplier value (e.g., "K", "UA", "JA")
    // Value: list of bands where it's been worked
    QMap<QString, QList<BandType>> m_workedMultipliers;

    // Full list of possible multipliers for this type
    QStringList m_allMultipliers;

    // Display options
    bool m_hideWorked;  // Hide worked multipliers when true
};

} // namespace TR4QT

#endif // MULTIPLIERWIDGET_H
