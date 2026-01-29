#ifndef RATEDISPLAYWIDGET_H
#define RATEDISPLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "RateCalculator.h"

namespace TR4QT {

/**
 * Mini bar graph widget for rate visualization
 */
class RateBarGraph : public QWidget {
    Q_OBJECT

public:
    explicit RateBarGraph(QWidget* parent = nullptr);

    /**
     * Set the bar values (up to 4 bars)
     * @param values List of values to display
     * @param labels List of labels for each bar
     */
    void setValues(const QVector<int>& values, const QStringList& labels);

    /**
     * Set maximum value for scaling (auto-scales if 0)
     */
    void setMaxValue(int max) { m_maxValue = max; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return QSize(120, 60); }
    QSize minimumSizeHint() const override { return QSize(80, 40); }

private:
    QVector<int> m_values;
    QStringList m_labels;
    int m_maxValue{0};
};

/**
 * Mini line graph widget for hourly rate history
 */
class RateLineGraph : public QWidget {
    Q_OBJECT

public:
    explicit RateLineGraph(QWidget* parent = nullptr);

    /**
     * Set hourly rate history data
     */
    void setHistory(const QVector<HourlyRatePoint>& history);

    /**
     * Set goal line value (0 to disable)
     */
    void setGoalRate(int rate) { m_goalRate = rate; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override { return QSize(200, 60); }
    QSize minimumSizeHint() const override { return QSize(100, 40); }

private:
    QVector<HourlyRatePoint> m_history;
    int m_goalRate{0};
};

/**
 * RateDisplayWidget
 *
 * Displays rate information with large numbers and mini-graphs.
 *
 * Layout:
 * ┌────────────────────────────────────────────────────────┐
 * │  10-QSO     60-min     This Hr    Average              │
 * │   210        175        162        145                 │
 * │  [▓▓▓▓] [▓▓▓] [▓▓] [▓▓]  │ ▁▂▃▅▆▇█▆▅▃▂▁              │
 * │   10  100 60m  Hr        │  (hourly history)           │
 * └────────────────────────────────────────────────────────┘
 */
class RateDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit RateDisplayWidget(QWidget* parent = nullptr);

    /**
     * Update display with new rate snapshot
     */
    void updateRates(const RateSnapshot& snapshot);

    /**
     * Update hourly history graph
     */
    void updateHistory(const QVector<HourlyRatePoint>& history);

    /**
     * Set goal rate for overlay line
     */
    void setGoalRate(int rate);

private:
    void setupUI();

    // Big number labels
    QLabel* m_rate10Label{nullptr};
    QLabel* m_rate60Label{nullptr};
    QLabel* m_rateThisHrLabel{nullptr};
    QLabel* m_rateAvgLabel{nullptr};

    // Value labels (below titles)
    QLabel* m_rate10Value{nullptr};
    QLabel* m_rate60Value{nullptr};
    QLabel* m_rateThisHrValue{nullptr};
    QLabel* m_rateAvgValue{nullptr};

    // Mini graphs
    RateBarGraph* m_barGraph{nullptr};
    RateLineGraph* m_lineGraph{nullptr};

    // Last snapshot for bar graph
    RateSnapshot m_lastSnapshot;
};

} // namespace TR4QT

#endif // RATEDISPLAYWIDGET_H
