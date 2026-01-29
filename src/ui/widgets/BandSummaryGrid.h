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
 * Mults     8   11    34    35     9    34    112
 * Zones     5    7    11    13     6    18     48       Both: [needed bands]
 */
class BandSummaryGrid : public QWidget {
    Q_OBJECT

public:
    explicit BandSummaryGrid(QWidget* parent = nullptr);
    ~BandSummaryGrid() override = default;

    // Configure grid based on contest rules
    void configureForContest(bool usesModeGroupBreakdown, bool usesZoneMultipliers);

    // Update counts (for single-mode contests or legacy)
    void setQSOCount(BandType band, int count);
    void setMultCount(BandType band, int count);
    void setZoneCount(BandType band, int count);
    void setPointsCount(BandType band, int points);  // Points per band

    // Update counts for mode groups (mixed-mode contests)
    void setModeGroupQSOCount(BandType band, ModeGroup group, int count);
    void setAllModeGroupQSOs(ModeGroup group, int count);

    // Update "All" column totals
    void setAllQSOs(int count);
    void setAllMults(int count);
    void setAllZones(int count);
    void setAllPoints(int points);   // Total QSO points in All column
    void setFinalScore(int score);   // Final contest score (e.g., "36 Pts")

    void setBothNeeded(const QString& bands);

    // Clear all counts
    void clearAll();

    // Font size
    void setFontSize(int pointSize);

    // Enable/disable multiplier row (gray out for contests that don't use mults)
    void setMultipliersEnabled(bool enabled);

    // Enable/disable band selection buttons (when radio disconnected)
    void setBandSelectionEnabled(bool enabled);

    // Set which bands are visible (for contest-specific band restrictions)
    void setVisibleBands(const QList<BandType>& bands);

signals:
    /**
     * Emitted when user clicks on a band header to change bands (legacy, active radio)
     */
    void bandClicked(BandType band);

    /**
     * Emitted when user clicks on a band header with radio target info (SO2R support)
     * @param band The band to switch to
     * @param forNonActiveRadio true if Shift+click or Right-click (change non-active radio)
     */
    void bandClickedWithTarget(BandType band, bool forNonActiveRadio);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void rebuildGrid();  // Rebuild grid based on current configuration
    QString bandToColumnLabel(BandType band) const;
    void applyTheme();

    QGridLayout* m_gridLayout;
    QLabel* m_totalPointsLabel;
    QLabel* m_bothLabel;

    // Configuration
    bool m_usesModeGroupBreakdown;
    bool m_usesZoneMultipliers;
    QList<BandType> m_visibleBands;  // Bands allowed by contest

    // Band columns (160, 80, 40, 20, 15, 10, All)
    QMap<BandType, QLabel*> m_qsoLabels;
    QMap<BandType, QLabel*> m_multLabels;
    QMap<BandType, QLabel*> m_zoneLabels;
    QMap<BandType, QLabel*> m_pointsLabels;  // Points per band
    QMap<BandType, QLabel*> m_bandHeaders;  // Clickable band headers

    // Mode group labels (for mixed-mode contests)
    QMap<ModeGroup, QMap<BandType, QLabel*>> m_modeGroupLabels;
    QMap<ModeGroup, QLabel*> m_modeGroupAllLabels;
    QMap<ModeGroup, QLabel*> m_modeGroupRowHeaders;

    QLabel* m_qsoAllLabel;
    QLabel* m_multAllLabel;
    QLabel* m_zoneAllLabel;
    QLabel* m_pointsAllLabel;  // Total points across all bands
};

} // namespace TR4QT

#endif // BANDSUMMARYGRID_H
