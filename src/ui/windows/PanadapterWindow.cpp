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

#include "PanadapterWindow.h"
#include "../../panadapter/renderers/QmlPanadapterRenderer.h"
#include "../../radio/RadioFactory.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include "../../core/Constants.h"
#include <QSettings>
#include <QCloseEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QApplication>

namespace TR4QT {

// Control ranges
static constexpr int REF_LEVEL_MIN = -30;
static constexpr int REF_LEVEL_MAX = 30;
static constexpr int REF_LEVEL_DEFAULT = 0;
static constexpr int AVERAGING_MIN = 1;
static constexpr int AVERAGING_MAX = 10;
static constexpr int AVERAGING_DEFAULT = 3;
static constexpr int WF_RANGE_MIN = 30;
static constexpr int WF_RANGE_MAX = 120;
static constexpr int WF_RANGE_DEFAULT = 80;

// Window size
static constexpr int DEFAULT_WIDTH = 600;
static constexpr int DEFAULT_HEIGHT = 400;

PanadapterWindow::PanadapterWindow(QWidget* parent)
    : QWidget(parent)
    , m_renderer(std::make_unique<QmlPanadapterRenderer>())
    , m_dataModel(std::make_unique<PanadapterDataModel>())
    , m_reader(std::make_unique<K4PanadapterReader>())
{
    setWindowTitle("Panadapter");
    setMinimumSize(400, 300);
    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    setupUI();
    restoreWindowState();

    // Connect reader signals
    QObject::connect(m_reader.get(), &K4PanadapterReader::packetReceived,
            this, &PanadapterWindow::onPacketReceived);
    QObject::connect(m_reader.get(), &K4PanadapterReader::connected,
            this, &PanadapterWindow::onConnected);
    QObject::connect(m_reader.get(), &K4PanadapterReader::disconnected,
            this, &PanadapterWindow::onDisconnected);
    QObject::connect(m_reader.get(), &K4PanadapterReader::error,
            this, &PanadapterWindow::onError);

    // Connect renderer signals
    QObject::connect(m_renderer.get(), &IPanadapterRenderer::frequencyClicked,
            this, &PanadapterWindow::onRendererFrequencyClicked);
    QObject::connect(m_renderer.get(), &IPanadapterRenderer::cursorMoved,
            this, &PanadapterWindow::onRendererCursorMoved);

    LOG_INFO("PanadapterWindow", "Panadapter window created");
}

PanadapterWindow::~PanadapterWindow()
{
    LOG_DEBUG("PanadapterWindow", QString("Destructor called, m_wasVisible=%1").arg(m_wasVisible));
    disconnectFromRadio();
    saveWindowState();
}

void PanadapterWindow::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Control bar at top
    auto* controlBar = new QWidget();
    auto* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(5, 5, 5, 5);

    // Palette selector
    auto* paletteLabel = new QLabel("Palette:");
    m_paletteCombo = new QComboBox();
    m_paletteCombo->addItem("Heat Map", "heatmap");
    m_paletteCombo->addItem("Grayscale", "grayscale");
    m_paletteCombo->addItem("Sepia", "sepia");
    m_paletteCombo->addItem("Blue-Green", "bluegreen");
    m_paletteCombo->addItem("Fire-Ice", "fireice");
    QObject::connect(m_paletteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PanadapterWindow::onPaletteChanged);

    // Reference level slider
    auto* refLabel = new QLabel("Ref:");
    m_refLevelSlider = new QSlider(Qt::Horizontal);
    m_refLevelSlider->setRange(REF_LEVEL_MIN, REF_LEVEL_MAX);
    m_refLevelSlider->setValue(REF_LEVEL_DEFAULT);
    m_refLevelSlider->setFixedWidth(100);
    m_refLevelLabel = new QLabel(QString("%1 dB").arg(REF_LEVEL_DEFAULT));
    m_refLevelLabel->setFixedWidth(50);
    QObject::connect(m_refLevelSlider, &QSlider::valueChanged,
            this, &PanadapterWindow::onRefLevelChanged);

    // Averaging slider
    auto* avgLabel = new QLabel("Avg:");
    m_averagingSlider = new QSlider(Qt::Horizontal);
    m_averagingSlider->setRange(AVERAGING_MIN, AVERAGING_MAX);
    m_averagingSlider->setValue(AVERAGING_DEFAULT);
    m_averagingSlider->setFixedWidth(80);
    m_averagingLabel = new QLabel(QString::number(AVERAGING_DEFAULT));
    m_averagingLabel->setFixedWidth(20);
    QObject::connect(m_averagingSlider, &QSlider::valueChanged,
            this, &PanadapterWindow::onAveragingChanged);

