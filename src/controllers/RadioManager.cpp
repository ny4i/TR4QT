#include "RadioManager.h"
#include "KPA1500UdpPoller.h"
#include "../logging/LogMacros.h"
#include "../utils/DialogHelper.h"
#include <QApplication>
#include <QThread>
#include <QHostAddress>

namespace TR4QT {

RadioManager::RadioManager(QObject* parent)
    : QObject(parent)
    , m_radioFlashTimer(new QTimer(this))
{
    // Initialize radio controllers and reconnection managers for both radios
    for (int i = 0; i < MAX_RADIOS; ++i) {
        m_radios[i] = new RadioController(this);
        m_reconnectManagers[i] = new ReconnectionManager(RECONNECT_INTERVAL_MS, 0, this);
    }

    // Set legacy pointer to Radio 1 by default
    m_radio = m_radios[RADIO_1];

    // Setup reconnection managers
    connect(m_reconnectManagers[RADIO_1], &ReconnectionManager::retryRequested,
            this, &RadioManager::onRetryRequested0);
    connect(m_reconnectManagers[RADIO_2], &ReconnectionManager::retryRequested,
            this, &RadioManager::onRetryRequested1);

    // Setup flash timer (500ms flash rate)
    m_radioFlashTimer->setInterval(FLASH_INTERVAL_MS);
    connect(m_radioFlashTimer, &QTimer::timeout, this, &RadioManager::onFlashTimeout);

    // Setup signal connections for both radios
    setupRadioConnections(RADIO_1);
    setupRadioConnections(RADIO_2);

    // Setup amplifier poller (KPA1500)
    AppSettings& settings = AppSettings::instance();
    if (settings.getAmplifierEnabled()) {
        QString ipAddress = settings.getAmplifierIpAddress();
        int port = settings.getAmplifierPortNumber();

        if (!ipAddress.isEmpty()) {
            m_amplifier = new KPA1500UdpPoller(this);
            m_amplifier->setAmplifierAddress(QHostAddress(ipAddress), port);
            m_amplifier->setPollIntervalMs(100);  // Fast polling (100ms) during TX
            m_amplifier->setPollCommands({"^PWF;", "^OS;"});  // Forward power + Operating status

            // Connect amplifier signals
            connect(m_amplifier, &KPA1500UdpPoller::forwardPowerChanged,
                    this, &RadioManager::onAmplifierPowerChanged);
            connect(m_amplifier, &KPA1500UdpPoller::operatingStatusChanged,
                    this, &RadioManager::onAmplifierOperatingStatusChanged);
            connect(m_amplifier, &KPA1500UdpPoller::errorOccurred,
                    this, &RadioManager::onAmplifierError);

            LOG_INFO("RadioManager", QString("KPA1500 amplifier configured at %1:%2").arg(ipAddress).arg(port));
        } else {
            LOG_WARN("RadioManager", "Amplifier enabled but no IP address configured");
        }
    }

    // Load SO2R enabled state from active station profile
    QString activeStationProfile = settings.getActiveStationProfile();
    if (!activeStationProfile.isEmpty()) {
        StationProfile profile = settings.getStationProfile(activeStationProfile);
        m_so2rEnabled = profile.so2rEnabled;
    } else {
        // Fall back to legacy setting for backward compatibility
        m_so2rEnabled = settings.isSO2REnabled();
    }
    LOG_INFO("RadioManager", QString("RadioManager initialized, SO2R mode: %1")
             .arg(m_so2rEnabled ? "enabled" : "disabled"));
}

RadioManager::~RadioManager()
{
    // Cleanup handled by Qt parent-child relationship
    // RadioControllers, ReconnectionManagers, and timers will be deleted automatically
}

void RadioManager::setupRadioConnections(int radioIndex)
{
    RadioController* radio = m_radios[radioIndex];
    if (!radio) return;

    // Use indexed slots to route signals to the correct handler
    if (radioIndex == RADIO_1) {
        connect(radio, &RadioController::connectionStatusChanged,
                this, &RadioManager::onRadioConnected0);
        connect(radio, &RadioController::stateUpdated,
                this, &RadioManager::onRadioStateUpdated0);
        connect(radio, &RadioController::errorOccurred,
                this, &RadioManager::onRadioError0);
        connect(radio, &RadioController::frequencyChanged,
                this, &RadioManager::onFrequencyChanged0);
    } else {
        connect(radio, &RadioController::connectionStatusChanged,
                this, &RadioManager::onRadioConnected1);
        connect(radio, &RadioController::stateUpdated,
                this, &RadioManager::onRadioStateUpdated1);
        connect(radio, &RadioController::errorOccurred,
                this, &RadioManager::onRadioError1);
        connect(radio, &RadioController::frequencyChanged,
                this, &RadioManager::onFrequencyChanged1);
    }
}

// ========== Single-Radio API (backward compatible) ==========

RadioController* RadioManager::radioController() const
{
    return m_radios[m_activeRadioIndex];
}

bool RadioManager::isConnected() const
{
    return m_radioConnected[m_activeRadioIndex];
}

RadioState RadioManager::currentState() const
{
    return m_radioStates[m_activeRadioIndex];
}

bool RadioManager::connectToRadio()
{
    AppSettings& settings = AppSettings::instance();

    // Try to use new StationProfile system first
    QString stationProfileName = settings.getActiveStationProfile();
    if (!stationProfileName.isEmpty()) {
        StationProfile stationProfile = settings.getStationProfile(stationProfileName);
        if (!stationProfile.name.isEmpty()) {
            return connectWithStationProfile(stationProfile);
        }
        LOG_WARN("RadioManager", QString("Station profile '%1' not found, falling back to legacy mode")
                 .arg(stationProfileName));
    }

    // Legacy single-radio mode for backward compatibility
    if (!settings.hasRadioProfiles()) {
        emit statusMessage("No radio configuration - please configure your radio first");
        return false;
    }

    QString activeProfileName = settings.getActiveRadioProfile();
    QList<RadioProfile> profiles = settings.loadRadioProfiles();

    LOG_DEBUG("RadioManager", QString("Legacy mode - Active profile name: '%1'").arg(activeProfileName));

    RadioProfile* activeProfile = nullptr;
    for (auto& profile : profiles) {
        if (profile.name == activeProfileName) {
            activeProfile = &profile;
            break;
        }
    }

    if (!activeProfile) {
        emit statusMessage(QString("Active profile '%1' not found - please reconfigure").arg(activeProfileName));
        LOG_ERROR("RadioManager", QString("Active profile '%1' not found").arg(activeProfileName));
        return false;
    }

    if (!activeProfile->isValid()) {
        emit statusMessage("Invalid active profile - please select a valid radio model");
        return false;
    }

    // Update last used timestamp
    activeProfile->lastUsed = QDateTime::currentDateTime();
    settings.saveRadioProfiles(profiles);

    // Connect Radio 1 with the active profile
    bool result = connectRadio(RADIO_1, activeProfile->config);
    m_radioAutoReconnect = result;
    return result;
}

bool RadioManager::connectWithStationProfile(const StationProfile& stationProfile)
{
    AppSettings& settings = AppSettings::instance();

    LOG_INFO("RadioManager", QString("Connecting with station profile '%1': Radio1='%2', Radio2='%3', SO2R=%4, DefaultActive=%5")
             .arg(stationProfile.name)
             .arg(stationProfile.radio1Name)
             .arg(stationProfile.radio2Name)
             .arg(stationProfile.so2rEnabled ? "yes" : "no")
             .arg(stationProfile.defaultActive));

    // Update SO2R state from station profile
    m_so2rEnabled = stationProfile.so2rEnabled;

    QList<RadioProfile> radioProfiles = settings.loadRadioProfiles();
    bool success = false;

    // Connect Radio 1 if assigned
    if (!stationProfile.radio1Name.isEmpty()) {
        bool found = false;
        for (const auto& profile : radioProfiles) {
            if (profile.name == stationProfile.radio1Name) {
                found = true;
                if (profile.isValid()) {
                    if (connectRadio(RADIO_1, profile.config)) {
                        success = true;
                    }
                } else {
                    LOG_WARN("RadioManager", QString("Radio 1 profile '%1' is invalid")
                             .arg(stationProfile.radio1Name));
                }
                break;
            }
        }
        if (!found) {
            LOG_WARN("RadioManager", QString("Radio 1 profile '%1' not found")
                     .arg(stationProfile.radio1Name));
        }
    }

    // Connect Radio 2 if SO2R enabled and assigned
    if (stationProfile.so2rEnabled && !stationProfile.radio2Name.isEmpty()) {
        bool found = false;
        for (const auto& profile : radioProfiles) {
            if (profile.name == stationProfile.radio2Name) {
                found = true;
                if (profile.isValid()) {
                    if (connectRadio(RADIO_2, profile.config)) {
                        success = true;
                    }
                } else {
                    LOG_WARN("RadioManager", QString("Radio 2 profile '%1' is invalid")
                             .arg(stationProfile.radio2Name));
                }
                break;
            }
        }
        if (!found) {
            LOG_WARN("RadioManager", QString("Radio 2 profile '%1' not found")
                     .arg(stationProfile.radio2Name));
        }
    }

    // Set default active radio based on station profile
    if (stationProfile.defaultActive == 1 && stationProfile.so2rEnabled) {
        m_activeRadioIndex = RADIO_2;
        m_radio = m_radios[RADIO_2];
        LOG_INFO("RadioManager", "Default active radio set to Radio 2");
    } else {
        m_activeRadioIndex = RADIO_1;
        m_radio = m_radios[RADIO_1];
        LOG_INFO("RadioManager", "Default active radio set to Radio 1");
    }

    if (!success) {
        emit statusMessage("No valid radio profiles configured in station profile");
    }

    m_radioAutoReconnect = success;
    return success;
}

void RadioManager::disconnectFromRadio()
{
    m_radioAutoReconnect = false;

    // Disconnect all radios
    for (int i = 0; i < MAX_RADIOS; ++i) {
        if (m_radioConnected[i]) {
            disconnectRadio(i);
        }
        m_reconnectManagers[i]->reset();
    }

    emit statusMessage("Disconnected from radio(s)");
    LOG_DEBUG("RadioManager", "Manual disconnect completed");
}

// ========== SO2R Multi-Radio API ==========

void RadioManager::setSO2REnabled(bool enabled)
{
    if (m_so2rEnabled == enabled) return;

    m_so2rEnabled = enabled;
    AppSettings::instance().setSO2REnabled(enabled);

    LOG_INFO("RadioManager", QString("SO2R mode %1").arg(enabled ? "enabled" : "disabled"));

    // If disabling SO2R and Radio 2 is connected, disconnect it
    if (!enabled && m_radioConnected[RADIO_2]) {
        disconnectRadio(RADIO_2);
    }

    // Ensure active radio is Radio 1 when disabling SO2R
    if (!enabled && m_activeRadioIndex != RADIO_1) {
        setActiveRadio(RADIO_1);
    }
}

void RadioManager::setActiveRadio(int radioIndex)
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) {
        LOG_WARN("RadioManager", QString("Invalid radio index: %1").arg(radioIndex));
        return;
    }

    if (m_activeRadioIndex == radioIndex) return;

    int previousActive = m_activeRadioIndex;
    m_activeRadioIndex = radioIndex;

    // Update legacy pointer
    m_radio = m_radios[m_activeRadioIndex];
    m_currentState = m_radioStates[m_activeRadioIndex];

    LOG_INFO("RadioManager", QString("Active radio changed: Radio %1 -> Radio %2")
             .arg(previousActive + 1).arg(radioIndex + 1));

    // Emit signals for UI update
    emit activeRadioChanged(radioIndex);

    // Re-emit state for the new active radio
    if (m_radioConnected[radioIndex]) {
        emit connectionStatusChanged(true);
        emit radioStateUpdated(m_radioStates[radioIndex]);
        emit frequencyChanged(m_radioStates[radioIndex].frequencyA);
        emit bandChanged(m_radioStates[radioIndex].bandA);
    } else {
        emit connectionStatusChanged(false);
    }

    // Emit standby frequency for the previous active (now standby) radio
    if (m_radioConnected[previousActive]) {
        emit standbyFrequencyChanged(m_radioStates[previousActive].frequencyA);
        emit standbyBandChanged(m_radioStates[previousActive].bandA);
    }
}

