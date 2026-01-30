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

#ifndef RADIOMANAGER_H
#define RADIOMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <array>
#include "../core/Types.h"
#include "../radio/RadioController.h"
#include "../utils/AppSettings.h"
#include "../utils/ReconnectionManager.h"

namespace TR4QT {

class KPA1500UdpPoller;

/**
 * RadioManager
 *
 * Handles radio hardware interaction and connection management.
 * Supports 2 radios for SO2R (Single Operator Two Radio) operation.
 * Separates radio control logic from MainWindow UI updates.
 *
 * In SO2R mode:
 * - Both radios are connected simultaneously
 * - One radio is "active" (receives keyboard input, used for logging)
 * - One radio is "standby" (monitored, frequency displayed grayed)
 * - Toggle switches which is active/standby
 *
 * Responsibilities:
 * - Handle radio connect/disconnect for multiple radios
 * - Manage reconnection logic with timer per radio
 * - Process radio state updates (frequency, mode, band)
 * - Handle radio errors
 * - Track connection status for all radios
 * - Manage active radio selection for SO2R
 *
 * UI updates are NOT handled here - RadioManager emits signals
 * that UI components can connect to for updates.
 */
class RadioManager : public QObject {
    Q_OBJECT

public:
    /** Maximum number of radios supported for SO2R operation */
    static constexpr int MAX_RADIOS = 2;

    /** Radio indices for clarity */
    static constexpr int RADIO_1 = 0;
    static constexpr int RADIO_2 = 1;

