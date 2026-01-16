#ifndef IROTATORCONTROLLER_H
#define IROTATORCONTROLLER_H

#include <QObject>
#include <QMetaType>
#include <optional>

namespace TR4QT {

// Forward declare RotatorFactory for RotatorType enum
class RotatorFactory;

// Rotator configuration
struct RotatorConfig {
    QString ipAddress;           // IP address for network rotators
    int port{12000};             // UDP port (PSTRotator default: 12000)
    int responseTimeoutMs{1000}; // Timeout waiting for rotator response
    QString serialPort;          // Serial port (future use)
    int baudRate{9600};          // Serial baud rate (future use)
    int rotatorType{0};          // RotatorFactory::RotatorType (0=PSTRotator, future: 1=GS232, etc.)
};

// Rotator state (from queries)
struct RotatorState {
    int azimuth{0};              // Current azimuth (0-360 degrees)
    int elevation{0};            // Current elevation (-90 to +90 degrees, future use)
    bool isMoving{false};        // Whether rotator is currently rotating (future use)
    bool isConnected{false};     // Whether we have valid communication
    bool isValid{false};         // Whether state data is valid
};

// Abstract rotator interface
class IRotatorController : public QObject {
    Q_OBJECT

public:
    explicit IRotatorController(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IRotatorController() = default;

public slots:
    // Connection management (slots for thread-safe invocation)
    virtual bool connect(const RotatorConfig& config) = 0;
    virtual void disconnect() = 0;

    // Azimuth control (fire-and-forget, executes async in background thread)
    // Valid range: 0-360 degrees
    // Returns false if validation fails (invalid azimuth), true if command queued
    virtual bool setAzimuth(int degrees) = 0;

    // Stop rotator rotation (fire-and-forget, executes async in background thread)
    virtual void stop() = 0;

    // Elevation control (future use for az/el rotators)
    virtual bool setElevation(int degrees) = 0;

public:
    // Connection status (const, thread-safe)
    virtual bool isConnected() const = 0;

    // Query current azimuth (blocks briefly with timeout for response)
    // Returns std::nullopt if:
    // - Not connected
    // - Timeout waiting for response
    // - Malformed response
    virtual std::optional<int> getAzimuth(int timeoutMs = 1000) const = 0;

    // Get current state (const, thread-safe)
    virtual RotatorState getCurrentState() const = 0;

signals:
    // Emitted when azimuth changes (future use for continuous tracking)
    void azimuthChanged(int degrees);

    // Emitted when elevation changes (future use)
    void elevationChanged(int degrees);

    // Emitted when connection status changes
    void connectionStatusChanged(bool connected);

    // Emitted when error occurs (e.g., no UDP response, timeout, invalid response)
    // UI should display this in status bar
    void errorOccurred(QString errorMessage);

    // Emitted when rotator state updates (future use for polling)
    void stateUpdated(const RotatorState& state);
};

} // namespace TR4QT

// Register types as Qt metatypes for use with signals/slots and QMetaObject::invokeMethod
Q_DECLARE_METATYPE(TR4QT::RotatorConfig)
Q_DECLARE_METATYPE(TR4QT::RotatorState)

#endif // IROTATORCONTROLLER_H
