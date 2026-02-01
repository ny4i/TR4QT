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

#include "QmlPanadapterRenderer.h"
#include "../../logging/LogMacros.h"
#include <QQmlEngine>
#include <QtMath>

namespace TR4QT {

// ============================================================================
// QmlPanadapterRenderer
// ============================================================================

QmlPanadapterRenderer::QmlPanadapterRenderer(QObject* parent)
    : IPanadapterRenderer(parent)
    , m_provider(new PanadapterProvider(this))
{
    // Forward signals from provider
    connect(m_provider, &PanadapterProvider::frequencyClicked,
            this, &IPanadapterRenderer::frequencyClicked);
    connect(m_provider, &PanadapterProvider::cursorMoved,
            this, &IPanadapterRenderer::cursorMoved);
}

QmlPanadapterRenderer::~QmlPanadapterRenderer()
{
    shutdown();
}

QWidget* QmlPanadapterRenderer::widget()
{
    if (!m_quickWidget) {
        initialize();
    }
    return m_quickWidget;
}

void QmlPanadapterRenderer::initialize()
{
    if (m_initialized) {
        return;
    }

    LOG_INFO("QmlPanadapterRenderer", "Initializing QML renderer with ImageProvider");

    m_quickWidget = new QQuickWidget();
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Create waterfall image provider (QML engine takes ownership)
    m_waterfallProvider = new WaterfallImageProvider();

    // Register image provider with QML engine (must be done before setSource)
    m_quickWidget->engine()->addImageProvider("waterfall", m_waterfallProvider);

    // Register data provider with QML context
    m_quickWidget->rootContext()->setContextProperty("panadapter", m_provider);

    // Load QML
    m_quickWidget->setSource(QUrl("qrc:/qml/Panadapter.qml"));

    if (m_quickWidget->status() == QQuickWidget::Error) {
        for (const auto& error : m_quickWidget->errors()) {
            LOG_ERROR("QmlPanadapterRenderer", QString("QML Error: %1").arg(error.toString()));
        }
    }

    m_initialized = true;
}

void QmlPanadapterRenderer::shutdown()
{
    if (m_quickWidget) {
        delete m_quickWidget;
        m_quickWidget = nullptr;
    }
    m_initialized = false;
}

void QmlPanadapterRenderer::updateSamples(const QVector<float>& samples)
{
    // Update provider (for spectrum display and other properties)
    m_provider->updateSamples(samples);

    // Update waterfall image provider (renders to QImage)
    if (m_waterfallProvider) {
        m_waterfallProvider->addRow(m_provider->getAveragedSamples());
    }
}

void QmlPanadapterRenderer::setCenterFrequency(qint64 freqHz)
{
    m_provider->updateCenterFrequency(freqHz);
}

void QmlPanadapterRenderer::setSampleRate(int rateHz)
{
    m_provider->updateSampleRate(rateHz);
}

void QmlPanadapterRenderer::setNoiseFloor(float db)
{
    m_provider->updateNoiseFloor(db);
}

void QmlPanadapterRenderer::setAveraging(int value)
{
    m_provider->setAveraging(value);
}

void QmlPanadapterRenderer::setPalette(WaterfallPalette palette)
{
    m_provider->updatePalette(palette);
}

void QmlPanadapterRenderer::setRefLevel(float db)
{
    m_provider->setRefLevel(db);
    if (m_waterfallProvider) {
        m_waterfallProvider->setRefLevel(db);
    }
}

void QmlPanadapterRenderer::setPanId(char panId)
{
    m_provider->updatePanId(panId);
}

void QmlPanadapterRenderer::setPaused(bool paused)
{
    m_provider->setPaused(paused);
}

void QmlPanadapterRenderer::setWaterfallRange(float db)
{
    m_provider->setWaterfallRange(db);
    if (m_waterfallProvider) {
        m_waterfallProvider->setWaterfallRange(db);
    }
}

// ============================================================================
// PanadapterProvider
// ============================================================================

PanadapterProvider::PanadapterProvider(QObject* parent)
    : QObject(parent)
{
    m_samples.resize(PanadapterPacket::SAMPLE_COUNT);
    m_prevSamples.resize(PanadapterPacket::SAMPLE_COUNT);
    m_samples.fill(-130.0f);
    m_prevSamples.fill(-130.0f);

    // Pre-compute color LUT
    rebuildColorLUT();
}

QVariantList PanadapterProvider::samples() const
{
    // Return cached QVariantList (rebuilt in updateSamples)
    return m_samplesVariant;
}

QString PanadapterProvider::palette() const
{
    switch (m_palette) {
        case WaterfallPalette::HeatMap: return "heatmap";
        case WaterfallPalette::Grayscale: return "grayscale";
        case WaterfallPalette::Sepia: return "sepia";
        case WaterfallPalette::BlueGreen: return "bluegreen";
        case WaterfallPalette::FireIce: return "fireice";
    }
    return "heatmap";
}

void PanadapterProvider::setAveraging(int value)
{
    value = qBound(1, value, 10);
    if (m_averaging != value) {
        m_averaging = value;
        emit averagingChanged();
    }
}

void PanadapterProvider::setRefLevel(float db)
{
    if (!qFuzzyCompare(m_refLevel, db)) {
        m_refLevel = db;
        rebuildColorLUT();
        emit refLevelChanged();
    }
}

void PanadapterProvider::setPaused(bool paused)
{
    if (m_paused != paused) {
        m_paused = paused;
        emit pausedChanged();
    }
}

void PanadapterProvider::setPaletteString(const QString& palette)
{
    WaterfallPalette newPalette = WaterfallPalette::HeatMap;
    if (palette == "grayscale") newPalette = WaterfallPalette::Grayscale;
    else if (palette == "sepia") newPalette = WaterfallPalette::Sepia;
    else if (palette == "bluegreen") newPalette = WaterfallPalette::BlueGreen;
    else if (palette == "fireice") newPalette = WaterfallPalette::FireIce;

    if (m_palette != newPalette) {
        m_palette = newPalette;
        rebuildColorLUT();
        emit paletteChanged();
    }
}

void PanadapterProvider::setWaterfallRange(float db)
{
    db = qBound(20.0f, db, 120.0f);  // Reasonable range: 20-120 dB
    if (!qFuzzyCompare(m_waterfallRange, db)) {
        m_waterfallRange = db;
        rebuildColorLUT();
        emit waterfallRangeChanged();
    }
}

void PanadapterProvider::updateSamples(const QVector<float>& samples)
{
    if (m_paused) {
        return;
    }

    if (samples.size() != m_samples.size()) {
        return;
    }

    // Apply averaging
    float oldPortion = (m_averaging - 1.0f) / m_averaging;
    float newPortion = 1.0f / m_averaging;

    for (int i = 0; i < samples.size(); ++i) {
        m_samples[i] = oldPortion * m_prevSamples[i] + newPortion * samples[i];
    }

    // Save for next averaging
    m_prevSamples = m_samples;

    // Update cached QVariantList for QML (do this ONCE per update, not per property access)
    m_samplesVariant.clear();
    m_samplesVariant.reserve(m_samples.size());
    for (float sample : m_samples) {
        m_samplesVariant.append(sample);
    }

    // Increment frame number to trigger waterfall image refresh
    ++m_waterfallFrame;

    emit samplesChanged();
    emit waterfallFrameChanged();
}

void PanadapterProvider::updateCenterFrequency(qint64 freqHz)
{
    if (m_centerFrequency != freqHz) {
        m_centerFrequency = freqHz;
        emit centerFrequencyChanged();
    }
}

void PanadapterProvider::updateSampleRate(int rateHz)
{
    if (m_sampleRate != rateHz) {
        m_sampleRate = rateHz;
        emit sampleRateChanged();
    }
}

void PanadapterProvider::updateNoiseFloor(float db)
{
    if (!qFuzzyCompare(m_noiseFloor, db)) {
        m_noiseFloor = db;
        emit noiseFloorChanged();
    }
}

void PanadapterProvider::updatePanId(char panId)
{
    if (m_panId != panId) {
        m_panId = panId;
        emit panIdChanged();
    }
}

void PanadapterProvider::updatePalette(WaterfallPalette palette)
{
    if (m_palette != palette) {
        m_palette = palette;
        rebuildColorLUT();
        emit paletteChanged();
    }
}

void PanadapterProvider::onFrequencyClicked(qint64 freqHz, int vfo)
{
    emit frequencyClicked(freqHz, vfo);
}

void PanadapterProvider::onCursorMoved(qint64 freqHz, float db)
{
    emit cursorMoved(freqHz, db);
}

qint64 PanadapterProvider::frequencyAtPosition(qreal xRatio) const
{
    // xRatio is 0.0 at left edge, 1.0 at right edge
    // Convert to frequency offset from center
    qint64 halfSpan = m_sampleRate / 2;
    qint64 offset = static_cast<qint64>((xRatio - 0.5) * m_sampleRate);
    return m_centerFrequency + offset;
}

QString PanadapterProvider::colorForDb(float db) const
{
    // Use pre-computed LUT for fast color lookup
    // Normalize dB value to LUT index
    float maxDb = -20.0f + m_refLevel;
    float minDb = maxDb - m_waterfallRange;

    float normalized = (db - minDb) / (maxDb - minDb);
    normalized = qBound(0.0f, normalized, 1.0f);

    int index = static_cast<int>(normalized * (LUT_SIZE - 1));
    index = qBound(0, index, LUT_SIZE - 1);

    return m_colorLUT.at(index);
}

QColor PanadapterProvider::getHeatMapColor(float normalized) const
{
    // Dark Blue → Blue → Cyan → Green → Yellow → Red
    // Ensure minimum visibility with dark blue (not black) at lowest level
    if (normalized < 0.2f) {
        // Dark blue to Blue (always visible against black)
        float t = normalized / 0.2f;
        return QColor::fromRgbF(0.0, 0.1 + 0.2 * t, 0.4 + 0.6 * t);  // (0, 0.1-0.3, 0.4-1.0)
    } else if (normalized < 0.4f) {
        // Blue to Cyan
        float t = (normalized - 0.2f) / 0.2f;
        return QColor::fromRgbF(0.0, 0.3 + 0.7 * t, 1.0);  // (0, 0.3-1.0, 1.0)
    } else if (normalized < 0.6f) {
        // Cyan to Green
        float t = (normalized - 0.4f) / 0.2f;
        return QColor::fromRgbF(0.0, 1.0, 1.0 - t);
    } else if (normalized < 0.8f) {
        // Green to Yellow
        float t = (normalized - 0.6f) / 0.2f;
        return QColor::fromRgbF(t, 1.0, 0.0);
    } else {
        // Yellow to Red
        float t = (normalized - 0.8f) / 0.2f;
        return QColor::fromRgbF(1.0, 1.0 - t, 0.0);
    }
}

QColor PanadapterProvider::getGrayscaleColor(float normalized) const
{
    int gray = static_cast<int>(normalized * 255);
    return QColor(gray, gray, gray);
}

QColor PanadapterProvider::getSepiaColor(float normalized) const
{
    // Sepia tone (from K4LanExample)
    int val = static_cast<int>(normalized * 255);
    int tr = qMin(static_cast<int>(0.393 * val + 0.769 * val + 0.189 * val), 255);
    int tg = qMin(static_cast<int>(0.349 * val + 0.686 * val + 0.168 * val), 255);
    int tb = qMin(static_cast<int>(0.272 * val + 0.534 * val + 0.131 * val), 255);
    return QColor(tr, tg, tb);
}

void PanadapterProvider::rebuildColorLUT()
{
    // Pre-compute color strings for all 256 levels
    // This avoids QString allocation on every pixel during rendering
    m_colorLUT.clear();
    m_colorLUT.reserve(LUT_SIZE);
    m_colorLUTVariant.clear();
    m_colorLUTVariant.reserve(LUT_SIZE);

    float maxDb = -20.0f + m_refLevel;
    float minDb = maxDb - m_waterfallRange;

    for (int i = 0; i < LUT_SIZE; ++i) {
        // Map LUT index to normalized value (0.0 to 1.0)
        float normalized = static_cast<float>(i) / (LUT_SIZE - 1);

        // Get color based on palette
        int r, g, b;
        if (normalized < 0.25f) {
            float t = normalized / 0.25f;
            r = 0;
            g = 0;
            b = static_cast<int>(40 * t);
        } else if (normalized < 0.4f) {
            float t = (normalized - 0.25f) / 0.15f;
            r = 0;
            g = static_cast<int>(30 * t);
            b = 40 + static_cast<int>(120 * t);
        } else if (normalized < 0.55f) {
            float t = (normalized - 0.4f) / 0.15f;
            r = 0;
            g = 30 + static_cast<int>(225 * t);
            b = 160 + static_cast<int>(95 * t);
        } else if (normalized < 0.7f) {
            float t = (normalized - 0.55f) / 0.15f;
            r = 0;
            g = 255;
            b = static_cast<int>(255 * (1.0f - t));
        } else if (normalized < 0.85f) {
            float t = (normalized - 0.7f) / 0.15f;
            r = static_cast<int>(255 * t);
            g = 255;
            b = 0;
        } else {
            float t = (normalized - 0.85f) / 0.15f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - t));
            b = 0;
        }

        QString colorStr = QString("#%1%2%3")
            .arg(r, 2, 16, QChar('0'))
            .arg(g, 2, 16, QChar('0'))
            .arg(b, 2, 16, QChar('0'));

        m_colorLUT.append(colorStr);
        m_colorLUTVariant.append(colorStr);
    }

    emit colorLUTChanged();
}

} // namespace TR4QT
