/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef SPECTRUMRHIITEM_H
#define SPECTRUMRHIITEM_H

#include <QQuickRhiItem>
#include <QMutex>
#include <QVector>
#include <rhi/qrhi.h>

namespace TR4QT {

class SpectrumRhiRenderer;

/**
 * @brief GPU-accelerated spectrum display using Qt RHI
 *
 * Renders spectrum data as a gradient-filled area with glow effect,
 * inspired by QK4's BlueAmplitude style. Uses a fragment shader for
 * LUT-based coloring and exponential glow falloff above the spectrum peak.
 *
 * Works with Metal (macOS), Vulkan, D3D11/12 (Windows), and OpenGL.
 */
class SpectrumRhiItem : public QQuickRhiItem {
    Q_OBJECT
    Q_PROPERTY(float refLevel READ refLevel WRITE setRefLevel NOTIFY refLevelChanged)
    Q_PROPERTY(float noiseFloor READ noiseFloor WRITE setNoiseFloor NOTIFY noiseFloorChanged)
    QML_ELEMENT

public:
    explicit SpectrumRhiItem(QQuickItem* parent = nullptr);
    ~SpectrumRhiItem() override;

    float refLevel() const { return m_refLevel; }
    void setRefLevel(float db);

    float noiseFloor() const { return m_noiseFloor; }
    void setNoiseFloor(float db);

    QQuickRhiItemRenderer* createRenderer() override;

    Q_INVOKABLE void updateSpectrum(const QVariantList& samples);

    // Internal: accessed by renderer from render thread
    bool hasPendingData() const;
    QVector<float> takePendingData();
    float getRefLevel() const { return m_refLevel; }
    float getNoiseFloor() const { return m_noiseFloor; }

signals:
    void refLevelChanged();
    void noiseFloorChanged();

private:
    float m_refLevel{0.0f};
    float m_noiseFloor{-130.0f};

    mutable QMutex m_mutex;
    QVector<float> m_pendingData;
    bool m_hasPendingData{false};
};

/**
 * @brief Renderer for SpectrumRhiItem - runs on Qt's render thread
 *
 * Uses a 1D R32F texture for normalized spectrum data, an RGBA8 color LUT,
 * and a fragment shader that renders gradient fill below the spectrum peak
 * with a glow effect above it.
 */
class SpectrumRhiRenderer : public QQuickRhiItemRenderer {
public:
    SpectrumRhiRenderer();
    ~SpectrumRhiRenderer() override;

    void initialize(QRhiCommandBuffer* cb) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* cb) override;

private:
    void createResources();
    void createSpectrumColorLUT();

    // RHI resources
    QRhi* m_rhi{nullptr};
    std::unique_ptr<QRhiTexture> m_spectrumDataTexture;      // R32F, 2048x1
    std::unique_ptr<QRhiTexture> m_spectrumColorLutTexture;  // RGBA8, 256x1
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    // Spectrum processing state
    static constexpr int SPECTRUM_WIDTH = 2048;
    static constexpr int LUT_SIZE = 256;

    float m_refLevel{0.0f};
    float m_noiseFloor{-130.0f};

    // Smoothed baseline for noise floor tracking
    float m_smoothedBaseline{0.0f};
    bool m_baselineInitialized{false};
    static constexpr float BASELINE_ALPHA = 0.05f;

    // Pending data from synchronize()
    QVector<float> m_pendingData;
    bool m_hasPendingData{false};

    // Initialization state
    bool m_resourcesCreated{false};
    QRhiResourceUpdateBatch* m_initialBatch{nullptr};

    // Color LUT data
    QVector<quint8> m_colorLUTData;  // 256 entries × 4 bytes (RGBA)
};

} // namespace TR4QT

#endif // SPECTRUMRHIITEM_H
