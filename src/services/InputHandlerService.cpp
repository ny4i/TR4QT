#include "InputHandlerService.h"
#include "../core/Constants.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"
#include <QApplication>

namespace TR4QT {

InputHandlerService::InputHandlerService(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
}

InputHandlerService::~InputHandlerService() = default;

bool InputHandlerService::handleKeyPress(QKeyEvent* event, const KeyContext& context)
{
    const int key = event->key();

    // PgUp: Increase CW speed
    if (key == Qt::Key_PageUp) {
        return handleCWSpeedKey(true);
    }

    // PgDown: Decrease CW speed
    if (key == Qt::Key_PageDown) {
        return handleCWSpeedKey(false);
    }

    // Tab key in callsign field: Switch to S&P mode
    if (key == Qt::Key_Tab) {
        return handleTabKey(context);
    }

    // ESC key: Stop CW, clear fields, mode switch
    if (key == Qt::Key_Escape) {
        return handleEscapeKey(context);
    }

    // F1-F12: CW messages
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return handleFunctionKey(event);
    }

    // = key: Repeat last CW message
    if (key == Qt::Key_Equal) {
        return handleRepeatKey(context.lastCWMessage);
    }

    return false;  // Not handled
}

bool InputHandlerService::handleCWSpeedKey(bool increase)
{
    // Only allow CW speed change in CW mode with radio connected
    if (!canDoCWOperations()) {
        emit statusMessage("CW speed adjust requires CW mode and radio connection");
        return true;  // Event handled, don't propagate
    }

    const int increment = AppSettings::instance().getMorseWPMIncrement();
    const int currentWpm = m_config.currentState->cwSpeed;

    int newWpm;
    if (increase) {
        newWpm = qMin(currentWpm + increment, K4Limits::CW_WPM_MAX);
    } else {
        newWpm = qMax(currentWpm - increment, K4Limits::CW_WPM_MIN);
    }

    // Send to radio - display will update when radio responds via stateUpdated
    m_config.radio->setCWSpeed(newWpm);

    const QString direction = increase ? "increased" : "decreased";
    const QString keyName = increase ? "PgUp" : "PgDn";
    emit statusMessage(QString("CW Speed: %1 WPM").arg(newWpm));
    LOG_DEBUG("InputHandler", QString("WPM %1 to %2 (%3)")
              .arg(direction)
              .arg(newWpm)
              .arg(keyName));

    emit cwSpeedChanged(newWpm);
    return true;
}

bool InputHandlerService::handleTabKey(const KeyContext& context)
{
    // Only handle Tab in callsign field
    if (context.focusWidget != context.callsignEntry) {
        return false;
    }

    emit switchToSPMode();
    LOG_DEBUG("InputHandler", "Tab pressed in callsign field - switched to S&P mode");
    return true;  // Event handled, don't tab to next field
}

bool InputHandlerService::handleEscapeKey(const KeyContext& context)
{
    // ALWAYS stop CW transmission first, regardless of where focus is
    if (isRadioConnected() && m_config.radio) {
        m_config.radio->stopCW();
        emit statusMessage("CW transmission aborted");
        emit stopCW();
        LOG_DEBUG("InputHandler", "CW transmission aborted via ESC key");
    }

    // ESC in callsign field: clear if not empty, or switch to CQ if empty & in S&P
    if (context.focusWidget == context.callsignEntry) {
        if (!context.callsignEmpty) {
            // First ESC: Clear callsign (stay in current mode)
            emit clearCallsign();
            LOG_DEBUG("InputHandler", "ESC pressed in callsign field - cleared");
        } else if (context.inSPMode) {
            // Second ESC (empty field in S&P mode): Return to CQ mode
            emit switchToCQMode();
            LOG_DEBUG("InputHandler", "ESC pressed in empty callsign field (S&P mode) - switched to CQ mode");
        }
        return true;  // Event handled
    }

    // ESC in exchange field: clear exchange and return focus to callsign
    if (context.focusWidget == context.exchangeEntry) {
        emit clearExchange();
        emit focusCallsign();
        LOG_DEBUG("InputHandler", "ESC pressed in exchange field - cleared and returned to callsign");
        return true;  // Event handled
    }

    return true;  // ESC always handled (we stopped CW at minimum)
}

bool InputHandlerService::handleFunctionKey(QKeyEvent* event)
{
    const int fKey = event->key() - Qt::Key_F1 + 1;  // Convert to 1-12

    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool ctrlPressed = mods & Qt::ControlModifier;
    const bool altPressed = mods & Qt::AltModifier;

    emit sendFunctionKey(fKey, ctrlPressed, altPressed);
    return true;  // Event handled
}

bool InputHandlerService::handleRepeatKey(const QString& lastCWMessage)
{
    if (!isRadioConnected() || !m_config.radio) {
        LOG_WARN("InputHandler", "Cannot send CW: radio not connected");
        emit statusMessage("CW requires radio connection");
        return true;
    }

    if (!isCWMode(m_config.currentState->modeA)) {
        LOG_WARN("InputHandler", "Cannot send CW: not in CW mode");
        emit statusMessage("CW requires CW mode");
        return true;
    }

    if (lastCWMessage.isEmpty()) {
        LOG_INFO("InputHandler", "= key pressed but no previous CW message to repeat");
        emit statusMessage("No previous CW message to repeat");
        return true;
    }

    // Resend the last message
    const int wpm = AppSettings::instance().getMorseWPM();
    m_config.radio->setCWSpeed(wpm);
    m_config.radio->sendCW(lastCWMessage);

    emit statusMessage(QString("Repeating CW: %1").arg(lastCWMessage));
    emit repeatLastCW(lastCWMessage);
    LOG_INFO("InputHandler", QString("Repeated CW: %1 (via = key)").arg(lastCWMessage));
    return true;
}

bool InputHandlerService::canDoCWOperations() const
{
    if (!isRadioConnected() || !m_config.radio || !m_config.currentState) {
        return false;
    }

    return isCWMode(m_config.currentState->modeA);
}

bool InputHandlerService::isRadioConnected() const
{
    return m_config.radioConnected && *m_config.radioConnected;
}

} // namespace TR4QT
