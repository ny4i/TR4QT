/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "SpectrumRhiItem.h"
#include "../logging/LogMacros.h"
#include <QFile>
#include <QtMath>

namespace TR4QT {

// ============================================================================
// SpectrumRhiItem
// ============================================================================

SpectrumRhiItem::SpectrumRhiItem(QQuickItem* parent)
    : QQuickRhiItem(parent)
{
    setFlag(ItemHasContents, true);
}

SpectrumRhiItem::~SpectrumRhiItem() = default;

void SpectrumRhiItem::setRefLevel(float db)
{
    if (!qFuzzyCompare(m_refLevel, db)) {
        m_refLevel = db;
        emit refLevelChanged();
        update();
    }
}

void SpectrumRhiItem::setNoiseFloor(float db)
{
    if (!qFuzzyCompare(m_noiseFloor, db)) {
        m_noiseFloor = db;
        emit noiseFloorChanged();
        update();
    }
}

QQuickRhiItemRenderer* SpectrumRhiItem::createRenderer()
{
    return new SpectrumRhiRenderer();
}

void SpectrumRhiItem::updateSpectrum(const QVariantList& samples)
{
    QMutexLocker locker(&m_mutex);

    m_pendingData.clear();
    m_pendingData.reserve(samples.size());
    for (const QVariant& v : samples) {
        m_pendingData.append(v.toFloat());
    }
    m_hasPendingData = true;

    update();
}

bool SpectrumRhiItem::hasPendingData() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasPendingData;
}

QVector<float> SpectrumRhiItem::takePendingData()
{
    QMutexLocker locker(&m_mutex);
    m_hasPendingData = false;
    return std::move(m_pendingData);
}

// ============================================================================
// SpectrumRhiRenderer
// ============================================================================

SpectrumRhiRenderer::SpectrumRhiRenderer() = default;

SpectrumRhiRenderer::~SpectrumRhiRenderer() = default;

void SpectrumRhiRenderer::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb)

    m_rhi = rhi();
    if (!m_rhi) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to get QRhi instance");
        return;
    }

    LOG_INFO("SpectrumRhiRenderer", QString("Initializing with RHI backend: %1")
             .arg(m_rhi->backendName()));

    createResources();
}

void SpectrumRhiRenderer::synchronize(QQuickRhiItem* item)
{
    auto* spectrumItem = static_cast<SpectrumRhiItem*>(item);

    m_refLevel = spectrumItem->getRefLevel();
    m_noiseFloor = spectrumItem->getNoiseFloor();

    if (spectrumItem->hasPendingData()) {
        m_pendingData = spectrumItem->takePendingData();
        m_hasPendingData = true;
    }
}

void SpectrumRhiRenderer::render(QRhiCommandBuffer* cb)
{
    if (!m_resourcesCreated || !m_pipeline) {
        return;
    }

    QRhiResourceUpdateBatch* batch = nullptr;

    // Submit initial batch on first render
    if (m_initialBatch) {
        batch = m_initialBatch;
        m_initialBatch = nullptr;
    } else {
        batch = m_rhi->nextResourceUpdateBatch();
    }

    // Process and upload new spectrum data
    if (m_hasPendingData && !m_pendingData.isEmpty()) {
        // Normalize dB samples to 0.0-1.0 range
        const float minDb = qBound(-150.0f, m_noiseFloor, -100.0f) - 8.0f + m_refLevel;
        const float maxDb = -20.0f + m_refLevel;
        const float dbRange = maxDb - minDb;

        QVector<float> normalized(SPECTRUM_WIDTH, 0.0f);

        // Resample if needed and normalize
        const int srcSize = m_pendingData.size();
        const float ratio = static_cast<float>(srcSize) / SPECTRUM_WIDTH;

        float frameMin = 1.0f;
        for (int x = 0; x < SPECTRUM_WIDTH; ++x) {
            const int srcIndex = qBound(0, static_cast<int>(x * ratio), srcSize - 1);
            const float db = m_pendingData[srcIndex];
            float norm = qBound(0.0f, (db - minDb) / dbRange, 1.0f);
            normalized[x] = norm;
            if (norm < frameMin) {
                frameMin = norm;
            }
        }

        // Smoothed baseline (exponential moving average of frame minimum)
        if (!m_baselineInitialized) {
            m_smoothedBaseline = frameMin;
            m_baselineInitialized = true;
        } else {
            m_smoothedBaseline = m_smoothedBaseline * (1.0f - BASELINE_ALPHA)
                               + frameMin * BASELINE_ALPHA;
        }

        // Subtract baseline and scale
        const float ceiling = 0.95f;
        for (int x = 0; x < SPECTRUM_WIDTH; ++x) {
            float val = normalized[x] - m_smoothedBaseline;
            val = qBound(0.0f, val / ceiling, 1.0f);
            normalized[x] = val;
        }

        // Upload to R32F texture
        QRhiTextureSubresourceUploadDescription rowDesc(
            reinterpret_cast<const char*>(normalized.constData()),
            normalized.size() * sizeof(float)
        );
        rowDesc.setDestinationTopLeft(QPoint(0, 0));
        rowDesc.setSourceSize(QSize(SPECTRUM_WIDTH, 1));

        batch->uploadTexture(m_spectrumDataTexture.get(),
            QRhiTextureUploadEntry(0, 0, rowDesc)
        );

        m_hasPendingData = false;
    }

    // Update uniform buffer
    const QSize outputSize = renderTarget()->pixelSize();

    // std140 layout: vec4 glowColor (16), float×4 (16), vec2+float+float (16) = 48 bytes
    struct {
        float glowColor[4];      // offset 0
        float glowIntensity;     // offset 16
        float glowWidth;         // offset 20
        float spectrumHeight;    // offset 24
        float binCount;          // offset 28
        float viewportWidth;     // offset 32
        float viewportHeight;    // offset 36
        float textureWidth;      // offset 40
        float padding;           // offset 44
    } uniforms;

    uniforms.glowColor[0] = 0.0f;     // R
    uniforms.glowColor[1] = 0.83f;    // G (cyan)
    uniforms.glowColor[2] = 1.0f;     // B
    uniforms.glowColor[3] = 1.0f;     // A
    uniforms.glowIntensity = 0.8f;
    uniforms.glowWidth = 0.04f;
    uniforms.spectrumHeight = static_cast<float>(outputSize.height());
    uniforms.binCount = static_cast<float>(SPECTRUM_WIDTH);
    uniforms.viewportWidth = static_cast<float>(outputSize.width());
    uniforms.viewportHeight = static_cast<float>(outputSize.height());
    uniforms.textureWidth = static_cast<float>(SPECTRUM_WIDTH);
    uniforms.padding = 0.0f;

    batch->updateDynamicBuffer(m_uniformBuffer.get(), 0, sizeof(uniforms), &uniforms);

    // Render with transparent clear color (spectrum overlays on dark background)
    cb->beginPass(renderTarget(), QColor(0, 0, 0, 0), { 1.0f, 0 }, batch);

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources(m_bindings.get());

    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(6);

    cb->endPass();
}

