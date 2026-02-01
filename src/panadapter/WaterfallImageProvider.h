/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef WATERFALLIMAGEPROVIDER_H
#define WATERFALLIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>
#include <QVector>
#include <QColor>

namespace TR4QT {

/**
 * @brief Provides waterfall images to QML via the image:// protocol
 *
 * This class renders the waterfall display efficiently in C++ using direct
 * pixel manipulation, then provides the image to QML for display.
 * Much faster than QML Canvas per-pixel fillRect calls.
 */
class WaterfallImageProvider : public QQuickImageProvider {
public:
    explicit WaterfallImageProvider();

    // QQuickImageProvider interface
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // Add a new row of samples to the waterfall
    void addRow(const QVector<float>& samples);

    // Clear the waterfall
    void clear();

    // Settings
    void setRefLevel(float db);
    void setWaterfallRange(float db);
    void setSize(int width, int height);

    // Get current frame number (for cache busting in QML)
    int frameNumber() const { return m_frameNumber; }

private:
    void rebuildColorLUT();
    QRgb colorForNormalized(float normalized) const;
    void scrollDown();

    QImage m_image;
    QMutex m_mutex;

    // Color lookup table (256 pre-computed colors)
    static constexpr int LUT_SIZE = 256;
    QVector<QRgb> m_colorLUT;

    // Settings
    float m_refLevel{0.0f};
    float m_waterfallRange{80.0f};
    int m_width{600};
    int m_height{200};

    // Frame counter for QML cache invalidation
    int m_frameNumber{0};
};

} // namespace TR4QT

#endif // WATERFALLIMAGEPROVIDER_H
