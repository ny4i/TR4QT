#ifndef ROTATORSERVICE_H
#define ROTATORSERVICE_H

#include <QObject>
#include <QString>
#include "../rotator/IRotatorController.h"
#include "../controllers/RotatorController.h"

namespace TR4QT {

/**
 * RotatorService
 *
 * Business logic layer for rotator control
 * Handles rotator connection management and command execution
 * Separates rotator control logic from MainWindow UI updates
 *
 * Responsibilities:
 * - Handle rotator connect/disconnect
 * - Validate azimuth commands before passing to controller
 * - Forward rotator errors to UI (status bar)
 * - Track connection status
 * - Provide clean interface for MainWindow
 *
 * UI updates are NOT handled here - RotatorService emits signals
 * that UI components can connect to for updates.
 *
 * Example usage in MainWindow:
 *   m_rotatorService = new RotatorService(rotatorController, this);
 *   connect(m_rotatorService, &RotatorService::errorOccurred,
 *           this, [this](QString msg) {
 *       statusBar()->showMessage(msg, 5000);
 *   });
 */
class RotatorService : public QObject {
    Q_OBJECT

public:
    /**
     * Construct a RotatorService
     * @param controller Rotator controller instance (ownership not transferred)
     *        RotatorController runs the actual device in a worker thread to prevent UI freezing
     * @param parent Parent QObject
     */
    explicit RotatorService(RotatorController* controller, QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~RotatorService() override;

    /**
     * Get rotator controller instance
     * @return Pointer to RotatorController (never null)
     */
    RotatorController* rotatorController() const { return m_rotator; }

    /**
     * Get current rotator connection status
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * Get current rotator state
     * @return Current RotatorState
     */
    RotatorState currentState() const;

    /**
     * Connect to rotator (async - connection result via connectionStatusChanged signal)
     * @param rotatorType Type of rotator (RotatorFactory::RotatorType)
     * @param config Rotator configuration (IP, port, etc.)
     *
     * Note: Connection happens asynchronously in worker thread.
     * Listen for connectionStatusChanged(bool) signal to know when connected.
     */
    void connectToRotator(int rotatorType, const RotatorConfig& config);

    /**
     * Disconnect from rotator
     */
    void disconnectFromRotator();

    /**
     * Set rotator azimuth (fire-and-forget, executes async)
     * @param degrees Azimuth in degrees (0-360)
     * @return true if command queued, false if validation failed or not connected
     */
    bool setAzimuth(int degrees);

    /**
     * Stop rotator rotation (fire-and-forget, executes async)
     */
    void stop();

    /**
     * Query current azimuth (blocks briefly with timeout)
     * @param timeoutMs Timeout in milliseconds (default 1000ms)
     * @return Current azimuth in degrees, or std::nullopt if query failed
     */
    std::optional<int> getCurrentAzimuth(int timeoutMs = 1000) const;

signals:
    /**
     * Emitted when connection status changes
     * @param connected true if connected, false otherwise
     */
    void connectionStatusChanged(bool connected);

    /**
     * Emitted when azimuth changes (future use for continuous tracking)
     * @param degrees Current azimuth in degrees
     */
    void azimuthChanged(int degrees);

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

    /**
     * Emitted when rotator state updates (future use for polling)
     * @param state Updated RotatorState
     */
    void stateUpdated(const RotatorState& state);

private slots:
    /**
     * Handle rotator connection status change
     * @param connected true if connected
     */
    void onRotatorConnected(bool connected);

    /**
     * Handle rotator error
     * @param error Error message
     */
    void onRotatorError(const QString& error);

    /**
     * Handle azimuth change
     * @param degrees New azimuth
     */
    void onAzimuthChanged(int degrees);

    /**
     * Handle rotator state update
     * @param state Updated state
     */
    void onStateUpdated(const RotatorState& state);

private:
    RotatorController* m_rotator;  // Rotator controller instance (not owned, runs device in worker thread)
    RotatorState m_currentState;   // Current rotator state
};

} // namespace TR4QT

#endif // ROTATORSERVICE_H
