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

#ifndef MULTIPLIERWIDGET_H
#define MULTIPLIERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QStringList>
#include "../../core/Types.h"
#include "../PersistentWindow.h"

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
class MultiplierWidget : public PersistentWindow<QWidget> {
    Q_OBJECT

public:
    explicit MultiplierWidget(QWidget* parent = nullptr);
    ~MultiplierWidget() override = default;

    /**
     * Set multiplier type to display
     */
    void setMultiplierType(MultiplierType type);

    /**
     * Set DXCC country list (for Country multiplier type)
     * Should be called with CountryFile::getAllPrimaryPrefixes()
     */
    void setCountryList(const QStringList& prefixes);

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

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void loadMultiplierList();
    void updateDisplay();
    void calculateColumnCount();
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

    // Dynamic layout
    int m_columnCount;  // Dynamically calculated based on window width
    static constexpr int MIN_COLUMN_WIDTH = 70;  // Minimum width per column
};

} // namespace TR4QT

#endif // MULTIPLIERWIDGET_H
