#include "AmplifierControlWindow.h"
#include "../widgets/SvgPanelWidget.h"
#include "../../services/AmplifierService.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include <QPainter>
#include <QMouseEvent>
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
    setWindowTitle("Amplifier Control");
    setMinimumSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

    // Create SVG panel widget with KPA1500 front panel
    QString svgPath = QApplication::applicationDirPath() + "/../../../resources/images/kpa1500_panel.svg";
    m_svgPanel = new SvgPanelWidget(svgPath, this);

    // Fallback: try absolute path if relative path fails (for development)
    if (!m_svgPanel->isValid()) {
        delete m_svgPanel;
        svgPath = "/Users/toms/projects/TR4QT/resources/images/kpa1500_panel.svg";
        m_svgPanel = new SvgPanelWidget(svgPath, this);
    }

    // Layout: SVG panel fills the entire window
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_svgPanel);
    setLayout(layout);

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
    m_disconnectedLabel->setStyleSheet(
        "QLabel { "
        "  background-color: rgba(0, 0, 0, 180); "
        "  color: #ff6600; "
        "  font-size: 18pt; "
        "  font-weight: bold; "
        "  padding: 20px; "
        "  border: 2px solid #ff6600; "
        "}"
    );

    m_testConnectionButton = new QPushButton("Test Connection", this);
    m_testConnectionButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #006600; "
        "  color: white; "
        "  font-size: 12pt; "
        "  padding: 10px 20px; "
        "  border: 2px solid #00aa00; "
        "  border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #008800; "
        "}"
    );

    connect(m_testConnectionButton, &QPushButton::clicked,
            this, &AmplifierControlWindow::onTestConnection);

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
    // All coordinates are proportional to image dimensions (0.0 = left/top, 1.0 = right/bottom)
    // These values are measured from the KPA1500 front panel SVG
    // Button names match SVG element IDs (btn_*)

    // ON/OFF button (right side of front panel)
    m_buttonRegions.append({
        "btn_ON_OFF",
        0.82,   // x: 82% from left
        0.60,   // y: 60% from top
        0.08,   // width: 8% of image width
        0.25,   // height: 25% of image height
        "Toggle Operate/Standby mode",
        true    // is toggle button
    });

    // RESET/INFO button
    m_buttonRegions.append({
        "btn_RESET_INFO",
        0.42,   // x position
        0.40,   // y position
        0.06,   // width
        0.15,   // height
        "Reset fault / Show info",
        false   // momentary button
    });

    // MODE button
    m_buttonRegions.append({
        "btn_MODE",
        0.50,   // x position
        0.40,
        0.06,
        0.15,
        "Change operating mode",
        false
    });

    // ANTENNA button
    m_buttonRegions.append({
        "btn_ANTENNA",
        0.58,   // x position
        0.40,
        0.06,
        0.15,
        "Select antenna",
        false
    });

    // AUX button
    m_buttonRegions.append({
        "btn_AUX",
        0.66,   // x position
        0.40,
        0.06,
        0.15,
        "Auxiliary function",
        false
    });

    // Band buttons - use actual SVG element bounds for hit detection
    QStringList bandButtonIds = {
        "btn_160m", "btn_80m", "btn_40m", "btn_20m", "btn_15m", "btn_10m",
        "btn_60m", "btn_30m", "btn_17m", "btn_12m", "btn_6m"
    };

    QStringList bandLabels = {
        "160m", "80m", "40m", "20m", "15m", "10m",
        "60m", "30m", "17m", "12m", "6m"
    };

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

    for (int i = 0; i < bandButtonIds.size(); ++i) {
        QString buttonId = bandButtonIds[i];
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

        LOG_INFO("AmplifierControl", QString("Button %1: viewBox(%2,%3 %4x%5) -> proportional(%6,%7 %8x%9)")
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
            bandLabels[i],
            false
        });
    }
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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Calculate scaled image rectangle maintaining aspect ratio
    QRect targetRect = rect();
    int scaledWidth = targetRect.width();
    int scaledHeight = static_cast<int>(scaledWidth / m_aspectRatio);

    if (scaledHeight > targetRect.height()) {
        // Height constrained - scale by height
        scaledHeight = targetRect.height();
        scaledWidth = static_cast<int>(scaledHeight * m_aspectRatio);
    }

    // Center the image in the window
    int offsetX = (targetRect.width() - scaledWidth) / 2;
    int offsetY = (targetRect.height() - scaledHeight) / 2;
    QRect scaledRect(offsetX, offsetY, scaledWidth, scaledHeight);

    // Draw front panel image
    painter.drawPixmap(scaledRect, m_frontPanelImage);

    // Only draw live elements if connected
    if (m_connected) {
        // Set clipping to image area
        painter.setClipRect(scaledRect);

        // Draw live meters and displays
        drawPowerMeter(painter);
        drawSwrMeter(painter);
        drawLcdDisplay(painter);
        drawLedIndicators(painter);

        // Draw button overlays if using placeholder image (so buttons are visible)
        if (m_frontPanelImage.size() == QSize(1200, 400)) {  // Placeholder dimensions
            drawButtonOverlays(painter);
        }

        // Draw button highlight if button is pressed
        if (!m_pressedButton.isEmpty()) {
            drawButtonHighlight(painter, m_pressedButton);
        }
    }
}

