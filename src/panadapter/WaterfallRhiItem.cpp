/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "WaterfallRhiItem.h"
#include "../logging/LogMacros.h"
#include <QFile>
#include <QtMath>

namespace TR4QT {

// ============================================================================
// WaterfallRhiItem
// ============================================================================

WaterfallRhiItem::WaterfallRhiItem(QQuickItem* parent)
    : QQuickRhiItem(parent)
{
    setFlag(ItemHasContents, true);
}

WaterfallRhiItem::~WaterfallRhiItem() = default;

void WaterfallRhiItem::setWaterfallHeight(int height)
{
    height = qBound(50, height, 500);
    if (m_waterfallHeight != height) {
        m_waterfallHeight = height;
        emit waterfallHeightChanged();
        update();
    }
}

void WaterfallRhiItem::setRefLevel(float db)
{
    if (!qFuzzyCompare(m_refLevel, db)) {
        m_refLevel = db;
        emit refLevelChanged();
        update();
    }
}

void WaterfallRhiItem::setWaterfallRange(float db)
{
    db = qBound(20.0f, db, 120.0f);
    if (!qFuzzyCompare(m_waterfallRange, db)) {
        m_waterfallRange = db;
        emit waterfallRangeChanged();
        update();
    }
}

void WaterfallRhiItem::setWaterfallRefLevel(float db)
{
    if (!qFuzzyCompare(m_waterfallRefLevel, db)) {
        m_waterfallRefLevel = db;
        emit waterfallRefLevelChanged();
        update();
    }
}

QQuickRhiItemRenderer* WaterfallRhiItem::createRenderer()
{
    return new WaterfallRhiRenderer();
}

void WaterfallRhiItem::addRow(const QVariantList& samples)
{
    QMutexLocker locker(&m_mutex);

    // Convert QVariantList to QVector<float>
    m_pendingRow.clear();
    m_pendingRow.reserve(samples.size());
    for (const QVariant& v : samples) {
        m_pendingRow.append(v.toFloat());
    }
    m_hasPendingRow = true;

    // Trigger re-render
    update();
}

bool WaterfallRhiItem::hasPendingRow() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasPendingRow;
}

QVector<float> WaterfallRhiItem::takePendingRow()
{
    QMutexLocker locker(&m_mutex);
    m_hasPendingRow = false;
    return std::move(m_pendingRow);
}

// ============================================================================
// WaterfallRhiRenderer
// ============================================================================

WaterfallRhiRenderer::WaterfallRhiRenderer() = default;

WaterfallRhiRenderer::~WaterfallRhiRenderer() = default;

void WaterfallRhiRenderer::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb)

    m_rhi = rhi();
    if (!m_rhi) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to get QRhi instance");
        return;
    }

    LOG_INFO("WaterfallRhiRenderer", QString("Initializing with RHI backend: %1")
             .arg(m_rhi->backendName()));

    createResources();
}

void WaterfallRhiRenderer::synchronize(QQuickRhiItem* item)
{
    auto* waterfallItem = static_cast<WaterfallRhiItem*>(item);

    // Get updated settings
    // Use waterfall-specific ref level if set, otherwise fall back to spectrum ref level
    m_waterfallRefLevel = waterfallItem->getWaterfallRefLevel();
    m_refLevel = waterfallItem->getRefLevel();
    m_waterfallRange = waterfallItem->getWaterfallRange();

    // Check for new row data
    if (waterfallItem->hasPendingRow()) {
        m_pendingRow = waterfallItem->takePendingRow();
        m_hasPendingRow = true;
    }
}

