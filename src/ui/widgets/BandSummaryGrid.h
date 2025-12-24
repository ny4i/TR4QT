#ifndef BANDSUMMARYGRID_H
#define BANDSUMMARYGRID_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QMap>
#include "../../core/Types.h"

namespace TR4QT {

/**
 * Band summary grid widget (TR4W style)
 *
 * Displays QSO counts, multiplier counts, and zone counts per band
 *
 * Layout:
 *         160   80    40    20    15    10    All       [Total Points]
 * QSOs     12   34    48    37    12    42    139
 * DX Mults  8   11    34    35     9    34    112
 * Zones     5    7    11    13     6    18     48       Both: [needed bands]
 */
class BandSummaryGrid : public QWidget {
    Q_OBJECT

public:
    explicit BandSummaryGrid(QWidget* parent = nullptr);
    ~BandSummaryGrid() override = default;

    // Update counts
    void setQSOCount(BandType band, int count);
    void setMultCount(BandType band, int count);
    void setZoneCount(BandType band, int count);
    void setTotalPoints(int points);
    void setBothNeeded(const QString& bands);

    // Clear all counts
    void clearAll();

    // Font size
    void setFontSize(int pointSize);

private:
    void setupUI();
    QString bandToColumnLabel(BandType band) const;

    QGridLayout* m_gridLayout;
    QLabel* m_totalPointsLabel;
    QLabel* m_bothLabel;

    // Band columns (160, 80, 40, 20, 15, 10, All)
    QMap<BandType, QLabel*> m_qsoLabels;
    QMap<BandType, QLabel*> m_multLabels;
    QMap<BandType, QLabel*> m_zoneLabels;

    QLabel* m_qsoAllLabel;
    QLabel* m_multAllLabel;
    QLabel* m_zoneAllLabel;
};

} // namespace TR4QT

#endif // BANDSUMMARYGRID_H
