#ifndef KPA1500DIRECT_H
#define KPA1500DIRECT_H

#include "IAmplifierController.h"
#include <QHostAddress>
#include <QHash>
#include <functional>

class QUdpSocket;
class QTimer;

namespace TR4QT {

/**
 * @brief KPA1500 Direct UDP Controller
 *
 * Direct implementation of IAmplifierController for Elecraft KPA1500 amplifier
 * using UDP protocol (bypasses Hamlib for performance).
 *
 * Periodically polls the amplifier over UDP using the ASCII command set
 * described in the KPA1500 Programming Reference.
 *
 * Protocol:
 * - Commands and responses start with '^' and end with ';'
 * - UDP server at port 1500 accepts commands
 * - Send only one command per UDP packet
 * - Expect at most one response per command
 *
 * Features:
 * - Configurable list of commands sent each poll cycle
 * - Dispatch table for parsing responses by command prefix
 * - Emits signals when values change for UI/state updates
 *
 * Typical usage:
 * - Create via AmplifierFactory
 * - Call connect() with AmplifierConfig (IP address and port)
 * - Connect to signals like forwardPowerChanged(), swrChanged(), etc.
 */
class KPA1500Direct : public IAmplifierController {
    Q_OBJECT

public:
    explicit KPA1500Direct(QObject* parent = nullptr);
    ~KPA1500Direct() override;

public slots:
    // IAmplifierController interface implementation
    bool connect(const AmplifierConfig& config) override;
    void disconnect() override;
    void setFrequency(freq_t freq) override;
    void queryStatus() override;
    void sendRawCommand(const QString& command) override;

public:
    // Query methods
    bool isConnected() const override;
    AmplifierState getState() const override;

    // Configuration methods (optional, for fine-tuning)
    void setPollIntervalMs(int intervalMs);
    void setPollCommands(const QStringList& commands);

signals:
    // Additional signals beyond IAmplifierController (for detailed monitoring)
    // These provide finer-grained updates than the base interface

    // Power and SWR monitoring (duplicates of base class signals for compatibility)
    // void forwardPowerChanged(int watts);     // Inherited from IAmplifierController
    // void reflectedPowerChanged(int watts);   // Inherited from IAmplifierController
    // void swrChanged(float swr);              // Inherited from IAmplifierController

    // Additional KPA1500-specific signals
    void inputPowerChanged(int watts);       // ^PWI - Input power from radio
    void bandNumberChanged(int band);        // ^BN - Band number (00-10 for 160-6m)
    void antennaChanged(int antenna);        // ^AN - Selected antenna number
    void atuInlineChanged(bool inlineMode);  // ^AI - ATU inline (true) or bypassed (false)
    void atuModeChanged(QString mode);       // ^AM - ATU mode ("Inline" or "Bypassed")
    void bannerTextChanged(QString text);    // ^BT - Front panel banner text
    void drivePowerChanged(int watts);       // ^PC - Drive power setting
    void serialNumberChanged(QString serial); // ^SN - Amplifier serial number
    void wsStatusChanged(int fwd, int ref, double swr); // ^WS - Combined status

    // New KPA1500-specific signals
    void swrBypassChanged(double swr);       // ^SB - SWR when ATU last bypassed (tenths)
    void tuneInProgressChanged(bool tuning); // ^TP - ATU tune in progress
    void displayContentChanged(QString line1, QString line2); // ^DS - LCD display content
    void txFrequencyChanged(int kHz);        // ^FQ - TX frequency counter (kHz, 8kHz increments)
    void ledStateChanged(quint32 powerBar, quint16 swrBar, quint8 statusLeds); // ^LQ - LED states
    void voltage50VChanged(double volts);    // ^VMH - 50V supply voltage
    void powerStateChanged(bool on);         // ^ON - Main power supplies on/off

