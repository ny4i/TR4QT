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

#include "AmplifierControlWindow.h"
#include "../widgets/SvgPanelWidget.h"
#include "../panels/AmplifierPanelFactory.h"
#include "../panels/KPA1500PanelController.h"
#include "../../services/AmplifierService.h"
#include "../../amplifiers/AmplifierFactory.h"
#include "../../utils/AppSettings.h"
#include "../../utils/FontManager.h"
#include "../../utils/ThemeManager.h"
#include "../../logging/LogMacros.h"
#include <QPainter>
#include <QMouseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QSvgRenderer>
#include <QColor>
#include <cmath>

namespace TR4QT {

AmplifierControlWindow::AmplifierControlWindow(AmplifierService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    // Create panel controller for KPA1500 (TODO: get from service/settings)
    m_panelController = AmplifierPanelFactory::createPanelController(
        AmplifierPanelFactory::AmplifierType::KPA1500);

    if (!m_panelController) {
        LOG_ERROR("AmplifierControlWindow", "Failed to create panel controller");
        return;
    }

    setWindowTitle(m_panelController->getAmplifierName() + " Control");
    setMinimumSize(m_panelController->getMinimumWindowSize());

    // Windows performance: Disable smooth resize to prevent main thread blocking
    // Paint events only fire after resize completes, not during drag
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);

    // Create SVG panel widget using path from panel controller
    QString svgPath = m_panelController->getSvgResourcePath();
    m_svgPanel = new SvgPanelWidget(svgPath, this);

    // Fallback: try development path if resource path fails
    if (!m_svgPanel->isValid()) {
        delete m_svgPanel;
        svgPath = m_panelController->getSvgFallbackPath();
        m_svgPanel = new SvgPanelWidget(svgPath, this);
    }

    // Layout: SVG panel fills the entire window
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_svgPanel);
    setLayout(layout);

    // Configure resize debounce timer (Windows performance fix)
    m_resizeDebounceTimer = new QTimer(this);
    m_resizeDebounceTimer->setSingleShot(true);
    m_resizeDebounceTimer->setInterval(16);  // ~60fps
    connect(m_resizeDebounceTimer, &QTimer::timeout, this, [this]() {
        LOG_INFO("AmplifierControlWindow", "Timer fired - repositioning overlays");
        QElapsedTimer timer;
        timer.start();
        repositionOverlays();
        qint64 overlaysTime = timer.elapsed();
        repositionLcdLabel();
        qint64 lcdTime = timer.elapsed() - overlaysTime;
        LOG_INFO("AmplifierControlWindow", QString("Repositioning done: overlays=%1ms, lcd=%2ms").arg(overlaysTime).arg(lcdTime));
    });

    // Configure amplifier update debounce timer (Windows performance fix - prevents event queue flooding)
    // Amplifier polls every ~130ms, but we only need to repaint at ~60fps max (16ms)
    m_updateDebounceTimer = new QTimer(this);
    m_updateDebounceTimer->setSingleShot(true);
    m_updateDebounceTimer->setInterval(50);  // Max 20 repaints/sec instead of 8/sec constant flooding
    connect(m_updateDebounceTimer, &QTimer::timeout, this, [this]() {
        update();  // Actually trigger the repaint
    });

    // Get actual SVG intrinsic dimensions from the renderer
    // CRITICAL: Must match what SvgPanelWidget::paintEvent() uses for scaling
    m_originalImageSize = m_svgPanel->getSvgSize();
    if (m_originalImageSize.isEmpty()) {
        m_originalImageSize = QSize(1386, 453);  // Fallback
    }
    m_aspectRatio = static_cast<double>(m_originalImageSize.width()) / m_originalImageSize.height();
    LOG_INFO("AmplifierControl", QString("Using SVG dimensions from renderer: %1x%2, aspect ratio: %3")
        .arg(m_originalImageSize.width()).arg(m_originalImageSize.height()).arg(m_aspectRatio));

    // Initialize interactive elements (will be replaced by SVG click regions)
    initializeButtonRegions();
    initializeLedIndicators();
    initializeDisplayRegions();

    // Create disconnected overlay widgets
    m_disconnectedLabel = new QLabel("Amplifier Not Connected", this);
    m_disconnectedLabel->setAlignment(Qt::AlignCenter);
    m_disconnectedLabel->setStyleSheet(QString(
        "QLabel { "
        "  background-color: rgba(0, 0, 0, 180); "
        "  color: %1; "
        "  font-size: 18pt; "
        "  font-weight: bold; "
        "  padding: 20px; "
        "  border: 2px solid %1; "
        "}")
        .arg(ThemeManager::instance().colorName(ColorRole::WarningText))
    );

    m_testConnectionButton = new QPushButton("Test Connection", this);
    m_testConnectionButton->setStyleSheet(QString(
        "QPushButton { "
        "  background-color: %1; "
        "  color: white; "
        "  font-size: 12pt; "
        "  padding: 10px 20px; "
        "  border: 2px solid %2; "
        "  border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "  background-color: %3; "
        "}")
        .arg(ThemeManager::instance().colorName(ColorRole::AmplifierSuccessBackground))
        .arg(ThemeManager::instance().colorName(ColorRole::ValidationValidBorder))
        .arg(ThemeManager::instance().colorName(ColorRole::AmplifierSuccessHover))
    );

    connect(m_testConnectionButton, &QPushButton::clicked,
            this, &AmplifierControlWindow::onTestConnection);

    // Create LCD display overlay label (positioned over label_MAIN in SVG)
    m_lcdLabel = new QLabel("---", this);
    m_lcdLabel->setAlignment(Qt::AlignCenter);
    m_lcdLabel->setStyleSheet(QString(
        "QLabel { "
        "  background-color: transparent; "
        "  color: %1; "
        "  font-family: 'Courier New', Courier, monospace; "
        "  font-size: 10pt; "
        "  font-weight: bold; "
        "}")
        .arg(ThemeManager::instance().colorName(ColorRole::LcdDisplayText))
    );
    m_lcdLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);  // Don't block clicks
    m_lcdLabel->setGeometry(-1000, -1000, 1, 1);  // Move offscreen until properly positioned in showEvent

    // Connect to amplifier service signals
    if (m_service) {
        connect(m_service, &AmplifierService::stateUpdated,
                this, &AmplifierControlWindow::onAmplifierStateUpdated);
        connect(m_service, &AmplifierService::connectionStatusChanged,
                this, &AmplifierControlWindow::onConnectionStatusChanged);

        // Get initial state
        m_connected = m_service->isConnected();
        LOG_INFO("AmplifierControlWindow", QString("Initial connection state: %1").arg(m_connected ? "connected" : "disconnected"));
        if (m_connected) {
            m_currentState = m_service->currentState();
            // Apply initial state to LEDs
            onAmplifierStateUpdated(m_currentState);
        } else {
            // Not connected - set all LEDs to OFF state
            initializeLedStates();
        }
    }

    // Enable mouse tracking for button hover effects
    setMouseTracking(true);

    // Update overlay visibility and mouse transparency
    m_disconnectedLabel->setVisible(!m_connected);
    m_testConnectionButton->setVisible(!m_connected);

    // Make overlay widgets transparent to mouse events when hidden
    // This prevents them from blocking clicks on the painted buttons
    m_disconnectedLabel->setAttribute(Qt::WA_TransparentForMouseEvents, m_connected);
    m_testConnectionButton->setAttribute(Qt::WA_TransparentForMouseEvents, m_connected);
}

