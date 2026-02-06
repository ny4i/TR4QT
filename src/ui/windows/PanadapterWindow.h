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

#ifndef PANADAPTERWINDOW_H
#define PANADAPTERWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <memory>
#include "../../panadapter/IPanadapterRenderer.h"
#include "../../panadapter/PanadapterDataModel.h"
#include "../../radio/K4PanadapterReader.h"

namespace TR4QT {

// Forward declaration
struct Spot;

/**
 * @brief Window for displaying K4 panadapter and waterfall
 *
 * This window hosts the panadapter renderer and provides controls for:
 * - Palette selection
 * - Reference level adjustment
 * - Averaging control
 * - Pause/Resume
 *
 * Integrates with K4PanadapterReader for live data from the radio.
 */
class PanadapterWindow : public QWidget {
    Q_OBJECT

public:
    explicit PanadapterWindow(QWidget* parent = nullptr);
    ~PanadapterWindow() override;

    /**
     * @brief Check if window was visible when app last closed
     * @return true if window should be restored on startup
     */
    static bool wasVisibleOnClose();

    /**
     * @brief Connect to a K4 radio's panadapter port
     * @param host IP address or hostname
     * @param port Port number (default CAT port + 1 = 9201)
     */
    void connectToRadio(const QString& host, int port = 9201);

    /**
     * @brief Auto-connect to any K4 Direct radio found in profiles
     */
    void connectToK4();

    /**
     * @brief Disconnect from the radio
     */
    void disconnectFromRadio();

    /**
     * @brief Check if connected to radio panadapter
     */
    bool isConnected() const;

    /**
     * @brief Set the pan ID to display (A, B, Y, or Z)
     * @param panId The pan ID character
     */
    void setPanId(char panId);

    /**
     * @brief Update DX spots to display on the panadapter
     * @param spots List of all current spots (will be filtered by frequency range)
     */
    void updateSpots(const QList<Spot>& spots);

signals:
    /**
     * @brief Emitted when user clicks on waterfall to tune
     * @param freqHz Clicked frequency in Hz
     * @param vfo 0 for VFO A (left click), 1 for VFO B (right click)
     */
    void frequencyClicked(qint64 freqHz, int vfo);

    /**
     * @brief Emitted when cursor moves over waterfall
     * @param freqHz Current frequency under cursor
     * @param db Signal level in dB
     */
    void cursorMoved(qint64 freqHz, float db);

    /**
     * @brief Emitted when connection status changes
     */
    void connectionStatusChanged(bool connected);

    /**
     * @brief Emitted when user closes the window (not during app shutdown)
     */
    void windowClosed();

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onPacketReceived(const PanadapterPacket& packet);
    void onConnected();
    void onDisconnected();
    void onError(const QString& message);
    void onPaletteChanged(int index);
    void onRefLevelChanged(int value);
    void onAveragingChanged(int value);
    void onWaterfallRangeChanged(int value);
    void onWaterfallRefLevelChanged(int value);
    void onPauseToggled();
    void onRendererFrequencyClicked(qint64 freqHz, int vfo);
    void onRendererCursorMoved(qint64 freqHz, float db);

private:
    void setupUI();
    void saveWindowState();
    void restoreWindowState();

    // Renderer
    std::unique_ptr<IPanadapterRenderer> m_renderer;

    // Data model (thread-safe buffer)
    std::unique_ptr<PanadapterDataModel> m_dataModel;

    // Network reader
    std::unique_ptr<K4PanadapterReader> m_reader;

    // Current pan ID
    char m_panId{'A'};

    // UI controls
    QComboBox* m_paletteCombo{nullptr};
    QSlider* m_refLevelSlider{nullptr};
    QSlider* m_averagingSlider{nullptr};
    QSlider* m_wfRangeSlider{nullptr};
    QSlider* m_wfRefLevelSlider{nullptr};
    QLabel* m_refLevelLabel{nullptr};
    QLabel* m_averagingLabel{nullptr};
    QLabel* m_wfRangeLabel{nullptr};
    QLabel* m_wfRefLevelLabel{nullptr};
    QPushButton* m_pauseButton{nullptr};
    QLabel* m_statusLabel{nullptr};

    // Connection state
    QString m_host;
    int m_port{9201};
    bool m_paused{false};
    bool m_wasVisible{false};  // Track visibility for persistence (isVisible() fails during shutdown)
    bool m_wasConnected{false};  // Track if we should reconnect when window is shown again

    // Frequency state for spot filtering
    qint64 m_centerFrequency{7200000};
    int m_sampleRate{48000};
    QList<Spot> m_allSpots;  // All spots from MainWindow
};

} // namespace TR4QT

#endif // PANADAPTERWINDOW_H
