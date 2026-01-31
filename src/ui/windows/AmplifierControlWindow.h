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

#ifndef AMPLIFIERCONTROLWINDOW_H
#define AMPLIFIERCONTROLWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QRect>
#include <QEvent>
#include <memory>
#include "../../amplifiers/IAmplifierController.h"
#include "../panels/IAmplifierPanelController.h"

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
    void showEvent(QShowEvent* event) override;
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

    // Rendering helpers
    void drawButtonHighlight(QPainter& painter, const QString& buttonName);
    void drawLcdText(QPainter& painter);  // Draw LCD text into label_MAIN region

    // Power meter scaling (watts to proportion)
    double powerWattsToMeterProportion(int watts) const;
    double swrToMeterProportion(float swr) const;

    // SVG LED control helpers
    void initializeLedStates();  // Set all LEDs to OFF state
    void updateSwrMeter(float swr);
    void updatePowerMeter(int watts);  // TODO: Implement when power LEDs are renamed

    // Overlay positioning (called from resizeEvent and showEvent)
    void repositionOverlays();
    void repositionLcdLabel();

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
    bool m_isResizing{false};  // Guard flag to prevent infinite resizeEvent loop

    // Services
    AmplifierService* m_service;

    // Panel controller (handles amplifier-specific SVG layout, buttons, LEDs)
    std::unique_ptr<IAmplifierPanelController> m_panelController;

    // Test mode controls (shown when not connected)
    QLabel* m_disconnectedLabel{nullptr};
    QPushButton* m_testConnectionButton{nullptr};

    // LCD display overlay (positioned over label_MAIN in SVG)
    QLabel* m_lcdLabel{nullptr};
    QString m_lastLcdContent;  // Track last content to avoid redundant logs
    int m_lastLcdFontSize{0};  // Track last font size to avoid redundant setStyleSheet calls

    // Cache SVG element bounds (never change, only calculated once)
    QRectF m_cachedLcdBounds;  // Cached bounds for label_MAIN element
    QRectF m_cachedViewBox;    // Cached SVG viewBox
    bool m_boundsInitialized{false};  // Whether bounds cache is valid

    // Resize debouncing (Windows performance fix)
    QTimer* m_resizeDebounceTimer{nullptr};  // Debounce overlay repositioning

    // Amplifier update debouncing (Windows performance fix - Issue #XX)
    QTimer* m_updateDebounceTimer{nullptr};  // Debounce amplifier state repaints

    // Generic constants (not amplifier-specific)
    static constexpr float MIN_SWR = 1.0f;  // Perfect SWR

    // LED size (as proportion of window height for consistent sizing)
    static constexpr double LED_SIZE_PROPORTION = 0.04;  // 4% of window height
};

} // namespace TR4QT

#endif // AMPLIFIERCONTROLWINDOW_H
