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

#ifndef NEEDSDISPLAYWIDGET_H
#define NEEDSDISPLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "../../core/Types.h"
#include "../../contests/ContestBase.h"

namespace TR4QT {

/**
 * @brief Widget that displays which bands a callsign/multiplier is still needed on
 *
 * Shows two lines:
 * - "QSO needs for [CALLSIGN]:" with colored band indicators
 * - "Mult needs for [CALLSIGN]:" with colored band indicators
 *
 * Mimics TR4W's upper-right corner display for quick visual reference
 */
class NeedsDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit NeedsDisplayWidget(QWidget* parent = nullptr);

    /**
     * @brief Update display for a specific callsign
     * @param callsign The callsign to display needs for
     * @param contest Current contest (for valid bands)
     * @param workedBands Bands this callsign has been worked on
     * @param workedMultBands Bands this multiplier has been worked on
     */
    void updateForCallsign(const QString& callsign,
                          ContestBase* contest,
                          const QList<BandType>& workedBands,
                          const QList<BandType>& workedMultBands);

    /**
     * @brief Clear the display
     */
    void clear();

    /**
     * @brief Set font size for the display
     */
    void setFontSize(int pointSize);

private:
    /**
     * @brief Generate HTML for band list with color coding
     * @param allBands All valid bands for contest
     * @param workedBands Bands already worked
     * @return HTML string with colored band indicators
     */
    QString generateBandListHtml(const QList<BandType>& allBands,
                                 const QList<BandType>& workedBands);

    /**
     * @brief Get short band name (e.g., "20" for Band20M)
     */
    QString getBandShortName(BandType band);

    // UI Components
    QLabel* m_qsoNeedsHeaderLabel;   // "QSO needs for CALLSIGN:"
    QLabel* m_qsoNeedsBandsLabel;    // Band list (colored)
    QLabel* m_multNeedsHeaderLabel;  // "Mult needs for CALLSIGN:"
    QLabel* m_multNeedsBandsLabel;   // Band list (colored)

    int m_fontSize;
};

} // namespace TR4QT

#endif // NEEDSDISPLAYWIDGET_H
