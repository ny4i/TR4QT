#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QWidget>
#include "../plots/qcustomplot.h"

namespace TR4QT {

/**
 * Statistics window - displays QSO statistics using plots
 *
 * Shows:
 * - QSOs over time (line graph)
 * - QSOs by band (bar chart)
 * - QSOs by operator (bar chart)
 * - Cumulative score (line graph)
 */
class StatisticsWindow : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsWindow(QWidget* parent = nullptr);
    ~StatisticsWindow() override = default;

    /**
     * Update plots with current QSO data
     */
    void updatePlots();

private:
    void setupUI();
    void createQSOsOverTimePlot();
    void createQSOsByBandPlot();

    QCustomPlot* m_qsosOverTimePlot;
    QCustomPlot* m_qsosByBandPlot;
};

} // namespace TR4QT

#endif // STATISTICSWINDOW_H
