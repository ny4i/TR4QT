/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
    int maxPowerWatts() const;  // Maximum TX power rating
    QList<ModeType> getSupportedModes() const;
    bool supportsCWSending() const;  // Check if radio supports CW via Hamlib

public slots:
    // Commands (executed in worker thread)
    void connectToRadio(const RadioConfig& config);
    void disconnectFromRadio();
    void setFrequency(freq_t freq, VFO vfo = VFO::VFO_A);
    void setBand(BandType band, VFO vfo = VFO::VFO_A);
    void setMode(ModeType mode, VFO vfo = VFO::VFO_A);
    void setPTT(bool transmit);
    void sendCW(const QString& text);
    void setCWSpeed(int wpm);
    int getCWSpeed() const;
    void getCWSpeedRange(int& minWpm, int& maxWpm) const;
    void stopCW();
    bool waitForMorseComplete();
    void enableRIT(bool enable, VFO vfo = VFO::VFO_A);
    void enableXIT(bool enable, VFO vfo = VFO::VFO_A);
    void setSplit(bool enable, VFO txVfo = VFO::VFO_B);

    // CW keying via PTT (for software iambic keyer paddle input)
    void sendKeyDown();
    void sendKeyUp();

signals:
    // Status signals (emitted from worker thread, safe to connect to UI)
    void connectionStatusChanged(bool connected);
    void stateUpdated(const RadioState& state);
    void frequencyChanged(freq_t freq, VFO vfo);
    void modeChanged(ModeType mode, VFO vfo);
    void pttChanged(bool transmitting);
    void errorOccurred(const QString& error);
    void radioModelChanged(const QString& model);
    void statusMessageReceived(int code, const QString& message);  // K4 ER command status

    // DEBUG: Test signal to verify signal/slot mechanism
    void debugTestSignal(int testValue);

    // Internal signals to trigger worker thread operations (Qt signal/slot pattern)
    void requestSetFrequency(freq_t freq, VFO vfo);
    void requestSetBand(BandType band, VFO vfo);
    void requestSetMode(ModeType mode, VFO vfo);
    void requestSetPTT(bool transmit);
    void requestSendCW(QString text);
    void requestSetCWSpeed(int wpm);
    void requestStopCW();
    void requestEnableRIT(bool enable, VFO vfo);
    void requestEnableXIT(bool enable, VFO vfo);
    void requestSetSplit(bool enable, VFO txVfo);

private:
    void recreateRadio(int radioType, const RadioConfig& config);
    void connectRadioSignals();
    void connectCommandSignals();  // Connect internal command signals to m_radio slots

    QThread m_workerThread;
    RadioInterface* m_radio;  // Lives in worker thread (created by RadioFactory)
    mutable QMutex m_stateMutex;
    RadioState m_lastState;
    bool m_connected;
    QString m_radioModel;
    std::atomic<bool> m_shutdownRequested{false};  // Signal to worker thread during shutdown
};

} // namespace TR4QT

#endif // RADIOCONTROLLER_H
