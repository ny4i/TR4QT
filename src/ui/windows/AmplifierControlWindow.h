#ifndef AMPLIFIERCONTROLWINDOW_H
#define AMPLIFIERCONTROLWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QRect>
#include <QEvent>
#include "../../amplifiers/IAmplifierController.h"

class QLabel;
class QPushButton;

namespace TR4QT {

class AmplifierService;
class SvgPanelWidget;

/**
 * @brief Graphical front panel control window for amplifiers
 *
 * Displays a realistic front panel image with clickable controls.
 * All coordinates are proportional to image dimensions for proper scaling.
 */
class AmplifierControlWindow : public QWidget {
    Q_OBJECT

public:
    explicit AmplifierControlWindow(AmplifierService* service, QWidget* parent = nullptr);
    ~AmplifierControlWindow() override;

protected:
    bool event(QEvent* event) override;  // Debug: catch all events
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onAmplifierStateUpdated(const AmplifierState& state);
    void onConnectionStatusChanged(bool connected);
    void onTestConnection();

private:
    // Button region definition (coordinates as proportions of image size)
    struct ButtonRegion {
        QString name;           // Button identifier (e.g., "operate_standby")
        double x;               // X position as proportion (0.0-1.0)
        double y;               // Y position as proportion (0.0-1.0)
        double width;           // Width as proportion (0.0-1.0)
        double height;          // Height as proportion (0.0-1.0)
        QString tooltip;        // Hover text
        bool isToggle;          // True if button has on/off state
    };

    // LED indicator definition
    struct LedIndicator {
        QString name;           // LED identifier
        double x;               // X position as proportion (0.0-1.0)
        double y;               // Y position as proportion (0.0-1.0)
        double size;            // Size as proportion (0.0-1.0)
        QColor activeColor;     // Color when active
        QColor inactiveColor;   // Color when inactive
        bool isActive;          // Current state
    };

    // Display region (LCD, meter, etc.)
    struct DisplayRegion {
        QString name;
        double x, y, width, height;  // All as proportions (0.0-1.0)
    };

    // Initialize button regions (proportional coordinates)
    void initializeButtonRegions();
    void initializeLedIndicators();
    void initializeDisplayRegions();

    // Coordinate conversion
    QRect scaleRegionToWidget(double x, double y, double width, double height) const;
    QPoint scalePointToImage(const QPoint& widgetPoint) const;
    QPointF imageToProportional(const QPoint& imagePoint) const;

    // Button handling
    void handleButtonPress(const QString& buttonName);
    void handleButtonRelease(const QString& buttonName);
    QString findButtonAtPoint(const QPoint& point) const;

    // Rendering helpers (legacy - will be replaced by SVG control)
    void drawPowerMeter(QPainter& painter);
    void drawSwrMeter(QPainter& painter);
    void drawLedIndicators(QPainter& painter);
    void drawLcdDisplay(QPainter& painter);
    void drawButtonOverlays(QPainter& painter);  // Draw visible buttons on placeholder
    void drawButtonHighlight(QPainter& painter, const QString& buttonName);

    // Power meter scaling (watts to proportion)
    double powerWattsToMeterProportion(int watts) const;
    double swrToMeterProportion(float swr) const;

    // SVG LED control helpers
    void updateSwrMeter(float swr);
    void updatePowerMeter(int watts);  // TODO: Implement when power LEDs are renamed

    // SVG front panel widget (replaces static QPixmap rendering)
    SvgPanelWidget* m_svgPanel{nullptr};

    // Legacy (kept for backward compatibility during transition)
    QPixmap m_frontPanelImage;
    QSize m_originalImageSize;
    double m_aspectRatio;

    // Interactive elements
    QList<ButtonRegion> m_buttonRegions;
    QList<LedIndicator> m_ledIndicators;
    QList<DisplayRegion> m_displayRegions;

    // State tracking
    AmplifierState m_currentState;
    QString m_pressedButton;  // Currently pressed button (for visual feedback)
    bool m_connected{false};

    // Services
    AmplifierService* m_service;

    // Test mode controls (shown when not connected)
    QLabel* m_disconnectedLabel{nullptr};
    QPushButton* m_testConnectionButton{nullptr};

    // Constants for power meter ranges (named, not magic numbers)
    static constexpr int MAX_POWER_WATTS = 1500;  // KPA1500 maximum power
    static constexpr float MIN_SWR = 1.0f;         // Perfect SWR
    static constexpr float MAX_SWR_DISPLAY = 5.0f; // Maximum SWR to display on meter

    // Power meter color thresholds (as proportions of max power)
    static constexpr double POWER_GREEN_THRESHOLD = 0.66;   // 0-66% power = green
    static constexpr double POWER_YELLOW_THRESHOLD = 0.85;  // 66-85% power = yellow
    // Above 85% = red

    // SWR color thresholds
    static constexpr float SWR_GREEN_THRESHOLD = 1.5f;   // SWR < 1.5 = green
    static constexpr float SWR_YELLOW_THRESHOLD = 2.0f;  // SWR 1.5-2.0 = yellow
    // SWR > 2.0 = red

    // LED size (as proportion of window height for consistent sizing)
    static constexpr double LED_SIZE_PROPORTION = 0.04;  // 4% of window height

    // Minimum window size (to maintain readability)
    static constexpr int MIN_WINDOW_WIDTH = 600;
    static constexpr int MIN_WINDOW_HEIGHT = 200;
};

} // namespace TR4QT

#endif // AMPLIFIERCONTROLWINDOW_H
