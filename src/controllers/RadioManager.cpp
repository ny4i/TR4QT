#include "RadioManager.h"
#include "../logging/LogMacros.h"
#include "../utils/DialogHelper.h"
#include <QApplication>
#include <QThread>

// TODO: UX Improvement - Don't show Hamlib model ID when using direct interfaces
// When radioType is K4_DIRECT or ICOM_DIRECT, the Hamlib model ID is only used
// internally for configuration (e.g., CI-V address auto-config) but NOT for
// actual radio communication. Displaying it in status messages/tooltips is
// misleading because it implies Hamlib is being used.
//
// Suggested changes:
// 1. Status messages: Show radio name instead of model ID for direct interfaces
//    - CURRENT: "Connecting to radio: Model 3092, Port 192.168.1.100:50001..."
//    - BETTER:  "Connecting to radio: IC-7760 (Icom Direct), Port 192.168.1.100:50001..."
//    - BETTER:  "Connecting to radio: K4 (K4 Direct), Port 192.168.73.108:9200..."
//
// 2. Error messages: Clarify which interface failed
//    - CURRENT: "Failed to connect to radio Model 3092"
//    - BETTER:  "Failed to connect to IC-7760 via Icom Direct"
//
// 3. Tooltips: Only mention Hamlib when actually using Hamlib interface
//
// Implementation approach:
// - Add RadioFactory::getRadioDisplayName(hamlibModelId, radioType) helper
// - Returns model name from rig_get_caps() for all types
// - Appends interface type for clarity: "(K4 Direct)", "(Icom Direct)", "(Hamlib)"
// - Use this helper throughout status messages instead of raw model ID

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

    // Connect fast individual field signals for instant transceive updates
    // Note: Only frequency/band need fast updates for VFO display
    // Mode changes are less frequent and handled by periodic stateUpdated
    connect(m_radio, &RadioController::frequencyChanged,
            this, &RadioManager::onFrequencyChanged);
}

RadioManager::~RadioManager()
{
    // Cleanup handled by Qt parent-child relationship
    // RadioController and timers will be deleted automatically
}

bool RadioManager::connectToRadio()
{
    AppSettings& settings = AppSettings::instance();

    // Check if radio profiles exist
    if (!settings.hasRadioProfiles()) {
        emit statusMessage("No radio configuration - please configure your radio first");
        return false;
    }

    // Load active profile
    QString activeProfileName = settings.getActiveRadioProfile();
    QList<RadioProfile> profiles = settings.loadRadioProfiles();

    LOG_DEBUG("RadioManager", QString("Active profile name: '%1'").arg(activeProfileName));
    LOG_DEBUG("RadioManager", QString("Total profiles loaded: %1").arg(profiles.size()));

    // Debug: Log all profile names
    for (const auto& p : profiles) {
        LOG_DEBUG("RadioManager", QString("  Profile: '%1' (model: %2, port: %3)")
            .arg(p.name).arg(p.config.hamlibModelId).arg(p.config.port));
    }

    // Find active profile
    RadioProfile* activeProfile = nullptr;
    for (auto& profile : profiles) {
        if (profile.name == activeProfileName) {
            activeProfile = &profile;
            break;
        }
    }

    if (!activeProfile) {
        emit statusMessage(QString("Active profile '%1' not found - please reconfigure").arg(activeProfileName));
        LOG_ERROR("RadioManager", QString("Active profile '%1' not found in loaded profiles").arg(activeProfileName));
        return false;
    }

    // Validate that a valid radio model is selected
    if (!activeProfile->isValid()) {
        emit statusMessage("Invalid active profile - please select a valid radio model");
        return false;
    }

    RadioConfig config = activeProfile->config;

    // Update last used timestamp
    activeProfile->lastUsed = QDateTime::currentDateTime();
    settings.saveRadioProfiles(profiles);

    // CRITICAL: If switching to a different radio (different port/model), disconnect first
    // This prevents auto-reconnect from trying to reconnect to the OLD radio
    bool isDifferentRadio = (m_lastRadioConfig.port != config.port) ||
                            (m_lastRadioConfig.hamlibModelId != config.hamlibModelId);

    if (m_radio->isConnected() && isDifferentRadio) {
        LOG_INFO("RadioManager", QString("Profile switch detected: disconnecting from old radio before connecting to %1")
            .arg(activeProfile->name));

        // Disable auto-reconnect BEFORE disconnecting (prevent reconnect to old radio)
        m_radioAutoReconnect = false;
        m_radioReconnectTimer->stop();

        // Disconnect from old radio
        m_radio->disconnectFromRadio();

        // Wait for disconnect to complete (500ms should be plenty)
        // This ensures the old radio is fully closed before opening new one
        QThread::msleep(500);
    }

    // Update config FIRST (before enabling auto-reconnect)
    m_lastRadioConfig = config;     // Save config for reconnection attempts

    // Stop any pending reconnect timer
    m_radioReconnectTimer->stop();
    m_radioReconnectAttempts = 0;   // Reset retry counter

    // TODO: Show radio name + interface type instead of model ID (see TODO at top of file)
    // Should be: "Connecting to radio: IC-7760 (Icom Direct), Port 192.168.1.100:50001..."
    emit statusMessage(QString("Connecting to radio: %1, Port %2...")
                          .arg(activeProfile->name)
                          .arg(config.port));

    // Force UI update
    QApplication::processEvents();

    // Connect happens asynchronously in worker thread
    // connectionStatusChanged signal will indicate success/failure
    m_radio->connectToRadio(config);

    // IMPORTANT: Only NOW enable auto-reconnect (after connecting to new radio)
    // This ensures that if the OLD radio's disconnect signal fires late,
    // it won't trigger a reconnect attempt (because m_lastRadioConfig is now updated)
    m_radioAutoReconnect = true;

    return true;
}

