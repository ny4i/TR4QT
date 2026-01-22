#include "UdpBroadcastManager.h"
#include "RadioInfo.h"
#include "ContactInfo.h"
#include "../core/Types.h"
#include "../logging/LogMacros.h"
#include <QHostInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace TR4QT {

// Convert BandType to N1MM+ compatible band string (frequency in MHz)
// N1MM+ uses frequency values like "1.8", "3.5", "7", "14", "21", "28", etc.
static QString bandToUdpBandString(BandType band)
{
    switch (band) {
    case BandType::Band160M: return "1.8";
    case BandType::Band80M:  return "3.5";
    case BandType::Band60M:  return "5";
    case BandType::Band40M:  return "7";
    case BandType::Band30M:  return "10";
    case BandType::Band20M:  return "14";
    case BandType::Band17M:  return "18";
    case BandType::Band15M:  return "21";
    case BandType::Band12M:  return "24";
    case BandType::Band10M:  return "28";
    case BandType::Band6M:   return "50";
    case BandType::Band4M:   return "70";
    case BandType::Band2M:   return "144";
    case BandType::Band1_25M: return "222";
    case BandType::Band70CM: return "420";
    case BandType::Band23CM: return "1.2G";
    default: return "14";  // Default to 20m
    }
}

// Get computer name for StationName field in UDP broadcasts
// Windows: NetBIOS computer name
// macOS/Linux: hostname without domain suffix (e.g., "macstudio" not "macstudio.lan")
static QString getComputerName()
{
#ifdef Q_OS_WIN
    // On Windows, use GetComputerName for NetBIOS name
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(buffer, &size)) {
        return QString::fromWCharArray(buffer);
    }
    // Fallback to Qt method
#endif

    // Get hostname and strip domain suffix
    QString hostname = QHostInfo::localHostName();
    int dotIndex = hostname.indexOf('.');
    if (dotIndex > 0) {
        hostname = hostname.left(dotIndex);
    }
    return hostname;
}

UdpBroadcastManager::UdpBroadcastManager(QObject* parent)
    : QObject(parent)
    , m_broadcaster(new UdpBroadcaster(this))
    , m_throttleTimer(new QTimer(this))
{
    // Configure throttle timer as single-shot
    m_throttleTimer->setSingleShot(true);
    connect(m_throttleTimer, &QTimer::timeout,
            this, &UdpBroadcastManager::onThrottleTimeout);

    // Forward error signals from broadcaster
    connect(m_broadcaster, &UdpBroadcaster::sendError,
            this, [this](const QString& destination, const QString& error) {
        QString fullError = QString("UDP send error to %1: %2").arg(destination, error);
        emit errorOccurred(fullError);
    });

    // Forward success signals
    connect(m_broadcaster, &UdpBroadcaster::messageSent,
            this, [this](const QByteArray& data, int destinationCount, qint64 bytesSent) {
        Q_UNUSED(data);
        Q_UNUSED(bytesSent);
        // Determine message type from XML content
        QString messageType = "Unknown";
        if (data.contains("<RadioInfo>")) {
            messageType = "RadioInfo";
        } else if (data.contains("<contactinfo>")) {
            messageType = "contactinfo";
        }
        emit messageSent(messageType, destinationCount);
    });
}

UdpBroadcastManager::~UdpBroadcastManager()
{
}

void UdpBroadcastManager::setEnabled(bool enabled)
{
    m_enabled = enabled;

    // Cancel pending throttled messages if disabled
    if (!enabled && m_throttleTimer->isActive()) {
        m_throttleTimer->stop();
        m_hasPendingRadioState = false;
    }
}

void UdpBroadcastManager::setRadioInfoEnabled(bool enabled)
{
    m_radioInfoEnabled = enabled;
}

void UdpBroadcastManager::setContactInfoEnabled(bool enabled)
{
    m_contactInfoEnabled = enabled;
}

void UdpBroadcastManager::setThrottleInterval(int milliseconds)
{
    m_throttleInterval = milliseconds;
}