void AmplifierControlWindow::drawPowerMeter(QPainter& painter) {
    // Find power meter display region
    const DisplayRegion* meterRegion = nullptr;
    for (const auto& region : m_displayRegions) {
        if (region.name == "power_meter") {
            meterRegion = &region;
            break;
        }
    }
    if (!meterRegion) return;

    QRect meterRect = scaleRegionToWidget(meterRegion->x, meterRegion->y,
                                          meterRegion->width, meterRegion->height);

    // Calculate meter fill proportion
    double fillProportion = powerWattsToMeterProportion(m_currentState.forwardPowerWatts);

    // Determine color based on power level
    QColor meterColor;
    if (fillProportion < POWER_GREEN_THRESHOLD) {
        meterColor = QColor(0, 255, 0);  // Green
    } else if (fillProportion < POWER_YELLOW_THRESHOLD) {
        meterColor = QColor(255, 200, 0);  // Yellow
    } else {
        meterColor = QColor(255, 0, 0);  // Red
    }

    // Draw meter bar
    int fillWidth = static_cast<int>(meterRect.width() * fillProportion);
    QRect fillRect(meterRect.x(), meterRect.y(), fillWidth, meterRect.height());

    painter.fillRect(fillRect, meterColor);

    // Draw power value text
    QFont font = painter.font();
    font.setFamily("Courier");  // Monospaced font similar to LCD
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    QString powerText = QString("%1 W").arg(m_currentState.forwardPowerWatts);
    painter.drawText(meterRect, Qt::AlignCenter, powerText);
}

void AmplifierControlWindow::drawSwrMeter(QPainter& painter) {
    const DisplayRegion* meterRegion = nullptr;
    for (const auto& region : m_displayRegions) {
        if (region.name == "swr_meter") {
            meterRegion = &region;
            break;
        }
    }
    if (!meterRegion) return;

    QRect meterRect = scaleRegionToWidget(meterRegion->x, meterRegion->y,
                                          meterRegion->width, meterRegion->height);

    // Calculate meter fill proportion
    double fillProportion = swrToMeterProportion(m_currentState.swr);

    // Determine color based on SWR
    QColor meterColor;
    if (m_currentState.swr < SWR_GREEN_THRESHOLD) {
        meterColor = QColor(0, 255, 0);  // Green
    } else if (m_currentState.swr < SWR_YELLOW_THRESHOLD) {
        meterColor = QColor(255, 200, 0);  // Yellow
    } else {
        meterColor = QColor(255, 0, 0);  // Red
    }

    // Draw meter bar
    int fillWidth = static_cast<int>(meterRect.width() * fillProportion);
    QRect fillRect(meterRect.x(), meterRect.y(), fillWidth, meterRect.height());

    painter.fillRect(fillRect, meterColor);

    // Draw SWR value text
    QFont font = painter.font();
    font.setFamily("Courier");
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    QString swrText = QString("%1:1").arg(m_currentState.swr, 0, 'f', 1);
    painter.drawText(meterRect, Qt::AlignCenter, swrText);
}