AmplifierControlWindow::~AmplifierControlWindow() {
    // Qt automatically disconnects signals when QObjects are destroyed
    // No manual cleanup needed
}

void AmplifierControlWindow::initializeButtonRegions() {
    // ALL button regions are now read from SVG element bounds
    // This ensures click detection matches the actual button positions in the SVG

    if (!m_svgPanel || !m_panelController) {
        LOG_ERROR("AmplifierControl", "Cannot initialize button regions - missing SVG panel or controller");
        return;
    }

    // CRITICAL: Use viewBox dimensions, NOT pixel dimensions!
    // boundsOnElement() returns coordinates in viewBox space
    QRectF viewBox = m_svgPanel->getViewBox();
    double svgWidth = viewBox.width();
    double svgHeight = viewBox.height();

    LOG_INFO("AmplifierControl", QString("SVG viewBox: %1x%2 (pixel size: %3x%4)")
        .arg(svgWidth, 0, 'f', 1).arg(svgHeight, 0, 'f', 1)
        .arg(m_originalImageSize.width()).arg(m_originalImageSize.height()));

    if (svgWidth <= 0 || svgHeight <= 0) {
        LOG_ERROR("AmplifierControl", "Invalid SVG viewBox - cannot determine button positions");
        return;
    }

    // Get all button IDs from the panel controller
    QStringList allButtonIds = m_panelController->getButtonIds();

    for (const QString& buttonId : allButtonIds) {
        QRectF bounds = m_svgPanel->getElementBounds(buttonId);

        if (bounds.isEmpty()) {
            LOG_WARN("AmplifierControl", QString("Button element not found in SVG: %1").arg(buttonId));
            continue;
        }

        // Convert viewBox coordinates to proportional (0.0-1.0)
        double x = bounds.x() / svgWidth;
        double y = bounds.y() / svgHeight;
        double w = bounds.width() / svgWidth;
        double h = bounds.height() / svgHeight;

        // Get button label from panel controller
        QString label = m_panelController->getButtonLabel(buttonId);

        // Determine if it's a toggle button (ON_OFF and MODE toggle operate/standby)
        bool isToggle = (buttonId == "btn_ON_OFF" || buttonId == "btn_MODE");

        LOG_TRACE("AmplifierControl", QString("Button %1: viewBox(%2,%3 %4x%5) -> proportional(%6,%7 %8x%9)")
            .arg(buttonId)
            .arg(bounds.x(), 0, 'f', 1).arg(bounds.y(), 0, 'f', 1)
            .arg(bounds.width(), 0, 'f', 1).arg(bounds.height(), 0, 'f', 1)
            .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3)
            .arg(w, 0, 'f', 3).arg(h, 0, 'f', 3));

        m_buttonRegions.append({
            buttonId,
            x,
            y,
            w,
            h,
            label,
            isToggle
        });
    }

    LOG_TRACE("AmplifierControl", QString("Initialized %1 button regions from SVG").arg(m_buttonRegions.size()));
}