void UdpBroadcastManager::setDestinations(const QList<UdpDestination>& destinations)
{
    m_broadcaster->setDestinations(destinations);
}

void UdpBroadcastManager::setOperatingMode(bool isRunMode)
{
    m_isRunMode = isRunMode;
}

void UdpBroadcastManager::onRadioStateChanged(const RadioState& state,
                                              const QString& stationCall)
{
    // Check if broadcasting is enabled
    if (!m_enabled || !m_radioInfoEnabled) {
        return;
    }

    // Store pending state
    m_pendingRadioState = state;
    m_pendingStationCall = stationCall;
    m_hasPendingRadioState = true;

    // If timer not already running, start it
    if (!m_throttleTimer->isActive()) {
        m_throttleTimer->start(m_throttleInterval);
    }
    // Otherwise, just update the pending state (debounce)
}

void UdpBroadcastManager::onQSOLogged(const QSO& qso, const QString& stationCall,
                                     const QString& adifContestId, int wa7bnmContestId)
{
    LOG_DEBUG("UdpBroadcastManager", QString("onQSOLogged called: enabled=%1 contactInfoEnabled=%2 callsign=%3")
              .arg(m_enabled)
              .arg(m_contactInfoEnabled)
              .arg(qso.callsign));

    // Check if broadcasting is enabled
    if (!m_enabled || !m_contactInfoEnabled) {
        LOG_DEBUG("UdpBroadcastManager", "UDP broadcast skipped (disabled)");
        return;
    }

    // Send immediately (no throttling for QSO logging)
    ContactInfo info = createContactInfo(qso, stationCall, adifContestId, wa7bnmContestId);
    LOG_DEBUG("UdpBroadcastManager", QString("Sending ContactInfo UDP broadcast for %1").arg(qso.callsign));
    m_broadcaster->sendContactInfo(info);
}

void UdpBroadcastManager::onThrottleTimeout()
{
    if (!m_hasPendingRadioState) {
        return;
    }

    // Check if state has actually changed
    if (!hasRadioStateChanged(m_pendingRadioState, m_lastSentRadioState)) {
        m_hasPendingRadioState = false;
        return;
    }

    // Send RadioInfo message
    RadioInfo info = createRadioInfo(m_pendingRadioState, m_pendingStationCall);
    if (m_broadcaster->sendRadioInfo(info)) {
        m_lastSentRadioState = m_pendingRadioState;
    }

    m_hasPendingRadioState = false;
}

RadioInfo UdpBroadcastManager::createRadioInfo(const RadioState& state,
                                               const QString& stationCall)
{
    RadioInfo info;

    // Application identity
    info.app = "TR4QT";
    info.stationName = getComputerName();

    // Radio identification
    info.radioNr = 1;  // For now, single radio only (TODO: SO2R support)
    info.radioName = state.radioModel;

    // Frequencies (convert Hz to tens of Hz)
    info.freq = RadioInfo::hzToTensOfHz(state.frequencyA);
    info.txFreq = state.isSplitEnabled ?
                 RadioInfo::hzToTensOfHz(state.frequencyB) :
                 RadioInfo::hzToTensOfHz(state.frequencyA);

    // Operating parameters
    info.mode = getModeString(state.modeA);
    info.mycall = stationCall;
    info.opCall = stationCall;  // Could be different in multi-op contests

    // Status flags
    info.isRunning = true;  // Assume contest is running if broadcasting
    info.isTransmitting = state.isTransmitting;
    info.isSplit = state.isSplitEnabled;
    info.isStereo = false;  // Not implemented yet (SO2R)
    info.isConnected = state.isValid;
    info.isRunMode = m_isRunMode;  // CQ/Run mode vs S&P mode

    // UI state (mostly unused, set defaults)
    info.focusEntry = 0;
    info.entryWindowHwnd = 0;
    info.focusRadioNr = 1;
    info.activeRadioNr = 1;
    info.functionKeyCaption = "";

    // Antenna/Rotor (not implemented yet)
    info.antenna = 0;
    info.rotors = "";
    info.auxAntSelected = -1;
    info.auxAntSelectedName = "";

    return info;
}

