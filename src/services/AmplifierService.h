#ifndef AMPLIFIERSERVICE_H
#define AMPLIFIERSERVICE_H

#include <QObject>
#include <QString>
#include "../amplifiers/IAmplifierController.h"

namespace TR4QT {

/**
 * AmplifierService
 *
 * Business logic layer for amplifier control
 * Handles amplifier connection management and command execution
 * Separates amplifier control logic from UI updates
 *
 * Responsibilities:
 * - Handle amplifier connect/disconnect
 * - Send raw commands to amplifier (for button presses)
 * - Forward amplifier state changes to UI
 * - Track connection status
 * - Provide clean interface for UI windows
 *
 * UI updates are NOT handled here - AmplifierService emits signals
 * that UI components can connect to for updates.
 *
 * Example usage in AmplifierControlWindow:
 *   connect(m_service, &AmplifierService::stateUpdated,
 *           this, &AmplifierControlWindow::onStateUpdated);
 *   m_service->sendCommand("^OS1;");  // Set operate mode
 */
class AmplifierService : public QObject {
    Q_OBJECT

public:
    /**
     * Construct an AmplifierService
     * @param controller Amplifier controller instance (ownership not transferred)
     * @param parent Parent QObject
     */
    explicit AmplifierService(IAmplifierController* controller, QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~AmplifierService() override;

    /**
     * Get amplifier controller instance
     * @return Pointer to IAmplifierController (never null)
     */
    IAmplifierController* amplifierController() const { return m_amplifier; }

    /**
     * Get current amplifier connection status
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * Get current amplifier state
     * @return Current AmplifierState
     */
    AmplifierState currentState() const;

    /**
     * Connect to amplifier
     * @param config Amplifier configuration (IP, port, etc.)
     * @return true if connection successful, false otherwise
     */
    bool connectToAmplifier(const AmplifierConfig& config);

    /**
     * Disconnect from amplifier
     */
    void disconnectFromAmplifier();

    /**
     * Send raw command to amplifier (for button presses from UI)
     * @param command Raw amplifier command (e.g., "^OS1;" for operate mode)
     *
     * COMMAND REFERENCE (KPA1500 UDP Protocol):
     * ==========================================
     * All commands start with '^' and end with ';'
     *
     * OPERATION MODE:
     * ^OS1;  - Set operate mode (amplifier active)
     * ^OS0;  - Set standby mode (amplifier inactive)
     *
     * RESET:
     * ^RS;   - Reset fault condition
     *
     * BAND SELECTION (auto-selected by frequency, manual override):
     * ^BN00; - Select 160m band
     * ^BN01; - Select 80m band
     * ^BN02; - Select 60m band
     * ^BN03; - Select 40m band
     * ^BN04; - Select 30m band
     * ^BN05; - Select 20m band
     * ^BN06; - Select 17m band
     * ^BN07; - Select 15m band
     * ^BN08; - Select 12m band
     * ^BN09; - Select 10m band
     * ^BN10; - Select 6m band
     *
     * ANTENNA SELECTION:
     * ^AN1;  - Select antenna 1
     * ^AN2;  - Select antenna 2
     *
     * ATU MODE:
     * ^AMI;  - Set ATU inline mode
     * ^AMB;  - Set ATU bypassed mode
     *
     * ATU TUNE:
     * ^TU;   - Start ATU tuning
     *
     * STATUS QUERIES (read-only, for polling):
     * ^PWF;  - Query forward power
     * ^PWR;  - Query reflected power
     * ^SW;   - Query SWR
     * ^TM;   - Query temperature
     * ^VI;   - Query input voltage
     * ^FL;   - Query fault code
     * ^OS;   - Query operating status
     * ^BN;   - Query current band
     * ^AN;   - Query current antenna
     * ^AI;   - Query ATU inline status
     * ^AM;   - Query ATU mode
     *
     * Note: For KPA1500Direct, commands are sent via UDP datagram.
     *       For HamlibAmplifier, commands are translated to Hamlib API calls.
     */
    void sendCommand(const QString& command);

    /**
     * Query amplifier status (triggers immediate poll)
     */
    void queryStatus();

signals:
    /**
     * Emitted when connection status changes
     * @param connected true if connected, false otherwise
     */
    void connectionStatusChanged(bool connected);

    /**
     * Emitted when amplifier state updates
     * UI should update displays based on this
     * @param state Updated AmplifierState
     */
    void stateUpdated(const AmplifierState& state);

    /**
     * Emitted when forward power changes
     * @param watts Current forward power in watts
     */
    void forwardPowerChanged(int watts);

    /**
     * Emitted when SWR changes
     * @param swr Current SWR ratio
     */
    void swrChanged(float swr);

    /**
     * Emitted when fault is detected
     * @param faultCode Fault code string
     */
    void faultDetected(QString faultCode);

    /**
     * Emitted when operating status changes (operate/standby)
     * @param operateMode true if operate, false if standby
     */
    void operatingStatusChanged(bool operateMode);

    /**
     * Emitted when an error occurs
     * UI should display this in status bar
     * @param errorMessage Error message
     */
    void errorOccurred(QString errorMessage);

    /**
     * Emitted when status message should be displayed
     * @param message Status message for UI
     */
    void statusMessage(QString message);

private slots:
    /**
     * Handle amplifier connection status change
     * @param connected true if connected
     */
    void onAmplifierConnected(bool connected);

    /**
     * Handle amplifier error
     * @param error Error message
     */
    void onAmplifierError(const QString& error);

    /**
     * Handle amplifier state update
     * @param state Updated state
     */
    void onStateUpdated(const AmplifierState& state);

    /**
     * Handle forward power change
     * @param watts Forward power
     */
    void onForwardPowerChanged(int watts);

    /**
     * Handle SWR change
     * @param swr SWR value
     */
    void onSwrChanged(float swr);

    /**
     * Handle fault detection
     * @param faultCode Fault code
     */
    void onFaultDetected(const QString& faultCode);

    /**
     * Handle operating status change
     * @param operateMode Operating mode
     */
    void onOperatingStatusChanged(bool operateMode);

private:
    IAmplifierController* m_amplifier;  // Amplifier controller instance (not owned)
    AmplifierState m_currentState;      // Current amplifier state
};

} // namespace TR4QT

#endif // AMPLIFIERSERVICE_H
