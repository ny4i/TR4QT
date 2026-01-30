/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "QSOLogger.h"
#include "../utils/CallsignValidator.h"
#include "../utils/AppSettings.h"
#include "../contests/RSTValidator.h"
#include "../logging/LogMacros.h"
#include <QDateTime>

QSOLogger::QSOLogger(const Config& config)
    : m_config(config)
{
}

QSOLogger::Result QSOLogger::logQSO(const Input& input, const QList<QSO>& existingQSOs)
{
    Result result;
    result.updatedSerialNumber = input.serialNumber;

    // Validation Phase
    QString errorMsg;

    if (!validateCallsign(input.callsign, errorMsg)) {
        result.errorMessage = errorMsg;
        return result;
    }

    if (!validateExchange(input.exchange, errorMsg)) {
        result.errorMessage = errorMsg;
        return result;
    }

    if (!validateBandMode(input.radioState, errorMsg)) {
        result.errorMessage = errorMsg;
        return result;
    }

    // QSO Creation Phase
    QSO qso;
    populateQSOFromInput(qso, input);

    // Parse exchange into QSO fields (RST, zone, serial, etc.)
    parseExchangeIntoQSO(qso, input.exchange);

    // Set default RST if not populated by contest
    if (qso.rstReceived.isEmpty()) {
        qso.rstReceived = RSTValidator::getDefault(qso.mode);
    }

    // Sent exchange handling
    if (m_config.contest) {
        // Get exchange template from contest (e.g., "599 {ZONE}" for CQ WW)
        QString exchangeTemplate = m_config.contest->formatSentExchange(
            input.serialNumber, qso.rstSent);

        // Substitute template placeholders with actual values
        qso.exchangeSent = substituteSentExchangeTemplate(
            exchangeTemplate, input.serialNumber, qso.rstSent);

        // Increment serial number if contest uses them
        if (m_config.contest->usesSerialNumbers()) {
            result.updatedSerialNumber = input.serialNumber + 1;
        }
    } else {
        qso.exchangeSent = "";  // No contest active
    }

    // DXCC lookup phase
    populateDXCCFields(qso);

    // Duplicate checking phase (BEFORE calculating points)
    result.isDuplicate = checkForDuplicate(
        qso.callsign, qso.band, qso.mode, existingQSOs, result.dupeInfo);
    qso.isDupe = result.isDuplicate;

    // Points calculation phase
    qso.qsoPoints = calculateQSOPoints(qso);

    // Duplicates get 0 points
    if (qso.isDupe) {
        qso.qsoPoints = 0;
        LOG_INFO("QSOLogger", QString("Duplicate QSO detected: %1 - %2")
                 .arg(qso.callsign, result.dupeInfo));
    }

    // Multiplier checking phase
    checkForMultipliers(qso, existingQSOs, result.isNewMultiplier, result.multiplierValues);
    qso.isMultiplier = result.isNewMultiplier;
    qso.multipliers = result.multiplierValues;

    // Success!
    result.success = true;
    result.qso = qso;
    return result;
}

// ============================================================================
// VALIDATION HELPERS
// ============================================================================

bool QSOLogger::validateCallsign(const QString& callsign, QString& errorMsg)
{
    if (callsign.isEmpty()) {
        errorMsg = "Error: Callsign is required";
        return false;
    }

    // Validate callsign format (non-blocking warning)
    QString validationError;
    if (!CallsignValidator::validate(callsign, &validationError)) {
        // Log warning but allow QSO (user can still proceed)
        LOG_WARN("QSOLogger", QString("Invalid callsign format: %1 - %2")
                 .arg(callsign, validationError));
        // Note: Not returning false - allowing user to proceed despite warning
    }

    return true;
}

bool QSOLogger::validateExchange(const QString& exchange, QString& errorMsg)
{
    if (!m_config.contest) {
        return true;  // No contest - exchange is optional
    }

    // Check if exchange is required for this contest
    if (exchange.isEmpty() && m_config.contest->requiresExchange()) {
        errorMsg = "Error: Exchange is required";
        return false;
    }

    // Validate exchange format (skip if empty and not required)
    if (!exchange.isEmpty()) {
        QString contestErrorMsg;
        if (!m_config.contest->validateReceivedExchange(exchange, contestErrorMsg)) {
            errorMsg = QString("Invalid exchange: %1").arg(contestErrorMsg);
            return false;
        }
    }

    return true;
}

