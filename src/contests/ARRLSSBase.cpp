#include "ARRLSSBase.h"
#include "../models/QSO.h"
#include "../exchanges/SmartExchangeParser.h"
#include "../utils/ArrlSectionHelper.h"
#include <QRegularExpression>

namespace TR4QT {

QList<ExchangeField> ARRLSSBase::getReceivedExchangeFields() const {
    return {
        {"Serial", "Serial number", false, false},
        {"Precedence", "Q/A/B/M/U/S", false, false},
        {"Check", "Last 2 digits of year", false, false},
        {"Section", "ARRL Section", false, false}
    };
}

QList<ExchangeField> ARRLSSBase::getSentExchangeFields() const {
    return {
        {"Serial", "Serial number", true, true},  // Auto-filled
        {"Precedence", "Your precedence", false, false},
        {"Check", "Your check", false, false},
        {"Section", "Your section", false, false}
    };
}

QList<TableColumn> ARRLSSBase::getTableColumns() const {
    return {
        TableColumn("Serial", "#", 50, TableColumn::Alignment::Right),
        TableColumn("Precedence", "Prec", 50, TableColumn::Alignment::Center),
        TableColumn("Check", "Chk", 50, TableColumn::Alignment::Center),
        TableColumn("Section", "QTH", 60, TableColumn::Alignment::Left)
    };
}

QString ARRLSSBase::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(rst);  // SS doesn't use RST in exchange
    // Example: "123 A 95 WMA"
    // User must configure precedence, check, and section in contest setup
    return QString::number(serialNumber) + " [PREC] [CHK] [SEC]";
}

bool ARRLSSBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() != 4) {
        errorMsg = "Exchange must be: Serial# Precedence Check Section";
        return false;
    }

    // Validate serial number (1-9999)
    bool ok;
    int serial = parts[0].toInt(&ok);
    if (!ok || serial < 1 || serial > 9999) {
        errorMsg = "Invalid serial number (must be 1-9999)";
        return false;
    }

    // Validate precedence
    if (!isValidPrecedence(parts[1].at(0))) {
        errorMsg = "Invalid precedence (must be Q, A, B, M, U, or S)";
        return false;
    }

    // Validate check (00-99)
    if (!isValidCheck(parts[2])) {
        errorMsg = "Invalid check (must be 00-99)";
        return false;
    }

    // Validate section
    if (!Arrl::isValidSection(parts[3])) {
        errorMsg = "Invalid ARRL section: " + parts[3];
        return false;
    }

    return true;
}

void ARRLSSBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    // Simple parsing: Serial Prec Check Section
    if (parts.size() >= 4) {
        qso.serialNumberReceived = parts[0].toInt();
        qso.precedence = parts[1].toUpper();
        qso.check = parts[2];
        qso.arrlSection = parts[3].toUpper();
    }

    // Format exchangeReceived (Sweepstakes does not include RST)
    formatExchangeReceived(exchange, qso);
}

int ARRLSSBase::calculateQSOPoints(
    const QSO& qso,
    const StationInfo& myStation) const
{
    Q_UNUSED(qso);
    Q_UNUSED(myStation);

    // All QSOs are worth 2 points
    return 2;
}

int ARRLSSBase::calculateTotalScore(
    int totalQSOPoints,
    const QMap<MultiplierType, int>& multiplierCounts) const
{
    // Score = QSO Points × Sections Worked
    int sections = multiplierCounts.value(MultiplierType::Section, 0);
    return totalQSOPoints * sections;
}

QList<MultiplierDefinition> ARRLSSBase::getMultiplierTypes() const {
    return {
        {MultiplierType::Section, MultiplierScope::AllBands, "ARRL Sections"}
    };
}

QString ARRLSSBase::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    Q_UNUSED(alreadyWorkedValues);

    if (multType == MultiplierType::Section) {
        // Extract section from dedicated field
        QString section = qso.arrlSection.toUpper();
        if (!section.isEmpty() && Arrl::isValidSection(section)) {
            return section;
        }
    }

    return QString();
}

bool ARRLSSBase::isValidPrecedence(QChar prec) {
    QChar upper = prec.toUpper();
    return (upper == 'Q' || upper == 'A' || upper == 'B' ||
            upper == 'M' || upper == 'U' || upper == 'S');
}

bool ARRLSSBase::isValidCheck(const QString& check) {
    bool ok;
    int checkNum = check.toInt(&ok);
    return ok && checkNum >= 0 && checkNum <= 99 && check.length() == 2;
}

} // namespace TR4QT
