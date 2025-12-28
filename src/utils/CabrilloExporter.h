#ifndef CABRILLOEXPORTER_H
#define CABRILLOEXPORTER_H

#include <QString>
#include <QList>
#include "../models/QSO.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

/**
 * Cabrillo contest log file exporter
 *
 * Exports QSO log to Cabrillo format for contest submission.
 * Cabrillo is the standard format for submitting contest logs.
 *
 * Format specification: https://www.cqwpx.com/cabrillo.htm
 *
 * Example usage:
 *   CabrilloExporter exporter;
 *   exporter.setStationInfo(callsign, gridSquare, etc.);
 *   bool success = exporter.exportToFile(qsos, contest, "/path/to/file.cbr");
 */
class CabrilloExporter {
public:
    CabrilloExporter() = default;
    ~CabrilloExporter() = default;

    /**
     * Set station information for Cabrillo header
     */
    void setStationInfo(const QString& callsign,
                       const QString& gridSquare,
                       const QString& name,
                       const QString& address,
                       const QString& city,
                       const QString& stateProvince,
                       const QString& postalCode,
                       const QString& country,
                       const QString& email);

    /**
     * Set contest category information
     */
    void setCategory(const QString& assisted,
                    const QString& band,
                    const QString& mode,
                    const QString& operator_,
                    const QString& power,
                    const QString& station,
                    const QString& time,
                    const QString& transmitter,
                    const QString& overlay);

    /**
     * Set claimed score
     */
    void setClaimedScore(int score) { m_claimedScore = score; }

    /**
     * Set operator(s) - comma-separated list for multi-op
     */
    void setOperators(const QString& operators) { m_operators = operators; }

    /**
     * Set club affiliation
     */
    void setClub(const QString& club) { m_club = club; }

    /**
     * Export QSO list to Cabrillo file
     *
     * @param qsos List of QSOs to export
     * @param contest Contest instance for format guidance
     * @param filePath Path to output file (typically .cbr or .log extension)
     * @return true if export succeeded, false on error
     */
    bool exportToFile(const QList<QSO>& qsos,
                     ContestBase* contest,
                     const QString& filePath);

    /**
     * Generate Cabrillo text from QSO list
     *
     * @param qsos List of QSOs to export
     * @param contest Contest instance for format guidance
     * @return Cabrillo-formatted text
     */
    QString generateCabrillo(const QList<QSO>& qsos, ContestBase* contest);

    /**
     * Get the last error message
     *
     * @return Error message from last export operation
     */
    QString lastError() const { return m_lastError; }

private:
    // Station information
    QString m_callsign;
    QString m_gridSquare;
    QString m_name;
    QString m_address;
    QString m_city;
    QString m_stateProvince;
    QString m_postalCode;
    QString m_country;
    QString m_email;

    // Category information
    QString m_categoryAssisted = "NON-ASSISTED";
    QString m_categoryBand = "ALL";
    QString m_categoryMode = "MIXED";
    QString m_categoryOperator = "SINGLE-OP";
    QString m_categoryPower = "LOW";
    QString m_categoryStation = "FIXED";
    QString m_categoryTime = "";
    QString m_categoryTransmitter = "ONE";
    QString m_categoryOverlay = "";

    // Other information
    int m_claimedScore = 0;
    QString m_operators;
    QString m_club;
    QString m_lastError;

    /**
     * Generate Cabrillo header
     *
     * Analyzes QSO list to count QSOs per operator and sorts operators
     * by descending QSO count for the OPERATORS field.
     */
    QString generateHeader(ContestBase* contest, const QList<QSO>& qsos);

    /**
     * Format a single QSO as Cabrillo line
     */
    QString formatQSO(const QSO& qso, ContestBase* contest, int serialNumber);

    /**
     * Get contest name for Cabrillo CONTEST field
     */
    QString getContestName(ContestBase* contest);

    /**
     * Get Cabrillo mode string from QSO mode
     */
    QString getCabrilloMode(ModeType mode);

    /**
     * Get Cabrillo frequency string from Hz
     */
    QString getCabrilloFreq(freq_t frequency);
};

} // namespace TR4QT

#endif // CABRILLOEXPORTER_H
