#include "HamlibRadio.h"
#include "../core/Constants.h"
#include "../logging/LogMacros.h"
#include "../utils/PerformanceProfiler.h"

namespace TR4QT {

HamlibRadio::HamlibRadio(QObject* parent)
    : RadioInterface(parent)
    , m_pollTimer(new QTimer(this))
{
    QObject::connect(m_pollTimer, &QTimer::timeout, this, &HamlibRadio::pollRadio);
}

HamlibRadio::~HamlibRadio() {
    disconnect();
}

// DEBUG: Test slot to verify signal/slot mechanism works
void HamlibRadio::debugTestSlot(int testValue)
{
    LOG_ERROR("HamlibRadio", QString("***** DEBUG TEST SLOT CALLED WITH VALUE: %1 *****").arg(testValue));
    LOG_ERROR("HamlibRadio", QString("***** This proves worker thread is processing slot calls *****"));
}

bool HamlibRadio::connect(const RadioConfig& config) {
    QMutexLocker locker(&m_rigMutex);

    // Disconnect if already connected
    if (m_rig) {
        rig_close(m_rig);
        rig_cleanup(m_rig);
        m_rig = nullptr;
    }

    m_config = config;

    // Initialize rig
    m_rig = rig_init(config.hamlibModelId);
    if (!m_rig) {
        // TODO: Enhancement - provide more detailed error message with model ID
        // Example: QString("Invalid radio model ID: %1. Check PreferencesDialog radio selection.").arg(config.hamlibModelId)
        emit errorOccurred("Failed to initialize radio (invalid model ID?)");
        return false;
    }

    // Configure port
    strncpy(m_rig->state.rigport.pathname,
            config.port.toStdString().c_str(),
            HAMLIB_FILPATHLEN - 1);

    // Configure serial port parameters for serial connections
    m_rig->state.rigport.parm.serial.rate = config.baudRate;
    m_rig->state.rigport.parm.serial.data_bits = config.dataBits;
    m_rig->state.rigport.parm.serial.stop_bits = config.stopBits;

    // Map parity index to Hamlib parity enum (0=None, 1=Odd, 2=Even)
    switch (config.parity) {
        case 0:  // None
            m_rig->state.rigport.parm.serial.parity = RIG_PARITY_NONE;
            break;
        case 1:  // Odd
            m_rig->state.rigport.parm.serial.parity = RIG_PARITY_ODD;
            break;
        case 2:  // Even
            m_rig->state.rigport.parm.serial.parity = RIG_PARITY_EVEN;
            break;
        default:
            m_rig->state.rigport.parm.serial.parity = RIG_PARITY_NONE;
            break;
    }

    // Configure CI-V address for Icom radios
    if (config.civAddress > 0) {
        char civAddr[16];
        snprintf(civAddr, sizeof(civAddr), "0x%02X", config.civAddress);
        rig_set_conf(m_rig, rig_token_lookup(m_rig, "civaddr"), civAddr);
    }

    // Disable hamlib's internal polling - we'll use Qt timer instead
    m_rig->state.poll_interval = 0;

    // Set network timeout to 1 second (instead of default 10+ seconds)
    // This allows faster shutdown if connection attempt happens during application exit
    m_rig->state.rigport.timeout = 1000;  // 1000ms = 1 second

    // Open connection
    int retcode = rig_open(m_rig);
    if (retcode != RIG_OK) {
        logHamlibError("rig_open", retcode);
        rig_cleanup(m_rig);
        m_rig = nullptr;
        emit connectionStatusChanged(false);
        return false;
    }

    m_connected = true;

    // Get radio model (mutex already locked, access directly)
    QString model = m_rig->caps->model_name;
    LOG_DEBUG("HamlibRadio", QString("HamlibRadio::connect: SUCCESS - Connected to radio: %1").arg(model));

    // Start polling timer
    LOG_DEBUG("HamlibRadio", QString("HamlibRadio::connect: Checking poll interval: %1").arg(config.pollInterval));
    if (config.pollInterval > 0) {
        LOG_DEBUG("HamlibRadio", "HamlibRadio::connect: About to start poll timer");
        m_pollTimer->start(config.pollInterval);
        LOG_DEBUG("HamlibRadio", QString("HamlibRadio::connect: Poll timer started with interval %1 ms").arg(config.pollInterval));
    } else {
        LOG_DEBUG("HamlibRadio", "HamlibRadio::connect: Poll interval is 0, not starting timer");
    }

    // Reset error counter on successful connection
    m_consecutiveErrors = 0;

    LOG_DEBUG("HamlibRadio", "HamlibRadio::connect: About to emit connectionStatusChanged(true)");
    emit connectionStatusChanged(true);
    LOG_DEBUG("HamlibRadio", "HamlibRadio::connect: Signal emitted");

    LOG_DEBUG("HamlibRadio", "HamlibRadio::connect: Returning true (initial state will come from poll timer)");
    // NOTE: Do NOT call updateState() here - mutex is already locked!
    // The poll timer will provide the first state update shortly.
    return true;
}

void HamlibRadio::disconnect() {
    QMutexLocker locker(&m_rigMutex);

    m_pollTimer->stop();

    if (m_rig) {
        rig_close(m_rig);
        rig_cleanup(m_rig);
        m_rig = nullptr;
    }

    m_connected = false;
    m_currentState = RadioState{};
    emit connectionStatusChanged(false);
}

bool HamlibRadio::isConnected() const {
    return m_connected;
}

bool HamlibRadio::setFrequency(freq_t freq, VFO vfo) {
    PROFILE_FUNCTION("Hamlib");
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setFrequency")) return false;

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_set_freq(m_rig, hamlib_vfo, freq);

    // TODO: Enhancement - retry logic for transient errors
    // Some radios return -RIG_ETIMEOUT when busy. Could retry 2-3 times with 50ms delay:
    // if (retcode == -RIG_ETIMEOUT) { QThread::msleep(50); retcode = rig_set_freq(...); }

    if (retcode == RIG_OK) {
        emit frequencyChanged(freq, vfo);
        return true;
    }

    logHamlibError("rig_set_freq", retcode);
    return false;
}

bool HamlibRadio::setBand(BandType band, VFO vfo) {
    // Hamlib doesn't have a native "set band" command
    // Convert band to frequency (band edge) and call setFrequency
    freq_t freq = bandToFrequency(band);
    if (freq == 0) {
        LOG_ERROR("HamlibRadio", QString("Invalid band: %1").arg(static_cast<int>(band)));
        return false;
    }

    LOG_DEBUG("HamlibRadio", QString("setBand: %1 -> %2 Hz").arg(bandToString(band)).arg(freq));
    return setFrequency(freq, vfo);
}

freq_t HamlibRadio::getFrequency(VFO vfo) const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getFrequency")) return 0;

