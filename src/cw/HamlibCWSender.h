#ifndef HAMLIBCWSENDER_H
#define HAMLIBCWSENDER_H

#include "CWSender.h"
#include <QThread>
#include <QMutex>
#include <atomic>

// Forward declare hamlib types
typedef struct s_rig RIG;

namespace TR4QT {

class RadioController;

/**
 * CW sender implementation using Hamlib
 *
 * Uses rig_send_morse to start transmission and rig_wait_morse
 * to detect completion. Runs blocking operations in a worker thread
 * to keep the UI responsive.
 */
class HamlibCWSender : public CWSender {
    Q_OBJECT

public:
    explicit HamlibCWSender(RadioController* radio, QObject* parent = nullptr);
    ~HamlibCWSender() override;

    // CWSender interface
    State state() const override;
    bool isAvailable() const override;
    QString backendName() const override { return "Hamlib"; }

    int wpm() const override;
    void setWpm(int wpm) override;

public slots:
    void send(const QString& text) override;
    void stop() override;

signals:
    void startWorkerSend(const QString& text, int wpm);

private slots:
    void onWorkerFinished(bool success, const QString& errorMsg);

private:
    RadioController* m_radio;
    QThread m_workerThread;
    std::atomic<State> m_state{State::Idle};
    std::atomic<bool> m_stopRequested{false};
    int m_wpm;
    mutable QMutex m_mutex;
};

/**
 * Worker object that runs in a separate thread
 * Performs blocking rig_wait_morse call
 */
class HamlibCWWorker : public QObject {
    Q_OBJECT

public:
    explicit HamlibCWWorker(QObject* parent = nullptr);

    void setRig(RIG* rig) { m_rig = rig; }
    void setStopFlag(std::atomic<bool>* flag) { m_stopRequested = flag; }

public slots:
    void doSend(const QString& text, int wpm);

signals:
    void finished(bool success, const QString& errorMsg);
    void transmissionStarted(const QString& text);

private:
    RIG* m_rig = nullptr;
    std::atomic<bool>* m_stopRequested = nullptr;
};

} // namespace TR4QT

#endif // HAMLIBCWSENDER_H