    // Generic hook for inspecting raw responses
    void valueUpdated(const QString& commandKey, const QString& rawResponse);

private slots:
    void doPollCycle();
    void onReadyRead();

private:
    void initDispatchTable();
    void sendCommand(const QByteArray& cmd);
    void processResponse(const QByteArray& datagram);
    void updateStateFromPolling();  // Update cached AmplifierState

    // Command response handlers (same as KPA1500UdpPoller)
    void handlePWF(const QString& resp);  // Forward power
    void handlePWI(const QString& resp);  // Input power
    void handlePWR(const QString& resp);  // Reflected power
    void handleSW(const QString& resp);   // SWR
    void handleFR(const QString& resp);   // Frequency
    void handleBN(const QString& resp);   // Band number
    void handleOS(const QString& resp);   // Operating status
    void handleAI(const QString& resp);   // ATU inline
    void handleAM(const QString& resp);   // ATU mode
    void handleAN(const QString& resp);   // Antenna
    void handleFL(const QString& resp);   // Fault code
    void handleBT(const QString& resp);   // Banner text
    void handleLQ(const QString& resp);   // LED Query (LED states)
    void handlePC(const QString& resp);   // Drive power
    void handleSN(const QString& resp);   // Serial number
    void handleTM(const QString& resp);   // Temperature
    void handleVI(const QString& resp);   // Input voltage
    void handleWS(const QString& resp);   // Combined status
    void handleSB(const QString& resp);   // SWR Bypass
    void handleTP(const QString& resp);   // Tune Poll
    void handleDS(const QString& resp);   // Display LCD content
    void handleFQ(const QString& resp);   // TX Frequency counter
    void handleVMH(const QString& resp);  // 50V supply voltage
    void handleON(const QString& resp);   // Power state

    // Network configuration
    QHostAddress m_addr;
    quint16 m_port{1500};
    int m_intervalMs{250};  // Default poll interval (configurable via settings)

    // DS command has longer interval to reduce load (500ms minimum)
    static constexpr int DS_POLL_INTERVAL_MS = 500;
    qint64 m_lastDsSendTime{0};  // Timestamp of last ^DS; send

    // Network objects
    QUdpSocket* m_socket{nullptr};
    QTimer* m_timer{nullptr};

    // Poll configuration
    QStringList m_pollCommands;

    // Connection state
    bool m_connected{false};
    AmplifierConfig m_config;

    // Cached state (for IAmplifierController::getState())
    AmplifierState m_currentState;

    // Last-known values for change detection
    int m_lastForwardPower{-1};
    int m_lastInputPower{-1};
    int m_lastReflectedPower{-1};
    double m_lastSwr{-1.0};
    int m_lastBandNumber{-1};
    bool m_lastOperatingStatus{false};
    bool m_lastAtuInline{false};
    QString m_lastAtuMode;
    int m_lastAntenna{-1};
    int m_lastFaultCode{-1};
    QString m_lastBannerText;
    int m_lastDrivePower{-1};
    QString m_lastSerialNumber;
    double m_lastTemperature{-273.15};
    double m_lastInputVoltage{-1.0};
    int m_lastWsFwd{-1};
    int m_lastWsRef{-1};
    double m_lastWsSwr{-1.0};

    // New state tracking for additional commands
    double m_lastSwrBypass{-1.0};           // ^SB
    bool m_lastTuneInProgress{false};       // ^TP
    QString m_lastDisplayLine1;             // ^DS line 1
    QString m_lastDisplayLine2;             // ^DS line 2
    int m_lastTxFrequency{-1};              // ^FQ (kHz)
    quint32 m_lastLedPowerBar{0};           // ^LQ power bar bitmap
    quint16 m_lastLedSwrBar{0};             // ^LQ SWR bar bitmap
    quint8 m_lastLedStatus{0};              // ^LQ status LEDs
    double m_lastVoltage50V{-1.0};          // ^VMH (volts)
    bool m_lastPowerState{false};           // ^ON

    // Dispatch table: maps command prefix to handler function
    QHash<QString, std::function<void(const QString&)>> m_dispatch;
};

} // namespace TR4QT

#endif // KPA1500DIRECT_H