    freq_t freq = 0;
    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_get_freq(m_rig, hamlib_vfo, &freq);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_freq", retcode);
        return 0;
    }

    return freq;
}

bool HamlibRadio::setMode(ModeType mode, VFO vfo) {
    PROFILE_FUNCTION("Hamlib");
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setMode")) return false;

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    rmode_t hamlib_mode = toHamlibMode(mode);
    pbwidth_t width = RIG_PASSBAND_NORMAL;  // Let radio choose default width

    int retcode = rig_set_mode(m_rig, hamlib_vfo, hamlib_mode, width);

    if (retcode == RIG_OK) {
        emit modeChanged(mode, vfo);
        return true;
    }

    logHamlibError("rig_set_mode", retcode);
    return false;
}

ModeType HamlibRadio::getMode(VFO vfo) const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getMode")) return ModeType::None;

    rmode_t mode = RIG_MODE_NONE;
    pbwidth_t width = 0;
    vfo_t hamlib_vfo = toHamlibVFO(vfo);

    int retcode = rig_get_mode(m_rig, hamlib_vfo, &mode, &width);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_mode", retcode);
        return ModeType::None;
    }

    return fromHamlibMode(mode);
}

bool HamlibRadio::setPTT(bool transmit) {
    PROFILE_FUNCTION("Hamlib");
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setPTT")) return false;

    ptt_t ptt = transmit ? RIG_PTT_ON : RIG_PTT_OFF;
    int retcode = rig_set_ptt(m_rig, RIG_VFO_CURR, ptt);

    if (retcode == RIG_OK) {
        emit pttChanged(transmit);
        return true;
    }

    logHamlibError("rig_set_ptt", retcode);
    return false;
}

bool HamlibRadio::getPTT() const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getPTT")) return false;

    ptt_t ptt = RIG_PTT_OFF;
    int retcode = rig_get_ptt(m_rig, RIG_VFO_CURR, &ptt);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_ptt", retcode);
        return false;
    }

    return (ptt == RIG_PTT_ON);
}

