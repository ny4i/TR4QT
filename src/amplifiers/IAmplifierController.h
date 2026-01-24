#ifndef IAMPLIFIERCONTROLLER_H
#define IAMPLIFIERCONTROLLER_H

#include <QObject>
#include <QString>
#include <QMetaType>
#include <hamlib/rig.h>  // For freq_t type

namespace TR4QT {

// Forward declare AmplifierFactory for AmplifierType enum
class AmplifierFactory;

// Amplifier configuration
struct AmplifierConfig {
    int hamlibModelId{0};          // Hamlib AMP_MODEL_* constant (e.g., AMP_MODEL_ELECRAFT_KPA1500)
    QString connectionType;         // "direct" or "hamlib"
    QString port;                   // Serial port (e.g., "/dev/ttyUSB0", "COM3") or IP:port (e.g., "192.168.1.100:1500")
    int baudRate{38400};           // Serial baud rate (default: 38400)
    int dataBits{8};               // Serial data bits
    int stopBits{1};               // Serial stop bits
    QString parity;                // Serial parity ("None", "Even", "Odd", "Space", "Mark")
    QString flowControl;           // Flow control ("None", "Hardware", "Software")
    int responseTimeoutMs{1000};   // Timeout waiting for amplifier response
    int amplifierType{0};          // AmplifierFactory::AmplifierType (0=Hamlib, 1=KPA1500_DIRECT, etc.)
};

// Amplifier state (from queries)
struct AmplifierState {
    bool connected{false};          // Connection status
    bool isValid{false};            // Whether state data is valid
    int forwardPowerWatts{0};      // Forward power output (Watts)
    int reflectedPowerWatts{0};    // Reflected power (Watts)
    float swr{1.0};                // Standing Wave Ratio
    bool faultDetected{false};     // Fault/error detected
    QString faultCode;             // Fault code description
    freq_t frequency{0};           // Current frequency (Hz) for LPF tracking
    int temperature{0};            // PA temperature (Celsius)
    double inputVoltage{0.0};      // Input DC voltage
    bool operateMode{false};       // Operating mode (true=operate, false=standby)

    // LCD display content (from ^DS; command - KPA1500 specific but useful for UI)
    QString lcdLine1;              // First line of LCD display (up to 16 chars)
    QString lcdLine2;              // Second line of LCD display (up to 16 chars)
};

// Abstract amplifier interface
class IAmplifierController : public QObject {
    Q_OBJECT

public:
    explicit IAmplifierController(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAmplifierController() = default;

public slots:
    // Connection management (slots for thread-safe invocation)
    virtual bool connect(const AmplifierConfig& config) = 0;
    virtual void disconnect() = 0;

    // Frequency control (for LPF tracking)
    // Sets the amplifier's frequency for automatic low-pass filter selection
    virtual void setFrequency(freq_t freq) = 0;

    // Status query (fire-and-forget, executes async in background)
    // Triggers polling of amplifier state (power, SWR, temperature, etc.)
    virtual void queryStatus() = 0;

    // Send raw command to amplifier
    // For KPA1500: Send ASCII command (e.g., "^BN01;" for 80m band)
    // For Hamlib: Command is translated to Hamlib API calls
    virtual void sendRawCommand(const QString& command) = 0;

public:
    // Connection status (const, thread-safe)
    virtual bool isConnected() const = 0;

    // Get current state (const, thread-safe)
    // Returns cached state from last successful query
    virtual AmplifierState getState() const = 0;

signals:
    // Emitted when connection status changes
    void connectionStatusChanged(bool connected);

    // Emitted when amplifier state updates
    void stateUpdated(const AmplifierState& state);

    // Emitted when forward power changes
    void forwardPowerChanged(int watts);

    // Emitted when reflected power changes
    void reflectedPowerChanged(int watts);

    // Emitted when SWR changes
    void swrChanged(float swr);

    // Emitted when fault detected
    void faultDetected(const QString& faultCode);

    // Emitted when operating status changes (operate/standby)
    void operatingStatusChanged(bool operateMode);

    // Emitted when temperature changes
    void temperatureChanged(int celsius);

    // Emitted when error occurs (e.g., no response, timeout, invalid response)
    // UI should display this in status bar
    void errorOccurred(QString errorMessage);
};

} // namespace TR4QT

// Register types as Qt metatypes for use with signals/slots and QMetaObject::invokeMethod
Q_DECLARE_METATYPE(TR4QT::AmplifierConfig)
Q_DECLARE_METATYPE(TR4QT::AmplifierState)

#endif // IAMPLIFIERCONTROLLER_H