ContactInfo UdpBroadcastManager::createContactInfo(const QSO& qso,
                                                   const QString& stationCall,
                                                   const QString& adifContestId,
                                                   int wa7bnmContestId)
{
    ContactInfo info;

    // Application identity
    info.app = "TR4QT";
    info.contestName = adifContestId;      // ADIF Contest-ID (e.g., "CQ-WW-CW")
    info.contestNr = wa7bnmContestId;       // WA7BNM Contest Calendar ID
    info.stationName = getComputerName();

    // Timestamp (N1MM+ format: "YYYY-MM-DD HH:MM:SS")
    info.timestamp = qso.timestamp.toUTC().toString("yyyy-MM-dd HH:mm:ss");

    // Station identification
    info.mycall = stationCall;
    info.call = qso.callsign;

    // Frequency and mode
    // For simplex QSOs, rxfreq and txfreq are the same
    // TODO: Support split operation if QSO has separate TX frequency
    info.rxfreq = RadioInfo::hzToTensOfHz(qso.frequency);
    info.txfreq = RadioInfo::hzToTensOfHz(qso.frequency);
    info.freq = info.rxfreq;  // Legacy compatibility
    info.band = bandToUdpBandString(qso.band);
    info.mode = getModeString(qso.mode);

    // Exchange
    info.rstSent = qso.rstSent;
    info.rstRcvd = qso.rstReceived;
    info.exchangeSent = qso.exchangeSent;
    info.exchangeRcvd = qso.exchangeReceived;

    // DXCC/Geographic info
    info.dxccPrefix = qso.dxccPrefix;
    info.continent = qso.continent;
    info.cqZone = qso.cqZone;
    info.ituZone = qso.ituZone;
    info.state = qso.state;

    // Contest scoring
    info.points = qso.qsoPoints;
    info.isDupe = qso.isDupe;
    info.isMultiplier = qso.isMultiplier;

    // Station info
    info.radioNr = 1;  // TODO: SO2R support

    // Operator
    info.operator_ = qso.operatorCall;

    // Serial number
    info.serialNumber = qso.serialNumber;
    // Note: serialNumberRcvd not currently in QSO struct

    // Unique identifier (GUID without hyphens)
    if (!qso.guid.isEmpty()) {
        info.id = QString(qso.guid).remove('-');  // Remove hyphens for N1MM+ format
    }

    return info;
}

bool UdpBroadcastManager::hasRadioStateChanged(const RadioState& a, const RadioState& b)
{
    // Compare significant fields that warrant a broadcast
    return a.frequencyA != b.frequencyA ||
           a.frequencyB != b.frequencyB ||
           a.modeA != b.modeA ||
           a.modeB != b.modeB ||
           a.isTransmitting != b.isTransmitting ||
           a.isSplitEnabled != b.isSplitEnabled ||
           a.isValid != b.isValid;
}

QString UdpBroadcastManager::getModeString(ModeType mode)
{
    // N1MM+ expects specific mode strings
    // Map TR4QT modes to N1MM+ compatible strings
    switch (mode) {
    case ModeType::CW:
    case ModeType::CWR:
        return "CW";

    case ModeType::LSB:
        return "LSB";

    case ModeType::USB:
        return "USB";

    case ModeType::FM:
        return "FM";

    case ModeType::AM:
        return "AM";

    case ModeType::RTTY:
    case ModeType::RTTYR:
        return "RTTY";

    case ModeType::PSK:
    case ModeType::PSKR:
        return "PSK";

    case ModeType::FT8:
        return "FT8";

    case ModeType::FT4:
        return "FT4";

    case ModeType::DATA:
    case ModeType::DATAR:
        return "DATA";

    case ModeType::None:
    default:
        return "";  // Empty if unknown/none
    }
}

} // namespace TR4QT