bool HamlibRadio::sendCW(const QString& text) {
    PROFILE_FUNCTION("Hamlib");
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("sendCW")) return false;

    LOG_DEBUG("HamlibRadio", QString("Sending CW: '%1'").arg(text));
    LOG_TRACE("HamlibRadio", QString("rig_send_morse called with text: '%1'").arg(text));

    int retcode = rig_send_morse(m_rig, RIG_VFO_CURR, text.toStdString().c_str());

    if (retcode == RIG_OK) {
        LOG_TRACE("HamlibRadio", "Radio started sending CW");
        LOG_DEBUG("HamlibRadio", QString("CW transmission started for '%1'").arg(text));
        return true;
    }

    logHamlibError("rig_send_morse", retcode);
    return false;
}

bool HamlibRadio::setCWSpeed(int wpm) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setCWSpeed")) return false;

    LOG_DEBUG("HamlibRadio", QString("Setting CW speed to %1 WPM").arg(wpm));
    LOG_TRACE("HamlibRadio", QString("rig_set_level(RIG_LEVEL_KEYSPD, %1)").arg(wpm));

    // Set CW speed via level parameter
    value_t val;
    val.i = wpm;
    int retcode = rig_set_level(m_rig, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, val);

    if (retcode == RIG_OK) {
        LOG_DEBUG("HamlibRadio", QString("CW speed set to %1 WPM successfully").arg(wpm));
        return true;
    }

    logHamlibError("setCWSpeed", retcode);
    return false;
}

int HamlibRadio::getCWSpeed() const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getCWSpeed")) return 0;

    LOG_TRACE("HamlibRadio", "rig_get_level(RIG_LEVEL_KEYSPD)");

    value_t val;
    int retcode = rig_get_level(m_rig, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, &val);

    if (retcode == RIG_OK) {
        int wpm = val.i;
        LOG_DEBUG("HamlibRadio", QString("Got CW speed from radio: %1 WPM").arg(wpm));
        return wpm;
    }

    logHamlibError("getCWSpeed", retcode);
    return 0;  // Return 0 on error
}

bool HamlibRadio::stopCW() {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("stopCW")) return false;

    LOG_DEBUG("HamlibRadio", "Stopping CW transmission");
    LOG_TRACE("HamlibRadio", "rig_stop_morse called");

    int retcode = rig_stop_morse(m_rig, RIG_VFO_CURR);

    if (retcode == RIG_OK) {
        LOG_TRACE("HamlibRadio", "Radio stopped sending CW");
        LOG_DEBUG("HamlibRadio", "CW transmission stopped successfully");
        return true;
    }

    logHamlibError("rig_stop_morse", retcode);
    return false;
}

bool HamlibRadio::waitForMorseComplete() {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("waitForMorseComplete")) return false;

    LOG_DEBUG("HamlibRadio", "Waiting for CW transmission to complete");
    LOG_TRACE("HamlibRadio", "rig_wait_morse called");

    int retcode = rig_wait_morse(m_rig, RIG_VFO_CURR);

    if (retcode == RIG_OK) {
        LOG_DEBUG("HamlibRadio", "CW transmission complete");
        return true;
    }

    // -1 typically means the function is not implemented for this rig
    if (retcode == -RIG_ENAVAIL || retcode == -RIG_ENIMPL) {
        LOG_DEBUG("HamlibRadio", "rig_wait_morse not supported by this rig");
        return false;
    }

    logHamlibError("rig_wait_morse", retcode);
    return false;
}

bool HamlibRadio::setRIT(int offset_hz, VFO vfo) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setRIT")) return false;

    // TODO: Enhancement - pre-check radio capabilities before operation
    // Example: if (!(m_rig->caps->has_set_func & RIG_FUNC_RIT)) {
    //     LOG_WARN("HamlibRadio", "Radio does not support RIT");
    //     return false;
    // }

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_set_rit(m_rig, hamlib_vfo, offset_hz);

    if (retcode == RIG_OK) {
        emit ritChanged(offset_hz, vfo);
        return true;
    }

    logHamlibError("rig_set_rit", retcode);
    return false;
}