void SpectrumRhiRenderer::createResources()
{
    if (m_resourcesCreated) {
        return;
    }

    // Spectrum data texture: R32F, 2048x1 (1D normalized amplitude data)
    m_spectrumDataTexture.reset(m_rhi->newTexture(
        QRhiTexture::R32F,
        QSize(SPECTRUM_WIDTH, 1),
        1,
        QRhiTexture::Flags()
    ));
    if (!m_spectrumDataTexture->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create spectrum data texture");
        return;
    }

    // Spectrum color LUT texture: RGBA8, 256x1
    m_spectrumColorLutTexture.reset(m_rhi->newTexture(
        QRhiTexture::RGBA8,
        QSize(LUT_SIZE, 1),
        1,
        QRhiTexture::Flags()
    ));
    if (!m_spectrumColorLutTexture->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create spectrum color LUT texture");
        return;
    }

    // Sampler: linear filtering, clamp to edge
    m_sampler.reset(m_rhi->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge
    ));
    if (!m_sampler->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create sampler");
        return;
    }

    // Fullscreen quad with position + texcoord (4 floats per vertex, 6 vertices)
    static const float quadVertices[] = {
        // position (x,y)  texcoord (u,v)
        -1.0f, -1.0f,     0.0f, 1.0f,   // bottom-left  (texCoord v=1 = bottom)
         1.0f, -1.0f,     1.0f, 1.0f,   // bottom-right
         1.0f,  1.0f,     1.0f, 0.0f,   // top-right    (texCoord v=0 = top)
        -1.0f, -1.0f,     0.0f, 1.0f,   // bottom-left
         1.0f,  1.0f,     1.0f, 0.0f,   // top-right
        -1.0f,  1.0f,     0.0f, 0.0f    // top-left
    };

    m_vertexBuffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::VertexBuffer,
        sizeof(quadVertices)
    ));
    if (!m_vertexBuffer->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create vertex buffer");
        return;
    }

    // Uniform buffer: 48 bytes (3 × vec4 std140 blocks)
    static constexpr int UNIFORM_BUFFER_SIZE = 48;
    m_uniformBuffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        UNIFORM_BUFFER_SIZE
    ));
    if (!m_uniformBuffer->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create uniform buffer");
        return;
    }

    // Load shaders
    QShader vertShader = QShader::fromSerialized(
        []() {
            QFile f(QStringLiteral(":/shaders/spectrum.vert.qsb"));
            if (f.open(QIODevice::ReadOnly)) {
                return f.readAll();
            }
            LOG_ERROR("SpectrumRhiRenderer", "Failed to open vertex shader: :/shaders/spectrum.vert.qsb");
            return QByteArray();
        }()
    );

    QShader fragShader = QShader::fromSerialized(
        []() {
            QFile f(QStringLiteral(":/shaders/spectrum.frag.qsb"));
            if (f.open(QIODevice::ReadOnly)) {
                return f.readAll();
            }
            LOG_ERROR("SpectrumRhiRenderer", "Failed to open fragment shader: :/shaders/spectrum.frag.qsb");
            return QByteArray();
        }()
    );

    if (!vertShader.isValid() || !fragShader.isValid()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to load shaders");
        return;
    }

    // Shader resource bindings
    m_bindings.reset(m_rhi->newShaderResourceBindings());
    m_bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_uniformBuffer.get()
        ),
        QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage,
            m_spectrumDataTexture.get(), m_sampler.get()
        ),
        QRhiShaderResourceBinding::sampledTexture(
            2, QRhiShaderResourceBinding::FragmentStage,
            m_spectrumColorLutTexture.get(), m_sampler.get()
        )
    });
    if (!m_bindings->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create shader resource bindings");
        return;
    }

    // Graphics pipeline with alpha blending
    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });

    // Vertex input: 4 floats per vertex (position + texcoord)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },                    // position
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }     // texcoord
    });
    m_pipeline->setVertexInputLayout(inputLayout);

    // Alpha blending for glow transparency
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({ blend });

    m_pipeline->setShaderResourceBindings(m_bindings.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());

    if (!m_pipeline->create()) {
        LOG_ERROR("SpectrumRhiRenderer", "Failed to create graphics pipeline");
        return;
    }

    // Upload initial data
    QRhiResourceUpdateBatch* initialBatch = m_rhi->nextResourceUpdateBatch();

    // Upload vertex data
    initialBatch->uploadStaticBuffer(m_vertexBuffer.get(), quadVertices);

    // Create and upload color LUT
    createSpectrumColorLUT();
    initialBatch->uploadTexture(m_spectrumColorLutTexture.get(),
        QRhiTextureUploadEntry(0, 0,
            QRhiTextureSubresourceUploadDescription(
                reinterpret_cast<const char*>(m_colorLUTData.constData()),
                m_colorLUTData.size()
            )
        )
    );

    // Clear spectrum data to zero
    QVector<float> clearData(SPECTRUM_WIDTH, 0.0f);
    QRhiTextureSubresourceUploadDescription clearDesc(
        reinterpret_cast<const char*>(clearData.constData()),
        clearData.size() * sizeof(float)
    );
    clearDesc.setDestinationTopLeft(QPoint(0, 0));
    clearDesc.setSourceSize(QSize(SPECTRUM_WIDTH, 1));
    initialBatch->uploadTexture(m_spectrumDataTexture.get(),
        QRhiTextureUploadEntry(0, 0, clearDesc)
    );

    m_initialBatch = initialBatch;

    m_resourcesCreated = true;
    LOG_INFO("SpectrumRhiRenderer", "GPU resources created successfully");
}