void AmplifierControlWindow::initializeLedIndicators() {
    // LED positions (proportional coordinates)
    const double LED_Y = 0.20;  // All status LEDs at same height

    // OPER LED (green when operating)
    m_ledIndicators.append({
        "oper",
        0.55,   // x position
        LED_Y,
        LED_SIZE_PROPORTION,
        QColor(0, 255, 0),      // Green when active
        QColor(50, 50, 50),     // Dark gray when inactive
        false
    });

    // STBY LED (yellow when standby)
    m_ledIndicators.append({
        "stby",
        0.60,   // x position
        LED_Y,
        LED_SIZE_PROPORTION,
        QColor(255, 170, 0),    // Yellow/orange when active
        QColor(50, 50, 50),
        false
    });

    // FAULT LED (red when fault)
    m_ledIndicators.append({
        "fault",
        0.43,   // x position
        0.15,   // Fault LED higher up
        LED_SIZE_PROPORTION,
        QColor(255, 0, 0),      // Red when active
        QColor(50, 50, 50),
        false
    });
}

void AmplifierControlWindow::initializeDisplayRegions() {
    // LCD display region (proportional coordinates)
    m_displayRegions.append({
        "lcd",
        0.08,   // x
        0.12,   // y
        0.22,   // width
        0.22    // height
    });

    // Power meter region
    m_displayRegions.append({
        "power_meter",
        0.42,   // x
        0.05,   // y
        0.48,   // width
        0.08    // height
    });

    // SWR meter region
    m_displayRegions.append({
        "swr_meter",
        0.75,   // x
        0.15,   // y
        0.13,   // width
        0.08    // height
    });
}

bool AmplifierControlWindow::event(QEvent* event) {
    // Debug: Log only button press/release events (not continuous mouse moves)
    if (event->type() == QEvent::MouseButtonPress) {
        LOG_TRACE("AmplifierControl", QString("MOUSE PRESS at widget-level (type=%1)").arg(event->type()));
    } else if (event->type() == QEvent::MouseButtonRelease) {
        LOG_TRACE("AmplifierControl", QString("MOUSE RELEASE at widget-level (type=%1)").arg(event->type()));
    }
    return QWidget::event(event);
}

void AmplifierControlWindow::paintEvent(QPaintEvent* event) {
    // Let the base class and child widgets (SvgPanelWidget) paint first
    QWidget::paintEvent(event);

    // Draw overlays on top of the SVG panel
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // LCD text is now handled by m_lcdLabel QLabel widget (proper z-order)

    // Draw button highlight for visual feedback when pressed
    if (!m_pressedButton.isEmpty() && m_connected) {
        drawButtonHighlight(painter, m_pressedButton);
    }
}