bool HamlibRadio::setXIT(int offset_hz, VFO vfo) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setXIT")) return false;

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_set_xit(m_rig, hamlib_vfo, offset_hz);

    if (retcode == RIG_OK) {
        emit xitChanged(offset_hz, vfo);
        return true;
    }

    logHamlibError("rig_set_xit", retcode);
    return false;
}

int HamlibRadio::getRIT(VFO vfo) const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getRIT")) return 0;

    shortfreq_t rit = 0;
    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_get_rit(m_rig, hamlib_vfo, &rit);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_rit", retcode);
        return 0;
    }

    return rit;
}

int HamlibRadio::getXIT(VFO vfo) const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getXIT")) return 0;

    shortfreq_t xit = 0;
    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_get_xit(m_rig, hamlib_vfo, &xit);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_xit", retcode);
        return 0;
    }

    return xit;
}

bool HamlibRadio::clearRIT(VFO vfo) {
    return setRIT(0, vfo);
}

bool HamlibRadio::clearXIT(VFO vfo) {
    return setXIT(0, vfo);
}

bool HamlibRadio::enableRIT(bool enable, VFO vfo) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("enableRIT")) return false;

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_set_func(m_rig, hamlib_vfo, RIG_FUNC_RIT, enable ? 1 : 0);

    if (retcode == RIG_OK) {
        LOG_INFO("HamlibRadio", QString("RIT %1").arg(enable ? "enabled" : "disabled"));
        return true;
    }

    logHamlibError("rig_set_func(RIG_FUNC_RIT)", retcode);
    return false;
}

bool HamlibRadio::enableXIT(bool enable, VFO vfo) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("enableXIT")) return false;

    vfo_t hamlib_vfo = toHamlibVFO(vfo);
    int retcode = rig_set_func(m_rig, hamlib_vfo, RIG_FUNC_XIT, enable ? 1 : 0);

    if (retcode == RIG_OK) {
        LOG_INFO("HamlibRadio", QString("XIT %1").arg(enable ? "enabled" : "disabled"));
        return true;
    }

    logHamlibError("rig_set_func(RIG_FUNC_XIT)", retcode);
    return false;
}

bool HamlibRadio::setSplit(bool enable, VFO txVfo) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setSplit")) return false;

    split_t split = enable ? RIG_SPLIT_ON : RIG_SPLIT_OFF;
    vfo_t tx_vfo = toHamlibVFO(txVfo);

    int retcode = rig_set_split_vfo(m_rig, RIG_VFO_CURR, split, tx_vfo);

    if (retcode == RIG_OK) {
        emit splitChanged(enable);
        return true;
    }

    logHamlibError("rig_set_split_vfo", retcode);
    return false;
}

bool HamlibRadio::getSplit() const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getSplit")) return false;

    split_t split = RIG_SPLIT_OFF;
    vfo_t tx_vfo = RIG_VFO_NONE;

    int retcode = rig_get_split_vfo(m_rig, RIG_VFO_CURR, &split, &tx_vfo);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_split_vfo", retcode);
        return false;
    }

    return (split == RIG_SPLIT_ON);
}

bool HamlibRadio::vfoBumpUp(VFO vfo) {
    // Get current frequency, add typical tuning step, set new frequency
    freq_t current = getFrequency(vfo);
    if (current == 0) return false;

    // Determine step size based on band/mode (simplified)
    freq_t step = 100;  // 100 Hz default for CW
    ModeType mode = getMode(vfo);
    if (mode == ModeType::LSB || mode == ModeType::USB) {
        step = 1000;  // 1 kHz for SSB
    }

    return setFrequency(current + step, vfo);
}

bool HamlibRadio::vfoBumpDown(VFO vfo) {
    freq_t current = getFrequency(vfo);
    if (current == 0) return false;

    freq_t step = 100;  // 100 Hz default for CW
    ModeType mode = getMode(vfo);
    if (mode == ModeType::LSB || mode == ModeType::USB) {
        step = 1000;  // 1 kHz for SSB
    }

    return setFrequency(current - step, vfo);
}

bool HamlibRadio::setFilterWidth(int width_hz) {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("setFilterWidth")) return false;

    // Get current mode first
    rmode_t mode = RIG_MODE_NONE;
    pbwidth_t current_width = 0;
    int retcode = rig_get_mode(m_rig, RIG_VFO_CURR, &mode, &current_width);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_mode (for filter)", retcode);
        return false;
    }

    // Set mode with new filter width
    retcode = rig_set_mode(m_rig, RIG_VFO_CURR, mode, width_hz);

    if (retcode == RIG_OK) {
        return true;
    }

    logHamlibError("rig_set_mode (filter width)", retcode);
    return false;
}

