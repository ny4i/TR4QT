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

#ifndef QMLPANADAPTERRENDERER_H
#define QMLPANADAPTERRENDERER_H

#include "../IPanadapterRenderer.h"
#include "../WaterfallImageProvider.h"
#include <QQuickWidget>
#include <QQmlContext>

namespace TR4QT {

class PanadapterProvider;
class WaterfallImageProvider;

/**
 * @brief QML-based panadapter renderer using Qt Quick for GPU acceleration
 *
 * This renderer embeds a QQuickWidget containing QML content that uses
 * custom QQuickItems for efficient GPU-accelerated rendering.
 */
class QmlPanadapterRenderer : public IPanadapterRenderer {
    Q_OBJECT

public:
    explicit QmlPanadapterRenderer(QObject* parent = nullptr);
    ~QmlPanadapterRenderer() override;

    // IPanadapterRenderer interface
    QWidget* widget() override;
    void initialize() override;
    void shutdown() override;
    Type type() const override { return Type::Qml; }

    void updateSamples(const QVector<float>& samples) override;
    void setCenterFrequency(qint64 freqHz) override;
    void setSampleRate(int rateHz) override;
    void setNoiseFloor(float db) override;
    void setAveraging(int value) override;
    void setPalette(WaterfallPalette palette) override;
    void setRefLevel(float db) override;
    void setPanId(char panId) override;
    void setPaused(bool paused) override;
    void setWaterfallRange(float db) override;

private:
    QQuickWidget* m_quickWidget{nullptr};
    PanadapterProvider* m_provider{nullptr};
    WaterfallImageProvider* m_waterfallProvider{nullptr};  // Owned by QML engine
    bool m_initialized{false};
};

/**
 * @brief QObject that exposes panadapter data to QML
 *
 * This object is registered with the QML context and provides
 * properties and methods that QML can access for rendering.
 */
class PanadapterProvider : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList samples READ samples NOTIFY samplesChanged)
    Q_PROPERTY(qint64 centerFrequency READ centerFrequency NOTIFY centerFrequencyChanged)
    Q_PROPERTY(int sampleRate READ sampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(float noiseFloor READ noiseFloor NOTIFY noiseFloorChanged)
    Q_PROPERTY(int averaging READ averaging WRITE setAveraging NOTIFY averagingChanged)
    Q_PROPERTY(float refLevel READ refLevel WRITE setRefLevel NOTIFY refLevelChanged)
    Q_PROPERTY(QString panId READ panId NOTIFY panIdChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(QString palette READ palette WRITE setPaletteString NOTIFY paletteChanged)
    Q_PROPERTY(float waterfallRange READ waterfallRange WRITE setWaterfallRange NOTIFY waterfallRangeChanged)
    Q_PROPERTY(int waterfallFrame READ waterfallFrame NOTIFY waterfallFrameChanged)

public:
    explicit PanadapterProvider(QObject* parent = nullptr);

    QVariantList samples() const;
    qint64 centerFrequency() const { return m_centerFrequency; }
    int sampleRate() const { return m_sampleRate; }
    float noiseFloor() const { return m_noiseFloor; }
    int averaging() const { return m_averaging; }
    float refLevel() const { return m_refLevel; }
    QString panId() const { return QString(m_panId); }
    bool paused() const { return m_paused; }
    QString palette() const;
    float waterfallRange() const { return m_waterfallRange; }
    int waterfallFrame() const { return m_waterfallFrame; }

    void setAveraging(int value);
    void setRefLevel(float db);
    void setPaused(bool paused);
    void setPaletteString(const QString& palette);
    void setWaterfallRange(float db);

    // Called by renderer
    void updateSamples(const QVector<float>& samples);
    const QVector<float>& getAveragedSamples() const { return m_samples; }
    void updateCenterFrequency(qint64 freqHz);
    void updateSampleRate(int rateHz);
    void updateNoiseFloor(float db);
    void updatePanId(char panId);
    void updatePalette(WaterfallPalette palette);

    // Called from QML for click-to-tune
    Q_INVOKABLE void onFrequencyClicked(qint64 freqHz, int vfo);
    Q_INVOKABLE void onCursorMoved(qint64 freqHz, float db);

    // Utility for QML
    Q_INVOKABLE qint64 frequencyAtPosition(qreal xRatio) const;
    Q_INVOKABLE QString colorForDb(float db) const;

    // Color lookup table property for efficient QML access
    Q_PROPERTY(QVariantList colorLUT READ colorLUT NOTIFY colorLUTChanged)
    QVariantList colorLUT() const { return m_colorLUTVariant; }

signals:
    void colorLUTChanged();
    void samplesChanged();
    void waterfallFrameChanged();
    void centerFrequencyChanged();
    void sampleRateChanged();
    void noiseFloorChanged();
    void averagingChanged();
    void refLevelChanged();
    void panIdChanged();
    void pausedChanged();
    void paletteChanged();
    void waterfallRangeChanged();

    // Forward to renderer
    void frequencyClicked(qint64 freqHz, int vfo);
    void cursorMoved(qint64 freqHz, float db);

private:
    QColor getHeatMapColor(float normalized) const;
    QColor getGrayscaleColor(float normalized) const;
    QColor getSepiaColor(float normalized) const;
    void rebuildColorLUT();

    // Color lookup table (256 entries, pre-computed)
    static constexpr int LUT_SIZE = 256;
    QStringList m_colorLUT;
    QVariantList m_colorLUTVariant;  // For QML property access

    QVector<float> m_samples;
    QVector<float> m_prevSamples;  // For averaging
    QVariantList m_samplesVariant;  // Cached for QML property access
    qint64 m_centerFrequency{7200000};
    int m_sampleRate{48000};
    float m_noiseFloor{-130.0f};
    int m_averaging{3};
    float m_refLevel{0.0f};
    char m_panId{'A'};
    bool m_paused{false};
    WaterfallPalette m_palette{WaterfallPalette::HeatMap};
    float m_waterfallRange{80.0f};  // Default 80dB dynamic range
    int m_waterfallFrame{0};  // Frame counter for QML image refresh
};

} // namespace TR4QT

#endif // QMLPANADAPTERRENDERER_H