void RadioManager::toggleActiveRadio()
{
    if (!m_so2rEnabled) {
        LOG_DEBUG("RadioManager", "Toggle ignored - SO2R not enabled");
        return;
    }

    // Only toggle if we have at least one connected radio to switch to
    int otherRadio = getStandbyRadioIndex();
    if (m_radioConnected[otherRadio]) {
        setActiveRadio(otherRadio);
    } else {
        LOG_DEBUG("RadioManager", QString("Cannot toggle - Radio %1 not connected").arg(otherRadio + 1));
        emit statusMessage(QString("Radio %1 not connected").arg(otherRadio + 1));
    }
}

bool RadioManager::connectRadio(int radioIndex, const RadioConfig& config)
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) {
        LOG_ERROR("RadioManager", QString("Invalid radio index: %1").arg(radioIndex));
        return false;
    }

    RadioController* radio = m_radios[radioIndex];
    if (!radio) {
        LOG_ERROR("RadioManager", QString("Radio %1 controller is null").arg(radioIndex + 1));
        return false;
    }

    // If already connected to a different config, disconnect first
    if (radio->isConnected()) {
        bool isDifferentRadio = (m_lastRadioConfigs[radioIndex].port != config.port) ||
                                (m_lastRadioConfigs[radioIndex].hamlibModelId != config.hamlibModelId);
        if (isDifferentRadio) {
            LOG_INFO("RadioManager", QString("Radio %1: Disconnecting before switching to new config")
                     .arg(radioIndex + 1));
            radio->disconnectFromRadio();
            QThread::msleep(500);
        }
    }

    m_lastRadioConfigs[radioIndex] = config;
    m_reconnectManagers[radioIndex]->reset();

    emit statusMessage(QString("Connecting Radio %1: %2...")
                       .arg(radioIndex + 1)
                       .arg(config.port));

    QApplication::processEvents();

    radio->connectToRadio(config);
    return true;
}

