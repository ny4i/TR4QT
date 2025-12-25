#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H

#include <QWidget>

namespace TR4QT {

/**
 * Custom S-meter widget with traditional radio meter styling
 *
 * Displays signal strength in S-units (S0-S9) and dB over S9.
 * Features:
 * - Traditional horizontal bar graph
 * - Color gradient: green → yellow → red
 * - Scale markings and labels
 * - Smooth value updates
 */
class SMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget* parent = nullptr);
    ~SMeterWidget() override = default;

    /**
     * Set signal strength value in dBm
     * @param dbm Signal strength (-127 to -33 dBm typical range)
     */
    void setValue(int dbm);

    /**
     * Get current value in dBm
     */
    int value() const { return m_currentValue; }

    /**
     * Convert dBm to S-meter string (e.g., "S7", "S9+10")
     */
    static QString dbmToSMeter(int dbm);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void drawBackground(QPainter& painter);
    void drawScale(QPainter& painter);
    void drawBar(QPainter& painter);
    void drawValue(QPainter& painter);

    // Convert dBm to percentage (0-100)
    int dbmToPercentage(int dbm) const;

    int m_currentValue;  // Current value in dBm
};

} // namespace TR4QT

#endif // SMETERWIDGET_H
