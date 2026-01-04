#ifndef HAMLIBRADIO_H
#define HAMLIBRADIO_H

#include "RadioInterface.h"
#include <hamlib/rig.h>
#include <QMutex>
#include <QTimer>

namespace TR4QT {

/**
 * Concrete implementation of RadioInterface using hamlib C library
 *
 * Supports all radios through hamlib, with focus on:
 * - Elecraft K4 (RIG_MODEL_K4, model 2047)
 * - Icom IC-7610 (RIG_MODEL_IC7610, model 3078)
 * - Icom IC-7760 (RIG_MODEL_IC7760, model 3092)
 */
class HamlibRadio : public RadioInterface {
    Q_OBJECT

public:
    explicit HamlibRadio(QObject* parent = nullptr);
    ~HamlibRadio() override;

    // Connection management
    bool connect(const RadioConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    // Frequency control
    bool setFrequency(freq_t freq, VFO vfo = VFO::VFO_A) override;
    freq_t getFrequency(VFO vfo = VFO::VFO_A) const override;

    // Mode control
    bool setMode(ModeType mode, VFO vfo = VFO::VFO_A) override;
    ModeType getMode(VFO vfo = VFO::VFO_A) const override;

    // PTT control
    bool setPTT(bool transmit) override;
    bool getPTT() const override;

    // CW functions
    bool sendCW(const QString& text) override;
    bool setCWSpeed(int wpm) override;
    int getCWSpeed() const override;
    bool stopCW() override;
    bool waitForMorseComplete() override;

    // RIT/XIT control
    bool setRIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    bool setXIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    int getRIT(VFO vfo = VFO::VFO_A) const override;
    int getXIT(VFO vfo = VFO::VFO_A) const override;
    bool clearRIT(VFO vfo = VFO::VFO_A) override;
    bool clearXIT(VFO vfo = VFO::VFO_A) override;
    bool enableRIT(bool enable, VFO vfo = VFO::VFO_A) override;
    bool enableXIT(bool enable, VFO vfo = VFO::VFO_A) override;

    // Split operation
    bool setSplit(bool enable, VFO txVfo = VFO::VFO_B) override;
    bool getSplit() const override;

    // VFO tuning
    bool vfoBumpUp(VFO vfo = VFO::VFO_A) override;
    bool vfoBumpDown(VFO vfo = VFO::VFO_A) override;

    // Filter control
    bool setFilterWidth(int width_hz) override;
    int getFilterWidth() const override;

    // Get current state
    RadioState getCurrentState() const override;

    // Radio information
    QString getRadioModel() const;
    QString getRadioVersion() const;
    Q_INVOKABLE QList<ModeType> getSupportedModes() const;

    // Capability checking
    bool supportsCWSending() const;  // Check if radio supports rig_send_morse

private slots:
    void pollRadio();

private:
    // Hamlib conversion helpers
    vfo_t toHamlibVFO(VFO vfo) const;
    VFO fromHamlibVFO(vfo_t vfo) const;
    rmode_t toHamlibMode(ModeType mode) const;
    ModeType fromHamlibMode(rmode_t mode) const;
    BandType frequencyToBand(freq_t freq) const;

    // Error handling
    void logHamlibError(const QString& operation, int retcode) const;
    bool checkRigPointer(const QString& operation) const;

    // State management
    void updateState();
    RadioState pollCurrentState();

    // Member variables
    RIG* m_rig{nullptr};
    mutable QMutex m_rigMutex;
    RadioConfig m_config;
    RadioState m_currentState;
    QTimer* m_pollTimer{nullptr};
    bool m_connected{false};
    int m_consecutiveErrors{0};  // Track consecutive polling errors for disconnect detection
    static constexpr int MAX_CONSECUTIVE_ERRORS = 3;  // Disconnect after 3 failed polls
};

} // namespace TR4QT

#endif // HAMLIBRADIO_H