void AmplifierControlWindow::drawButtonHighlight(QPainter& painter, const QString& buttonName) {
    const ButtonRegion* button = nullptr;
    for (const auto& region : m_buttonRegions) {
        if (region.name == buttonName) {
            button = &region;
            break;
        }
    }
    if (!button) return;

    QRect buttonRect = scaleRegionToWidget(button->x, button->y, button->width, button->height);

    // Draw semi-transparent highlight overlay
    painter.fillRect(buttonRect, QColor(255, 255, 255, 80));

    // Draw border
    painter.setPen(QPen(QColor(255, 255, 0), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(buttonRect);
}

void AmplifierControlWindow::drawLcdText(QPainter& painter) {
    if (!m_svgPanel) return;

    // Get the bounds of label_MAIN from the SVG (in SVG viewBox coordinates)
    QRectF svgBounds = m_svgPanel->getElementBounds("label_MAIN");
    if (svgBounds.isEmpty()) {
        LOG_WARN("AmplifierControl", "label_MAIN element not found in SVG");
        return;
    }

    // Get viewBox dimensions for coordinate conversion
    QRectF viewBox = m_svgPanel->getViewBox();
    if (viewBox.isEmpty()) return;

    // Convert SVG viewBox coordinates to proportional coordinates (0.0-1.0)
    double propX = svgBounds.x() / viewBox.width();
    double propY = svgBounds.y() / viewBox.height();
    double propW = svgBounds.width() / viewBox.width();
    double propH = svgBounds.height() / viewBox.height();

    // Convert proportional coordinates to widget coordinates
    // (accounting for SVG scaling and centering within m_svgPanel)
    int widgetWidth = m_svgPanel->width();
    int widgetHeight = m_svgPanel->height();

    double widthRatio = static_cast<double>(widgetWidth) / m_originalImageSize.width();
    double heightRatio = static_cast<double>(widgetHeight) / m_originalImageSize.height();
    double scale = qMin(widthRatio, heightRatio);

    int scaledWidth = static_cast<int>(m_originalImageSize.width() * scale);
    int scaledHeight = static_cast<int>(m_originalImageSize.height() * scale);

    // Center offset within the SVG panel widget
    int offsetX = (widgetWidth - scaledWidth) / 2;
    int offsetY = (widgetHeight - scaledHeight) / 2;

    // Calculate LCD rect in widget coordinates (relative to m_svgPanel)
    int lcdX = offsetX + static_cast<int>(propX * scaledWidth);
    int lcdY = offsetY + static_cast<int>(propY * scaledHeight);
    int lcdW = static_cast<int>(propW * scaledWidth);
    int lcdH = static_cast<int>(propH * scaledHeight);

    // Convert from m_svgPanel coordinates to AmplifierControlWindow coordinates
    QPoint svgPanelPos = m_svgPanel->mapTo(this, QPoint(0, 0));
    QRect lcdRect(svgPanelPos.x() + lcdX, svgPanelPos.y() + lcdY, lcdW, lcdH);

    // Set up font - scale based on LCD height for consistent appearance
    QFont lcdFont = FontManager::instance().courierFont(1);  // Start with size 1, will be scaled
    lcdFont.setBold(true);
    int fontSize = qMax(8, lcdH / 3);  // Scale font to ~1/3 of LCD height
    lcdFont.setPixelSize(fontSize);
    painter.setFont(lcdFont);
    painter.setPen(ThemeManager::instance().color(ColorRole::LcdDisplayText));

    // Format display text based on connection state
    QString displayText;
    if (m_connected) {
        // Show amplifier state data
        double freqMhz = m_currentState.frequency / 1000000.0;
        QString line1 = QString("F:%1W R:%2W")
            .arg(m_currentState.forwardPowerWatts, 4)
            .arg(m_currentState.reflectedPowerWatts, 3);
        QString line2 = QString("%1MHz %2C")
            .arg(freqMhz, 0, 'f', 2)
            .arg(m_currentState.temperature);
        displayText = line1 + "\n" + line2;
    } else {
        displayText = "---";
    }

    // Draw text centered in LCD region
    const int LCD_PADDING = 2;
    QRect textRect = lcdRect.adjusted(LCD_PADDING, LCD_PADDING, -LCD_PADDING, -LCD_PADDING);
    painter.drawText(textRect, Qt::AlignCenter, displayText);
}

void AmplifierControlWindow::mousePressEvent(QMouseEvent* event) {
    LOG_TRACE("AmplifierControl", QString("MOUSE EVENT RECEIVED at (%1, %2), connected=%3")
        .arg(event->pos().x()).arg(event->pos().y()).arg(m_connected));

    if (!m_connected) {
        LOG_WARN("AmplifierControl", "Mouse click ignored - amplifier not connected");
        return;
    }

    // CRITICAL: Convert click from window coordinates to SVG widget coordinates
    // event->pos() is relative to AmplifierControlWindow
    // We need coordinates relative to m_svgPanel
    QPoint svgWidgetPos = m_svgPanel->mapFrom(this, event->pos());

    LOG_INFO("AmplifierControl", QString("Window click (%1,%2) -> SVG widget (%3,%4)")
        .arg(event->pos().x()).arg(event->pos().y())
        .arg(svgWidgetPos.x()).arg(svgWidgetPos.y()));

    QPoint imagePoint = scalePointToImage(svgWidgetPos);
    QString buttonName = findButtonAtPoint(imagePoint);

    QPointF proportional = imageToProportional(imagePoint);
    LOG_INFO("AmplifierControl", QString("Proportional(%1,%2) button: %3")
        .arg(proportional.x(), 0, 'f', 3).arg(proportional.y(), 0, 'f', 3)
        .arg(buttonName.isEmpty() ? "NONE" : buttonName));

    if (!buttonName.isEmpty()) {
        m_pressedButton = buttonName;
        handleButtonPress(buttonName);
        update();  // Redraw to show highlight
    }
}

void AmplifierControlWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_connected) return;

    if (!m_pressedButton.isEmpty()) {
        handleButtonRelease(m_pressedButton);
        m_pressedButton.clear();
        update();  // Redraw to remove highlight
    }
}

void AmplifierControlWindow::handleButtonPress(const QString& buttonName) {
    if (!m_service || !m_panelController) return;

    LOG_TRACE("AmplifierControl", QString("Button pressed: %1").arg(buttonName));

    // Get command from panel controller (handles amplifier-specific mappings)
    QString command = m_panelController->getButtonCommand(buttonName, m_currentState);

    if (!command.isEmpty()) {
        QString label = m_panelController->getButtonLabel(buttonName);
        LOG_INFO("AmplifierControl", QString("Button '%1' pressed, sending: %2")
            .arg(label).arg(command));
        m_service->sendCommand(command);
    }
}

void AmplifierControlWindow::handleButtonRelease(const QString& buttonName) {
    // Handle button release if needed (for momentary buttons)
    LOG_TRACE("AmplifierControl", QString("Button released: %1").arg(buttonName));
}

QString AmplifierControlWindow::findButtonAtPoint(const QPoint& point) const {
    QPointF proportional = imageToProportional(point);

    for (const auto& button : m_buttonRegions) {
        if (proportional.x() >= button.x &&
            proportional.x() <= button.x + button.width &&
            proportional.y() >= button.y &&
            proportional.y() <= button.y + button.height) {
            return button.name;
        }
    }

    return QString();
}

