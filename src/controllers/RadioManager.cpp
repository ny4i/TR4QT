#include "RadioManager.h"
#include "../logging/LogMacros.h"
#include "../utils/DialogHelper.h"
#include <QApplication>

namespace TR4QT {

RadioManager::RadioManager(QObject* parent)
    : QObject(parent)
    , m_radio(new RadioController(this))
    , m_currentState()
    , m_radioConnected(false)
    , m_radioAutoReconnect(false)
    , m_radioReconnectTimer(new QTimer(this))
    , m_lastRadioConfig()
    , m_radioReconnectAttempts(0)
    , m_radioFlashTimer(new QTimer(this))
    , m_radioFlashState(false)
{
    // Setup reconnection timer (single-shot, 10 seconds)
    m_radioReconnectTimer->setSingleShot(true);
    m_radioReconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    connect(m_radioReconnectTimer, &QTimer::timeout, this, &RadioManager::onReconnectTimeout);

    // Setup flash timer (500ms flash rate)
    m_radioFlashTimer->setInterval(FLASH_INTERVAL_MS);
    connect(m_radioFlashTimer, &QTimer::timeout, this, &RadioManager::onFlashTimeout);

    // Connect radio controller signals
    connect(m_radio, &RadioController::connectionStatusChanged,
            this, &RadioManager::onRadioConnected);
    connect(m_radio, &RadioController::stateUpdated,
            this, &RadioManager::onRadioStateUpdated);
    connect(m_radio, &RadioController::errorOccurred,
            this, &RadioManager::onRadioError);
}

RadioManager::~RadioManager()
{
    // Cleanup handled by Qt parent-child relationship
    // RadioController and timers will be deleted automatically
}

bool RadioManager::connectToRadio()
{
    AppSettings& settings = AppSettings::instance();

    // Check if radio configuration exists
    if (!settings.hasRadioConfig()) {
        emit statusMessage("No radio configuration - please configure your radio first");
        return false;
    }

    RadioConfig config = settings.loadRadioConfig();

    // Validate that a valid radio model is selected
    if (config.hamlibModelId <= 0) {
        emit statusMessage("Invalid radio model - please select a valid radio model");
        return false;
    }

    // Enable auto-reconnect when user initiates connection
    m_radioAutoReconnect = true;
    m_radioReconnectTimer->stop();  // Stop any pending reconnect attempt
    m_radioReconnectAttempts = 0;   // Reset retry counter
    m_lastRadioConfig = config;     // Save config for reconnection attempts

    emit statusMessage(QString("Connecting to radio: Model %1, Port %2...")
                          .arg(config.hamlibModelId)
                          .arg(config.port));

    // Force UI update
    QApplication::processEvents();

    // Connect happens asynchronously in worker thread
    // connectionStatusChanged signal will indicate success/failure
    m_radio->connectToRadio(config);

    return true;
}

void RadioManager::disconnectFromRadio()
{
    // Disable auto-reconnect when user manually disconnects
    m_radioAutoReconnect = false;
    m_radioReconnectTimer->stop();

    emit statusMessage("Disconnecting from radio...");
    m_radio->disconnectFromRadio();
}

void RadioManager::onRadioConnected(bool connected)
{
    LOG_DEBUG("RadioManager", QString("onRadioConnected called with connected = %1")
        .arg(connected ? "true" : "false"));

    m_radioConnected = connected;

    if (connected) {
        // Stop reconnect timer on successful connection
        m_radioReconnectTimer->stop();
        m_radioReconnectAttempts = 0;  // Reset retry counter on success

        // Stop flashing indicator
        m_radioFlashTimer->stop();
        m_radioFlashState = false;
        emit flashStateChanged(false);  // Update to normal color

        // Don't show "waiting for state" - stateUpdated will arrive immediately
        // Status will be updated when radio model arrives in state update
    } else {
        emit statusMessage("Radio disconnected");

        // Start flashing red indicator only if a radio is configured
        if (AppSettings::instance().hasRadioConfig()) {
            m_radioFlashTimer->start();
        }

        // Start auto-reconnect timer if enabled
        if (m_radioAutoReconnect) {
            LOG_DEBUG("RadioManager", "Radio disconnected - will attempt reconnect in 10 seconds");
            emit statusMessage("Radio disconnected - will retry in 10 seconds...");
            m_radioReconnectTimer->start();
        }
    }

    // Emit connection status change
    emit connectionStatusChanged(connected);
}

void RadioManager::onRadioStateUpdated(const RadioState& state)
{
    // DEBUG: Confirm this slot is being called
    static int updateCount = 0;
    updateCount++;
    double freqKHz = state.frequencyA / 1000.0;
    LOG_INFO("RadioManager", QString("onRadioStateUpdated called (count=%1, model=%2, freq=%3 kHz)")
             .arg(updateCount)
             .arg(state.radioModel)
             .arg(freqKHz, 0, 'f', 1));

    // Update status with radio model (always, not just when changed)
    if (!state.radioModel.isEmpty()) {
        static QString lastModel;
        if (state.radioModel != lastModel) {
            LOG_DEBUG("RadioManager", QString("Radio model from state: %1").arg(state.radioModel));
            lastModel = state.radioModel;
            emit radioModelChanged(state.radioModel);
        }
        // Always update status bar with radio model (even on reconnects)
        emit statusMessage(QString("Radio: %1").arg(state.radioModel));
    }

    // Check for frequency/band changes
    bool frequencyChanged = (state.frequencyA != m_currentState.frequencyA);
    bool bandChanged = (state.bandA != m_currentState.bandA);

    // Update cached state
    m_currentState = state;

    // Emit radio state updated signal
    emit radioStateUpdated(state);

    // Emit frequency/band change signals if changed
    if (frequencyChanged && state.frequencyA > 0) {
        emit this->frequencyChanged(state.frequencyA);
    }
    if (bandChanged && state.bandA != BandType::None) {
        emit this->bandChanged(state.bandA);
    }
}

void RadioManager::onRadioError(const QString& error)
{
    emit statusMessage(QString("Radio error: %1").arg(error));
    emit radioErrorOccurred(error);

    // If auto-reconnect is enabled and we're not connected, restart the reconnect timer
    // This handles pre-flight failures during reconnect attempts
    if (m_radioAutoReconnect && !m_radioConnected) {
        LOG_DEBUG("RadioManager", QString("Radio error during reconnect (attempt %1), will retry in 10 seconds")
            .arg(m_radioReconnectAttempts));
        m_radioReconnectTimer->start();
    }
}

void RadioManager::onReconnectTimeout()
{
    if (m_radioAutoReconnect) {
        m_radioReconnectAttempts++;
        emit statusMessage(QString("Reconnecting to radio (attempt %1)...")
            .arg(m_radioReconnectAttempts));
        LOG_DEBUG("RadioManager", QString("Auto-reconnect: Attempt %1")
            .arg(m_radioReconnectAttempts));
        m_radio->connectToRadio(m_lastRadioConfig);
        // Note: No attempt limit - will keep trying until radio comes back or user clicks Disconnect
    }
}

void RadioManager::onFlashTimeout()
{
    m_radioFlashState = !m_radioFlashState;
    emit flashStateChanged(m_radioFlashState);
}

} // namespace TR4QT