void AmplifierControlWindow::drawLcdDisplay(QPainter& painter) {
    const DisplayRegion* lcdRegion = nullptr;
    for (const auto& region : m_displayRegions) {
        if (region.name == "lcd") {
            lcdRegion = &region;
            break;
        }
    }
    if (!lcdRegion) return;

    QRect lcdRect = scaleRegionToWidget(lcdRegion->x, lcdRegion->y,
                                        lcdRegion->width, lcdRegion->height);

    // Draw LCD background (cyan/blue-green typical of KPA1500 LCD)
    painter.fillRect(lcdRect, QColor(100, 180, 180));

    // Use monospaced font to emulate LCD characters
    QFont lcdFont;
    lcdFont.setFamily("Courier");  // Monospaced
    lcdFont.setPointSize(14);
    lcdFont.setBold(true);
    painter.setFont(lcdFont);
    painter.setPen(QColor(0, 0, 50));  // Dark blue text on cyan background

    // Format display text (2 lines like KPA1500)
    // Line 1: Forward power and reflected power
    QString line1 = QString("F:%1W  R:%2W")
        .arg(m_currentState.forwardPowerWatts, 4)
        .arg(m_currentState.reflectedPowerWatts, 3);

    // Line 2: Frequency and temperature
    double freqMhz = m_currentState.frequency / 1000000.0;
    QString line2 = QString("%1MHz   %2C")
        .arg(freqMhz, 0, 'f', 2)
        .arg(m_currentState.temperature);

    // Draw text with padding
    const int LCD_PADDING = 5;
    QRect line1Rect = lcdRect.adjusted(LCD_PADDING, LCD_PADDING, -LCD_PADDING, -lcdRect.height()/2);
    QRect line2Rect = lcdRect.adjusted(LCD_PADDING, lcdRect.height()/2, -LCD_PADDING, -LCD_PADDING);

    painter.drawText(line1Rect, Qt::AlignLeft | Qt::AlignVCenter, line1);
    painter.drawText(line2Rect, Qt::AlignLeft | Qt::AlignVCenter, line2);
}

void AmplifierControlWindow::drawLedIndicators(QPainter& painter) {
    // Update LED states based on amplifier state
    for (auto& led : m_ledIndicators) {
        if (led.name == "oper") {
            led.isActive = m_currentState.operateMode;
        } else if (led.name == "stby") {
            led.isActive = !m_currentState.operateMode;
        } else if (led.name == "fault") {
            led.isActive = m_currentState.faultDetected;
        }
    }

    // Draw all LEDs
    for (const auto& led : m_ledIndicators) {
        QRect ledRect = scaleRegionToWidget(led.x, led.y, led.size, led.size);

        // Make LED circular
        painter.setRenderHint(QPainter::Antialiasing, true);
        QColor color = led.isActive ? led.activeColor : led.inactiveColor;
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(ledRect);

        // Add glow effect if active
        if (led.isActive) {
            const int GLOW_ITERATIONS = 3;
            for (int i = 1; i <= GLOW_ITERATIONS; i++) {
                QColor glowColor = led.activeColor;
                glowColor.setAlpha(80 / i);  // Fade out
                int expand = i * 2;
                QRect glowRect = ledRect.adjusted(-expand, -expand, expand, expand);
                painter.setBrush(glowColor);
                painter.drawEllipse(glowRect);
            }
        }
    }
}