QRect AmplifierControlWindow::scaleRegionToWidget(double x, double y, double width, double height) const {
    // IMPORTANT: Must use SAME scaling logic as SvgPanelWidget::paintEvent()
    int windowWidth = this->width();
    int windowHeight = this->height();

    // Calculate scaling (same as SvgPanelWidget and scalePointToImage)
    double widthRatio = static_cast<double>(windowWidth) / m_originalImageSize.width();
    double heightRatio = static_cast<double>(windowHeight) / m_originalImageSize.height();
    double scale = qMin(widthRatio, heightRatio);

    int scaledWidth = static_cast<int>(m_originalImageSize.width() * scale);
    int scaledHeight = static_cast<int>(m_originalImageSize.height() * scale);

    // Center offset (same as SvgPanelWidget)
    int offsetX = (windowWidth - scaledWidth) / 2;
    int offsetY = (windowHeight - scaledHeight) / 2;

    // Convert proportional coordinates to widget coordinates
    int rectX = offsetX + static_cast<int>(x * scaledWidth);
    int rectY = offsetY + static_cast<int>(y * scaledHeight);
    int rectWidth = static_cast<int>(width * scaledWidth);
    int rectHeight = static_cast<int>(height * scaledHeight);

    return QRect(rectX, rectY, rectWidth, rectHeight);
}

QPoint AmplifierControlWindow::scalePointToImage(const QPoint& svgWidgetPoint) const {
    // IMPORTANT: Must use SAME scaling logic as SvgPanelWidget::paintEvent()
    // widgetPoint is now relative to m_svgPanel, not AmplifierControlWindow

    // Use SVG widget dimensions (not window dimensions)
    int widgetWidth = m_svgPanel->width();
    int widgetHeight = m_svgPanel->height();

    // Calculate scaling to fit widget while preserving aspect ratio
    // EXACT same logic as SvgPanelWidget::paintEvent()
    double widthRatio = static_cast<double>(widgetWidth) / m_originalImageSize.width();
    double heightRatio = static_cast<double>(widgetHeight) / m_originalImageSize.height();
    double scale = qMin(widthRatio, heightRatio);  // Pick constraining dimension

    int scaledWidth = static_cast<int>(m_originalImageSize.width() * scale);
    int scaledHeight = static_cast<int>(m_originalImageSize.height() * scale);

    // Center the SVG (same as SvgPanelWidget)
    int offsetX = (widgetWidth - scaledWidth) / 2;
    int offsetY = (widgetHeight - scaledHeight) / 2;

    LOG_INFO("AmplifierControl", QString("Scaling: widget=%1x%2, svg=%3x%4, scale=%5, scaled=%6x%7, offset=(%8,%9)")
        .arg(widgetWidth).arg(widgetHeight)
        .arg(m_originalImageSize.width()).arg(m_originalImageSize.height())
        .arg(scale, 0, 'f', 3)
        .arg(scaledWidth).arg(scaledHeight)
        .arg(offsetX).arg(offsetY));

    // Convert widget point to image point
    int imageX = static_cast<int>(((svgWidgetPoint.x() - offsetX) / static_cast<double>(scaledWidth)) * m_originalImageSize.width());
    int imageY = static_cast<int>(((svgWidgetPoint.y() - offsetY) / static_cast<double>(scaledHeight)) * m_originalImageSize.height());

    LOG_INFO("AmplifierControl", QString("Point transform: widget(%1,%2) - offset(%3,%4) = (%5,%6) / scaled(%7x%8) * svg(%9x%10) = image(%11,%12)")
        .arg(svgWidgetPoint.x()).arg(svgWidgetPoint.y())
        .arg(offsetX).arg(offsetY)
        .arg(svgWidgetPoint.x() - offsetX).arg(svgWidgetPoint.y() - offsetY)
        .arg(scaledWidth).arg(scaledHeight)
        .arg(m_originalImageSize.width()).arg(m_originalImageSize.height())
        .arg(imageX).arg(imageY));

    return QPoint(imageX, imageY);
}

QPointF AmplifierControlWindow::imageToProportional(const QPoint& imagePoint) const {
    double x = static_cast<double>(imagePoint.x()) / m_originalImageSize.width();
    double y = static_cast<double>(imagePoint.y()) / m_originalImageSize.height();
    return QPointF(x, y);
}

double AmplifierControlWindow::powerWattsToMeterProportion(int watts) const {
    if (watts <= 0) return 0.0;
    int maxPower = m_panelController ? m_panelController->getMaxPowerWatts() : 1500;
    if (watts >= maxPower) return 1.0;
    return static_cast<double>(watts) / maxPower;
}

double AmplifierControlWindow::swrToMeterProportion(float swr) const {
    if (swr <= MIN_SWR) return 0.0;
    float maxSwr = m_panelController ? m_panelController->getMaxSwrDisplay() : 5.0f;
    if (swr >= maxSwr) return 1.0;
    return (swr - MIN_SWR) / (maxSwr - MIN_SWR);
}

