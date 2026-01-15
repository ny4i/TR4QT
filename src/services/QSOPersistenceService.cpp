/**
 * QSOPersistenceService - Implementation
 */

#include "QSOPersistenceService.h"
#include "../data/QSORepository.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include "../logging/LogMacros.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

namespace TR4QT {

QSOPersistenceService::QSOPersistenceService(const Config& config)
    : m_config(config)
    , m_repository(new QSORepository())
{
}

QSOPersistenceService::~QSOPersistenceService() {
    delete m_repository;
}

QSOPersistenceService::SaveResult QSOPersistenceService::saveQSO(const QSO& qso, int contestDbId) {
    SaveResult result;
    result.attemptCount = 0;

    // Try to save to database with retries
    for (int attempt = 0; attempt < m_config.maxRetries; ++attempt) {
        result.attemptCount++;

        // Make a mutable copy for repository (it needs to set the ID)
        QSO mutableQso = qso;
        if (m_repository->saveQSO(mutableQso, contestDbId)) {
            // Success!
            result.status = SaveResult::SavedToDatabase;
            result.databaseAvailable = true;
            result.databaseId = mutableQso.id;  // Return the assigned database ID
            LOG_DEBUG("QSOPersistenceService",
                     QString("QSO saved to database on attempt %1 (id=%2)")
                     .arg(attempt + 1).arg(mutableQso.id));
            return result;
        }

        // Save failed
        m_lastError = m_repository->lastError();
        LOG_ERROR("QSOPersistenceService",
                 QString("Database save failed (attempt %1/%2): %3")
                 .arg(attempt + 1).arg(m_config.maxRetries).arg(m_lastError));
    }

    // All retry attempts exhausted
    result.status = SaveResult::NeedsUserDecision;
    result.errorMessage = m_lastError;
    result.databaseAvailable = false;

    LOG_WARN("QSOPersistenceService",
            QString("Failed to save QSO after %1 attempts: %2")
            .arg(m_config.maxRetries).arg(m_lastError));

    return result;
}

bool QSOPersistenceService::saveToEmergencyFile(const QSO& qso, QString& filePath) {
    // Construct emergency file path
    filePath = m_config.appDataDir + "/emergency_log.adi";

    // Ensure directory exists
    QDir().mkpath(QFileInfo(filePath).dir().path());

    // Check if file already exists (determines if we need header)
    QFile emergencyFile(filePath);
    bool fileExists = emergencyFile.exists();

    // Open file for append
    if (!emergencyFile.open(QIODevice::Append | QIODevice::Text)) {
        m_lastError = QString("Failed to open emergency file: %1").arg(emergencyFile.errorString());
        LOG_ERROR("QSOPersistenceService", m_lastError);
        return false;
    }

    QTextStream out(&emergencyFile);

    // Write header if new file
    if (!fileExists) {
        writeEmergencyFileHeader(out);
    }

    // Write QSO record
    writeADIFRecord(out, qso);

    emergencyFile.close();

    LOG_INFO("QSOPersistenceService",
            QString("QSO saved to emergency file: %1").arg(filePath));

    return true;
}

QString QSOPersistenceService::lastError() const {
    return m_lastError;
}

void QSOPersistenceService::writeEmergencyFileHeader(QTextStream& out) {
    out << "TR4QT Emergency Log - QSOs that could not be saved to database\n";
    out << "<ADIF_VER:5>3.1.4\n";
    out << "<PROGRAMID:5>TR4QT\n";
    out << "<PROGRAMVERSION:" << QString::number(QString(APP_VERSION).length())
        << ">" << APP_VERSION << "\n";
    out << "<EOH>\n\n";
}

void QSOPersistenceService::writeADIFRecord(QTextStream& out, const QSO& qso) {
    // Write QSO in ADIF format
    out << "<CALL:" << qso.callsign.length() << ">" << qso.callsign << " ";
    out << "<QSO_DATE:8>" << qso.timestamp.toString("yyyyMMdd") << " ";
    out << "<TIME_ON:6>" << qso.timestamp.toString("HHmmss") << " ";
    out << "<BAND:" << bandToString(qso.band).length() << ">" << bandToString(qso.band) << " ";
    out << "<MODE:" << modeToString(qso.mode).length() << ">" << modeToString(qso.mode) << " ";
    out << "<RST_SENT:" << qso.rstSent.length() << ">" << qso.rstSent << " ";
    out << "<RST_RCVD:" << qso.rstReceived.length() << ">" << qso.rstReceived << " ";

    // Optional fields
    if (!qso.exchangeSent.isEmpty()) {
        out << "<STX_STRING:" << qso.exchangeSent.length() << ">" << qso.exchangeSent << " ";
    }
    if (!qso.exchangeReceived.isEmpty()) {
        out << "<SRX_STRING:" << qso.exchangeReceived.length() << ">" << qso.exchangeReceived << " ";
    }

    out << "<EOR>\n";
}

} // namespace TR4QT