    /**
     * Construct a RadioManager
     * @param parent Parent QObject
     */
    explicit RadioManager(QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~RadioManager() override;

    // ========== Single-Radio API (backward compatible) ==========

    /**
     * Get active radio controller instance
     * @return Pointer to active RadioController (never null)
     */
    RadioController* radioController() const;

    /**
     * Get current (active) radio connection status
     * @return true if active radio is connected, false otherwise
     */
    bool isConnected() const;

    /**
     * Get current (active) radio state
     * @return Current RadioState of active radio
     */
    RadioState currentState() const;

    /**
     * Get auto-reconnect status
     * @return true if auto-reconnect is enabled
     */
    bool isAutoReconnectEnabled() const { return m_radioAutoReconnect; }

    /**
     * Connect to radio(s)
     * In single-radio mode: connects to active profile
     * In SO2R mode: connects to both configured radios
     * Emits statusMessage signal for UI feedback.
     * @return true if connection initiated, false if configuration invalid
     */
    bool connectToRadio();

    /**
     * Disconnect from radio(s)
     * Disables auto-reconnect and disconnects all radios.
     */
    void disconnectFromRadio();

    // ========== SO2R Multi-Radio API ==========

    /**
     * Check if SO2R mode is enabled
     * @return true if SO2R mode is active
     */
    bool isSO2REnabled() const { return m_so2rEnabled; }

    /**
     * Enable or disable SO2R mode
     * @param enabled true to enable SO2R
     */
    void setSO2REnabled(bool enabled);

    /**
     * Get the active radio index (0 or 1)
     * @return Active radio index
     */
    int getActiveRadioIndex() const { return m_activeRadioIndex; }

    /**
     * Get the standby radio index (the other radio)
     * @return Standby radio index
     */
    int getStandbyRadioIndex() const { return m_activeRadioIndex == 0 ? 1 : 0; }

    /**
     * Set which radio is active
     * @param radioIndex Radio index (0 or 1)
     */
    void setActiveRadio(int radioIndex);

    /**
     * Toggle between active and standby radio
     * Convenience method that swaps active/standby
     */
    void toggleActiveRadio();

    /**
     * Connect a specific radio
     * @param radioIndex Radio index (0 or 1)
     * @param config Radio configuration
     * @return true if connection initiated
     */
    bool connectRadio(int radioIndex, const RadioConfig& config);

    /**
     * Disconnect a specific radio
     * @param radioIndex Radio index (0 or 1)
     */
    void disconnectRadio(int radioIndex);

    /**
     * Check if a specific radio is connected
     * @param radioIndex Radio index (0 or 1)
     * @return true if connected
     */
    bool isRadioConnected(int radioIndex) const;

    /**
     * Get state of a specific radio
     * @param radioIndex Radio index (0 or 1)
     * @return RadioState for that radio
     */
    RadioState getRadioState(int radioIndex) const;

    /**
     * Get radio controller for a specific radio
     * @param radioIndex Radio index (0 or 1)
     * @return Pointer to RadioController (may be null if not initialized)
     */
    RadioController* getRadioController(int radioIndex) const;

    /**
     * Get count of connected radios
     * @return Number of connected radios (0, 1, or 2)
     */
    int getConnectedRadioCount() const;

signals:
    // ========== Single-Radio Signals (backward compatible) ==========

    /**
     * Emitted when active radio connection status changes
     * @param connected true if connected, false otherwise
     */
    void connectionStatusChanged(bool connected);

    /**
     * Emitted when active radio state is updated
     * @param state Updated RadioState
     */
    void radioStateUpdated(const RadioState& state);

    /**
     * Emitted when active radio model changes
     * @param model Radio model name
     */
    void radioModelChanged(const QString& model);

    /**
     * Emitted when a specific radio's model is identified
     * @param radioIndex Which radio (0 = Radio 1, 1 = Radio 2)
     * @param model Radio model name
     */
    void radioModelIdentified(int radioIndex, const QString& model);

    /**
     * Emitted when a radio error occurs
     * @param error Error message
     */
    void radioErrorOccurred(const QString& error);

    /**
     * Emitted when status message should be displayed
     * @param message Status message for UI
     */
    void statusMessage(const QString& message);

    /**
     * Emitted when active radio frequency changes
     * @param frequency Frequency in Hz
     */
    void frequencyChanged(freq_t frequency);

    /**
     * Emitted when active radio band changes
     * @param band Current band
     */
    void bandChanged(BandType band);

    /**
     * Emitted when flashing state should be updated
     * Controls red flash indicator when disconnected
     * @param flashState true to flash red, false for normal
     */
    void flashStateChanged(bool flashState);

    /**
     * Emitted when TX power meter max power should be updated
     * @param maxPowerWatts Maximum power in watts
     */
    void maxPowerChanged(int maxPowerWatts);

    // ========== SO2R Multi-Radio Signals ==========

    /**
     * Emitted when active radio changes (SO2R toggle)
     * @param radioIndex New active radio index (0 or 1)
     */
    void activeRadioChanged(int radioIndex);

    /**
     * Emitted when any radio's state is updated
     * @param radioIndex Which radio (0 or 1)
     * @param state Updated RadioState
     */
    void radioStateUpdatedIndexed(int radioIndex, const RadioState& state);

    /**
     * Emitted when a specific radio connects
     * @param radioIndex Which radio (0 or 1)
     */
    void radioConnectedIndexed(int radioIndex);

    /**
     * Emitted when a specific radio disconnects
     * @param radioIndex Which radio (0 or 1)
     */
    void radioDisconnectedIndexed(int radioIndex);

    /**
     * Emitted when standby radio frequency changes
     * Used by UI to update grayed-out standby frequency display
     * @param frequency Standby radio frequency in Hz
     */
    void standbyFrequencyChanged(freq_t frequency);

    /**
     * Emitted when standby radio band changes
     * @param band Standby radio band
     */
    void standbyBandChanged(BandType band);

private slots:
    // Indexed slots for multi-radio support
    void onRadioConnected0(bool connected);
    void onRadioConnected1(bool connected);
    void onRadioStateUpdated0(const RadioState& state);
    void onRadioStateUpdated1(const RadioState& state);
    void onRadioError0(const QString& error);
    void onRadioError1(const QString& error);
    void onFrequencyChanged0(freq_t freq, VFO vfo);
    void onFrequencyChanged1(freq_t freq, VFO vfo);
    void onRetryRequested0(int attempt);
    void onRetryRequested1(int attempt);

    /**
     * Handle flash timer timeout
     * Toggles flash state for disconnected indicator
     */
    void onFlashTimeout();

    /**
     * Handle amplifier forward power change
     * @param watts Forward power in watts
     */
    void onAmplifierPowerChanged(int watts);

    /**
     * Handle amplifier operating status change
     * @param operateMode true if amplifier is in operate mode, false if standby
     */
    void onAmplifierOperatingStatusChanged(bool operateMode);

    /**
     * Handle amplifier error
     * @param error Error message
     */
    void onAmplifierError(const QString& error);

private:
    // Helper methods
    void onRadioConnectedIndexed(int radioIndex, bool connected);
    void onRadioStateUpdatedIndexed(int radioIndex, const RadioState& state);
    void onRadioErrorIndexed(int radioIndex, const QString& error);
    void onFrequencyChangedIndexed(int radioIndex, freq_t freq, VFO vfo);
    void onRetryRequestedIndexed(int radioIndex, int attempt);
    void setupRadioConnections(int radioIndex);

    /**
     * Connect radios using StationProfile configuration
     * @param stationProfile Station profile with radio assignments
     * @return true if at least one radio connection initiated
     */
    bool connectWithStationProfile(const StationProfile& stationProfile);

    static const int RECONNECT_INTERVAL_MS = 10000;  // 10 seconds between reconnect attempts
    static const int FLASH_INTERVAL_MS = 500;        // 500ms flash rate

    // SO2R state
    bool m_so2rEnabled{false};          // SO2R mode enabled
    int m_activeRadioIndex{0};          // Which radio is active (0 or 1)

    // Per-radio state (indexed by radio number 0-1)
    std::array<RadioController*, MAX_RADIOS> m_radios{nullptr, nullptr};
    std::array<RadioState, MAX_RADIOS> m_radioStates{};
    std::array<bool, MAX_RADIOS> m_radioConnected{false, false};
    std::array<ReconnectionManager*, MAX_RADIOS> m_reconnectManagers{nullptr, nullptr};
    std::array<RadioConfig, MAX_RADIOS> m_lastRadioConfigs{};

    // Legacy single-radio pointers (for backward compatibility, point to active radio)
    RadioController* m_radio{nullptr};  // Points to m_radios[m_activeRadioIndex]
    RadioState m_currentState;          // Copy of m_radioStates[m_activeRadioIndex]

    bool m_radioAutoReconnect{false};   // Auto-reconnect enabled flag
    QTimer* m_radioFlashTimer{nullptr}; // Timer for flashing red indicator
    bool m_radioFlashState{false};      // Current flash state (on/off)

    // Amplifier (KPA1500) state - shared across radios
    KPA1500UdpPoller* m_amplifier{nullptr};
    bool m_amplifierOperateMode{false};
    int m_amplifierForwardPower{0};
};

} // namespace TR4QT

#endif // RADIOMANAGER_H