void AmplifierControlWindow::resizeEvent(QResizeEvent* event) {
    LOG_INFO("AmplifierControlWindow", QString("resizeEvent: %1x%2").arg(event->size().width()).arg(event->size().height()));
    QWidget::resizeEvent(event);

    // TEMPORARY: Aspect ratio enforcement disabled to debug flickering issue
    // TODO: Implement more robust aspect ratio enforcement that won't cause event loops

    /*
    // Enforce SVG aspect ratio: adjust height based on width
    // This ensures the window always perfectly fits the panel with no whitespace
    int newWidth = event->size().width();
    int expectedHeight = static_cast<int>(newWidth / m_aspectRatio);

    // Only resize if height doesn't match (with small tolerance to prevent loops)
    const int HEIGHT_TOLERANCE = 2;
    if (!m_isResizing && qAbs(event->size().height() - expectedHeight) > HEIGHT_TOLERANCE) {
        // Guard against infinite recursion: set flag before calling resize()
        // Don't reset flag here - let the next resizeEvent (triggered by resize()) reset it
        m_isResizing = true;
        resize(newWidth, expectedHeight);
        return;  // Will trigger another resizeEvent with correct size
    }

    // If we reach here with m_isResizing=true, we're in the recursive resizeEvent
    // after our resize() call above, so reset the flag and proceed with repositioning
    if (m_isResizing) {
        m_isResizing = false;
    }
    */

    // Debounce overlay repositioning (Windows performance fix)
    // Only reposition after resizing stops for 16ms, not on every resize event
    LOG_INFO("AmplifierControlWindow", "Starting debounce timer");
    m_resizeDebounceTimer->start();
    LOG_INFO("AmplifierControlWindow", "resizeEvent complete");
}

void AmplifierControlWindow::repositionOverlays() {
    LOG_INFO("AmplifierControlWindow", "repositionOverlays START");
    if (!m_disconnectedLabel || !m_testConnectionButton) {
        LOG_INFO("AmplifierControlWindow", "repositionOverlays SKIP (null widgets)");
        return;
    }

    if (!m_connected) {
        // Disconnected: Position overlay in center
        const int OVERLAY_SPACING = 20;
        QSize labelSize = m_disconnectedLabel->sizeHint();
        QSize buttonSize = m_testConnectionButton->sizeHint();

        int totalHeight = labelSize.height() + OVERLAY_SPACING + buttonSize.height();
        int startY = (height() - totalHeight) / 2;

        m_disconnectedLabel->setGeometry(
            (width() - labelSize.width()) / 2,
            startY,
            labelSize.width(),
            labelSize.height()
        );

        m_testConnectionButton->setGeometry(
            (width() - buttonSize.width()) / 2,
            startY + labelSize.height() + OVERLAY_SPACING,
            buttonSize.width(),
            buttonSize.height()
        );
    } else {
        // Connected: Move overlay FAR offscreen so it can't block mouse
        m_disconnectedLabel->setGeometry(-10000, -10000, 1, 1);
        m_testConnectionButton->setGeometry(-10000, -10000, 1, 1);
    }
    LOG_INFO("AmplifierControlWindow", "repositionOverlays END");
}

void AmplifierControlWindow::repositionLcdLabel() {
    LOG_INFO("AmplifierControlWindow", "repositionLcdLabel START");
    if (!m_lcdLabel || !m_svgPanel) {
        LOG_INFO("AmplifierControlWindow", "repositionLcdLabel SKIP (null widgets)");
        return;
    }

    // Cache SVG element bounds (only query once, not on every resize!)
    // This is critical for Windows performance where resize events fire frequently
    if (!m_boundsInitialized) {
        m_cachedLcdBounds = m_svgPanel->getElementBounds("label_MAIN");
        m_cachedViewBox = m_svgPanel->getViewBox();
        m_boundsInitialized = true;

        LOG_TRACE("AmplifierControlWindow", QString("LCD bounds cached: svgBounds=%1,%2 %3x%4, viewBox=%5,%6 %7x%8")
            .arg(m_cachedLcdBounds.x()).arg(m_cachedLcdBounds.y()).arg(m_cachedLcdBounds.width()).arg(m_cachedLcdBounds.height())
            .arg(m_cachedViewBox.x()).arg(m_cachedViewBox.y()).arg(m_cachedViewBox.width()).arg(m_cachedViewBox.height()));
    }

    if (m_cachedLcdBounds.isEmpty() || m_cachedViewBox.isEmpty()) {
        // Fallback: position label at approximate LCD location (proportional to panel)
        QPoint svgPanelPos = m_svgPanel->mapTo(this, QPoint(0, 0));
        int panelW = m_svgPanel->width();
        int panelH = m_svgPanel->height();
        // LCD is roughly at 12% from left, 18% from top, 22% wide, 12% tall
        int lcdX = svgPanelPos.x() + static_cast<int>(panelW * 0.12);
        int lcdY = svgPanelPos.y() + static_cast<int>(panelH * 0.18);
        int lcdW = static_cast<int>(panelW * 0.22);
        int lcdH = static_cast<int>(panelH * 0.12);
        // LOG_TRACE removed: was called on every resize, causing performance issues on Windows
        m_lcdLabel->setGeometry(lcdX, lcdY, lcdW, lcdH);

        // Only update stylesheet if font size changed (setStyleSheet is expensive!)
        int fontSize = qMax(8, lcdH / 3);
        if (fontSize != m_lastLcdFontSize) {
            m_lastLcdFontSize = fontSize;
            m_lcdLabel->setStyleSheet(QString(
                "QLabel { "
                "  background-color: transparent; "
                "  color: %1; "
                "  font-family: 'Courier New', Courier, monospace; "
                "  font-size: %2px; "
                "  font-weight: bold; "
                "}")
                .arg(ThemeManager::instance().colorName(ColorRole::LcdDisplayText))
                .arg(fontSize));
        }
    } else {
        // Convert SVG viewBox coordinates to proportional (using cached bounds)
        double propX = m_cachedLcdBounds.x() / m_cachedViewBox.width();
        double propY = m_cachedLcdBounds.y() / m_cachedViewBox.height();
        double propW = m_cachedLcdBounds.width() / m_cachedViewBox.width();
        double propH = m_cachedLcdBounds.height() / m_cachedViewBox.height();

        // Convert to widget coordinates
        int widgetWidth = m_svgPanel->width();
        int widgetHeight = m_svgPanel->height();
        double widthRatio = static_cast<double>(widgetWidth) / m_originalImageSize.width();
        double heightRatio = static_cast<double>(widgetHeight) / m_originalImageSize.height();
        double scale = qMin(widthRatio, heightRatio);

        int scaledWidth = static_cast<int>(m_originalImageSize.width() * scale);
        int scaledHeight = static_cast<int>(m_originalImageSize.height() * scale);
        int offsetX = (widgetWidth - scaledWidth) / 2;
        int offsetY = (widgetHeight - scaledHeight) / 2;

        int lcdX = offsetX + static_cast<int>(propX * scaledWidth);
        int lcdY = offsetY + static_cast<int>(propY * scaledHeight);
        int lcdW = static_cast<int>(propW * scaledWidth);
        int lcdH = static_cast<int>(propH * scaledHeight);

        // Convert from SVG panel to window coordinates
        QPoint svgPanelPos = m_svgPanel->mapTo(this, QPoint(0, 0));
        // LOG_TRACE removed: was called on every resize, causing performance issues on Windows
        m_lcdLabel->setGeometry(svgPanelPos.x() + lcdX, svgPanelPos.y() + lcdY, lcdW, lcdH);

        // Scale font based on LCD height
        int fontSize = qMax(8, lcdH / 3);
        // Only update stylesheet if font size changed (setStyleSheet is expensive!)
        if (fontSize != m_lastLcdFontSize) {
            m_lastLcdFontSize = fontSize;
            m_lcdLabel->setStyleSheet(QString(
                "QLabel { "
                "  background-color: transparent; "
                "  color: %1; "
                "  font-family: 'Courier New', Courier, monospace; "
                "  font-size: %2px; "
                "  font-weight: bold; "
                "}")
                .arg(ThemeManager::instance().colorName(ColorRole::LcdDisplayText))
                .arg(fontSize));
        }
    }
    LOG_INFO("AmplifierControlWindow", "repositionLcdLabel END");
}

void AmplifierControlWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // Defer all overlay repositioning until after layout is complete
    // resizeEvent has an early return for aspect ratio enforcement,
    // so overlays may not be positioned on first resize (Windows issue #64)
    QTimer::singleShot(50, this, [this]() {
        repositionOverlays();
        repositionLcdLabel();
    });
}

void AmplifierControlWindow::closeEvent(QCloseEvent* event) {
    // Save window geometry
    AppSettings::instance().saveAmplifierControlGeometry(saveGeometry());
    QWidget::closeEvent(event);
}

void AmplifierControlWindow::onAmplifierStateUpdated(const AmplifierState& state) {
    // Preserve last valid frequency if incoming frequency is 0
    // (avoids brief 0.00MHz display during band changes)
    freq_t preservedFrequency = m_currentState.frequency;
    m_currentState = state;
    if (state.frequency == 0 && preservedFrequency > 0) {
        m_currentState.frequency = preservedFrequency;
    }

    // Update SVG panel LEDs based on amplifier state using panel controller
    if (m_svgPanel && m_panelController) {
        // CRITICAL: Use batch mode to avoid 29+ renderer reloads per update
        // Without batching: 19 LEDs + 10 SWR LEDs = 29 reloads every 250ms (116/sec)
        // With batching: 1 reload every 250ms (4/sec) - 29X performance improvement
        m_svgPanel->beginBatch();

        // Update all LEDs based on state via panel controller
        for (const QString& ledId : m_panelController->getLedIds()) {
            bool isOn = m_panelController->getLedState(ledId, state);
            if (isOn) {
                QColor color = m_panelController->getLedOnColor(ledId);
                m_svgPanel->setLedOn(ledId, color);
            } else {
                m_svgPanel->setLedOff(ledId);
            }
        }

        // SWR meter (handled separately for bargraph animation)
        updateSwrMeter(state.swr);

        // TODO: Power meter when LED IDs are renamed in SVG
        // updatePowerMeter(state.forwardPowerWatts);

        // End batch mode - renderer reloads ONCE for all LED updates
        m_svgPanel->endBatch();
    }

    // Update LCD label text from actual amplifier LCD content (^DS; command)
    if (m_lcdLabel) {
        // If label is still offscreen, trigger repositioning directly
        // (calling resizeEvent can hit early return due to aspect ratio check)
        if (m_lcdLabel->x() < 0) {
            repositionLcdLabel();
        }

        if (!m_currentState.lcdLine1.isEmpty() || !m_currentState.lcdLine2.isEmpty()) {
            // Use actual LCD content from amplifier
            QString newContent = m_currentState.lcdLine1 + "\n" + m_currentState.lcdLine2;
            if (newContent != m_lastLcdContent) {
                LOG_INFO("AmplifierControlWindow", QString("LCD from ^DS: '%1' / '%2' (label geom: %3,%4 %5x%6)")
                    .arg(m_currentState.lcdLine1, m_currentState.lcdLine2)
                    .arg(m_lcdLabel->x()).arg(m_lcdLabel->y())
                    .arg(m_lcdLabel->width()).arg(m_lcdLabel->height()));
                m_lastLcdContent = newContent;
            }
            m_lcdLabel->setText(newContent);
        } else {
            // Fallback: show computed values if no LCD content available
            double freqMhz = m_currentState.frequency / 1000000.0;
            QString line1 = QString("F:%1W R:%2W")
                .arg(m_currentState.forwardPowerWatts, 4)
                .arg(m_currentState.reflectedPowerWatts, 3);
            QString line2 = QString("%1MHz %2C")
                .arg(freqMhz, 0, 'f', 2)
                .arg(m_currentState.temperature);
            m_lcdLabel->setText(line1 + "\n" + line2);
        }
    }

    // Debounce update() calls - don't flood event queue with paint requests
    // Timer restarts on each update, only fires after 50ms of no updates (max 20 repaints/sec)
    m_updateDebounceTimer->start();
}