bool QSOLogger::validateBandMode(const RadioState& radioState, QString& errorMsg)
{
    // CRITICAL: Prevent logging QSO with invalid band/mode
    if (radioState.bandA == BandType::None || radioState.modeA == ModeType::None) {
        if (radioState.bandA == BandType::None && radioState.modeA == ModeType::None) {
            errorMsg = "Error: Band and Mode not set - use Band Up/Down (ALT-B/ALT-V) to select band";
        } else if (radioState.bandA == BandType::None) {
            errorMsg = "Error: Band not set - use Band Up/Down (ALT-B/ALT-V) to select band";
        } else {
            errorMsg = "Error: Mode not set - radio not connected and mode unknown";
        }
        return false;
    }

    return true;
}

// ============================================================================
// QSO CREATION HELPERS
// ============================================================================

void QSOLogger::populateQSOFromInput(QSO& qso, const Input& input)
{
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.callsign = input.callsign;
    qso.operatorCall = input.operatorCallsign;

    // Snapshot radio state
    qso.frequency = input.radioState.frequencyA;
    qso.mode = input.radioState.modeA;
    qso.band = input.radioState.bandA;

    // Track operating mode (CQ vs S&P)
    qso.isRunQSO = (input.operatingMode == OperatingMode::CQ);

    // Track which radio logged this QSO (for SO2R)
    qso.radioNr = input.radioNumber;

    // Set default sent RST based on mode
    qso.rstSent = RSTValidator::getDefault(qso.mode);
}

void QSOLogger::populateDXCCFields(QSO& qso)
{
    if (!m_config.countryFile) {
        LOG_WARN("QSOLogger", "CountryFile not configured - DXCC fields will be empty");
        return;
    }

    // Lookup country/zone from cty.dat
    m_config.countryFile->populateQSODXCCFields(qso);

    if (!qso.dxccEntity.isEmpty()) {
        LOG_DEBUG("QSOLogger", QString("Looked up %1: %2 (Zone %3, %4)")
                  .arg(qso.callsign)
                  .arg(qso.dxccEntity)
                  .arg(qso.cqZone)
                  .arg(qso.continent));
    } else {
        LOG_WARN("QSOLogger", QString("Country lookup failed for callsign: %1").arg(qso.callsign));
    }
}

void QSOLogger::parseExchangeIntoQSO(QSO& qso, const QString& exchange)
{
    if (!m_config.contest) {
        // No active contest - store exchange as-is
        qso.rstReceived = RSTValidator::getDefault(qso.mode);
        qso.exchangeReceived = exchange;
        return;
    }

    // Contest populates QSO fields AND sets qso.exchangeReceived
    m_config.contest->parseReceivedExchange(exchange, qso);
}

QString QSOLogger::formatSentExchange(int serialNumber, const QString& rstSent)
{
    if (!m_config.contest) {
        return "";
    }

    return m_config.contest->formatSentExchange(serialNumber, rstSent);
}

QString QSOLogger::substituteSentExchangeTemplate(const QString& exchangeTemplate,
                                                   int serialNumber,
                                                   const QString& rstSent)
{
    QString result = exchangeTemplate;

    // Substitute placeholders with actual values from settings/contest
    result.replace("{SERIAL}", QString::number(serialNumber).rightJustified(3, '0'));
    result.replace("{RST}", rstSent);
    result.replace("{ZONE}", QString::number(m_config.myStation.cqZone));
    result.replace("{STATE}", AppSettings::instance().getMyState());
    result.replace("{SECTION}", AppSettings::instance().getMyARRLSection());
    result.replace("{COUNTY}", AppSettings::instance().getMyCounty());

    // {NAME} uses contest-specific operator name, falls back to AppSettings first name
    QString operatorName = m_config.operatorName.isEmpty()
        ? AppSettings::instance().getMyFirstName()
        : m_config.operatorName;
    result.replace("{NAME}", operatorName);

    return result;
}

// ============================================================================
// DUPLICATE CHECKING
// ============================================================================

