#ifndef RADIOCONTROLLER_H
#define RADIOCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <atomic>
#include "RadioInterface.h"

namespace TR4QT {

/**
 * Radio controller that manages RadioInterface in a separate thread
 * All radio operations run in the worker thread to keep UI responsive
 */
class RadioController : public QObject {
    Q_OBJECT

public:
    explicit RadioController(QObject* parent = nullptr);
    ~RadioController() override;

    // Thread-safe radio control methods
    bool isConnected() const;
    RadioState getCurrentState() const;
    QString getRadioModel() const;
    QList<ModeType> getSupportedModes() const;
    bool supportsCWSending() const;  // Check if radio supports CW via Hamlib

public slots:
    // Commands (executed in worker thread)
    void connectToRadio(const RadioConfig& config);
    void disconnectFromRadio();
    void setFrequency(freq_t freq, VFO vfo = VFO::VFO_A);
    void setMode(ModeType mode, VFO vfo = VFO::VFO_A);
    void setPTT(bool transmit);
    void sendCW(const QString& text);
    void setCWSpeed(int wpm);
    int getCWSpeed() const;
    void stopCW();
    bool waitForMorseComplete();
    void enableRIT(bool enable, VFO vfo = VFO::VFO_A);
    void enableXIT(bool enable, VFO vfo = VFO::VFO_A);
    void setSplit(bool enable, VFO txVfo = VFO::VFO_B);

signals:
    // Status signals (emitted from worker thread, safe to connect to UI)
    void connectionStatusChanged(bool connected);
    void stateUpdated(const RadioState& state);
    void frequencyChanged(freq_t freq, VFO vfo);
    void modeChanged(ModeType mode, VFO vfo);
    void pttChanged(bool transmitting);
    void errorOccurred(const QString& error);
    void radioModelChanged(const QString& model);

private:
    void recreateRadio(int radioType, const RadioConfig& config);
    void connectRadioSignals();

    QThread m_workerThread;
    RadioInterface* m_radio;  // Lives in worker thread (created by RadioFactory)
    int m_currentRadioType;   // Current radio type (-1=Auto, 0=Hamlib, 1=K4_DIRECT)
    mutable QMutex m_stateMutex;
    RadioState m_lastState;
    bool m_connected;
    QString m_radioModel;
    std::atomic<bool> m_shutdownRequested{false};  // Signal to worker thread during shutdown
};

} // namespace TR4QT

#endif // RADIOCONTROLLER_H