// Initialize all LEDs to OFF state
void AmplifierControlWindow::initializeLedStates() {
    if (!m_svgPanel || !m_panelController) return;

    LOG_INFO("AmplifierControlWindow", "Initializing all LEDs to OFF state");

    // Set all LEDs to OFF
    for (const QString& ledId : m_panelController->getLedIds()) {
        m_svgPanel->setLedOff(ledId);
    }

    // Also turn off SWR meter LEDs
    for (const QString& ledId : m_panelController->getSwrMeterLedIds()) {
        m_svgPanel->setLedOff(ledId);
    }
}

// Helper method: Update SWR meter LEDs
void AmplifierControlWindow::updateSwrMeter(float swr) {
    if (!m_svgPanel || !m_panelController) return;

    // Get SWR meter LED IDs from panel controller
    QStringList swrLedIds = m_panelController->getSwrMeterLedIds();
    int numLeds = swrLedIds.size();
    if (numLeds == 0) return;

    // Only show SWR when transmitting (swr > 0 and swr >= 1.0)
    // When not transmitting, swr is typically 0 or undefined
    int ledsToLight = 0;
    if (swr >= MIN_SWR && m_currentState.forwardPowerWatts > 0) {
        // Calculate how many LEDs to light based on SWR
        float maxSwr = m_panelController->getMaxSwrDisplay();
        float swrRange = swr - MIN_SWR;  // 0.0 to maxSwr-1
        ledsToLight = static_cast<int>((swrRange / (maxSwr - MIN_SWR)) * numLeds);
        ledsToLight = qBound(0, ledsToLight, numLeds);
    }

    for (int i = 0; i < numLeds; i++) {
        const QString& ledId = swrLedIds[i];

        if (i < ledsToLight) {
            // Get LED color from gradient LUT based on position (green→yellow→red)
            QColor color = m_panelController->getSwrLedColor(i, numLeds);
            m_svgPanel->setLedOn(ledId, color);
        } else {
            m_svgPanel->setLedOff(ledId);
        }
    }
}

void AmplifierControlWindow::onConnectionStatusChanged(bool connected) {
    LOG_INFO("AmplifierControlWindow", QString("Connection status changed: %1").arg(connected ? "connected" : "disconnected"));
    m_connected = connected;
    m_disconnectedLabel->setVisible(!connected);
    m_testConnectionButton->setVisible(!connected);

    // Make overlay widgets transparent to mouse events when connected
    m_disconnectedLabel->setAttribute(Qt::WA_TransparentForMouseEvents, connected);
    m_testConnectionButton->setAttribute(Qt::WA_TransparentForMouseEvents, connected);

    // Reposition overlays (centers when disconnected, offscreen when connected)
    repositionOverlays();

    update();
}

void AmplifierControlWindow::onTestConnection() {
    if (!m_service) return;

    LOG_INFO("AmplifierControl", "User requested connection test");

    // Load amplifier config from settings
    AppSettings& settings = AppSettings::instance();
    int modelId = settings.getAmplifierModel();
    QString connectionType = settings.getAmplifierConnectionType();

    AmplifierConfig config;
    config.hamlibModelId = modelId;
    config.connectionType = connectionType;
    config.port = settings.getAmplifierPort();
    config.baudRate = settings.getAmplifierBaudRate();
    config.pollIntervalMs = settings.getAmplifierPollInterval();

    // Determine amplifier type
    int amplifierType;
    const int AMP_MODEL_ELECRAFT_KPA1500 = 1201;
    if (connectionType == "direct" && modelId == AMP_MODEL_ELECRAFT_KPA1500) {
        amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::KPA1500_DIRECT);
    } else {
        amplifierType = static_cast<int>(AmplifierFactory::AmplifierType::HAMLIB);
    }

    // Attempt to connect (async - result via connectionStatusChanged signal)
    m_service->connectToAmplifier(amplifierType, config);
    LOG_INFO("AmplifierControl", "Connection test initiated (async)");
}

} // namespace TR4QT