void AmplifierControlWindow::drawButtonOverlays(QPainter& painter) {
    // Draw visible button overlays on placeholder background
    for (const auto& button : m_buttonRegions) {
        QRect buttonRect = scaleRegionToWidget(button.x, button.y, button.width, button.height);

        // Draw button background
        painter.fillRect(buttonRect, QColor(60, 60, 60, 200));

        // Draw button border
        painter.setPen(QPen(QColor(100, 100, 100), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(buttonRect);

        // Draw button label
        QFont font = painter.font();
        font.setPointSize(8);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(Qt::white);

        // Extract short label from button name
        QString label = button.name;
        if (label == "operate_standby") label = "OPER/\nSTBY";
        else if (label == "reset") label = "RESET";
        else if (label == "mode") label = "MODE";
        else if (label == "antenna") label = "ANT";
        else if (label == "atu_tune") label = "ATU\nTUNE";
        else if (label.startsWith("band_")) label = label.mid(5) + "m";

        painter.drawText(buttonRect, Qt::AlignCenter, label);
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
    if (!m_service) return;

    LOG_TRACE("AmplifierControl", QString("Button pressed: %1").arg(buttonName));

    // Map button presses to amplifier commands
    if (buttonName == "btn_ON_OFF") {
        // Toggle between operate and standby
        if (m_currentState.operateMode) {
            m_service->sendCommand("^OS0;");  // Go to standby
        } else {
            m_service->sendCommand("^OS1;");  // Go to operate
        }
    } else if (buttonName == "btn_RESET_INFO") {
        m_service->sendCommand("^RS;");  // Reset fault
    } else if (buttonName.startsWith("btn_")) {
        // Map band buttons to KPA1500 band numbers
        // KPA1500 protocol: ^BN00; = 160m, ^BN01; = 80m, etc.
        QString bandCommand;
        if (buttonName == "btn_160m") {
            bandCommand = "^BN00;";  // 160m
        } else if (buttonName == "btn_80m") {
            bandCommand = "^BN01;";  // 80m
        } else if (buttonName == "btn_60m") {
            bandCommand = "^BN02;";  // 60m
        } else if (buttonName == "btn_40m") {
            bandCommand = "^BN03;";  // 40m
        } else if (buttonName == "btn_30m") {
            bandCommand = "^BN04;";  // 30m
        } else if (buttonName == "btn_20m") {
            bandCommand = "^BN05;";  // 20m
        } else if (buttonName == "btn_17m") {
            bandCommand = "^BN06;";  // 17m
        } else if (buttonName == "btn_15m") {
            bandCommand = "^BN07;";  // 15m
        } else if (buttonName == "btn_12m") {
            bandCommand = "^BN08;";  // 12m
        } else if (buttonName == "btn_10m") {
            bandCommand = "^BN09;";  // 10m
        } else if (buttonName == "btn_6m") {
            bandCommand = "^BN10;";  // 6m
        }

        if (!bandCommand.isEmpty()) {
            LOG_INFO("AmplifierControl", QString("Band button pressed: %1, sending: %2").arg(buttonName).arg(bandCommand));
            m_service->sendCommand(bandCommand);
        }
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
    if (watts >= MAX_POWER_WATTS) return 1.0;
    return static_cast<double>(watts) / MAX_POWER_WATTS;
}

double AmplifierControlWindow::swrToMeterProportion(float swr) const {
    if (swr <= MIN_SWR) return 0.0;
    if (swr >= MAX_SWR_DISPLAY) return 1.0;
    return (swr - MIN_SWR) / (MAX_SWR_DISPLAY - MIN_SWR);
}

void AmplifierControlWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Reposition disconnected overlay widgets
    if (m_disconnectedLabel && m_testConnectionButton) {
        if (!m_connected) {
            // Connected: Position overlay in center
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
            // Disconnected: Move overlay FAR offscreen so it can't block mouse
            m_disconnectedLabel->setGeometry(-10000, -10000, 1, 1);
            m_testConnectionButton->setGeometry(-10000, -10000, 1, 1);
        }
    }
}

void AmplifierControlWindow::closeEvent(QCloseEvent* event) {
    // Save window geometry
    AppSettings::instance().saveAmplifierControlGeometry(saveGeometry());
    QWidget::closeEvent(event);
}

void AmplifierControlWindow::onAmplifierStateUpdated(const AmplifierState& state) {
    m_currentState = state;

    // Update SVG panel LEDs based on amplifier state
    if (m_svgPanel) {
        // Operate/Standby LEDs
        if (state.operateMode) {
            m_svgPanel->setLedOn("led_OPER", QColor("#00ff00"));  // Green
            m_svgPanel->setLedOff("led_STBY");
        } else {
            m_svgPanel->setLedOff("led_OPER");
            m_svgPanel->setLedOn("led_STBY", QColor("#ffaa00"));  // Amber
        }

        // Fault LED
        if (state.faultDetected) {
            m_svgPanel->setLedOn("led_FAULT", QColor("#ff0000"));  // Red
        } else {
            m_svgPanel->setLedOff("led_FAULT");
        }

        // TX indicator (example - amplifier doesn't report this directly)
        // m_svgPanel->setLedOn("led_TX", QColor("#ff0000"));

        // SWR meter (10 LEDs: led_SWR_1 through led_SWR_10)
        updateSwrMeter(state.swr);

        // TODO: Power meter when LED IDs are renamed in SVG
        // updatePowerMeter(state.forwardPowerWatts);
    }

    update();  // Trigger repaint
}

// Helper method: Update SWR meter LEDs
void AmplifierControlWindow::updateSwrMeter(float swr) {
    if (!m_svgPanel) return;

    // SWR ranges: 1.0-1.5 (green), 1.5-2.5 (yellow), 2.5+ (red)
    const int NUM_SWR_LEDS = 10;

    // Calculate how many LEDs to light based on SWR
    // SWR 1.0 = 0 LEDs, SWR 3.0+ = 10 LEDs
    float swrRange = swr - 1.0f;  // 0.0 to 2.0+ (1.0=perfect, 3.0=bad)
    int ledsToLight = static_cast<int>((swrRange / 2.0f) * NUM_SWR_LEDS);
    ledsToLight = qBound(0, ledsToLight, NUM_SWR_LEDS);

    for (int i = 1; i <= NUM_SWR_LEDS; i++) {
        QString ledId = QString("led_SWR_%1").arg(i);

        if (i <= ledsToLight) {
            // Determine LED color based on SWR level
            QColor color;
            if (swr < 1.5f) {
                color = QColor("#00ff00");  // Green (SWR 1.0-1.5)
            } else if (swr < 2.5f) {
                color = QColor("#ffff00");  // Yellow (SWR 1.5-2.5)
            } else {
                color = QColor("#ff0000");  // Red (SWR 2.5+)
            }
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

    update();
}

void AmplifierControlWindow::onTestConnection() {
    if (!m_service) return;

    LOG_INFO("AmplifierControl", "User requested connection test");

    // Load amplifier config from settings
    AppSettings& settings = AppSettings::instance();
    AmplifierConfig config;
    config.hamlibModelId = settings.getAmplifierModel();
    config.connectionType = settings.getAmplifierConnectionType();
    config.port = settings.getAmplifierPort();
    config.baudRate = settings.getAmplifierBaudRate();

    // Attempt to connect
    bool connected = m_service->connectToAmplifier(config);

    if (connected) {
        LOG_INFO("AmplifierControl", "Connection test successful");
    } else {
        LOG_WARN("AmplifierControl", "Connection test failed");
    }
}

} // namespace TR4QT