void RadioManager::disconnectRadio(int radioIndex)
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) return;

    RadioController* radio = m_radios[radioIndex];
    if (!radio) return;

    m_reconnectManagers[radioIndex]->reset();

    LOG_INFO("RadioManager", QString("Disconnecting Radio %1").arg(radioIndex + 1));
    radio->disconnectFromRadio();

    QApplication::processEvents();
    QThread::msleep(100);
}

bool RadioManager::isRadioConnected(int radioIndex) const
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) return false;
    return m_radioConnected[radioIndex];
}

RadioState RadioManager::getRadioState(int radioIndex) const
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) return RadioState();
    return m_radioStates[radioIndex];
}

RadioController* RadioManager::getRadioController(int radioIndex) const
{
    if (radioIndex < 0 || radioIndex >= MAX_RADIOS) return nullptr;
    return m_radios[radioIndex];
}

int RadioManager::getConnectedRadioCount() const
{
    int count = 0;
    for (int i = 0; i < MAX_RADIOS; ++i) {
        if (m_radioConnected[i]) ++count;
    }
    return count;
}

// ========== Indexed Slot Trampolines ==========

void RadioManager::onRadioConnected0(bool connected)
{
    onRadioConnectedIndexed(RADIO_1, connected);
}

void RadioManager::onRadioConnected1(bool connected)
{
    onRadioConnectedIndexed(RADIO_2, connected);
}

