/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef WATERFALLRHIITEM_H
#define WATERFALLRHIITEM_H

#include <QQuickRhiItem>
#include <QMutex>
#include <QVector>
#include <rhi/qrhi.h>

namespace TR4QT {

class WaterfallRhiRenderer;

/**
 * @brief GPU-accelerated waterfall display using Qt RHI (Rendering Hardware Interface)
 *
 * This QQuickRhiItem provides a high-performance waterfall display using GPU
 * texture scrolling instead of CPU memcpy operations. Key optimizations:
 *
 * - Only uploads new row data (2.4KB) instead of full image (480KB)
 * - GPU shader handles circular buffer scrolling
 * - GPU shader handles dB-to-color mapping via 1D LUT texture
 *
 * Works with Metal (macOS), Vulkan, D3D11/12 (Windows), and OpenGL.
 */
class WaterfallRhiItem : public QQuickRhiItem {
    Q_OBJECT
    Q_PROPERTY(int waterfallHeight READ waterfallHeight WRITE setWaterfallHeight NOTIFY waterfallHeightChanged)
    Q_PROPERTY(float refLevel READ refLevel WRITE setRefLevel NOTIFY refLevelChanged)
    Q_PROPERTY(float waterfallRange READ waterfallRange WRITE setWaterfallRange NOTIFY waterfallRangeChanged)
    Q_PROPERTY(float waterfallRefLevel READ waterfallRefLevel WRITE setWaterfallRefLevel NOTIFY waterfallRefLevelChanged)
    QML_ELEMENT

public:
    explicit WaterfallRhiItem(QQuickItem* parent = nullptr);
    ~WaterfallRhiItem() override;

    // Property accessors
    int waterfallHeight() const { return m_waterfallHeight; }
    void setWaterfallHeight(int height);

    float refLevel() const { return m_refLevel; }
    void setRefLevel(float db);

    float waterfallRange() const { return m_waterfallRange; }
    void setWaterfallRange(float db);

    float waterfallRefLevel() const { return m_waterfallRefLevel; }
    void setWaterfallRefLevel(float db);

    // QQuickRhiItem interface
    QQuickRhiItemRenderer* createRenderer() override;

    // Called from data model to add new waterfall row
    Q_INVOKABLE void addRow(const QVariantList& samples);

    // Internal: get pending row for renderer (called from render thread)
    bool hasPendingRow() const;
    QVector<float> takePendingRow();
    float getRefLevel() const { return m_refLevel; }
    float getWaterfallRange() const { return m_waterfallRange; }
    float getWaterfallRefLevel() const { return m_waterfallRefLevel; }

signals:
    void waterfallHeightChanged();
    void refLevelChanged();
    void waterfallRangeChanged();
    void waterfallRefLevelChanged();

private:
    int m_waterfallHeight{200};          // Number of rows in circular buffer
    float m_refLevel{0.0f};              // Reference level in dB (for spectrum)
    float m_waterfallRange{80.0f};       // Dynamic range in dB
    float m_waterfallRefLevel{0.0f};     // Independent waterfall ref level in dB

    // Thread-safe row buffer for passing data to render thread
    mutable QMutex m_mutex;
    QVector<float> m_pendingRow;
    bool m_hasPendingRow{false};
};

/**
 * @brief Renderer for WaterfallRhiItem - runs on Qt's render thread
 *
 * Uses a circular buffer texture where new rows are uploaded at incrementing
 * positions. The fragment shader adjusts UV coordinates based on the current
 * row offset to achieve scrolling without any CPU-side memory movement.
 */
class WaterfallRhiRenderer : public QQuickRhiItemRenderer {
public:
    WaterfallRhiRenderer();
    ~WaterfallRhiRenderer() override;

    // QQuickRhiItemRenderer interface
    void initialize(QRhiCommandBuffer* cb) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* cb) override;

private:
    void createResources();
    void uploadNewRow(QRhiResourceUpdateBatch* batch, const QVector<float>& samples);
    void createColorLUT();

    // RHI resources
    QRhi* m_rhi{nullptr};
    std::unique_ptr<QRhiTexture> m_waterfallTexture;    // R32F circular buffer
    std::unique_ptr<QRhiTexture> m_colorLutTexture;     // RGBA8 1D LUT (256 entries)
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;         // Fullscreen quad
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;        // Shader uniforms
    std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    // Circular buffer state
    int m_currentRow{0};                  // Next row to write
    int m_waterfallHeight{200};           // Texture height (rows)
    int m_waterfallWidth{2048};           // Texture width (samples)

    // Shader uniforms
    float m_refLevel{0.0f};
    float m_waterfallRange{80.0f};
    float m_waterfallRefLevel{0.0f};  // Independent waterfall ref level

    // Pending row data from synchronize()
    QVector<float> m_pendingRow;
    bool m_hasPendingRow{false};

    // Initialization state
    bool m_resourcesCreated{false};
    QRhiResourceUpdateBatch* m_initialBatch{nullptr};  // Deferred initial upload

    // Color LUT data (256 RGBA colors)
    static constexpr int LUT_SIZE = 256;
    QVector<quint32> m_colorLUTData;
};

} // namespace TR4QT

#endif // WATERFALLRHIITEM_H
