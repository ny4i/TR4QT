#include "HamlibAmplifier.h"
#include "../logging/LogMacros.h"
#include <cstring>

namespace TR4QT {

HamlibAmplifier::HamlibAmplifier(QObject* parent)
    : IAmplifierController(parent)
    , m_pollTimer(new QTimer(this))
{
    QObject::connect(m_pollTimer, &QTimer::timeout, this, &HamlibAmplifier::pollAmplifier);
}

HamlibAmplifier::~HamlibAmplifier() {
    disconnect();
}

bool HamlibAmplifier::connect(const AmplifierConfig& config) {
    QMutexLocker locker(&m_ampMutex);

    // Disconnect if already connected
    if (m_amp) {
        amp_close(m_amp);
        amp_cleanup(m_amp);
        m_amp = nullptr;
    }

    m_config = config;

    // Initialize amplifier
    m_amp = amp_init(config.hamlibModelId);
    if (!m_amp) {
        LOG_ERROR("HamlibAmplifier", QString("Failed to initialize amplifier (invalid model ID: %1)").arg(config.hamlibModelId));
        emit errorOccurred(QString("Failed to initialize amplifier (invalid model ID: %1)").arg(config.hamlibModelId));
        return false;
    }

    // Configure port
    strncpy(m_amp->state.ampport.pathname,
            config.port.toStdString().c_str(),
            HAMLIB_FILPATHLEN - 1);

    // Configure serial port parameters for serial connections
    m_amp->state.ampport.parm.serial.rate = config.baudRate;
    m_amp->state.ampport.parm.serial.data_bits = config.dataBits;
    m_amp->state.ampport.parm.serial.stop_bits = config.stopBits;

    // Map parity string to Hamlib parity enum
    if (config.parity == "None") {
        m_amp->state.ampport.parm.serial.parity = RIG_PARITY_NONE;
    } else if (config.parity == "Odd") {
        m_amp->state.ampport.parm.serial.parity = RIG_PARITY_ODD;
    } else if (config.parity == "Even") {
        m_amp->state.ampport.parm.serial.parity = RIG_PARITY_EVEN;
    } else {
        m_amp->state.ampport.parm.serial.parity = RIG_PARITY_NONE;
    }

    // Map flow control string to Hamlib handshake enum
    if (config.flowControl == "None") {
        m_amp->state.ampport.parm.serial.handshake = RIG_HANDSHAKE_NONE;
    } else if (config.flowControl == "Hardware") {
        m_amp->state.ampport.parm.serial.handshake = RIG_HANDSHAKE_HARDWARE;
    } else if (config.flowControl == "Software") {
        m_amp->state.ampport.parm.serial.handshake = RIG_HANDSHAKE_XONXOFF;
    } else {
        m_amp->state.ampport.parm.serial.handshake = RIG_HANDSHAKE_NONE;
    }

    // Set network timeout
    m_amp->state.ampport.timeout = config.responseTimeoutMs;

    // Open connection
    int retcode = amp_open(m_amp);
    if (retcode != RIG_OK) {
        logHamlibError("amp_open", retcode);
        amp_cleanup(m_amp);
        m_amp = nullptr;
        emit connectionStatusChanged(false);
        return false;
    }

    m_connected = true;

    // Get amplifier model (mutex already locked, access directly)
    QString model = m_amp->caps->model_name;
    LOG_INFO("HamlibAmplifier", QString("Connected to amplifier: %1").arg(model));

    // Start polling timer (default 100ms for amplifier status monitoring)
    m_pollTimer->start(m_pollIntervalMs);
    LOG_DEBUG("HamlibAmplifier", QString("Poll timer started with interval %1 ms").arg(m_pollIntervalMs));

    // Reset error counter on successful connection
    m_consecutiveErrors = 0;

    emit connectionStatusChanged(true);
    LOG_DEBUG("HamlibAmplifier", "Connection successful");

    return true;
}

void HamlibAmplifier::disconnect() {
    QMutexLocker locker(&m_ampMutex);

    m_pollTimer->stop();

    if (m_amp) {
        amp_close(m_amp);
        amp_cleanup(m_amp);
        m_amp = nullptr;
    }

    m_connected = false;
    m_currentState = AmplifierState{};
    emit connectionStatusChanged(false);
}

bool HamlibAmplifier::isConnected() const {
    return m_connected;
}

AmplifierState HamlibAmplifier::getState() const {
    QMutexLocker locker(&m_ampMutex);
    return m_currentState;
}

void HamlibAmplifier::setFrequency(freq_t freq) {
    QMutexLocker locker(&m_ampMutex);
    if (!checkAmpPointer("setFrequency")) return;

    int retcode = amp_set_freq(m_amp, freq);
    if (retcode == RIG_OK) {
        m_currentState.frequency = freq;
        LOG_DEBUG("HamlibAmplifier", QString("Frequency set to %1 Hz for LPF tracking").arg(freq));
    } else {
        logHamlibError("amp_set_freq", retcode);
    }
}

void HamlibAmplifier::queryStatus() {
    // Trigger immediate poll (without waiting for timer)
    pollAmplifier();
}

void HamlibAmplifier::sendRawCommand(const QString& command) {
    // Hamlib doesn't support raw command passing for amplifiers
    // Commands must be translated to Hamlib API calls
    LOG_WARN("HamlibAmplifier", QString("Raw command sending not supported via Hamlib: %1").arg(command));
    LOG_INFO("HamlibAmplifier", "Hamlib amplifier commands must use API methods (setFrequency, etc.)");
    emit errorOccurred("Raw commands not supported via Hamlib (use direct connection)");
}

void HamlibAmplifier::pollAmplifier() {
    QMutexLocker locker(&m_ampMutex);
    if (!checkAmpPointer("pollAmplifier")) return;

    updateState();
}

void HamlibAmplifier::updateState() {
    // NOTE: Mutex must already be locked by caller!

    if (!m_amp) return;

    bool stateChanged = false;
    AmplifierState newState = m_currentState;
    newState.connected = true;
    newState.isValid = true;

    // Query forward power
    value_t fwdPower;
    int retcode = amp_get_level(m_amp, AMP_LEVEL_PWR_FWD, &fwdPower);
    if (retcode == RIG_OK) {
        int watts = static_cast<int>(fwdPower.f);  // Power in watts
        if (watts != newState.forwardPowerWatts) {
            newState.forwardPowerWatts = watts;
            stateChanged = true;
            emit forwardPowerChanged(watts);
        }
        m_consecutiveErrors = 0;  // Reset error counter on successful query
    } else {
        logHamlibError("amp_get_level(PWR_FWD)", retcode);
        m_consecutiveErrors++;
    }

    // Query reflected power
    value_t refPower;
    retcode = amp_get_level(m_amp, AMP_LEVEL_PWR_REFLECTED, &refPower);
    if (retcode == RIG_OK) {
        int watts = static_cast<int>(refPower.f);
        if (watts != newState.reflectedPowerWatts) {
            newState.reflectedPowerWatts = watts;
            stateChanged = true;
            emit reflectedPowerChanged(watts);
        }
    } else {
        logHamlibError("amp_get_level(PWR_REFLECTED)", retcode);
        m_consecutiveErrors++;
    }

    // Query SWR
    value_t swr;
    retcode = amp_get_level(m_amp, AMP_LEVEL_SWR, &swr);
    if (retcode == RIG_OK) {
        float swrValue = swr.f;
        if (qAbs(swrValue - newState.swr) > 0.01) {  // Threshold to avoid floating point noise
            newState.swr = swrValue;
            stateChanged = true;
            emit swrChanged(swrValue);
        }
    } else {
        logHamlibError("amp_get_level(SWR)", retcode);
        m_consecutiveErrors++;
    }

    // Query fault status
    value_t fault;
    retcode = amp_get_level(m_amp, AMP_LEVEL_FAULT, &fault);
    if (retcode == RIG_OK) {
        bool hasFault = (fault.i != 0);
        if (hasFault != newState.faultDetected) {
            newState.faultDetected = hasFault;
            newState.faultCode = hasFault ? QString("Fault code: %1").arg(fault.i) : QString();
            stateChanged = true;
            if (hasFault) {
                emit faultDetected(newState.faultCode);
            }
        }
    } else {
        // Fault level might not be supported by all amplifiers
        if (retcode != -RIG_ENIMPL && retcode != -RIG_ENAVAIL) {
            logHamlibError("amp_get_level(FAULT)", retcode);
        }
    }

    // Update cached state
    if (stateChanged) {
        m_currentState = newState;
        emit stateUpdated(m_currentState);
    }

    // Auto-disconnect on persistent errors
    if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
        LOG_ERROR("HamlibAmplifier", QString("Too many consecutive errors (%1), disconnecting amplifier").arg(m_consecutiveErrors));
        emit errorOccurred(QString("Lost connection to amplifier (too many errors)"));
        // Release mutex before calling disconnect() which will lock it again
        // Store flag to disconnect after mutex release
        m_connected = false;
        m_pollTimer->stop();
        emit connectionStatusChanged(false);
    }
}

void HamlibAmplifier::logHamlibError(const char* operation, int retcode) {
    QString errorStr = QString::fromUtf8(rigerror(retcode));
    LOG_ERROR("HamlibAmplifier", QString("%1 failed: %2 (code: %3)")
        .arg(operation)
        .arg(errorStr)
        .arg(retcode));
}

bool HamlibAmplifier::checkAmpPointer(const char* context) const {
    if (!m_amp) {
        LOG_ERROR("HamlibAmplifier", QString("%1: Amplifier not connected").arg(context));
        return false;
    }
    return true;
}

} // namespace TR4QT
