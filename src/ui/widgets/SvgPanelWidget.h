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

#ifndef SVGPANELWIDGET_H
#define SVGPANELWIDGET_H

#include <QWidget>
#include <QSvgRenderer>
#include <QDomDocument>
#include <QColor>
#include <QHash>
#include <QSet>
#include <QMutex>
#include "../../core/Constants.h"

namespace TR4QT {

/**
 * @brief Widget for displaying and controlling an SVG front panel with runtime element manipulation
 *
 * Supports changing individual element colors (LEDs, buttons, etc.) by ID without
 * modifying the original SVG file. Uses QDomDocument to manipulate SVG XML and
 * QSvgRenderer for rendering.
 *
 * Example usage:
 *   SvgPanelWidget *panel = new SvgPanelWidget(":/svg/kpa1500_panel.svg");
 *   panel->setLedOn("led_pwr_01", QColor("#00ff00"));  // Turn LED green
 *   panel->setLedOff("led_pwr_01");                     // Turn LED off (dark gray)
 *
 * SVG Requirements:
 *   - All controllable elements must have unique IDs
 *   - LEDs: led_pwr_01, led_pwr_02, ..., led_swr_01, led_fault, led_oper, etc.
 *   - Buttons: btn_status, btn_menu, btn_edit, btn_band_1_8, etc.
 *   - LCD: lcd_main
 *   - Default "off" state should use dark gray fill (#202020)
 */
class SvgPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit SvgPanelWidget(const QString& svgPath, QWidget* parent = nullptr);
    ~SvgPanelWidget() override;

    /**
     * Turn on an LED or highlight an element with a specific color
     * @param elementId SVG element ID (e.g., "led_pwr_01")
     * @param colorOn Color when "on" (default: bright green #00ff00)
     */
    void setLedOn(const QString& elementId, const QColor& colorOn = QColor("#00ff00"));

    /**
     * Turn off an LED or unhighlight an element (dark gray)
     * @param elementId SVG element ID
     */
    void setLedOff(const QString& elementId);

    /**
     * Set element color directly (does not track on/off state)
     * @param elementId SVG element ID
     * @param color Color to apply
     */
    void setElementColor(const QString& elementId, const QColor& color);

    /**
     * Reset all element colors to default (dark gray #202020)
     */
    void resetAllColors();

    /**
     * Check if SVG loaded successfully
     */
    bool isValid() const { return m_svgRenderer && m_svgRenderer->isValid(); }

    /**
     * Get default "off" color for LEDs
     */
    static QColor defaultOffColor() { return QColor(TR4QT::LedColors::OFF); }

    /**
     * Get the SVG's intrinsic size from the renderer
     */
    QSize getSvgSize() const;

    /**
     * Get the bounding rectangle of an SVG element in SVG coordinates
     * Returns empty QRectF if element not found
     */
    QRectF getElementBounds(const QString& elementId) const;

    /**
     * Check if an SVG element exists
     */
    bool hasElement(const QString& elementId) const;

    /**
     * Get the SVG's viewBox (coordinate system for element bounds)
     */
    QRectF getViewBox() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    /**
     * Load SVG file into DOM and renderer
     */
    bool loadSvg(const QString& svgPath);

    /**
     * Update SVG element fill color in DOM and reload renderer
     * Thread-safe (uses mutex)
     */
    void updateElementFill(const QString& elementId, const QColor& color);

    /**
     * Update SVG element fill with a string value (e.g., "none" for transparent)
     * Thread-safe (uses mutex)
     */
    void updateElementFillWithValue(const QString& elementId, const QString& fillValue);

    /**
     * Reload SVG renderer from modified DOM
     */
    void reloadRenderer();

    // SVG data
    QDomDocument m_svgDom;           // Editable SVG XML
    QSvgRenderer* m_svgRenderer;     // Renderer (reloaded when DOM changes)
    QByteArray m_originalSvgData;    // Original SVG (for reset)

    // State tracking
    QHash<QString, QColor> m_elementColors;  // Current colors by element ID
    QSet<QString> m_missingElements;          // Cache of elements that don't exist (avoid repeated warnings)
    QHash<QString, QDomElement> m_elementCache;  // Cache of found elements
    mutable QMutex m_mutex;                   // Thread safety for DOM updates

    // Original SVG size (for aspect ratio)
    QSize m_svgSize;

    // Helper: Find element by ID (recursive search since elementById() doesn't work)
    QDomElement findElementById(const QString& elementId);
};

} // namespace TR4QT

#endif // SVGPANELWIDGET_H