void RadioManager::disconnectFromRadio()
{
    // Disable auto-reconnect when user manually disconnects
    m_radioAutoReconnect = false;
    m_radioReconnectTimer->stop();

    emit statusMessage("Disconnecting from radio...");
    m_radio->disconnectFromRadio();

    // CRITICAL: Wait for disconnect to complete (sends UDP packets to radio)
    // Without this, if user clicks Connect immediately after Disconnect,
    // the disconnect packets haven't been sent and radio stays in connected state
    QApplication::processEvents();  // Process queued disconnect
    QThread::msleep(100);           // Allow UDP packets to be transmitted
    LOG_DEBUG("RadioManager", "Manual disconnect completed");
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

void RadioManager::onFrequencyChanged(freq_t freq, VFO vfo)
{
    // Main window displays VFO A only - ignore VFO B updates
    if (vfo != VFO::VFO_A) {
        return;
    }

    // Throttle updates to display refresh rate (60 Hz = 16ms)
    // Radio may send transceive updates faster than display can refresh
    // Skipping intermediate updates reduces CPU usage without visible impact
    static QElapsedTimer throttle;
    if (throttle.isValid() && throttle.elapsed() < 16) {
        return;  // Skip update if less than 16ms since last
    }
    throttle.start();

    // TIMING: Measure how long RadioManager takes to forward frequency signal
    static QElapsedTimer fwdTimer;
    static bool fwdTimerStarted = false;
    if (!fwdTimerStarted) {
        fwdTimer.start();
        fwdTimerStarted = true;
    }
    qint64 fwdStart = fwdTimer.nsecsElapsed();

    // Fast path: update cached frequency and emit signal immediately for transceive updates
    // This bypasses the full state update for instant VFO display updates
    m_currentState.frequencyA = freq;
    m_currentState.bandA = frequencyToBand(freq);

    emit this->frequencyChanged(freq);
    emit this->bandChanged(m_currentState.bandA);

    qint64 fwdEnd = fwdTimer.nsecsElapsed();
    LOG_DEBUG("RadioManager", QString("Frequency forwarded: %1 Hz [forward=%2μs]")
        .arg(freq).arg((fwdEnd - fwdStart) / 1000));
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