void SpectrumRhiRenderer::createSpectrumColorLUT()
{
    // QK4-style spectrum color LUT: Royal Blue -> Cyan -> Green -> Yellow -> Red -> White
    // 8 color stops across 256 entries, stored as RGBA8 (quint8 array)
    m_colorLUTData.resize(LUT_SIZE * 4);

    struct ColorStop {
        float pos;
        quint8 r, g, b;
    };

    static const ColorStop stops[] = {
        { 0.000f,   30,   0, 120 },   // Dark Royal Blue
        { 0.125f,   60,  30, 200 },   // Royal Blue
        { 0.250f,    0, 140, 255 },   // Bright Blue
        { 0.375f,    0, 212, 255 },   // Cyan
        { 0.500f,    0, 220,  80 },   // Green
        { 0.625f,  180, 240,   0 },   // Yellow-Green
        { 0.750f,  255, 200,   0 },   // Yellow
        { 0.875f,  255,  80,   0 },   // Orange-Red
        { 1.000f,  255, 255, 255 },   // White (strong signals)
    };

    static constexpr int NUM_STOPS = sizeof(stops) / sizeof(stops[0]);

    for (int i = 0; i < LUT_SIZE; ++i) {
        float t = static_cast<float>(i) / (LUT_SIZE - 1);

        // Find surrounding color stops
        int lower = 0;
        for (int s = 0; s < NUM_STOPS - 1; ++s) {
            if (t >= stops[s].pos) {
                lower = s;
            }
        }
        int upper = qMin(lower + 1, NUM_STOPS - 1);

        // Interpolate between stops
        float segRange = stops[upper].pos - stops[lower].pos;
        float segT = (segRange > 0.0f) ? (t - stops[lower].pos) / segRange : 0.0f;
        segT = qBound(0.0f, segT, 1.0f);

        quint8 r = static_cast<quint8>(stops[lower].r + segT * (stops[upper].r - stops[lower].r));
        quint8 g = static_cast<quint8>(stops[lower].g + segT * (stops[upper].g - stops[lower].g));
        quint8 b = static_cast<quint8>(stops[lower].b + segT * (stops[upper].b - stops[lower].b));

        // RGBA8 format: store as bytes
        const int base = i * 4;
        m_colorLUTData[base + 0] = r;
        m_colorLUTData[base + 1] = g;
        m_colorLUTData[base + 2] = b;
        m_colorLUTData[base + 3] = 255;
    }
}

} // namespace TR4QT
