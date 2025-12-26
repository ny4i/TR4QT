#include "ADIFExporter.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace TR4QT {

bool ADIFExporter::exportToFile(const QList<QSO>& qsos,
                                const QString& filePath,
                                ContestBase* contest,
                                const QString& operatorCall) {
    m_lastError.clear();

    // Generate ADIF text
    QString adifText = generateADIF(qsos, contest, operatorCall);

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("Failed to open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << adifText;
    file.close();

    return true;
}

QString ADIFExporter::generateADIF(const QList<QSO>& qsos,
                                  ContestBase* contest,
                                  const QString& operatorCall) {
    QString result;
    QTextStream stream(&result);

    // Write header
    stream << generateHeader();

    // OPERATOR can go in header (applies to all QSOs)
    if (!operatorCall.isEmpty()) {
        stream << formatField("OPERATOR", operatorCall) << "\n";
    }

    stream << "<EOH>\n\n";

    // Get official ADIF contest ID from contest instance
    QString adifContestId;
    if (contest) {
        adifContestId = contest->getADIFContestId();
    }

    // Write each QSO (CONTEST_ID goes in each record)
    for (const QSO& qso : qsos) {
        if (!qso.isDupe) {  // Skip dupes in export
            stream << formatQSO(qso, adifContestId) << "\n";
        }
    }

    return result;
}

QString ADIFExporter::generateHeader() {
    QString header;
    QTextStream stream(&header);

    stream << "ADIF Export from " << APP_NAME << " v" << APP_VERSION << "\n";
    stream << "<ADIF_VER:5>3.1.4\n";
    stream << "<CREATED_TIMESTAMP:15>" << QDateTime::currentDateTimeUtc().toString("yyyyMMdd HHmmss") << "\n";
    stream << "<PROGRAMID:" << QString(APP_NAME).length() << ">" << APP_NAME << "\n";
    stream << "<PROGRAMVERSION:" << QString(APP_VERSION).length() << ">" << APP_VERSION << "\n";

    return header;
}

QString ADIFExporter::formatQSO(const QSO& qso, const QString& contestId) {
    QString result;
    QTextStream stream(&result);

    // Callsign (required)
    stream << formatField("CALL", qso.callsign);

    // Date and time (required)
    QString qsoDate = qso.timestamp.toUTC().toString("yyyyMMdd");
    QString qsoTime = qso.timestamp.toUTC().toString("HHmmss");
    stream << formatField("QSO_DATE", qsoDate);
    stream << formatField("TIME_ON", qsoTime);

    // Contest ID (official ADIF contest name)
    if (!contestId.isEmpty()) {
        stream << formatField("CONTEST_ID", contestId);
    }

    // Band
    stream << formatField("BAND", bandToString(qso.band).remove('M').toLower() + "m");

    // Mode (map our modes to ADIF modes)
    QString adifMode;
    switch (qso.mode) {
    case ModeType::CW:
        adifMode = "CW";
        break;
    case ModeType::USB:
    case ModeType::LSB:
        adifMode = "SSB";
        break;
    case ModeType::RTTY:
        adifMode = "RTTY";
        break;
    case ModeType::FM:
        adifMode = "FM";
        break;
    default:
        adifMode = "SSB";
        break;
    }
    stream << formatField("MODE", adifMode);

    // Frequency (in MHz)
    if (qso.frequency > 0) {
        double freqMhz = qso.frequency / 1000000.0;
        stream << formatField("FREQ", QString::number(freqMhz, 'f', 6));
    }

    // RST sent and received
    stream << formatField("RST_SENT", qso.rstSent);
    stream << formatField("RST_RCVD", qso.rstReceived);

    // Exchange sent and received
    if (!qso.exchangeSent.isEmpty()) {
        stream << formatField("STX_STRING", qso.exchangeSent);
    }
    if (!qso.exchangeReceived.isEmpty()) {
        stream << formatField("SRX_STRING", qso.exchangeReceived);
    }

    // DXCC and zone information (use ADIF DXCC Entity Code enumeration)
    if (qso.dxccEntityCode > 0) {
        stream << formatField("DXCC", QString::number(qso.dxccEntityCode));
    }
    if (qso.cqZone > 0) {
        stream << formatField("CQZ", QString::number(qso.cqZone));
    }
    if (qso.ituZone > 0) {
        stream << formatField("ITUZ", QString::number(qso.ituZone));
    }
    if (!qso.state.isEmpty()) {
        stream << formatField("STATE", qso.state);
    }

    // Operator
    if (!qso.operatorCall.isEmpty()) {
        stream << formatField("OPERATOR", qso.operatorCall);
    }

    // QSO points (contest-specific)
    if (qso.qsoPoints > 0) {
        stream << formatField("POINTS", QString::number(qso.qsoPoints));
    }

    // End of record
    stream << "<EOR>\n";

    return result;
}

QString ADIFExporter::formatField(const QString& fieldName, const QString& value) {
    if (value.isEmpty()) {
        return QString();
    }

    // ADIF field format: <FIELD_NAME:LENGTH>VALUE
    return QString("<%1:%2>%3 ").arg(fieldName).arg(value.length()).arg(value);
}

} // namespace TR4QT
