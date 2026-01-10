#ifndef K4RADIO_H
#define K4RADIO_H

#include "RadioInterface.h"
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>

namespace TR4QT {

/**
 * @brief Direct K4 radio control via TCP (bypasses Hamlib)
 *
 * Implements Elecraft K4 command protocol for improved performance and
 * access to K4-specific features not available in Hamlib.
 *
 * Protocol: ASCII commands terminated with ';' (e.g., "FA00014200000;")
 * Uses AI5 (Auto Information mode 5) for automatic push updates from radio.
 *
 * Benefits over Hamlib:
 * - 5-10x faster frequency/mode changes (10ms vs 50-100ms)
 * - No polling needed (AI mode pushes updates)
 * - Access to K4-specific features (option modules, filter presets, DVK)
 * - Lower CW latency (10-20ms vs 100-150ms)
 *
 * Based on TR4W's TK4Radio implementation (proven in contest use).
 */
class K4Radio : public RadioInterface {
    Q_OBJECT

public:
    // K4 CW speed limits (WPM - Words Per Minute)
    static constexpr int MIN_CW_SPEED_WPM = 8;
    static constexpr int MAX_CW_SPEED_WPM = 100;

    explicit K4Radio(QObject* parent = nullptr);
    ~K4Radio() override;

public slots:
    // DEBUG: Test slot to verify signal/slot mechanism works
    void debugTestSlot(int testValue);

    // RadioInterface slot overrides (must be in slots section for MOC)
    bool connect(const RadioConfig& config) override;
    void disconnect() override;

    bool setFrequency(freq_t freq, VFO vfo = VFO::VFO_A) override;
    bool setBand(BandType band, VFO vfo = VFO::VFO_A) override;
    bool setMode(ModeType mode, VFO vfo = VFO::VFO_A) override;
    bool setPTT(bool transmit) override;
    bool sendCW(const QString& text) override;
    bool setCWSpeed(int wpm) override;
    bool stopCW() override;
    bool waitForMorseComplete() override;
    bool setRIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    bool setXIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    bool clearRIT(VFO vfo = VFO::VFO_A) override;
    bool clearXIT(VFO vfo = VFO::VFO_A) override;
    bool enableRIT(bool enable, VFO vfo = VFO::VFO_A) override;
    bool enableXIT(bool enable, VFO vfo = VFO::VFO_A) override;
    bool setSplit(bool enable, VFO txVfo = VFO::VFO_B) override;
    bool vfoBumpUp(VFO vfo = VFO::VFO_A) override;
    bool vfoBumpDown(VFO vfo = VFO::VFO_A) override;
    bool setFilterWidth(int width_hz) override;

public:
    // Query methods (const, not slots)
    bool isConnected() const override;
    freq_t getFrequency(VFO vfo = VFO::VFO_A) const override;
    ModeType getMode(VFO vfo = VFO::VFO_A) const override;
    bool getPTT() const override;
    int getCWSpeed() const override;
    int getRIT(VFO vfo = VFO::VFO_A) const override;
    int getXIT(VFO vfo = VFO::VFO_A) const override;
    bool getSplit() const override;
    int getFilterWidth() const override;
    RadioState getCurrentState() const override;

    // Radio information
    Q_INVOKABLE QList<ModeType> getSupportedModes() const;
    Q_INVOKABLE bool supportsCWSending() const;

    // K4-specific features not in RadioInterface
    bool setFilterPreset(int preset, VFO vfo = VFO::VFO_A);
    bool queryOptionModules(QStringList& modules);
    bool playDVKMessage(int message);
    bool stopDVK();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onCWTimeout();

private:
    QTcpSocket* m_socket{nullptr};
    QString m_host;
    quint16 m_port{0};
    RadioState m_state;
    mutable QMutex m_stateMutex;  // Protect m_state access
    QString m_receiveBuffer;
    QString m_cwBuffer;
    QTimer* m_cwTimer{nullptr};
    bool m_cwInProgress{false};

    // K4 protocol helpers
    void sendCommand(const QString& cmd, VFO vfo = VFO::VFO_A);
    void processMessage(const QString& message);
    bool parseIFCommand(const QString& response, VFO vfo);

    // Mode conversion
    ModeType modeStringToMode(const QString& modeStr, const QString& dataModeStr);
    QString modeToModeString(ModeType mode, int& dataModeInt);

    // Band conversion
    BandType bandNumberToBand(int bandNum);
    int bandToBandNumber(BandType band);

    // Initialization
    void enableAIMode(int level = 5);
    void queryInitialState();

    // Helper to get VFO suffix for commands
    QString vfoSuffix(VFO vfo) const;
};

} // namespace TR4QT

#endif // K4RADIO_H
