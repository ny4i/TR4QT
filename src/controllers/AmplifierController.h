#ifndef AMPLIFIERCONTROLLER_H
#define AMPLIFIERCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <atomic>
#include "../amplifiers/IAmplifierController.h"

namespace TR4QT {

/**
 * AmplifierController - Thread-safe wrapper for IAmplifierController
 *
 * Manages amplifier devices in a dedicated worker thread to prevent
 * UI freezing from network I/O and polling timers.
 *
 * Pattern follows RadioController:
 * - Controller lives on main thread (safe to connect to UI)
 * - Actual amplifier device lives on worker thread (all I/O isolated)
 * - Signals cross thread boundaries automatically (Qt handles queued connections)
 *
 * Why this matters:
 * - KPA1500Direct sends 18+ UDP commands every 250ms
 * - On Windows, this blocks the main thread and causes "Not Responding"
 * - Moving to worker thread keeps UI responsive
 *
 * Issue #69: Windows UI Freezing
 */
class AmplifierController : public QObject {
    Q_OBJECT

public:
    explicit AmplifierController(QObject* parent = nullptr);
    ~AmplifierController() override;

    // Thread-safe accessors (called from main thread)
    bool isConnected() const;
    AmplifierState getState() const;

public slots:
    // Commands (executed in worker thread via signals)
    void connectToAmplifier(int amplifierType, const AmplifierConfig& config);
    void disconnectFromAmplifier();
    void setFrequency(freq_t freq);
    void sendRawCommand(const QString& command);
    void queryStatus();

signals:
    // Status signals (emitted from worker thread, safe to connect to UI)
    void connectionStatusChanged(bool connected);
    void stateUpdated(const AmplifierState& state);
    void forwardPowerChanged(int watts);
    void reflectedPowerChanged(int watts);
    void swrChanged(float swr);
    void faultDetected(const QString& faultCode);
    void operatingStatusChanged(bool operateMode);
    void temperatureChanged(int celsius);
    void errorOccurred(const QString& error);

private:
    void createAmplifier(int amplifierType, const AmplifierConfig& config);
    void connectAmplifierSignals();

    QThread m_workerThread;
    IAmplifierController* m_amplifier{nullptr};  // Lives in worker thread
    mutable QMutex m_stateMutex;
    AmplifierState m_lastState;
    bool m_connected{false};
    std::atomic<bool> m_shutdownRequested{false};
};

} // namespace TR4QT

#endif // AMPLIFIERCONTROLLER_H
