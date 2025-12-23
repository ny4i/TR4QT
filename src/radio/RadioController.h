#ifndef RADIOCONTROLLER_H
#define RADIOCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include "RadioInterface.h"
#include "HamlibRadio.h"

namespace TR4QT {

/**
 * Radio controller that manages HamlibRadio in a separate thread
 * All hamlib operations run in the worker thread to keep UI responsive
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

public slots:
    // Commands (executed in worker thread)
    void connectToRadio(const RadioConfig& config);
    void disconnectFromRadio();
    void setFrequency(freq_t freq, VFO vfo = VFO::VFO_A);
    void setMode(ModeType mode, VFO vfo = VFO::VFO_A);
    void setPTT(bool transmit);
    void sendCW(const QString& text);

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
    QThread m_workerThread;
    HamlibRadio* m_radio;  // Lives in worker thread
    mutable QMutex m_stateMutex;
    RadioState m_lastState;
    bool m_connected;
    QString m_radioModel;
};

} // namespace TR4QT

#endif // RADIOCONTROLLER_H