void RadioManager::onRadioStateUpdated0(const RadioState& state)
{
    onRadioStateUpdatedIndexed(RADIO_1, state);
}

void RadioManager::onRadioStateUpdated1(const RadioState& state)
{
    onRadioStateUpdatedIndexed(RADIO_2, state);
}

void RadioManager::onRadioError0(const QString& error)
{
    onRadioErrorIndexed(RADIO_1, error);
}

void RadioManager::onRadioError1(const QString& error)
{
    onRadioErrorIndexed(RADIO_2, error);
}

void RadioManager::onFrequencyChanged0(freq_t freq, VFO vfo)
{
    onFrequencyChangedIndexed(RADIO_1, freq, vfo);
}

void RadioManager::onFrequencyChanged1(freq_t freq, VFO vfo)
{
    onFrequencyChangedIndexed(RADIO_2, freq, vfo);
}

void RadioManager::onRetryRequested0(int attempt)
{
    onRetryRequestedIndexed(RADIO_1, attempt);
}

void RadioManager::onRetryRequested1(int attempt)
{
    onRetryRequestedIndexed(RADIO_2, attempt);
}

// ========== Indexed Handler Implementations ==========

void RadioManager::onRadioConnectedIndexed(int radioIndex, bool connected)
{
    LOG_DEBUG("RadioManager", QString("Radio %1 connected: %2")
              .arg(radioIndex + 1).arg(connected ? "true" : "false"));

    m_radioConnected[radioIndex] = connected;

    if (connected) {
        m_reconnectManagers[radioIndex]->recordSuccess();

        // Clear the "Connecting Radio X..." status message
        emit statusMessage(QString("Radio %1 connected").arg(radioIndex + 1));

        // Update max power for active radio
        if (radioIndex == m_activeRadioIndex) {
            m_radioFlashTimer->stop();
            m_radioFlashState = false;
            emit flashStateChanged(false);

            if (m_radios[radioIndex]) {
                int radioMaxPower = m_radios[radioIndex]->maxPowerWatts();
                emit maxPowerChanged(radioMaxPower);
            }
        }
    } else {
        // Start flashing if active radio disconnected
        if (radioIndex == m_activeRadioIndex) {
            if (AppSettings::instance().hasAnyRadioConfig()) {
                m_radioFlashTimer->start();
            }

            if (m_radioAutoReconnect) {
                LOG_DEBUG("RadioManager", QString("Radio %1 disconnected - will retry in 10 seconds")
                          .arg(radioIndex + 1));
                emit statusMessage(QString("Radio %1 disconnected - will retry...")
                                   .arg(radioIndex + 1));
                m_reconnectManagers[radioIndex]->start();
            }
        }
    }

    // Emit indexed signal for SO2R tracking
    if (connected) {
        emit radioConnectedIndexed(radioIndex);
    } else {
        emit radioDisconnectedIndexed(radioIndex);
    }

    // Emit legacy signal for active radio
    if (radioIndex == m_activeRadioIndex) {
        emit connectionStatusChanged(connected);
    }
}