    // Waterfall range slider (for contrast control)
    auto* wfRangeLabel = new QLabel("WF:");
    m_wfRangeSlider = new QSlider(Qt::Horizontal);
    m_wfRangeSlider->setRange(WF_RANGE_MIN, WF_RANGE_MAX);
    m_wfRangeSlider->setValue(WF_RANGE_DEFAULT);
    m_wfRangeSlider->setFixedWidth(80);
    m_wfRangeLabel = new QLabel(QString("%1 dB").arg(WF_RANGE_DEFAULT));
    m_wfRangeLabel->setFixedWidth(45);
    QObject::connect(m_wfRangeSlider, &QSlider::valueChanged,
            this, &PanadapterWindow::onWaterfallRangeChanged);

    // Pause button
    m_pauseButton = new QPushButton("Pause");
    m_pauseButton->setCheckable(true);
    QObject::connect(m_pauseButton, &QPushButton::clicked,
            this, &PanadapterWindow::onPauseToggled);

    // Status label
    m_statusLabel = new QLabel("Disconnected");
    m_statusLabel->setStyleSheet("color: #888;");

    // Layout controls
    controlLayout->addWidget(paletteLabel);
    controlLayout->addWidget(m_paletteCombo);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(refLabel);
    controlLayout->addWidget(m_refLevelSlider);
    controlLayout->addWidget(m_refLevelLabel);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(avgLabel);
    controlLayout->addWidget(m_averagingSlider);
    controlLayout->addWidget(m_averagingLabel);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(wfRangeLabel);
    controlLayout->addWidget(m_wfRangeSlider);
    controlLayout->addWidget(m_wfRangeLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(m_pauseButton);
    controlLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(controlBar);

    // Renderer widget
    QWidget* rendererWidget = m_renderer->widget();
    if (rendererWidget) {
        mainLayout->addWidget(rendererWidget, 1);  // Stretch to fill
    }

    // Apply initial settings to renderer
    m_renderer->setAveraging(AVERAGING_DEFAULT);
    m_renderer->setRefLevel(static_cast<float>(REF_LEVEL_DEFAULT));
    m_renderer->setWaterfallRange(static_cast<float>(WF_RANGE_DEFAULT));
    m_renderer->setPanId(m_panId);
}

void PanadapterWindow::connectToRadio(const QString& host, int port)
{
    m_host = host;
    m_port = port;

    LOG_INFO("PanadapterWindow", QString("Connecting to %1:%2").arg(host).arg(port));
    m_reader->connectToRadio(host, port);
}

void PanadapterWindow::disconnectFromRadio()
{
    m_reader->disconnectFromRadio();
}

bool PanadapterWindow::isConnected() const
{
    return m_reader->isConnected();
}

void PanadapterWindow::setPanId(char panId)
{
    m_panId = panId;
    m_renderer->setPanId(panId);
    setWindowTitle(QString("Panadapter - Pan %1").arg(panId));
}

void PanadapterWindow::onPacketReceived(const PanadapterPacket& packet)
{
    // Filter for our pan ID
    if (packet.panId != m_panId) {
        return;
    }

    // Update renderer with new data
    m_renderer->updateSamples(packet.samples);
    m_renderer->setCenterFrequency(packet.centerFreqHz);
    m_renderer->setSampleRate(packet.sampleRateHz);
    m_renderer->setNoiseFloor(packet.noiseFloor);
}

void PanadapterWindow::connectToK4()
{
    // Find a K4 Direct radio in the profiles and connect to it
    AppSettings& settings = AppSettings::instance();
    QList<RadioProfile> profiles = settings.loadRadioProfiles();

    for (const RadioProfile& profile : profiles) {
        // Check if this is a K4 Direct radio (radioType 1)
        if (profile.config.radioType == static_cast<int>(RadioFactory::RadioType::K4_DIRECT)) {
            QString hostPort = profile.config.port;
            QString host;
            int catPort = 9200;  // Default K4 CAT port

            // Parse host:port
            if (hostPort.contains(':')) {
                QStringList parts = hostPort.split(':');
                host = parts[0];
                catPort = parts[1].toInt();
            } else {
                host = hostPort;
            }

            if (!host.isEmpty()) {
                LOG_INFO("PanadapterWindow", QString("Connecting to K4 panadapter at %1:%2").arg(host).arg(catPort + 1));
                connectToRadio(host, catPort + 1);
                return;
            }
        }
    }

    LOG_WARN("PanadapterWindow", "No K4 Direct radio found in profiles");
}

void PanadapterWindow::onConnected()
{
    LOG_INFO("PanadapterWindow", "Connected to panadapter");
    m_statusLabel->setText("Connected");
    m_statusLabel->setStyleSheet("color: #0a0;");
    emit connectionStatusChanged(true);
}

void PanadapterWindow::onDisconnected()
{
    LOG_INFO("PanadapterWindow", "Disconnected from panadapter");
    m_statusLabel->setText("Disconnected");
    m_statusLabel->setStyleSheet("color: #888;");
    emit connectionStatusChanged(false);
}

void PanadapterWindow::onError(const QString& message)
{
    LOG_ERROR("PanadapterWindow", QString("Panadapter error: %1").arg(message));
    m_statusLabel->setText(QString("Error: %1").arg(message));
    m_statusLabel->setStyleSheet("color: #a00;");
}

void PanadapterWindow::onPaletteChanged(int index)
{
    QString paletteId = m_paletteCombo->itemData(index).toString();
    WaterfallPalette palette = WaterfallPalette::HeatMap;

    if (paletteId == "grayscale") palette = WaterfallPalette::Grayscale;
    else if (paletteId == "sepia") palette = WaterfallPalette::Sepia;
    else if (paletteId == "bluegreen") palette = WaterfallPalette::BlueGreen;
    else if (paletteId == "fireice") palette = WaterfallPalette::FireIce;

    m_renderer->setPalette(palette);
}

void PanadapterWindow::onRefLevelChanged(int value)
{
    m_refLevelLabel->setText(QString("%1 dB").arg(value));
    m_renderer->setRefLevel(static_cast<float>(value));
}

void PanadapterWindow::onAveragingChanged(int value)
{
    m_averagingLabel->setText(QString::number(value));
    m_renderer->setAveraging(value);
}

void PanadapterWindow::onWaterfallRangeChanged(int value)
{
    m_wfRangeLabel->setText(QString("%1 dB").arg(value));
    m_renderer->setWaterfallRange(static_cast<float>(value));
}

void PanadapterWindow::onPauseToggled()
{
    m_paused = m_pauseButton->isChecked();
    m_pauseButton->setText(m_paused ? "Resume" : "Pause");
    m_renderer->setPaused(m_paused);
}

void PanadapterWindow::onRendererFrequencyClicked(qint64 freqHz, int vfo)
{
    emit frequencyClicked(freqHz, vfo);
}

void PanadapterWindow::onRendererCursorMoved(qint64 freqHz, float db)
{
    emit cursorMoved(freqHz, db);
}

void PanadapterWindow::closeEvent(QCloseEvent* event)
{
    // Only set m_wasVisible=false if user explicitly closed window (not app shutdown)
    if (!QApplication::closingDown()) {
        m_wasVisible = false;  // User clicked X to close window
        LOG_DEBUG("PanadapterWindow", "User closed window, m_wasVisible=false");
        emit windowClosed();  // Notify MainWindow to update visibility tracking
    } else {
        LOG_DEBUG("PanadapterWindow", "App closing down, keeping m_wasVisible=true");
    }
    saveWindowState();
    disconnectFromRadio();
    event->accept();
}

void PanadapterWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_wasVisible = true;
    LOG_DEBUG("PanadapterWindow", "Window shown, m_wasVisible=true");
}

