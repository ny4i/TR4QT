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

#ifndef IPANADAPTERRENDERER_H
#define IPANADAPTERRENDERER_H

#include <QWidget>
#include <QVector>
#include "PanadapterTypes.h"

namespace TR4QT {

/**
 * @brief Abstract interface for panadapter renderers
 *
 * This interface allows swapping rendering implementations (QML, OpenGL, QPainter)
 * without changing the rest of the panadapter code.
 */
class IPanadapterRenderer : public QObject {
    Q_OBJECT

public:
    enum class Type {
        Qml,        // Qt Quick / QML (GPU accelerated)
        OpenGL,     // QOpenGLWidget (GPU accelerated)
        Widget      // QPainter / QWidget (CPU, fallback)
    };

    explicit IPanadapterRenderer(QObject* parent = nullptr) : QObject(parent) {}
    ~IPanadapterRenderer() override = default;

    /**
     * @brief Get the widget that displays the panadapter
     * @return The renderable widget to embed in the window
     */
    virtual QWidget* widget() = 0;

    /**
     * @brief Initialize the renderer
     */
    virtual void initialize() = 0;

    /**
     * @brief Shutdown and cleanup resources
     */
    virtual void shutdown() = 0;

    /**
     * @brief Get the renderer type
     */
    virtual Type type() const = 0;

    // --- Data Updates ---

    /**
     * @brief Update with new samples
     * @param samples Vector of dB values (2048 samples)
     */
    virtual void updateSamples(const QVector<float>& samples) = 0;

    /**
     * @brief Set the center frequency
     * @param freqHz Frequency in Hz
     */
    virtual void setCenterFrequency(qint64 freqHz) = 0;

    /**
     * @brief Set the sample rate (bandwidth)
     * @param rateHz Sample rate in Hz
     */
    virtual void setSampleRate(int rateHz) = 0;

    /**
     * @brief Set the noise floor for color scaling
     * @param db Noise floor in dB
     */
    virtual void setNoiseFloor(float db) = 0;

    // --- Settings ---

    /**
     * @brief Set sample averaging
     * @param value Averaging factor (1 = no averaging)
     */
    virtual void setAveraging(int value) = 0;

    /**
     * @brief Set the color palette
     * @param palette Palette enum value
     */
    virtual void setPalette(WaterfallPalette palette) = 0;

    /**
     * @brief Set reference level adjustment
     * @param db Reference level offset in dB
     */
    virtual void setRefLevel(float db) = 0;

    /**
     * @brief Set the active pan ID being displayed
     * @param panId 'A', 'B', 'Y', or 'Z'
     */
    virtual void setPanId(char panId) = 0;

    /**
     * @brief Pause/resume rendering
     */
    virtual void setPaused(bool paused) = 0;

    /**
     * @brief Set the waterfall dB range for contrast control
     * @param db Range in dB (e.g., 60 = 60dB dynamic range)
     */
    virtual void setWaterfallRange(float db) = 0;

signals:
    /**
     * @brief Emitted when user clicks on the display to tune
     * @param freqHz The frequency at click position
     * @param vfo 0 for VFO A (left click), 1 for VFO B (right click)
     */
    void frequencyClicked(qint64 freqHz, int vfo);

    /**
     * @brief Emitted when mouse hovers over display
     * @param freqHz Frequency at cursor position
     * @param db Signal level at cursor position
     */
    void cursorMoved(qint64 freqHz, float db);
};

} // namespace TR4QT

#endif // IPANADAPTERRENDERER_H