int HamlibRadio::getFilterWidth() const {
    QMutexLocker locker(&m_rigMutex);
    if (!checkRigPointer("getFilterWidth")) return 0;

    rmode_t mode = RIG_MODE_NONE;
    pbwidth_t width = 0;
    int retcode = rig_get_mode(m_rig, RIG_VFO_CURR, &mode, &width);

    if (retcode != RIG_OK) {
        logHamlibError("rig_get_mode (for filter)", retcode);
        return 0;
    }

    return width;
}

RadioState HamlibRadio::getCurrentState() const {
    QMutexLocker locker(&m_rigMutex);
    return m_currentState;
}

QString HamlibRadio::getRadioModel() const {
    QMutexLocker locker(&m_rigMutex);
    if (!m_rig) return "Not connected";

    return QString::fromStdString(m_rig->caps->model_name);
}

QString HamlibRadio::getRadioVersion() const {
    QMutexLocker locker(&m_rigMutex);
    if (!m_rig) return "";

    return QString::fromStdString(m_rig->caps->version);
}

QList<ModeType> HamlibRadio::getSupportedModes() const {
    QMutexLocker locker(&m_rigMutex);
    QList<ModeType> modes;

    if (!m_rig || !m_rig->caps) {
        LOG_DEBUG("HamlibRadio", "getSupportedModes: No rig or caps");
        return modes;
    }

    // Hamlib stores modes in the rx_range_list array as bitmasks
    // We need to OR together all the modes from all frequency ranges
    rmode_t supported = 0;

    // rx_range_list1 is for VFO A (rx_range_list2 is for VFO B)
    const struct freq_range_list *range_list = m_rig->caps->rx_range_list1;

    if (!range_list) {
        LOG_DEBUG("HamlibRadio", "getSupportedModes: No rx_range_list1");
        return modes;
    }

    // Iterate through all frequency ranges and collect all supported modes
    for (int i = 0; i < HAMLIB_FRQRANGESIZ && range_list[i].startf != 0; i++) {
        supported |= range_list[i].modes;
    }

    LOG_DEBUG("HamlibRadio", QString("getSupportedModes: supported bitmask = 0x%1").arg(supported, 0, 16));

    // Check each mode we support and add if the radio supports it
    if (supported & RIG_MODE_CW)      modes.append(ModeType::CW);
    if (supported & RIG_MODE_CWR)     modes.append(ModeType::CWR);
    if (supported & RIG_MODE_USB)     modes.append(ModeType::USB);
    if (supported & RIG_MODE_LSB)     modes.append(ModeType::LSB);
    if (supported & RIG_MODE_FM)      modes.append(ModeType::FM);
    if (supported & RIG_MODE_AM)      modes.append(ModeType::AM);
    if (supported & RIG_MODE_RTTY)    modes.append(ModeType::RTTY);
    if (supported & RIG_MODE_RTTYR)   modes.append(ModeType::RTTYR);
    if (supported & RIG_MODE_PKTUSB)  modes.append(ModeType::DATA);
    if (supported & RIG_MODE_PKTLSB)  modes.append(ModeType::DATAR);

    LOG_DEBUG("HamlibRadio", QString("getSupportedModes: returning %1 modes").arg(modes.size()));

    return modes;
}

bool HamlibRadio::supportsCWSending() const {
    QMutexLocker locker(&m_rigMutex);

    // Check if radio has send_morse capability
    if (!m_rig || !m_rig->caps) {
        return false;
    }

    // Hamlib radios with CW sending support have non-NULL send_morse function pointer
    bool hasCapability = (m_rig->caps->send_morse != nullptr);

    LOG_DEBUG("HamlibRadio", QString("supportsCWSending: %1 (model: %2)")
        .arg(hasCapability ? "YES" : "NO")
        .arg(m_rig->caps->model_name));

    return hasCapability;
}

