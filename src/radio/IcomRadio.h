#ifndef ICOMRADIO_H
#define ICOMRADIO_H

#include "RadioInterface.h"
#include "icomnetwork.h"
#include <QTimer>
#include <QMutex>

namespace TR4QT {

/**
 * @brief Direct Icom radio control via network (bypasses Hamlib)
 *
 * Implements Icom network protocol for improved performance and reliability.
 * Supports all Icom radios with network capability via CI-V over UDP.
 *
 * Supported radios:
 * - IC-905, IC-9700, IC-7850, IC-7851, IC-7610, IC-7600, IC-7300MK2, IC-705, IC-R8600
 *
 * Protocol: Proprietary UDP with authentication + CI-V commands
 * Uses automatic push updates from radio for low-latency state tracking.
 *
 * Benefits over Hamlib:
 * - 3-5x faster frequency/mode changes
 * - More reliable network connection (native protocol vs Hamlib's implementation)
 * - Access to all Icom CI-V commands
 * - Lower latency (direct UDP vs Hamlib abstraction layer)
 *
 * RadioConfig requirements:
 * - port: "IP:PORT" (e.g., "192.168.1.100:50001")
 * - civAddress: CI-V address from radio settings (e.g., 0x98 for IC-9700)
 * - Custom fields needed (add to RadioConfig):
 *   - icomUsername: Network username
 *   - icomPassword: Network password
 */
class IcomRadio : public RadioInterface {
    Q_OBJECT

public:
    explicit IcomRadio(QObject* parent = nullptr);
    ~IcomRadio() override;

public slots:
    // DEBUG: Test slot to verify signal/slot mechanism
    void debugTestSlot(int testValue) override;

    // RadioInterface slot overrides
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

private slots:
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onCivDataReceived(const QByteArray& data);
    void onNetworkError(const QString& error);
    void onNetworkAuthFailed(const QString& reason);
    void pollRadio();

private:
    // CI-V command builders
    QByteArray buildCivCommand(quint8 command, const QByteArray& data = QByteArray());
    QByteArray frequencyToBcd(freq_t freq);
    freq_t bcdToFrequency(const QByteArray& bcd);
    quint8 modeToIcom(ModeType mode);
    ModeType icomToMode(quint8 icomMode);

    // CI-V response parsers
    void parseCivResponse(const QByteArray& data);
    void parseFrequencyResponse(const QByteArray& data, VFO vfo);
    void parseModeResponse(const QByteArray& data, VFO vfo);
    void parsePTTResponse(const QByteArray& data);

    // Send CI-V command and optionally wait for response
    bool sendCommand(quint8 command, const QByteArray& data = QByteArray());

    IcomNetwork* m_network{nullptr};
    RadioState m_state;
    mutable QMutex m_stateMutex;
    QTimer* m_pollTimer{nullptr};
    quint8 m_civAddress{0};

    // Pending operations (for synchronous methods that need async network)
    bool m_waitingForResponse{false};
    QByteArray m_lastResponse;
};

} // namespace TR4QT

#endif // ICOMRADIO_H
