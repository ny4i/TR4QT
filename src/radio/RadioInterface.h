#ifndef RADIOINTERFACE_H
#define RADIOINTERFACE_H

#include <QObject>
#include <QMetaType>
#include <QMap>
#include <QDateTime>
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
    int pollInterval{5000};     // Polling interval in ms (5s slow fallback; transceive provides instant updates)
    int radioType{0};           // RadioFactory::RadioType (0=Hamlib, 1=K4_DIRECT, 2=ICOM_DIRECT, -1=Auto)
    int connectionMethod{0};    // Connection method (0=Auto, 1=Serial, 2=Network) - Auto detects from port format

    // Icom Network Support
    QString icomUsername;       // Icom network username (can be blank)
    QString icomPassword;       // Icom network password (can be blank)
    QString icomClientName{"TR4QT"};  // Client identifier
};

// Radio profile (named configuration set)
struct RadioProfile {
    QString name;                   // Profile display name (e.g., "K4 #1", "IC-7610 Contest")
    RadioConfig config;             // Radio configuration (composition)
    QDateTime lastUsed;             // Track last use for potential future sorting
    QString notes;                  // Optional user notes

    // Display string for combo boxes and lists
    QString displayString() const {
        if (config.hamlibModelId > 0) {
            return QString("%1 (%2)").arg(name).arg(config.port);
        }
        return name;
    }

    // Validate profile
    bool isValid() const {
        return !name.isEmpty() && config.hamlibModelId > 0;
    }
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
    int cwPitch{600};           // Hz (CW sidetone frequency)
    int filterWidth{0};         // Hz
    int signalStrength{0};      // Signal strength (0-255 Icom raw, 0-30 K4 segments, or dBm if negative)

    // Audio & Gain Controls
    int afGainA{50};            // AF gain VFO A (0-255)
    int afGainB{50};            // AF gain VFO B (0-255)
    int rfGainA{250};           // RF gain VFO A (0-255)
    int rfGainB{250};           // RF gain VFO B (0-255)
    int micGain{50};            // Microphone gain (0-80)
    int speechCompression{0};   // Speech compression (0-20)
    int squelchA{0};            // Squelch VFO A (0-255, FM mode)
    int squelchB{0};            // Squelch VFO B (0-255, FM mode)

    // Signal Processing
    int agcModeA{1};            // AGC mode VFO A (0=off, 1=slow, 2=fast)
    int agcModeB{1};            // AGC mode VFO B
    int preampA{0};             // Preamp VFO A (0=off, 1=10dB, 2=18/20dB, 3=dual)
    int preampB{0};             // Preamp VFO B
    int attenuatorA{0};         // Attenuator VFO A (0-30 dB in 3dB steps)
    int attenuatorB{0};         // Attenuator VFO B
    int noiseBlankerA{0};       // Noise blanker VFO A (0=off, 1=on, 2=auto)
    int noiseBlankerB{0};       // Noise blanker VFO B

    // Antenna & Hardware
    int txAntenna{1};           // TX antenna (1-6)
    int rxAntennaA{1};          // RX antenna VFO A (1-6)
    int rxAntennaB{1};          // RX antenna VFO B
    bool atuEnabled{false};     // ATU mode (auto tune enabled)

    // Power & Monitoring
    int powerOutput{0};         // Power output in tenths of watts (50 = 5.0W)

    // Sub Receiver (K4D/K4HD)
    bool subRxEnabled{false};   // Sub receiver enabled

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

    // CW speed range (radio-specific limits)
    virtual void getCWSpeedRange(int& minWpm, int& maxWpm) const = 0;

    // Band command capability
    virtual bool supportsDiscreteBandCommand() const = 0;

    // RIT/XIT query (const, called synchronously)
    virtual int getRIT(VFO vfo = VFO::VFO_A) const = 0;
    virtual int getXIT(VFO vfo = VFO::VFO_A) const = 0;

    // Split query (const, called synchronously)
    virtual bool getSplit() const = 0;

    // Filter query (const, called synchronously)
    virtual int getFilterWidth() const = 0;

    // Get current state
    virtual RadioState getCurrentState() const = 0;

protected:
    // Band memory helpers for radios without discrete band commands
    freq_t getLastFrequencyForBand(BandType band, freq_t fallback) const;
    void updateBandMemory(freq_t freq);

private:
    // Band memory storage (for pseudo-band button)
    mutable QMap<BandType, freq_t> m_bandMemory;

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
Q_DECLARE_METATYPE(TR4QT::RadioProfile)
Q_DECLARE_METATYPE(TR4QT::RadioState)

#endif // RADIOINTERFACE_H