void RadioManager::onRadioStateUpdatedIndexed(int radioIndex, const RadioState& state)
{
    static int updateCounts[MAX_RADIOS] = {0, 0};
    updateCounts[radioIndex]++;

    double freqKHz = state.frequencyA / 1000.0;
    LOG_TRACE("RadioManager", QString("Radio %1 state update (count=%2, freq=%3 kHz)")
              .arg(radioIndex + 1).arg(updateCounts[radioIndex]).arg(freqKHz, 0, 'f', 1));

    // Check for frequency/band changes on this radio
    bool frequencyChanged = (state.frequencyA != m_radioStates[radioIndex].frequencyA);
    bool bandChanged = (state.bandA != m_radioStates[radioIndex].bandA);

    // Handle TX state for amplifier (only for active radio)
    if (radioIndex == m_activeRadioIndex) {
        bool wasTransmitting = m_radioStates[radioIndex].isTransmitting;
        bool isTransmitting = state.isTransmitting;

        if (m_amplifier && m_radios[radioIndex]) {
            if (isTransmitting && !wasTransmitting) {
                m_amplifierOperateMode = false;
                int radioMaxPower = m_radios[radioIndex]->maxPowerWatts();
                emit maxPowerChanged(radioMaxPower);
                m_amplifier->queryNow();
                m_amplifier->start();
            } else if (!isTransmitting && wasTransmitting) {
                m_amplifier->stop();
            }
        }
    }

    // Update cached state
    m_radioStates[radioIndex] = state;

    // Apply amplifier power override if applicable
    if (radioIndex == m_activeRadioIndex && m_amplifier &&
        state.isTransmitting && m_amplifierOperateMode) {
        m_radioStates[radioIndex].powerOutput = m_amplifierForwardPower * 10;
    }

    // Update legacy state if this is the active radio
    if (radioIndex == m_activeRadioIndex) {
        m_currentState = m_radioStates[radioIndex];
    }

    // Emit indexed signal
    emit radioStateUpdatedIndexed(radioIndex, m_radioStates[radioIndex]);

    // Emit model identification for any radio that reports its model
    if (!state.radioModel.isEmpty()) {
        static QString lastModels[2];  // Track model changes per radio
        if (state.radioModel != lastModels[radioIndex]) {
            lastModels[radioIndex] = state.radioModel;
            emit radioModelIdentified(radioIndex, state.radioModel);
        }
    }

    // Emit legacy signals for active radio
    if (radioIndex == m_activeRadioIndex) {
        if (!state.radioModel.isEmpty()) {
            static QString lastModel;
            if (state.radioModel != lastModel) {
                lastModel = state.radioModel;
                emit radioModelChanged(state.radioModel);
            }
        }

        emit radioStateUpdated(m_radioStates[radioIndex]);

        if (frequencyChanged && state.frequencyA > 0) {
            emit this->frequencyChanged(state.frequencyA);
        }
        if (bandChanged && state.bandA != BandType::None) {
            emit this->bandChanged(state.bandA);
        }
    } else {
        // This is the standby radio - emit standby signals
        if (frequencyChanged && state.frequencyA > 0) {
            emit standbyFrequencyChanged(state.frequencyA);
        }
        if (bandChanged && state.bandA != BandType::None) {
            emit standbyBandChanged(state.bandA);
        }
    }
}