void WaterfallRhiRenderer::render(QRhiCommandBuffer* cb)
{
    if (!m_resourcesCreated || !m_pipeline) {
        return;
    }

    QRhiResourceUpdateBatch* batch = nullptr;

    // Submit initial batch on first render (contains vertex data, color LUT, initial texture)
    if (m_initialBatch) {
        batch = m_initialBatch;
        m_initialBatch = nullptr;
    } else {
        batch = m_rhi->nextResourceUpdateBatch();
    }

    // Upload new row if available
    if (m_hasPendingRow && !m_pendingRow.isEmpty()) {
        uploadNewRow(batch, m_pendingRow);
        m_hasPendingRow = false;
    }

    // Update uniform buffer with current settings
    // Layout: float rowOffset, float refLevel, float waterfallRange, float padding
    // rowOffset points to the newest row (just written), not the next write position
    int newestRow = (m_currentRow > 0) ? (m_currentRow - 1) : (m_waterfallHeight - 1);
    float rowOffset = static_cast<float>(newestRow) / m_waterfallHeight;
    // Use waterfall-specific ref level for color mapping (independent from spectrum)
    float uniforms[4] = { rowOffset, m_waterfallRefLevel, m_waterfallRange, 0.0f };
    batch->updateDynamicBuffer(m_uniformBuffer.get(), 0, sizeof(uniforms), uniforms);

    const QSize outputSize = renderTarget()->pixelSize();
    cb->beginPass(renderTarget(), QColor::fromRgbF(0.0f, 0.0f, 0.12f), { 1.0f, 0 }, batch);

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources(m_bindings.get());

    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(6);  // 2 triangles = 6 vertices

    cb->endPass();
}

void WaterfallRhiRenderer::createResources()
{
    if (m_resourcesCreated) {
        return;
    }

    // Create waterfall texture (R32F format for dB values)
    // Using R32F gives us single-channel float storage
    // Note: No special flag needed for upload destination in Qt 6.7+
    m_waterfallTexture.reset(m_rhi->newTexture(
        QRhiTexture::R32F,
        QSize(m_waterfallWidth, m_waterfallHeight),
        1,
        QRhiTexture::Flags()
    ));
    if (!m_waterfallTexture->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create waterfall texture");
        return;
    }

    // Create color LUT texture (RGBA8, 256x1)
    // Note: No special flag needed for upload destination in Qt 6.7+
    m_colorLutTexture.reset(m_rhi->newTexture(
        QRhiTexture::RGBA8,
        QSize(LUT_SIZE, 1),
        1,
        QRhiTexture::Flags()
    ));
    if (!m_colorLutTexture->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create color LUT texture");
        return;
    }

    // Create sampler
    m_sampler.reset(m_rhi->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge
    ));
    if (!m_sampler->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create sampler");
        return;
    }

    // Create fullscreen quad vertex buffer
    // Two triangles covering -1 to 1 in NDC
    static const float quadVertices[] = {
        // Position (x, y)
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    m_vertexBuffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::VertexBuffer,
        sizeof(quadVertices)
    ));
    if (!m_vertexBuffer->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create vertex buffer");
        return;
    }

    // Create uniform buffer
    // 4 floats: rowOffset, refLevel, waterfallRange, padding
    m_uniformBuffer.reset(m_rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        16  // 4 floats × 4 bytes
    ));
    if (!m_uniformBuffer->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create uniform buffer");
        return;
    }

    // Load shaders from Qt resources
    QShader vertShader = QShader::fromSerialized(
        []() {
            QFile f(QStringLiteral(":/shaders/waterfall.vert.qsb"));
            if (f.open(QIODevice::ReadOnly)) {
                return f.readAll();
            }
            LOG_ERROR("WaterfallRhiRenderer", "Failed to open vertex shader file: :/shaders/waterfall.vert.qsb");
            return QByteArray();
        }()
    );

    QShader fragShader = QShader::fromSerialized(
        []() {
            QFile f(QStringLiteral(":/shaders/waterfall.frag.qsb"));
            if (f.open(QIODevice::ReadOnly)) {
                return f.readAll();
            }
            LOG_ERROR("WaterfallRhiRenderer", "Failed to open fragment shader file: :/shaders/waterfall.frag.qsb");
            return QByteArray();
        }()
    );

    if (!vertShader.isValid() || !fragShader.isValid()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to load shaders");
        return;
    }

    // Create shader resource bindings
    m_bindings.reset(m_rhi->newShaderResourceBindings());
    m_bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_uniformBuffer.get()
        ),
        QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage,
            m_waterfallTexture.get(), m_sampler.get()
        ),
        QRhiShaderResourceBinding::sampledTexture(
            2, QRhiShaderResourceBinding::FragmentStage,
            m_colorLutTexture.get(), m_sampler.get()
        )
    });
    if (!m_bindings->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create shader resource bindings");
        return;
    }

    // Create graphics pipeline
    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 2 * sizeof(float) }  // 2 floats per vertex
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 }  // position at location 0
    });
    m_pipeline->setVertexInputLayout(inputLayout);

    m_pipeline->setShaderResourceBindings(m_bindings.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());

    if (!m_pipeline->create()) {
        LOG_ERROR("WaterfallRhiRenderer", "Failed to create graphics pipeline");
        return;
    }

    // Upload initial data
    QRhiResourceUpdateBatch* initialBatch = m_rhi->nextResourceUpdateBatch();

    // Upload vertex data
    initialBatch->uploadStaticBuffer(m_vertexBuffer.get(), quadVertices);

    // Create and upload color LUT
    createColorLUT();
    initialBatch->uploadTexture(m_colorLutTexture.get(),
        QRhiTextureUploadEntry(0, 0,
            QRhiTextureSubresourceUploadDescription(
                reinterpret_cast<const char*>(m_colorLUTData.constData()),
                m_colorLUTData.size() * sizeof(quint32)
            )
        )
    );

    // Clear waterfall texture to noise floor
    QVector<float> clearRow(m_waterfallWidth, -130.0f);
    for (int row = 0; row < m_waterfallHeight; ++row) {
        QRhiTextureSubresourceUploadDescription rowDesc(
            reinterpret_cast<const char*>(clearRow.constData()),
            clearRow.size() * sizeof(float)
        );
        rowDesc.setDestinationTopLeft(QPoint(0, row));
        rowDesc.setSourceSize(QSize(m_waterfallWidth, 1));
        initialBatch->uploadTexture(m_waterfallTexture.get(),
            QRhiTextureUploadEntry(0, 0, rowDesc)
        );
    }

    // Store initial batch to be submitted on first render
    m_initialBatch = initialBatch;

    m_resourcesCreated = true;
    LOG_INFO("WaterfallRhiRenderer", "GPU resources created successfully");
}

