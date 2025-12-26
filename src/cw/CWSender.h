#ifndef CWSENDER_H
#define CWSENDER_H

#include <QObject>
#include <QString>

namespace TR4QT {

/**
 * Abstract interface for CW (Morse code) sending
 *
 * Implementations can use different backends:
 * - HamlibCWSender: Uses Hamlib rig_send_morse/rig_wait_morse
 * - WinkeyerCWSender: Uses Winkeyer hardware (future)
 * - SimulatedCWSender: For testing without hardware
 *
 * All implementations run asynchronously and emit signals for status updates.
 */
class CWSender : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,           // Ready to send
        Sending,        // Currently transmitting
        Stopping,       // Abort requested, waiting for stop
        Error           // Error state
    };
    Q_ENUM(State)

    explicit CWSender(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~CWSender() = default;

    // Get current state
    virtual State state() const = 0;
    virtual bool isAvailable() const = 0;
    virtual QString backendName() const = 0;

    // Get/set WPM
    virtual int wpm() const = 0;
    virtual void setWpm(int wpm) = 0;

public slots:
    // Send morse code text (async - emits signals for status)
    virtual void send(const QString& text) = 0;

    // Stop current transmission
    virtual void stop() = 0;

signals:
    // Emitted when transmission starts
    void transmissionStarted(const QString& text);

    // Emitted when transmission completes normally
    void transmissionComplete();

    // Emitted when transmission is stopped/aborted
    void transmissionStopped();

    // Emitted on error
    void error(const QString& message);

    // Emitted when state changes
    void stateChanged(CWSender::State state);

    // Emitted when WPM changes
    void wpmChanged(int wpm);
};

} // namespace TR4QT

#endif // CWSENDER_H