void HamlibRadio::pollRadio() {
    if (!isConnected()) return;

    RadioState newState = pollCurrentState();

    // Check if state is valid (no I/O errors during polling)
    // pollCurrentState() calls handlePollError() on I/O errors which tracks consecutive errors
    if (!newState.isValid) {
        // State polling failed - handlePollError() already tracked the error
        // Don't emit state changes to avoid stale data
        return;
    }

    // Poll succeeded - reset error counter
    if (m_consecutiveErrors > 0) {
        LOG_DEBUG("HamlibRadio", QString("Polling recovered after %1 errors").arg(m_consecutiveErrors));
        m_consecutiveErrors = 0;
    }

    // Emit individual change signals if values changed
    if (newState.frequencyA != m_currentState.frequencyA) {
        emit frequencyChanged(newState.frequencyA, VFO::VFO_A);
    }
    if (newState.frequencyB != m_currentState.frequencyB) {
        emit frequencyChanged(newState.frequencyB, VFO::VFO_B);
    }
    if (newState.modeA != m_currentState.modeA) {
        emit modeChanged(newState.modeA, VFO::VFO_A);
    }
    if (newState.modeB != m_currentState.modeB) {
        emit modeChanged(newState.modeB, VFO::VFO_B);
    }
    if (newState.isTransmitting != m_currentState.isTransmitting) {
        emit pttChanged(newState.isTransmitting);
    }

    m_currentState = newState;
    emit stateUpdated(m_currentState);
}

RadioState HamlibRadio::pollCurrentState() {
    RadioState state;

    QMutexLocker locker(&m_rigMutex);
    if (!m_rig) {
        return state;
    }

    // Get radio model name
    state.radioModel = QString::fromStdString(m_rig->caps->model_name);

    // Poll VFO A frequency - use this as the critical "health check" call
    // If this fails with I/O error, the radio is likely disconnected
    freq_t freqA = 0;
    int ret = rig_get_freq(m_rig, RIG_VFO_A, &freqA);
    if (ret == RIG_OK) {
        state.frequencyA = freqA;
        state.bandA = frequencyToBand(freqA);
    } else {
        // Check if this is a communication error (radio likely disconnected)
        // RIG_EIO: IO error including open failed (e.g., "Broken pipe" when radio turned off)
        // RIG_ETIMEOUT: Communication timeout
        // RIG_EPROTO: Protocol error
        // RIG_BUSERROR: Error talking on the bus
        if (ret == -RIG_EIO || ret == -RIG_ETIMEOUT || ret == -RIG_EPROTO || ret == -RIG_BUSERROR) {
            LOG_WARN("HamlibRadio", QString("Communication error during poll: %1 (%2)")
                .arg(rigerror(ret)).arg(ret));
            handlePollError(ret);
            return state;  // Return invalid state (isValid = false)
        }
        // For other errors, continue polling but log the issue
        LOG_WARN("HamlibRadio", QString("Error getting frequency: %1 (%2)")
            .arg(rigerror(ret)).arg(ret));
    }

    rmode_t modeA = RIG_MODE_NONE;
    pbwidth_t widthA = 0;
    if (rig_get_mode(m_rig, RIG_VFO_A, &modeA, &widthA) == RIG_OK) {
        state.modeA = fromHamlibMode(modeA);
        state.filterWidth = widthA;
    }

    // Poll VFO B frequency and mode
    freq_t freqB = 0;
    if (rig_get_freq(m_rig, RIG_VFO_B, &freqB) == RIG_OK) {
        state.frequencyB = freqB;
        state.bandB = frequencyToBand(freqB);
    }

    rmode_t modeB = RIG_MODE_NONE;
    pbwidth_t widthB = 0;
    if (rig_get_mode(m_rig, RIG_VFO_B, &modeB, &widthB) == RIG_OK) {
        state.modeB = fromHamlibMode(modeB);
    }

    // Poll PTT status
    ptt_t ptt = RIG_PTT_OFF;
    if (rig_get_ptt(m_rig, RIG_VFO_CURR, &ptt) == RIG_OK) {
        state.isTransmitting = (ptt == RIG_PTT_ON);
    }

    // Poll RIT/XIT offsets
    shortfreq_t rit = 0;
    if (rig_get_rit(m_rig, RIG_VFO_A, &rit) == RIG_OK) {
        state.ritOffsetA = rit;
    }

    shortfreq_t xit = 0;
    if (rig_get_xit(m_rig, RIG_VFO_A, &xit) == RIG_OK) {
        state.xitOffsetA = xit;
    }

    // Poll RIT/XIT enable status (independent of offset value)
    int ritEnabled = 0;
    if (rig_get_func(m_rig, RIG_VFO_A, RIG_FUNC_RIT, &ritEnabled) == RIG_OK) {
        state.isRitEnabled = (ritEnabled != 0);
    }

    int xitEnabled = 0;
    if (rig_get_func(m_rig, RIG_VFO_A, RIG_FUNC_XIT, &xitEnabled) == RIG_OK) {
        state.isXitEnabled = (xitEnabled != 0);
    }

    // Poll split status
    split_t split = RIG_SPLIT_OFF;
    vfo_t tx_vfo = RIG_VFO_NONE;
    if (rig_get_split_vfo(m_rig, RIG_VFO_CURR, &split, &tx_vfo) == RIG_OK) {
        state.isSplitEnabled = (split == RIG_SPLIT_ON);
    }

    // Poll signal strength (S-meter reading)
    int strength = -54;  // Default to S9 if unavailable
    if (rig_get_strength(m_rig, RIG_VFO_CURR, &strength) == RIG_OK) {
        state.signalStrength = strength;  // Value in dBm
        LOG_TRACE("HamlibRadio", QString("Signal strength from hamlib: %1").arg(strength));
    }

    // Poll CW speed (only when in CW mode to avoid unnecessary hamlib calls)
    if (state.modeA == ModeType::CW || state.modeA == ModeType::CWR) {
        value_t val;
        if (rig_get_level(m_rig, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, &val) == RIG_OK) {
            state.cwSpeed = val.i;
            LOG_TRACE("HamlibRadio", QString("Polled CW speed: %1 WPM").arg(state.cwSpeed));
        }
    }

    state.isValid = true;
    return state;
}

