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

#include "SvgPanelWidget.h"
#include "../../utils/ThemeManager.h"
#include "../../logging/LogMacros.h"
#include <QPainter>
#include <QFile>
#include <QDomElement>
#include <QMutexLocker>
#include <QRegularExpression>

namespace TR4QT {

SvgPanelWidget::SvgPanelWidget(const QString& svgPath, QWidget* parent)
    : QWidget(parent)
    , m_svgRenderer(nullptr)
    , m_resizeTimer(new QTimer(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(400, 150);

    // Configure resize debounce timer (16ms = ~60fps, only repaint after resizing stops)
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(16);
    connect(m_resizeTimer, &QTimer::timeout, this, [this]() {
        update();  // Repaint SVG after resize completes
    });

    if (!loadSvg(svgPath)) {
        LOG_ERROR("SvgPanelWidget", QString("Failed to load SVG: %1").arg(svgPath));
    }
}

SvgPanelWidget::~SvgPanelWidget() {
    delete m_svgRenderer;
}

bool SvgPanelWidget::loadSvg(const QString& svgPath) {
    // Load SVG file
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("SvgPanelWidget", QString("Cannot open SVG file: %1").arg(svgPath));
        return false;
    }

    m_originalSvgData = file.readAll();
    file.close();

    // Parse into DOM
    QString errorMsg;
    int errorLine, errorColumn;
    if (!m_svgDom.setContent(m_originalSvgData, &errorMsg, &errorLine, &errorColumn)) {
        LOG_ERROR("SvgPanelWidget", QString("SVG parse error at line %1, col %2: %3")
            .arg(errorLine).arg(errorColumn).arg(errorMsg));
        return false;
    }

    // Verify DOM was parsed successfully
    QDomElement docElem = m_svgDom.documentElement();
    if (docElem.isNull()) {
        LOG_ERROR("SvgPanelWidget", "DOM document element is null after parsing");
        return false;
    }

    // Create renderer
    m_svgRenderer = new QSvgRenderer(m_originalSvgData, this);
    if (!m_svgRenderer->isValid()) {
        LOG_ERROR("SvgPanelWidget", "SVG renderer initialization failed");
        delete m_svgRenderer;
        m_svgRenderer = nullptr;
        return false;
    }

    m_svgSize = m_svgRenderer->defaultSize();
    LOG_INFO("SvgPanelWidget", QString("Loaded SVG: %1 (%2x%3)")
        .arg(svgPath).arg(m_svgSize.width()).arg(m_svgSize.height()));

    return true;
}

void SvgPanelWidget::setLedOn(const QString& elementId, const QColor& colorOn) {
    updateElementFill(elementId, colorOn);
    m_elementColors[elementId] = colorOn;
    // If in batch mode, skip update() (will update once at endBatch)
    if (!m_batchMode) {
        update();  // Trigger repaint
    }
}

void SvgPanelWidget::setLedOff(const QString& elementId) {
    // Set fill to "none" for transparent background when LED is off
    updateElementFillWithValue(elementId, "none");
    m_elementColors.remove(elementId);
    // If in batch mode, skip update() (will update once at endBatch)
    if (!m_batchMode) {
        update();
    }
}

void SvgPanelWidget::setElementColor(const QString& elementId, const QColor& color) {
    updateElementFill(elementId, color);
    m_elementColors[elementId] = color;
    update();
}

void SvgPanelWidget::resetAllColors() {
    QMutexLocker locker(&m_mutex);

    // Reload original SVG (resets all colors)
    m_svgDom.setContent(m_originalSvgData);
    m_elementColors.clear();
    m_elementCache.clear();      // Clear element cache (DOM was reloaded)
    // Don't clear m_missingElements - those IDs still don't exist
    reloadRenderer();
    update();
}

void SvgPanelWidget::beginBatch() {
    m_batchMode = true;
}

void SvgPanelWidget::endBatch() {
    if (!m_batchMode) return;

    m_batchMode = false;

    // Reload renderer once for all batched updates
    QMutexLocker locker(&m_mutex);
    reloadRenderer();
    update();
}

QDomElement SvgPanelWidget::findElementById(const QString& elementId) {
    // Check cache first
    if (m_elementCache.contains(elementId)) {
        return m_elementCache[elementId];
    }

    // Check if we've already determined this element doesn't exist
    if (m_missingElements.contains(elementId)) {
        return QDomElement();  // Return null element
    }

    // Search all elements for matching id attribute
    // IMPORTANT: Must search from documentElement(), not from QDomDocument directly
    // Qt quirk: elementsByTagName("*") doesn't work with namespaced SVG
    // So we search specific element types that might have IDs
    QDomElement docElem = m_svgDom.documentElement();

    QDomNodeList rectElements = docElem.elementsByTagName("rect");
    QDomNodeList pathElements = docElem.elementsByTagName("path");
    QDomNodeList circleElements = docElem.elementsByTagName("circle");
    QDomNodeList textElements = docElem.elementsByTagName("text");

    // Search each element type
    QList<QDomNodeList> elementLists = {rectElements, pathElements, circleElements, textElements};
    for (const QDomNodeList& elements : elementLists) {
        for (int i = 0; i < elements.count(); ++i) {
            QDomElement elem = elements.at(i).toElement();
            QString foundId = elem.attribute("id");
            if (!foundId.isEmpty() && foundId == elementId) {
                // Found it - cache for future use
                m_elementCache[elementId] = elem;
                return elem;
            }
        }
    }

    // Not found - cache as missing to avoid repeated searches
    m_missingElements.insert(elementId);
    LOG_WARN("SvgPanelWidget", QString("SVG element not found: %1 (will not warn again)").arg(elementId));
    return QDomElement();
}

void SvgPanelWidget::updateElementFill(const QString& elementId, const QColor& color) {
    updateElementFillWithValue(elementId, color.name());
}

void SvgPanelWidget::updateElementFillWithValue(const QString& elementId, const QString& fillValue) {
    QMutexLocker locker(&m_mutex);

    // Find element by ID using cached search
    QDomElement element = findElementById(elementId);
    if (element.isNull()) {
        return;  // Element doesn't exist, already logged warning once
    }

    // Update fill attribute
    element.setAttribute("fill", fillValue);

    // Also update style attribute if it contains fill
    QString style = element.attribute("style");
    if (style.contains("fill:")) {
        // Replace fill value in style
        QRegularExpression fillRegex("fill:\\s*[^;]+");
        style.replace(fillRegex, QString("fill:%1").arg(fillValue));
        element.setAttribute("style", style);
    }

    // If in batch mode, skip renderer reload (will reload once at endBatch)
    if (m_batchMode) {
        return;
    }

    // Reload renderer with modified DOM
    reloadRenderer();
}

void SvgPanelWidget::reloadRenderer() {
    // Caller must hold m_mutex

    if (!m_svgRenderer) return;

    // Convert DOM to QByteArray
    QByteArray svgData = m_svgDom.toByteArray();

    // Reload renderer
    delete m_svgRenderer;
    m_svgRenderer = new QSvgRenderer(svgData, this);

    if (!m_svgRenderer->isValid()) {
        LOG_ERROR("SvgPanelWidget", "Failed to reload SVG renderer after DOM update");
    }
}

QSize SvgPanelWidget::getSvgSize() const {
    if (m_svgRenderer && m_svgRenderer->isValid()) {
        QSize svgSize = m_svgRenderer->defaultSize();
        if (!svgSize.isEmpty()) {
            return svgSize;
        }
    }
    return m_svgSize;  // Fallback to cached size
}

QRectF SvgPanelWidget::getElementBounds(const QString& elementId) const {
    if (m_svgRenderer && m_svgRenderer->isValid() && m_svgRenderer->elementExists(elementId)) {
        return m_svgRenderer->boundsOnElement(elementId);
    }
    return QRectF();  // Empty rect if not found
}

bool SvgPanelWidget::hasElement(const QString& elementId) const {
    return m_svgRenderer && m_svgRenderer->isValid() && m_svgRenderer->elementExists(elementId);
}

QRectF SvgPanelWidget::getViewBox() const {
    if (m_svgRenderer && m_svgRenderer->isValid()) {
        return m_svgRenderer->viewBoxF();
    }
    return QRectF();
}

void SvgPanelWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_svgRenderer || !m_svgRenderer->isValid()) {
        // Draw placeholder if SVG not loaded
        painter.fillRect(rect(), ThemeManager::instance().color(ColorRole::SvgPanelBackground));
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "SVG Panel\n(No image loaded)");
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Calculate scaling to fit widget while preserving aspect ratio
    QSize svgSize = m_svgRenderer->defaultSize();
    if (svgSize.isEmpty()) {
        svgSize = m_svgSize;  // Use cached size
    }

    double widthRatio = static_cast<double>(width()) / svgSize.width();
    double heightRatio = static_cast<double>(height()) / svgSize.height();
    double scale = qMin(widthRatio, heightRatio);

    int scaledWidth = static_cast<int>(svgSize.width() * scale);
    int scaledHeight = static_cast<int>(svgSize.height() * scale);

    // Center the SVG
    int x = (width() - scaledWidth) / 2;
    int y = (height() - scaledHeight) / 2;

    // Render SVG
    m_svgRenderer->render(&painter, QRectF(x, y, scaledWidth, scaledHeight));
}

void SvgPanelWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // Debounce repaints: only update after resizing stops (Windows performance fix)
    // Restart timer on each resize event, only fires after 16ms of no resizing
    m_resizeTimer->start();
}

} // namespace TR4QT
