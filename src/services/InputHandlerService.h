#ifndef INPUTHANDLERSERVICE_H
#define INPUTHANDLERSERVICE_H

#include <QObject>
#include <QKeyEvent>
#include <QWidget>
#include "../radio/RadioController.h"
#include "../core/Types.h"

namespace TR4QT {

/**
 * InputHandlerService
 *
 * Handles keyboard input for CW operations, mode switching, and field navigation.
 * Extracted from MainWindow::eventFilter to separate input handling logic from UI.
 *
 * Responsibilities:
 * - CW speed control (PgUp/PgDn)
 * - Mode switching (Tab → S&P, ESC → CQ)
 * - Function key handling (F1-F12 for CW messages)
 * - Field clear/navigation (ESC in entry fields)
 * - CW repeat (= key)
 *
 * This service emits signals for UI updates - it does NOT directly manipulate
 * UI widgets. MainWindow connects these signals to appropriate slots.
 *
 * Example usage:
 *   // In MainWindow constructor:
 *   InputHandlerService::Config config;
 *   config.radio = m_radio;
 *   config.currentState = &m_currentState;
 *   m_inputHandler = new InputHandlerService(config, this);
 *
 *   connect(m_inputHandler, &InputHandlerService::cwSpeedChanged,
 *           this, &MainWindow::onCWSpeedChanged);
 *
 *   // In eventFilter:
 *   if (m_inputHandler->handleKeyPress(keyEvent, focusWidget)) {
 *       return true;
 *   }
 */
class InputHandlerService : public QObject {
    Q_OBJECT

public:
    /**
     * Configuration for InputHandlerService
     * Contains pointers to shared state - does not own these resources
     */
    struct Config {
        RadioController* radio = nullptr;       // Radio controller for CW operations
        const RadioState* currentState = nullptr;  // Current radio state (read-only)
        bool* radioConnected = nullptr;         // Pointer to connection status flag
    };

    /**
     * Context passed to key handlers to provide current UI state
     */
    struct KeyContext {
        QWidget* focusWidget = nullptr;         // Currently focused widget
        QWidget* callsignEntry = nullptr;       // Callsign entry widget
        QWidget* exchangeEntry = nullptr;       // Exchange entry widget
        bool callsignEmpty = true;              // Is callsign field empty?
        bool inSPMode = false;                  // Is in Search & Pounce mode?
        QString lastCWMessage;                  // Last CW message sent (for repeat)
    };

    explicit InputHandlerService(const Config& config, QObject* parent = nullptr);
    ~InputHandlerService() override;

    /**
     * Handle a key press event
     * @param event The key event
     * @param context Current UI context (focus, field state, etc.)
     * @return true if the event was handled, false to propagate
     */
    bool handleKeyPress(QKeyEvent* event, const KeyContext& context);

signals:
    // Mode switching
    void switchToSPMode();
    void switchToCQMode();

    // CW operations
    void cwSpeedChanged(int newWpm);
    void stopCW();
    void sendFunctionKey(int fKey, bool ctrlPressed, bool altPressed);
    void repeatLastCW(const QString& message);

    // Field operations
    void clearCallsign();
    void clearExchange();
    void focusCallsign();

    // Status messages (for status bar)
    void statusMessage(const QString& message);

private:
    /**
     * Handle CW speed change (PgUp/PgDn)
     * @param increase true for speed up, false for speed down
     * @return true if handled
     */
    bool handleCWSpeedKey(bool increase);

    /**
     * Handle ESC key
     * @param context Current UI context
     * @return true if handled
     */
    bool handleEscapeKey(const KeyContext& context);

    /**
     * Handle function keys F1-F12
     * @param event The key event
     * @return true if handled
     */
    bool handleFunctionKey(QKeyEvent* event);

    /**
     * Handle = key (repeat last CW)
     * @param lastCWMessage The last CW message to repeat
     * @return true if handled
     */
    bool handleRepeatKey(const QString& lastCWMessage);

    /**
     * Handle Tab key (switch to S&P mode)
     * @param context Current UI context
     * @return true if handled
     */
    bool handleTabKey(const KeyContext& context);

    /**
     * Check if radio is in CW mode and connected
     * @return true if CW operations are allowed
     */
    bool canDoCWOperations() const;

    /**
     * Check if radio is connected
     * @return true if connected
     */
    bool isRadioConnected() const;

    Config m_config;
};

} // namespace TR4QT

#endif // INPUTHANDLERSERVICE_H
