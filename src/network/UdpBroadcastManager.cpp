#include "UdpBroadcastManager.h"
#include "RadioInfo.h"
#include "ContactInfo.h"
#include "../core/Types.h"
#include <QDebug>

namespace TR4QT {

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
        } else if (data.contains("<ContactInfo>")) {
            messageType = "ContactInfo";
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
                                     const QString& contestName)
{
    // Check if broadcasting is enabled
    if (!m_enabled || !m_contactInfoEnabled) {
        return;
    }

    // Send immediately (no throttling for QSO logging)
    ContactInfo info = createContactInfo(qso, stationCall, contestName);
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
    info.stationName = "";  // Could be configured in settings

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
                                                   const QString& contestName)
{
    ContactInfo info;

    // Application identity
    info.app = "TR4QT";
    info.contestName = contestName;
    info.stationName = "";  // Could be configured in settings

    // Timestamp (ISO 8601 format)
    info.timestamp = qso.timestamp.toString(Qt::ISODate);

    // Station identification
    info.mycall = stationCall;
    info.call = qso.callsign;

    // Frequency and mode
    info.freq = RadioInfo::hzToTensOfHz(qso.frequency);
    info.band = bandToString(qso.band);
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

    default:
        return "SSB";  // Default fallback
    }
}

} // namespace TR4QT
