#ifndef RADIOINTERFACE_H
#define RADIOINTERFACE_H

#include <QObject>
#include <QMetaType>
#include "../core/Types.h"
#include <hamlib/rig.h>  // For freq_t and other hamlib types

namespace TR4QT {

// Forward declare RadioFactory for RadioType enum
class RadioFactory;

// Radio configuration
struct RadioConfig {
    int hamlibModelId{0};      // Hamlib RIG_MODEL_* constant
    QString port;               // Serial port or network address
    int baudRate{38400};        // Serial baud rate
    int dataBits{8};            // Serial data bits (5, 6, 7, 8) - default 8
    int stopBits{1};            // Serial stop bits (1, 2) - default 1
    int parity{0};              // Serial parity (0=None, 1=Odd, 2=Even) - default None
    int civAddress{0};          // CI-V address (for Icom radios)
    int pollInterval{100};      // Polling interval in ms
    int radioType{0};           // RadioFactory::RadioType (0=Hamlib, 1=K4_DIRECT, -1=Auto)
};

// Radio state (from polling)
struct RadioState {
    QString radioModel;         // Radio model name (e.g., "K4", "IC-7610")
    freq_t frequencyA{0};
    freq_t frequencyB{0};
    ModeType modeA{ModeType::None};
    ModeType modeB{ModeType::None};
    BandType bandA{BandType::None};
    BandType bandB{BandType::None};
    bool isTransmitting{false};
    int ritOffsetA{0};          // Hz
    int ritOffsetB{0};          // Hz
    int xitOffsetA{0};          // Hz
    int xitOffsetB{0};          // Hz
    bool isRitEnabled{false};   // RIT on/off (independent of offset value)
    bool isXitEnabled{false};   // XIT on/off (independent of offset value)
    bool isSplitEnabled{false};
    int cwSpeed{30};            // WPM
    int filterWidth{0};         // Hz
    int signalStrength{-54};    // Signal strength in dBm (S9 = -73 dBm)

    bool isValid{false};
};

// Abstract radio interface
class RadioInterface : public QObject {
    Q_OBJECT

public:
    explicit RadioInterface(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~RadioInterface() = default;

public slots:
    // DEBUG: Test slot to verify signal/slot mechanism (must be implemented by concrete classes)
    virtual void debugTestSlot(int testValue) = 0;

    // Connection management (slots for thread-safe invocation)
    virtual bool connect(const RadioConfig& config) = 0;
    virtual void disconnect() = 0;

    // Frequency control (slots for thread-safe cross-thread invocation)
    virtual bool setFrequency(freq_t freq, VFO vfo = VFO::VFO_A) = 0;

    // Band control - use when user intent is "change band" (not specific frequency)
    // K4: Uses BN command (radio remembers last frequency for band)
    // Hamlib: Calls setFrequency with band edge
    virtual bool setBand(BandType band, VFO vfo = VFO::VFO_A) = 0;

    // Mode control
    virtual bool setMode(ModeType mode, VFO vfo = VFO::VFO_A) = 0;

    // PTT control
    virtual bool setPTT(bool transmit) = 0;

    // CW functions
    virtual bool sendCW(const QString& text) = 0;
    virtual bool setCWSpeed(int wpm) = 0;
    virtual bool stopCW() = 0;
    virtual bool waitForMorseComplete() = 0;  // Blocks until CW transmission finishes

    // RIT/XIT control
    virtual bool setRIT(int offset_hz, VFO vfo = VFO::VFO_A) = 0;
    virtual bool setXIT(int offset_hz, VFO vfo = VFO::VFO_A) = 0;
    virtual bool clearRIT(VFO vfo = VFO::VFO_A) = 0;
    virtual bool clearXIT(VFO vfo = VFO::VFO_A) = 0;
    virtual bool enableRIT(bool enable, VFO vfo = VFO::VFO_A) = 0;
    virtual bool enableXIT(bool enable, VFO vfo = VFO::VFO_A) = 0;

    // Split operation
    virtual bool setSplit(bool enable, VFO txVfo = VFO::VFO_B) = 0;

    // VFO tuning
    virtual bool vfoBumpUp(VFO vfo = VFO::VFO_A) = 0;
    virtual bool vfoBumpDown(VFO vfo = VFO::VFO_A) = 0;

    // Filter control
    virtual bool setFilterWidth(int width_hz) = 0;

public:
    virtual bool isConnected() const = 0;

    // Frequency query (const, called synchronously)
    virtual freq_t getFrequency(VFO vfo = VFO::VFO_A) const = 0;

    // Mode query (const, called synchronously)
    virtual ModeType getMode(VFO vfo = VFO::VFO_A) const = 0;

    // PTT query (const, called synchronously)
    virtual bool getPTT() const = 0;

    // CW speed query (const, called synchronously)
    virtual int getCWSpeed() const = 0;

    // RIT/XIT query (const, called synchronously)
    virtual int getRIT(VFO vfo = VFO::VFO_A) const = 0;
    virtual int getXIT(VFO vfo = VFO::VFO_A) const = 0;

    // Split query (const, called synchronously)
    virtual bool getSplit() const = 0;

    // Filter query (const, called synchronously)
    virtual int getFilterWidth() const = 0;

    // Get current state
    virtual RadioState getCurrentState() const = 0;

signals:
    void frequencyChanged(freq_t freq, VFO vfo);
    void modeChanged(ModeType mode, VFO vfo);
    void pttChanged(bool transmitting);
    void ritChanged(int offset, VFO vfo);
    void xitChanged(int offset, VFO vfo);
    void splitChanged(bool enabled);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);
    void stateUpdated(const RadioState& state);
};

} // namespace TR4QT

// Register RadioConfig as a Qt metatype for use with signals/slots and QMetaObject::invokeMethod
Q_DECLARE_METATYPE(TR4QT::RadioConfig)
Q_DECLARE_METATYPE(TR4QT::RadioState)

#endif // RADIOINTERFACE_H