void HamlibRadio::updateState() {
    if (!isConnected()) return;
    m_currentState = pollCurrentState();
    emit stateUpdated(m_currentState);
}

// Conversion helpers

freq_t HamlibRadio::bandToFrequency(BandType band) const {
    // Convert band to frequency (band edge)
    // Returns frequency in Hz
    switch (band) {
        case BandType::Band160M: return 1800000;    // 1.800 MHz
        case BandType::Band80M:  return 3500000;    // 3.500 MHz
        case BandType::Band60M:  return 5330500;    // 5.3305 MHz (channel 1)
        case BandType::Band40M:  return 7000000;    // 7.000 MHz
        case BandType::Band30M:  return 10100000;   // 10.100 MHz
        case BandType::Band20M:  return 14000000;   // 14.000 MHz
        case BandType::Band17M:  return 18068000;   // 18.068 MHz
        case BandType::Band15M:  return 21000000;   // 21.000 MHz
        case BandType::Band12M:  return 24890000;   // 24.890 MHz
        case BandType::Band10M:  return 28000000;   // 28.000 MHz
        case BandType::Band6M:   return 50000000;   // 50.000 MHz
        case BandType::Band2M:   return 144000000;  // 144.000 MHz
        case BandType::Band70CM: return 420000000;  // 420.000 MHz
        default:
            LOG_WARN("HamlibRadio", QString("Unknown band: %1").arg(static_cast<int>(band)));
            return 0;
    }
}

vfo_t HamlibRadio::toHamlibVFO(VFO vfo) const {
    switch (vfo) {
        case VFO::VFO_A: return RIG_VFO_A;
        case VFO::VFO_B: return RIG_VFO_B;
        default: return RIG_VFO_CURR;
    }
}

VFO HamlibRadio::fromHamlibVFO(vfo_t vfo) const {
    switch (vfo) {
        case RIG_VFO_A: return VFO::VFO_A;
        case RIG_VFO_B: return VFO::VFO_B;
        default: return VFO::VFO_A;
    }
}

rmode_t HamlibRadio::toHamlibMode(ModeType mode) const {
    switch (mode) {
        case ModeType::CW:    return RIG_MODE_CW;
        case ModeType::CWR:   return RIG_MODE_CWR;
        case ModeType::LSB:   return RIG_MODE_LSB;
        case ModeType::USB:   return RIG_MODE_USB;
        case ModeType::FM:    return RIG_MODE_FM;
        case ModeType::AM:    return RIG_MODE_AM;
        case ModeType::RTTY:  return RIG_MODE_RTTY;
        case ModeType::RTTYR: return RIG_MODE_RTTYR;
        case ModeType::PSK:   return RIG_MODE_PKTUSB;  // PSK typically uses USB packet
        case ModeType::PSKR:  return RIG_MODE_PKTLSB;
        case ModeType::FT8:   return RIG_MODE_PKTUSB;  // FT8 uses USB data
        case ModeType::FT4:   return RIG_MODE_PKTUSB;
        case ModeType::DATA:  return RIG_MODE_PKTUSB;
        case ModeType::DATAR: return RIG_MODE_PKTLSB;
        default:              return RIG_MODE_NONE;
    }
}

