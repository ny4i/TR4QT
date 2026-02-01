/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "WaterfallImageProvider.h"
#include <QtMath>
#include <cstring>

namespace TR4QT {

WaterfallImageProvider::WaterfallImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    m_colorLUT.resize(LUT_SIZE);
    rebuildColorLUT();

    // Initialize image
    m_image = QImage(m_width, m_height, QImage::Format_RGB32);
    m_image.fill(qRgb(0, 0, 30));  // Dark blue background
}

QImage WaterfallImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    Q_UNUSED(id)

    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_image.size();
    }

    // Return scaled if requested, otherwise return as-is
    if (requestedSize.isValid() && requestedSize != m_image.size()) {
        return m_image.scaled(requestedSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    return m_image;
}

void WaterfallImageProvider::addRow(const QVector<float>& samples)
{
    QMutexLocker locker(&m_mutex);

    if (samples.isEmpty()) return;

    // Scroll existing content down by 1 pixel
    scrollDown();

    // Calculate dB range
    float maxDb = -20.0f + m_refLevel;
    float minDb = maxDb - m_waterfallRange;
    float dbRange = maxDb - minDb;

    // Get pointer to top row of pixels
    QRgb* topRow = reinterpret_cast<QRgb*>(m_image.scanLine(0));

    // Map samples to pixels
    float samplesPerPixel = static_cast<float>(samples.size()) / m_width;

    for (int x = 0; x < m_width; ++x) {
        // Get sample for this pixel
        int sampleIndex = static_cast<int>(x * samplesPerPixel);
        if (sampleIndex >= samples.size()) sampleIndex = samples.size() - 1;

        float db = samples[sampleIndex];

        // Normalize to 0-1 range
        float normalized = (db - minDb) / dbRange;
        normalized = qBound(0.0f, normalized, 1.0f);

        // Look up color from LUT
        int lutIndex = static_cast<int>(normalized * (LUT_SIZE - 1));
        topRow[x] = m_colorLUT[lutIndex];
    }

    ++m_frameNumber;
}

void WaterfallImageProvider::clear()
{
    QMutexLocker locker(&m_mutex);
    m_image.fill(qRgb(0, 0, 30));  // Dark blue background
    ++m_frameNumber;
}

void WaterfallImageProvider::setRefLevel(float db)
{
    QMutexLocker locker(&m_mutex);
    if (!qFuzzyCompare(m_refLevel, db)) {
        m_refLevel = db;
    }
}

void WaterfallImageProvider::setWaterfallRange(float db)
{
    QMutexLocker locker(&m_mutex);
    db = qBound(20.0f, db, 120.0f);
    if (!qFuzzyCompare(m_waterfallRange, db)) {
        m_waterfallRange = db;
    }
}

void WaterfallImageProvider::setSize(int width, int height)
{
    QMutexLocker locker(&m_mutex);
    if (m_width != width || m_height != height) {
        m_width = width;
        m_height = height;
        m_image = QImage(width, height, QImage::Format_RGB32);
        m_image.fill(qRgb(0, 0, 30));
        ++m_frameNumber;
    }
}

void WaterfallImageProvider::scrollDown()
{
    // Scroll image down by 1 pixel (copy each row to the row below)
    // Start from bottom and work up to avoid overwriting source data
    int bytesPerLine = m_image.bytesPerLine();

    for (int y = m_height - 1; y > 0; --y) {
        uchar* destLine = m_image.scanLine(y);
        const uchar* srcLine = m_image.scanLine(y - 1);
        std::memcpy(destLine, srcLine, bytesPerLine);
    }
}

void WaterfallImageProvider::rebuildColorLUT()
{
    // Pre-compute 256 colors for the waterfall gradient
    // Dark blue -> Blue -> Cyan -> Green -> Yellow -> Red
    for (int i = 0; i < LUT_SIZE; ++i) {
        float normalized = static_cast<float>(i) / (LUT_SIZE - 1);

        int r, g, b;

        if (normalized < 0.25f) {
            // Very dark blue/black noise floor
            float t = normalized / 0.25f;
            r = 0;
            g = 0;
            b = static_cast<int>(40 * t);
        } else if (normalized < 0.4f) {
            // Dark blue to medium blue
            float t = (normalized - 0.25f) / 0.15f;
            r = 0;
            g = static_cast<int>(30 * t);
            b = 40 + static_cast<int>(120 * t);
        } else if (normalized < 0.55f) {
            // Medium blue to cyan
            float t = (normalized - 0.4f) / 0.15f;
            r = 0;
            g = 30 + static_cast<int>(225 * t);
            b = 160 + static_cast<int>(95 * t);
        } else if (normalized < 0.7f) {
            // Cyan to green
            float t = (normalized - 0.55f) / 0.15f;
            r = 0;
            g = 255;
            b = static_cast<int>(255 * (1.0f - t));
        } else if (normalized < 0.85f) {
            // Green to yellow
            float t = (normalized - 0.7f) / 0.15f;
            r = static_cast<int>(255 * t);
            g = 255;
            b = 0;
        } else {
            // Yellow to red
            float t = (normalized - 0.85f) / 0.15f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
            b = 0;
        }

        m_colorLUT[i] = qRgb(r, g, b);
    }
}

} // namespace TR4QT
