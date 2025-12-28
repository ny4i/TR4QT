#include "CabrilloExporter.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>  // for std::sort

namespace TR4QT {

void CabrilloExporter::setStationInfo(const QString& callsign,
                                     const QString& gridSquare,
                                     const QString& name,
                                     const QString& address,
                                     const QString& city,
                                     const QString& stateProvince,
                                     const QString& postalCode,
                                     const QString& country,
                                     const QString& email) {
    m_callsign = callsign;
    m_gridSquare = gridSquare;
    m_name = name;
    m_address = address;
    m_city = city;
    m_stateProvince = stateProvince;
    m_postalCode = postalCode;
    m_country = country;
    m_email = email;
}

void CabrilloExporter::setCategory(const QString& assisted,
                                  const QString& band,
                                  const QString& mode,
                                  const QString& operator_,
                                  const QString& power,
                                  const QString& station,
                                  const QString& time,
                                  const QString& transmitter,
                                  const QString& overlay) {
    m_categoryAssisted = assisted;
    m_categoryBand = band;
    m_categoryMode = mode;
    m_categoryOperator = operator_;
    m_categoryPower = power;
    m_categoryStation = station;
    m_categoryTime = time;
    m_categoryTransmitter = transmitter;
    m_categoryOverlay = overlay;
}

bool CabrilloExporter::exportToFile(const QList<QSO>& qsos,
                                    ContestBase* contest,
                                    const QString& filePath) {
    m_lastError.clear();

    // Allow export without contest (will use generic formatting)
    // Generate Cabrillo text
    QString cabrilloText = generateCabrillo(qsos, contest);

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("Failed to open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << cabrilloText;
    file.close();

    return true;
}

QString CabrilloExporter::generateCabrillo(const QList<QSO>& qsos, ContestBase* contest) {
    QString result;
    QTextStream stream(&result);

    // Write header (analyzes QSOs for operator counts)
    stream << generateHeader(contest, qsos);

    // Write QSO lines
    int serialNumber = 1;
    for (const QSO& qso : qsos) {
        if (!qso.isDupe) {
            stream << formatQSO(qso, contest, serialNumber++);
        }
    }

    // End of log
    stream << "END-OF-LOG:\n";

    return result;
}

QString CabrilloExporter::generateHeader(ContestBase* contest, const QList<QSO>& qsos) {
    QString header;
    QTextStream stream(&header);

    // Count non-dupe QSOs and QSOs per operator
    int qsoCount = 0;
    QMap<QString, int> operatorCounts;

    for (const QSO& qso : qsos) {
        if (!qso.isDupe) {
            qsoCount++;

            // Track QSOs per operator
            QString op = qso.operatorCall.isEmpty() ? m_callsign : qso.operatorCall;
            if (!op.isEmpty()) {
                operatorCounts[op]++;
            }
        }
    }

    stream << "START-OF-LOG: 3.0\n";
    stream << "CREATED-BY: " << APP_NAME << " v" << APP_VERSION << "\n";
    stream << "CONTEST: " << getContestName(contest) << "\n";

    // Station information
    if (!m_callsign.isEmpty()) {
        stream << "CALLSIGN: " << m_callsign << "\n";
    }
    if (!m_gridSquare.isEmpty()) {
        stream << "GRID-LOCATOR: " << m_gridSquare << "\n";
    }
    if (!m_name.isEmpty()) {
        stream << "NAME: " << m_name << "\n";
    }
    if (!m_address.isEmpty()) {
        stream << "ADDRESS: " << m_address << "\n";
    }
    if (!m_city.isEmpty()) {
        stream << "ADDRESS-CITY: " << m_city << "\n";
    }
    if (!m_stateProvince.isEmpty()) {
        stream << "ADDRESS-STATE-PROVINCE: " << m_stateProvince << "\n";
    }
    if (!m_postalCode.isEmpty()) {
        stream << "ADDRESS-POSTALCODE: " << m_postalCode << "\n";
    }
    if (!m_country.isEmpty()) {
        stream << "ADDRESS-COUNTRY: " << m_country << "\n";
    }
    if (!m_email.isEmpty()) {
        stream << "EMAIL: " << m_email << "\n";
    }

    // Categories
    stream << "CATEGORY-ASSISTED: " << m_categoryAssisted << "\n";
    stream << "CATEGORY-BAND: " << m_categoryBand << "\n";
    stream << "CATEGORY-MODE: " << m_categoryMode << "\n";
    stream << "CATEGORY-OPERATOR: " << m_categoryOperator << "\n";
    stream << "CATEGORY-POWER: " << m_categoryPower << "\n";
    stream << "CATEGORY-STATION: " << m_categoryStation << "\n";
    if (!m_categoryTime.isEmpty()) {
        stream << "CATEGORY-TIME: " << m_categoryTime << "\n";
    }
    stream << "CATEGORY-TRANSMITTER: " << m_categoryTransmitter << "\n";
    if (!m_categoryOverlay.isEmpty()) {
        stream << "CATEGORY-OVERLAY: " << m_categoryOverlay << "\n";
    }

    // Score and operators
    stream << "CLAIMED-SCORE: " << m_claimedScore << "\n";

    // Output operators sorted by QSO count (descending)
    if (!operatorCounts.isEmpty()) {
        // Convert map to list of pairs for sorting
        QList<QPair<QString, int>> opList;
        for (auto it = operatorCounts.begin(); it != operatorCounts.end(); ++it) {
            opList.append({it.key(), it.value()});
        }

        // Sort by QSO count descending
        std::sort(opList.begin(), opList.end(),
            [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                return a.second > b.second;  // Descending by QSO count
            });

        // Build comma-separated list (callsigns only, no counts)
        QStringList opNames;
        for (const auto& pair : opList) {
            opNames.append(pair.first);
        }

        stream << "OPERATORS: " << opNames.join(", ") << "\n";
    }

    if (!m_club.isEmpty()) {
        stream << "CLUB: " << m_club << "\n";
    }

    // Soapbox (optional comments)
    stream << "SOAPBOX: Log created by " << APP_NAME << " v" << APP_VERSION << "\n";
    stream << "SOAPBOX: QSOs: " << qsoCount << "\n";

    return header;
}

QString CabrilloExporter::formatQSO(const QSO& qso, ContestBase* contest, int serialNumber) {
    // Cabrillo QSO format:
    // QSO: freq mo date       time call          exch   call          exch
    // QSO: 14020 CW 2023-11-25 1234 W1AW          599 05 DL1ABC        599 14

    QString line;
    QTextStream stream(&line);

    stream << "QSO: ";

    // Frequency (5 chars, right-aligned)
    stream << QString("%1 ").arg(getCabrilloFreq(qso.frequency), 5);

    // Mode (2 chars)
    stream << QString("%1 ").arg(getCabrilloMode(qso.mode), -2);

    // Date (10 chars: YYYY-MM-DD)
    stream << qso.timestamp.toUTC().toString("yyyy-MM-dd") << " ";

    // Time (4 chars: HHMM)
    stream << qso.timestamp.toUTC().toString("HHmm") << " ";

    // Sent call (13 chars, left-aligned)
    stream << QString("%1 ").arg(m_callsign.isEmpty() ? "MYCALL" : m_callsign, -13);

    // Sent exchange (varies by contest)
    QString sentExch = qso.exchangeSent;
    if (sentExch.isEmpty()) {
        sentExch = "599 001";  // Default RST + serial
    }
    stream << QString("%1 ").arg(sentExch, -14);

    // Received call (13 chars, left-aligned)
    stream << QString("%1 ").arg(qso.callsign, -13);

    // Received exchange
    QString rcvdExch = qso.exchangeReceived;
    if (rcvdExch.isEmpty()) {
        rcvdExch = "599";
    }
    stream << QString("%1").arg(rcvdExch, -14);

    stream << "\n";

    return line;
}

QString CabrilloExporter::getContestName(ContestBase* contest) {
    if (!contest) {
        return "UNKNOWN";
    }

    // Use the official ADIF Contest-ID, which matches Cabrillo CONTEST field format
    QString adifId = contest->getADIFContestId();
    if (!adifId.isEmpty()) {
        return adifId;
    }

    // Fallback: use contest name, replacing spaces with hyphens
    return contest->getContestName().replace(' ', '-').toUpper();
}

QString CabrilloExporter::getCabrilloMode(ModeType mode) {
    switch (mode) {
    case ModeType::CW:
        return "CW";
    case ModeType::USB:
    case ModeType::LSB:
        return "PH";  // Phone
    case ModeType::RTTY:
        return "RY";  // RTTY
    case ModeType::FM:
        return "FM";
    default:
        return "PH";
    }
}

QString CabrilloExporter::getCabrilloFreq(freq_t frequency) {
    // Convert Hz to kHz, rounded to nearest integer
    int freqKhz = (frequency + 500) / 1000;
    return QString::number(freqKhz);
}

} // namespace TR4QT