ModeType HamlibRadio::fromHamlibMode(rmode_t mode) const {
    switch (mode) {
        case RIG_MODE_CW:      return ModeType::CW;
        case RIG_MODE_CWR:     return ModeType::CWR;
        case RIG_MODE_LSB:     return ModeType::LSB;
        case RIG_MODE_USB:     return ModeType::USB;
        case RIG_MODE_FM:      return ModeType::FM;
        case RIG_MODE_AM:      return ModeType::AM;
        case RIG_MODE_RTTY:    return ModeType::RTTY;
        case RIG_MODE_RTTYR:   return ModeType::RTTYR;
        case RIG_MODE_PKTUSB:  return ModeType::DATA;  // Generic data mode
        case RIG_MODE_PKTLSB:  return ModeType::DATAR;
        default:               return ModeType::None;
    }
}

BandType HamlibRadio::frequencyToBand(freq_t freq) const {
    // Convert frequency to band (based on common ham radio band edges)
    if (freq >= 1800000 && freq <= 2000000)        return BandType::Band160M;
    if (freq >= 3500000 && freq <= 4000000)        return BandType::Band80M;
    if (freq >= 5330500 && freq <= 5406500)        return BandType::Band60M;
    if (freq >= 7000000 && freq <= 7300000)        return BandType::Band40M;
    if (freq >= 10100000 && freq <= 10150000)      return BandType::Band30M;
    if (freq >= 14000000 && freq <= 14350000)      return BandType::Band20M;
    if (freq >= 18068000 && freq <= 18168000)      return BandType::Band17M;
    if (freq >= 21000000 && freq <= 21450000)      return BandType::Band15M;
    if (freq >= 24890000 && freq <= 24990000)      return BandType::Band12M;
    if (freq >= 28000000 && freq <= 29700000)      return BandType::Band10M;
    if (freq >= 50000000 && freq <= 54000000)      return BandType::Band6M;
    if (freq >= 70000000 && freq <= 71000000)      return BandType::Band4M;
    if (freq >= 144000000 && freq <= 148000000)    return BandType::Band2M;
    if (freq >= 420000000 && freq <= 450000000)    return BandType::Band70CM;

    return BandType::None;
}

void HamlibRadio::logHamlibError(const QString& operation, int retcode) const {
    QString errorMsg = QString("%1 failed: %2")
                           .arg(operation)
                           .arg(rigerror(retcode));
    LOG_WARN("HamlibRadio", errorMsg);
    const_cast<HamlibRadio*>(this)->errorOccurred(errorMsg);
}

bool HamlibRadio::checkRigPointer(const QString& operation) const {
    if (!m_rig) {
        QString errorMsg = operation + ": Radio not connected";
        LOG_WARN("HamlibRadio", errorMsg);
        const_cast<HamlibRadio*>(this)->errorOccurred(errorMsg);
        return false;
    }
    return true;
}

void HamlibRadio::handlePollError(int retcode) {
    m_consecutiveErrors++;

    if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
        LOG_ERROR("HamlibRadio", QString("Radio communication lost after %1 consecutive errors (error: %2 - %3). Disconnecting.")
            .arg(m_consecutiveErrors)
            .arg(retcode)
            .arg(rigerror(retcode)));

        // Stop polling to avoid continuous error spam
        m_pollTimer->stop();

        // Mark as disconnected
        m_connected = false;
        m_currentState = RadioState{};

        // Emit signal to update UI
        emit connectionStatusChanged(false);

        // Reset error counter
        m_consecutiveErrors = 0;
    } else {
        LOG_WARN("HamlibRadio", QString("Radio communication error %1/%2 (error: %3 - %4)")
            .arg(m_consecutiveErrors)
            .arg(MAX_CONSECUTIVE_ERRORS)
            .arg(retcode)
            .arg(rigerror(retcode)));
    }
}

} // namespace TR4QT