bool QSOLogger::checkForDuplicate(const QString& callsign,
                                   BandType band,
                                   ModeType mode,
                                   const QList<QSO>& existingQSOs,
                                   QString& dupeInfo)
{
    if (!m_config.contest) {
        return false;  // No contest - no duplicate rules
    }

    DuplicateCheckingRule dupeRule = m_config.contest->getDuplicateCheckingRule();

    for (const QSO& existingQSO : existingQSOs) {
        if (existingQSO.callsign != callsign) {
            continue;  // Different callsign - not a dupe
        }

        bool isDupe = false;

        switch (dupeRule) {
        case DuplicateCheckingRule::PerBandMode:
            // Duplicate if same callsign + same band + same mode
            isDupe = (existingQSO.band == band && existingQSO.mode == mode);
            if (isDupe) {
                dupeInfo = QString("Already worked on %1 %2")
                           .arg(bandToString(band))
                           .arg(modeToString(mode));
            }
            break;

        case DuplicateCheckingRule::AllBandMode:
            // Duplicate if same callsign + same mode (any band)
            isDupe = (existingQSO.mode == mode);
            if (isDupe) {
                dupeInfo = QString("Already worked on %1 (any band)")
                           .arg(modeToString(mode));
            }
            break;

        case DuplicateCheckingRule::PerBand:
            // Duplicate if same callsign + same band (any mode)
            isDupe = (existingQSO.band == band);
            if (isDupe) {
                dupeInfo = QString("Already worked on %1 (any mode)")
                           .arg(bandToString(band));
            }
            break;

        case DuplicateCheckingRule::AllBand:
            // Duplicate if same callsign (once per contest)
            isDupe = true;
            dupeInfo = "Already worked (once per contest)";
            break;
        }

        if (isDupe) {
            return true;
        }
    }

    return false;  // Not a duplicate
}

// ============================================================================
// SCORING HELPERS
// ============================================================================

int QSOLogger::calculateQSOPoints(const QSO& qso)
{
    if (!m_config.contest) {
        return 1;  // Default 1 point if no contest
    }

    int points = m_config.contest->calculateQSOPoints(qso, m_config.myStation);

    LOG_DEBUG("QSOLogger", QString("QSO points calculated: %1").arg(points));

    return points;
}

// ============================================================================
// MULTIPLIER CHECKING
// ============================================================================

void QSOLogger::checkForMultipliers(QSO& qso,
                                     const QList<QSO>& existingQSOs,
                                     bool& isNewMultiplier,
                                     QStringList& multiplierValues)
{
    isNewMultiplier = false;
    multiplierValues.clear();

    if (!m_config.contest) {
        return;  // No contest - no multipliers
    }

    QList<MultiplierDefinition> multDefs = m_config.contest->getMultiplierTypes();

    // Build map of already worked multipliers from existing QSOs
    // IMPORTANT: Respect MultiplierScope when building this list
    QMap<MultiplierType, QStringList> workedMults;

    for (const QSO& existingQSO : existingQSOs) {
        for (const MultiplierDefinition& multDef : multDefs) {
            // For PerBand multipliers, only include mults from the SAME band
            // For AllBands multipliers, include all mults regardless of band
            bool includeMult = (multDef.scope == MultiplierScope::AllBands) ||
                               (multDef.scope == MultiplierScope::PerBand && existingQSO.band == qso.band);

            if (includeMult) {
                QString multValue = m_config.contest->getMultiplierValue(
                    existingQSO, multDef.type, QStringList());
                if (!multValue.isEmpty()) {
                    workedMults[multDef.type].append(multValue);
                }
            }
        }
    }

    // Check if this QSO provides any new multipliers
    for (const MultiplierDefinition& multDef : multDefs) {
        QString multValue = m_config.contest->getMultiplierValue(
            qso, multDef.type, workedMults[multDef.type]);

        if (!multValue.isEmpty()) {
            // This is a new multiplier!
            isNewMultiplier = true;

            // Store as "Type:Value" for display (e.g., "Prefix:W1", "CQZone:5")
            QString typeStr = multDef.type == MultiplierType::Country ? "Country" :
                             multDef.type == MultiplierType::CQZone ? "CQZone" :
                             multDef.type == MultiplierType::ITUZone ? "ITUZone" :
                             multDef.type == MultiplierType::State ? "State" :
                             multDef.type == MultiplierType::Section ? "Section" :
                             multDef.type == MultiplierType::Prefix ? "Prefix" : "Custom";

            multiplierValues.append(QString("%1:%2").arg(typeStr, multValue));
        }
    }
}