void WaterfallRhiRenderer::uploadNewRow(QRhiResourceUpdateBatch* batch,
                                         const QVector<float>& samples)
{
    if (samples.isEmpty()) {
        return;
    }

    // Resample if needed to match texture width
    QVector<float> rowData;
    if (samples.size() == m_waterfallWidth) {
        rowData = samples;
    } else {
        rowData.resize(m_waterfallWidth);
        float ratio = static_cast<float>(samples.size()) / m_waterfallWidth;
        for (int x = 0; x < m_waterfallWidth; ++x) {
            int srcIndex = qBound(0, static_cast<int>(x * ratio), samples.size() - 1);
            rowData[x] = samples[srcIndex];
        }
    }

    // Upload to current row position in circular buffer
    QRhiTextureSubresourceUploadDescription rowDesc(
        reinterpret_cast<const char*>(rowData.constData()),
        rowData.size() * sizeof(float)
    );
    rowDesc.setDestinationTopLeft(QPoint(0, m_currentRow));
    rowDesc.setSourceSize(QSize(m_waterfallWidth, 1));

    batch->uploadTexture(m_waterfallTexture.get(),
        QRhiTextureUploadEntry(0, 0, rowDesc)
    );

    // Advance circular buffer pointer
    m_currentRow = (m_currentRow + 1) % m_waterfallHeight;
}

void WaterfallRhiRenderer::createColorLUT()
{
    m_colorLUTData.resize(LUT_SIZE);

    // Same color gradient as WaterfallImageProvider
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

        // RGBA8 format: 0xAABBGGRR (little-endian)
        m_colorLUTData[i] = (255u << 24) | (static_cast<quint32>(b) << 16) |
                           (static_cast<quint32>(g) << 8) | static_cast<quint32>(r);
    }
}

} // namespace TR4QT
