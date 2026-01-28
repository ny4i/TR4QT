#ifndef RADIOMANAGER_H
#define RADIOMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>
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
 * Separates radio control logic from MainWindow UI updates.
 *
 * Responsibilities:
 * - Handle radio connect/disconnect
 * - Manage reconnection logic with timer
 * - Process radio state updates (frequency, mode, band)
 * - Handle radio errors
 * - Track connection status
 *
 * UI updates are NOT handled here - RadioManager emits signals
 * that UI components can connect to for updates.
 */
class RadioManager : public QObject {
    Q_OBJECT

public:
    /**
     * Construct a RadioManager
     * @param parent Parent QObject
     */
    explicit RadioManager(QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~RadioManager() override;

    /**
     * Get radio controller instance
     * @return Pointer to RadioController (never null)
     */
    RadioController* radioController() const { return m_radio; }

    /**
     * Get current radio connection status
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return m_radioConnected; }

    /**
     * Get current radio state
     * @return Current RadioState
     */
    RadioState currentState() const { return m_currentState; }

    /**
     * Get auto-reconnect status
     * @return true if auto-reconnect is enabled
     */
    bool isAutoReconnectEnabled() const { return m_radioAutoReconnect; }

    /**
     * Connect to radio
     * Validates configuration, enables auto-reconnect, and initiates connection.
     * Emits statusMessage signal for UI feedback.
     * @return true if connection initiated, false if configuration invalid
     */
    bool connectToRadio();

    /**
     * Disconnect from radio
     * Disables auto-reconnect and stops reconnection timer.
     */
    void disconnectFromRadio();

signals:
    /**
     * Emitted when connection status changes
     * @param connected true if connected, false otherwise
     */
    void connectionStatusChanged(bool connected);

    /**
     * Emitted when radio state is updated
     * @param state Updated RadioState
     */
    void radioStateUpdated(const RadioState& state);

    /**
     * Emitted when radio model changes
     * @param model Radio model name
     */
    void radioModelChanged(const QString& model);

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
     * Emitted when current frequency changes
     * @param frequency Frequency in Hz
     */
    void frequencyChanged(freq_t frequency);

    /**
     * Emitted when current band changes
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
     * @param maxPowerWatts Maximum power in watts (110W for K4, 1800W for KPA1500 in operate mode)
     */
    void maxPowerChanged(int maxPowerWatts);

private slots:
    /**
     * Handle radio connection status change
     * @param connected true if connected
     */
    void onRadioConnected(bool connected);

    /**
     * Handle radio state update
     * @param state Updated radio state
     */
    void onRadioStateUpdated(const RadioState& state);

    /**
     * Handle radio error
     * @param error Error message
     */
    void onRadioError(const QString& error);

    /**
     * Handle fast frequency updates from transceive mode
     * @param freq New frequency in Hz
     * @param vfo Which VFO changed (only VFO A is forwarded to main window)
     */
    void onFrequencyChanged(freq_t freq, VFO vfo);

    /**
     * Handle reconnection retry request
     * Attempts to reconnect to radio using last configuration
     * @param attempt The attempt number (1-based)
     */
    void onRetryRequested(int attempt);

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
    static const int RECONNECT_INTERVAL_MS = 10000;  // 10 seconds between reconnect attempts
    static const int FLASH_INTERVAL_MS = 500;        // 500ms flash rate

    RadioController* m_radio;           // Radio controller instance
    RadioState m_currentState;          // Current radio state
    bool m_radioConnected;              // Connection status
    bool m_radioAutoReconnect;          // Auto-reconnect enabled flag
    ReconnectionManager* m_reconnectManager;  // Auto-reconnect timer (unlimited retries)
    RadioConfig m_lastRadioConfig;      // Last configuration (for reconnection)
    QTimer* m_radioFlashTimer;          // Timer for flashing red indicator
    bool m_radioFlashState;             // Current flash state (on/off)

    // Amplifier (KPA1500) state
    KPA1500UdpPoller* m_amplifier;      // Amplifier UDP poller (null if disabled)
    bool m_amplifierOperateMode;        // Operating status (true=operate, false=standby)
    int m_amplifierForwardPower;        // Amplifier forward power in watts
};

} // namespace TR4QT

#endif // RADIOMANAGER_H
