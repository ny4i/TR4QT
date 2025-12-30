#include "ADIFFieldMapper.h"
#include "../utils/CountryFile.h"
#include "../logging/LogMacros.h"
#include "../core/Types.h"
#include <QDateTime>
#include <QTimeZone>
#include <QUuid>

namespace TR4QT {

ADIFFieldMapper::ADIFFieldMapper() {
}

bool ADIFFieldMapper::mapToQSO(const QMap<QString, QString>& adifFields, QSO& qso, int recordNumber) {
    m_lastError.clear();
    m_validationErrors.clear();

    // Required fields: CALL, QSO_DATE, TIME_ON, BAND, MODE
    if (!adifFields.contains("CALL")) {
        m_lastError = "Missing required field: CALL";
        return false;
    }

    if (!adifFields.contains("QSO_DATE")) {
        m_lastError = "Missing required field: QSO_DATE";
        return false;
    }

    if (!adifFields.contains("TIME_ON")) {
        m_lastError = "Missing required field: TIME_ON";
        return false;
    }

    // Core fields
    qso.callsign = adifFields["CALL"].trimmed().toUpper();

    // Date/Time
    QString dateStr = adifFields.value("QSO_DATE");
    QString timeStr = adifFields.value("TIME_ON");
    qso.timestamp = parseDateTime(dateStr, timeStr);

    if (!qso.timestamp.isValid()) {
        m_lastError = QString("Invalid date/time: %1 %2").arg(dateStr, timeStr);
        return false;
    }

    // Frequency/Band/Mode
    if (adifFields.contains("FREQ")) {
        qso.frequency = parseFrequency(adifFields["FREQ"]);
    }

    if (adifFields.contains("BAND")) {
        qso.band = parseBand(adifFields["BAND"]);
    } else {
        m_lastError = "Missing required field: BAND";
        return false;
    }

    if (adifFields.contains("MODE")) {
        qso.mode = parseMode(adifFields["MODE"]);
    } else {
        m_lastError = "Missing required field: MODE";
        return false;
    }

    // ADIF SUBMODE (e.g., "FT4" when MODE is "MFSK")
    if (adifFields.contains("SUBMODE")) {
        qso.submode = adifFields["SUBMODE"].trimmed().toUpper();
    }

    // Exchange fields
    qso.rstSent = adifFields.value("RST_SENT", "599");
    qso.rstReceived = adifFields.value("RST_RCVD", "599");

    if (adifFields.contains("STX")) {
        qso.serialNumber = adifFields["STX"].toInt();
    }

    if (adifFields.contains("STX_STRING")) {
        qso.exchangeSent = adifFields["STX_STRING"];
    } else if (adifFields.contains("STX")) {
        qso.exchangeSent = adifFields["STX"];
    }

    if (adifFields.contains("SRX_STRING")) {
        qso.exchangeReceived = adifFields["SRX_STRING"];
    } else if (adifFields.contains("SRX")) {
        qso.exchangeReceived = adifFields["SRX"];
    } else if (adifFields.contains("CQZ")) {
        // For CQ WW, zone is the exchange
        qso.exchangeReceived = adifFields["CQZ"];
    }

    // Geographic fields from ADIF standard
    if (adifFields.contains("STATE")) {
        qso.state = adifFields["STATE"];
    }

    if (adifFields.contains("CNTY")) {
        qso.county = adifFields["CNTY"];
    }

    if (adifFields.contains("ARRL_SECT")) {
        qso.arrlSection = adifFields["ARRL_SECT"];
    }

    if (adifFields.contains("GRIDSQUARE")) {
        qso.gridSquare = adifFields["GRIDSQUARE"];
    }

    if (adifFields.contains("IOTA")) {
        qso.iotaReference = adifFields["IOTA"];
    }

    if (adifFields.contains("CQZ")) {
        qso.cqZone = adifFields["CQZ"].toInt();
    }

    if (adifFields.contains("ITUZ")) {
        qso.ituZone = adifFields["ITUZ"].toInt();
    }

    if (adifFields.contains("DXCC")) {
        qso.dxccEntityCode = adifFields["DXCC"].toInt();
    }

    if (adifFields.contains("COUNTRY")) {
        qso.dxccEntity = adifFields["COUNTRY"];
    }

    if (adifFields.contains("CONT")) {
        qso.continent = parseContinent(adifFields["CONT"]);
    }

    if (adifFields.contains("PFX")) {
        qso.dxccPrefix = adifFields["PFX"];
    }

    // Custom N1MM fields
    if (adifFields.contains("APP_N1MM_ID")) {
        // Use N1MM's ID as our GUID (without dashes, N1MM format)
        qso.guid = adifFields["APP_N1MM_ID"];
    } else {
        // Generate new GUID
        qso.guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // N1MM continent overrides standard CONT (N1MM is more reliable)
    if (adifFields.contains("APP_N1MM_CONTINENT")) {
        qso.continent = parseContinent(adifFields["APP_N1MM_CONTINENT"]);
    }

    // N1MM Run/S&P indicator
    if (adifFields.contains("APP_N1MM_ISRUNQSO")) {
        bool isRunQSO = (adifFields["APP_N1MM_ISRUNQSO"] == "1");
        qso.parsedExchange["isRunQSO"] = isRunQSO ? "true" : "false";
    }

    // N1MM points (might differ from TR4QT scoring)
    if (adifFields.contains("APP_N1MM_POINTS")) {
        qso.qsoPoints = adifFields["APP_N1MM_POINTS"].toInt();
    }

    // Operator
    if (adifFields.contains("OPERATOR")) {
        qso.operatorCall = adifFields["OPERATOR"];
    }

    // Notes
    if (adifFields.contains("COMMENT")) {
        qso.notes = adifFields["COMMENT"];
    }

    // QSO is not marked as dupe on import (we'll check against existing log)
    qso.isDupe = false;
    qso.isMultiplier = false;

    // If CountryFile is available, look up callsign and populate/validate DXCC fields
    if (m_countryFile && !qso.callsign.isEmpty()) {
        CountryData countryData = m_countryFile->lookup(qso.callsign);

        if (countryData.isValid()) {
            // Populate DXCC fields from CTY.DAT if not already set from ADIF
            if (qso.dxccEntity.isEmpty()) {
                qso.dxccEntity = countryData.name;
            }

            if (qso.dxccEntityCode == 0) {
                qso.dxccEntityCode = countryData.dxccEntity;
            }

            if (qso.continent.isEmpty()) {
                qso.continent = continentToString(countryData.continent);
            }

            if (qso.cqZone == 0) {
                qso.cqZone = countryData.cqZone;
            }

            if (qso.ituZone == 0) {
                qso.ituZone = countryData.ituZone;
            }

            // Validate: Check if ADIF data conflicts with CTY.DAT
            // This catches errors like NS4X (US) with STATE=ON (Canada)
            if (!qso.state.isEmpty()) {
                // US callsigns (DXCC 291) can only have US states
                // VE callsigns (DXCC 1) can only have Canadian provinces
                bool isUS = (countryData.dxccEntity == 291);
                bool isCanada = (countryData.dxccEntity == 1);
                bool stateIsCanadian = (qso.state == "AB" || qso.state == "BC" || qso.state == "MB" ||
                                       qso.state == "NB" || qso.state == "NL" || qso.state == "NS" ||
                                       qso.state == "NT" || qso.state == "NU" || qso.state == "ON" ||
                                       qso.state == "PE" || qso.state == "QC" || qso.state == "SK" ||
                                       qso.state == "YT");

                if (isUS && stateIsCanadian) {
                    // Auto-correct: Clear invalid Canadian province for US callsign
                    QString originalState = qso.state;
                    qso.state.clear();

                    ADIFValidationError warning;
                    warning.recordNumber = recordNumber;
                    warning.callsign = qso.callsign;
                    warning.severity = ADIFValidationError::Warning;
                    warning.field = "STATE";
                    warning.value = originalState;
                    warning.message = QString("Corrected: US callsign (%1, DXCC %2) had Canadian province '%3' - cleared STATE field")
                        .arg(countryData.name)
                        .arg(countryData.dxccEntity)
                        .arg(originalState);
                    m_validationErrors.append(warning);
                } else if (isCanada && !stateIsCanadian && !qso.state.isEmpty()) {
                    // Auto-correct: Clear invalid US state for Canadian callsign
                    QString originalState = qso.state;
                    qso.state.clear();

                    ADIFValidationError warning;
                    warning.recordNumber = recordNumber;
                    warning.callsign = qso.callsign;
                    warning.severity = ADIFValidationError::Warning;
                    warning.field = "STATE";
                    warning.value = originalState;
                    warning.message = QString("Corrected: Canadian callsign (%1, DXCC %2) had US state '%3' - cleared STATE field")
                        .arg(countryData.name)
                        .arg(countryData.dxccEntity)
                        .arg(originalState);
                    m_validationErrors.append(warning);
                }
            }
        }
    }

    // Run additional validation
    if (recordNumber > 0) {
        validateQSO(qso, recordNumber);
    }

    return true;
}

QDateTime ADIFFieldMapper::parseDateTime(const QString& date, const QString& time) {
    // ADIF date format: YYYYMMDD
    // ADIF time format: HHMMSS or HHMM

    if (date.length() != 8) {
        return QDateTime();
    }

    int year = date.mid(0, 4).toInt();
    int month = date.mid(4, 2).toInt();
    int day = date.mid(6, 2).toInt();

    int hour = 0;
    int minute = 0;
    int second = 0;

    if (time.length() == 6) {
        // HHMMSS
        hour = time.mid(0, 2).toInt();
        minute = time.mid(2, 2).toInt();
        second = time.mid(4, 2).toInt();
    } else if (time.length() == 4) {
        // HHMM
        hour = time.mid(0, 2).toInt();
        minute = time.mid(2, 2).toInt();
    } else {
        return QDateTime();
    }

    QDate qdate(year, month, day);
    QTime qtime(hour, minute, second);

    if (!qdate.isValid() || !qtime.isValid()) {
        return QDateTime();
    }

    return QDateTime(qdate, qtime, QTimeZone::UTC);
}

BandType ADIFFieldMapper::parseBand(const QString& bandStr) {
    // ADIF uses "160M", "80M", "40M", etc.
    // TR4QT uses "160M", "80M", etc. (same format)
    // Use existing stringToBand function
    return stringToBand(bandStr.trimmed().toUpper());
}

ModeType ADIFFieldMapper::parseMode(const QString& modeStr) {
    // ADIF modes: CW, SSB, RTTY, PSK31, FT8, etc.
    // Use existing stringToMode function
    QString mode = modeStr.trimmed().toUpper();

    // Map common ADIF mode variants
    if (mode == "PSK31" || mode == "PSK63" || mode == "PSK125") {
        mode = "PSK";
    }

    return stringToMode(mode);
}

freq_t ADIFFieldMapper::parseFrequency(const QString& freqStr) {
    // ADIF frequency is in MHz (e.g., "14.11099")
    // TR4QT uses Hz
    bool ok;
    double freqMHz = freqStr.toDouble(&ok);

    if (!ok) {
        return 0;
    }

    // Convert MHz to Hz
    return static_cast<freq_t>(freqMHz * 1000000.0);
}

QString ADIFFieldMapper::parseContinent(const QString& contStr) {
    // ADIF continent codes: NA, SA, EU, AF, AS, OC
    // TR4QT uses the same format
    return contStr.trimmed().toUpper();
}

void ADIFFieldMapper::validateQSO(const QSO& qso, int recordNumber) {
    // Additional validation beyond the CTY.DAT validation in mapToQSO()

    // Validate frequency matches band (if both present)
    if (qso.frequency > 0 && qso.band != BandType::None) {
        // Get expected band from frequency using Types.h helper
        BandType expectedBand = frequencyToBand(qso.frequency);

        if (expectedBand != BandType::None && expectedBand != qso.band) {
            ADIFValidationError error;
            error.recordNumber = recordNumber;
            error.callsign = qso.callsign;
            error.severity = ADIFValidationError::Warning;
            error.field = "FREQ/BAND";
            error.value = QString("%1 Hz / %2").arg(qso.frequency).arg(bandToString(qso.band));
            error.message = QString("Frequency %1 Hz is in %2 band, but BAND field says %3")
                .arg(qso.frequency)
                .arg(bandToString(expectedBand))
                .arg(bandToString(qso.band));
            m_validationErrors.append(error);
        }
    }

    // Validate timestamp is reasonable (not in future, not before amateur radio existed)
    if (qso.timestamp.isValid()) {
        QDateTime now = QDateTime::currentDateTimeUtc();
        QDateTime firstQSO(QDate(1900, 1, 1), QTime(0, 0, 0), QTimeZone::UTC);

        if (qso.timestamp > now) {
            ADIFValidationError error;
            error.recordNumber = recordNumber;
            error.callsign = qso.callsign;
            error.severity = ADIFValidationError::Warning;
            error.field = "QSO_DATE/TIME_ON";
            error.value = qso.timestamp.toString(Qt::ISODate);
            error.message = "QSO date/time is in the future";
            m_validationErrors.append(error);
        } else if (qso.timestamp < firstQSO) {
            ADIFValidationError error;
            error.recordNumber = recordNumber;
            error.callsign = qso.callsign;
            error.severity = ADIFValidationError::Warning;
            error.field = "QSO_DATE/TIME_ON";
            error.value = qso.timestamp.toString(Qt::ISODate);
            error.message = "QSO date/time is before 1900";
            m_validationErrors.append(error);
        }
    }
}

} // namespace TR4QT