void PanadapterWindow::hideEvent(QHideEvent* event)
{
    // Don't set m_wasVisible=false here - hideEvent fires during app shutdown
    // Only closeEvent (user action) should set it to false
    QWidget::hideEvent(event);
    LOG_DEBUG("PanadapterWindow", QString("Window hidden, m_wasVisible=%1").arg(m_wasVisible));
}

void PanadapterWindow::saveWindowState()
{
    LOG_DEBUG("PanadapterWindow", QString("saveWindowState called, m_wasVisible=%1").arg(m_wasVisible));
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("PanadapterWindow");
    settings.setValue("geometry", saveGeometry());
    // NOTE: Visibility is saved by MainWindow/SettingsManager BEFORE child closeEvents fire,
    // so we don't save it here to avoid overwriting the correct value during app shutdown.
    // The "visible" key is managed by SettingsManager.
    settings.setValue("palette", m_paletteCombo->currentIndex());
    settings.setValue("refLevel", m_refLevelSlider->value());
    settings.setValue("averaging", m_averagingSlider->value());
    settings.setValue("waterfallRange", m_wfRangeSlider->value());
    settings.endGroup();
    settings.sync();  // Force write immediately
    LOG_DEBUG("PanadapterWindow", QString("saveWindowState completed"));
}

bool PanadapterWindow::wasVisibleOnClose()
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("PanadapterWindow");
    bool visible = settings.value("visible", false).toBool();
    settings.endGroup();
    return visible;
}

void PanadapterWindow::restoreWindowState()
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("PanadapterWindow");

    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    if (settings.contains("palette")) {
        int idx = settings.value("palette").toInt();
        m_paletteCombo->setCurrentIndex(idx);
    }

    if (settings.contains("refLevel")) {
        int val = settings.value("refLevel").toInt();
        m_refLevelSlider->setValue(val);
    }

    if (settings.contains("averaging")) {
        int val = settings.value("averaging").toInt();
        m_averagingSlider->setValue(val);
    }

    if (settings.contains("waterfallRange")) {
        int val = settings.value("waterfallRange").toInt();
        m_wfRangeSlider->setValue(val);
    }

    settings.endGroup();
}

} // namespace TR4QT