void RadioManager::onRadioErrorIndexed(int radioIndex, const QString& error)
{
    emit statusMessage(QString("Radio %1 error: %2").arg(radioIndex + 1).arg(error));
    emit radioErrorOccurred(error);

    if (m_radioAutoReconnect && !m_radioConnected[radioIndex]) {
        LOG_DEBUG("RadioManager", QString("Radio %1 error during reconnect, will retry")
                  .arg(radioIndex + 1));
        m_reconnectManagers[radioIndex]->start();
    }
}

void RadioManager::onFrequencyChangedIndexed(int radioIndex, freq_t freq, VFO vfo)
{
    // Only process VFO A updates
    if (vfo != VFO::VFO_A) return;

    // Throttle updates to 60 Hz
    static QElapsedTimer throttle[MAX_RADIOS];
    if (throttle[radioIndex].isValid() && throttle[radioIndex].elapsed() < 16) {
        return;
    }
    throttle[radioIndex].start();

    // Update cached state
    m_radioStates[radioIndex].frequencyA = freq;
    m_radioStates[radioIndex].bandA = frequencyToBand(freq);

    if (radioIndex == m_activeRadioIndex) {
        m_currentState.frequencyA = freq;
        m_currentState.bandA = m_radioStates[radioIndex].bandA;

        emit this->frequencyChanged(freq);
        emit this->bandChanged(m_currentState.bandA);

        double freqMhz = freq / 1000000.0;
        LOG_DEBUG("RadioManager", QString("Radio %1 frequency: %2 MHz")
                  .arg(radioIndex + 1).arg(freqMhz, 0, 'f', 4));
    } else {
        // Standby radio frequency changed
        emit standbyFrequencyChanged(freq);
        emit standbyBandChanged(m_radioStates[radioIndex].bandA);
    }
}

void RadioManager::onRetryRequestedIndexed(int radioIndex, int attempt)
{
    if (m_radioAutoReconnect && m_radios[radioIndex]) {
        emit statusMessage(QString("Reconnecting Radio %1 (attempt %2)...")
                           .arg(radioIndex + 1).arg(attempt));
        LOG_DEBUG("RadioManager", QString("Radio %1 auto-reconnect attempt %2")
                  .arg(radioIndex + 1).arg(attempt));
        m_radios[radioIndex]->connectToRadio(m_lastRadioConfigs[radioIndex]);
    }
}

// ========== Other Handlers ==========

void RadioManager::onFlashTimeout()
{
    m_radioFlashState = !m_radioFlashState;
    emit flashStateChanged(m_radioFlashState);
}

void RadioManager::onAmplifierPowerChanged(int watts)
{
    m_amplifierForwardPower = watts;
    LOG_DEBUG("RadioManager", QString("KPA1500 forward power: %1W").arg(watts));

    if (m_radioStates[m_activeRadioIndex].isTransmitting && m_amplifierOperateMode) {
        m_radioStates[m_activeRadioIndex].powerOutput = watts * 10;
        m_currentState.powerOutput = watts * 10;
        emit radioStateUpdated(m_currentState);
    }
}

void RadioManager::onAmplifierOperatingStatusChanged(bool operateMode)
{
    m_amplifierOperateMode = operateMode;
    LOG_INFO("RadioManager", QString("KPA1500 operating status: %1")
             .arg(operateMode ? "Operate (100-1800W)" : "Standby (0-110W)"));

    if (operateMode) {
        emit maxPowerChanged(1800);
    } else if (m_radios[m_activeRadioIndex]) {
        int radioMaxPower = m_radios[m_activeRadioIndex]->maxPowerWatts();
        emit maxPowerChanged(radioMaxPower);
    }
}

void RadioManager::onAmplifierError(const QString& error)
{
    LOG_ERROR("RadioManager", QString("KPA1500 error: %1").arg(error));
    emit statusMessage(QString("Amplifier error: %1").arg(error));
}

} // namespace TR4QT
