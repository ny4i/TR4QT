#ifndef SMETERWIDGET_H
#define SMETERWIDGET_H

#include <QWidget>

namespace TR4QT {

/**
 * S-Meter Widget - Displays received signal strength with discrete bars
 *
 * Shows signal levels as discrete bars (S1-S9, +20, +40, +60):
 * - 9 bars for S1 through S9 (baseline signal strength)
 * - 3 bars for +20, +40, +60 dB over S9 (strong signals)
 *
 * Supports both Icom (CI-V S-meter values 0-255) and K4 (direct S-meter 0-30).
 * Values are mapped to S-units:
 * - Icom: 0-120 → S0-S9 (13.3 per S-unit), 121-241 → +20/+40/+60 (40 per 20dB)
 * - K4:   0-18 → S0-S9 (2 per S-unit), 19-30 → +20/+40/+60 (4 per 20dB)
 *
 * Design principles:
 * - No magic numbers: all dimensions derived from font metrics
 * - No magic colors: all colors from ThemeManager
 * - Discrete bar visualization (not continuous gradient)
 */
class SMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit SMeterWidget(QWidget* parent = nullptr);
    ~SMeterWidget() override = default;

    /**
     * Update S-meter with new signal strength value
     * @param rawValue Raw S-meter value from radio (0-255 for Icom, 0-30 for K4)
     */
    void setValue(int rawValue);

    /**
     * Clear display (no signal)
     */
    void clear();

    /**
     * Get current raw value
     */
    int value() const { return m_rawValue; }

    // Size hints for layout
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    /**
     * Convert raw S-meter value to S-unit level (0-12)
     * 0 = no signal, 1-9 = S1-S9, 10 = +20, 11 = +40, 12 = +60
     */
    int rawToSUnit(int rawValue) const;

    /**
     * Get label for S-unit level
     */
    QString sUnitLabel(int sUnit) const;

    /**
     * Apply current theme colors
     */
    void applyTheme();

    int m_rawValue;         // Raw value from radio (0-255 or 0-30)
    int m_currentSUnit;     // Current S-unit level (0-12)

    // Layout dimensions (derived from font metrics in constructor)
    int m_barWidth;         // Width of each discrete bar
    int m_barHeight;        // Height of each S-meter bar
    int m_barSpacing;       // Spacing between bars
    int m_labelHeight;      // Height for label text
    int m_totalHeight;      // Total widget height
};

} // namespace TR4QT

#endif // SMETERWIDGET_H
