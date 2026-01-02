#include "GeneralLoggingContest.h"
#include "ContestRegistry.h"
#include "ContestMetadata.h"
#include "../models/QSO.h"
#include <QRegularExpression>

namespace TR4QT {

ContestMetadata GeneralLoggingContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "GENERAL";
    meta.displayName = "General Logging";
    meta.shortName = "General";
    meta.supportedModes = {ModeType::CW, ModeType::USB, ModeType::LSB, ModeType::RTTY,
                           ModeType::FT8, ModeType::FT4, ModeType::PSK, ModeType::FM,
                           ModeType::AM, ModeType::DATA};
    meta.hasSeparateContests = false;  // Single mode for all types of operating

    meta.wa7bnmIdCW = 0;      // Not in WA7BNM contest calendar
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = "GENERAL";
    meta.cabrilloNameSSB = "GENERAL";
    meta.cabrilloNameMixed = "GENERAL";

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = "";

    meta.schedule = "Anytime";
    meta.website = "";
    meta.description = "Simple QSO logging without contest scoring. Optional RS/RST exchange plus free-form comments.";

    return meta;
}

ContestBase* GeneralLoggingContest::create(ModeType mode, const StationInfo& myStation) {
    return new GeneralLoggingContest(mode, myStation);
}

GeneralLoggingContest::GeneralLoggingContest(ModeType mode, const StationInfo& myStation)
    : ContestBase(myStation)
    , m_mode(mode)
{
}

QList<ExchangeField> GeneralLoggingContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (optional)
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = "RST (optional)";
    rst.autoFill = false;
    rst.maxLength = 3;
    fields.append(rst);

    // Comments (any additional exchange information)
    ExchangeField comments;
    comments.name = "Comments";
    comments.hint = "Any exchange info";
    comments.autoFill = false;
    comments.maxLength = 100;
    fields.append(comments);

    return fields;
}

QList<ExchangeField> GeneralLoggingContest::getSentExchangeFields() const {
    QList<ExchangeField> fields;

    // RST (optional)
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = "RST (optional)";
    rst.autoFill = false;
    rst.maxLength = 3;
    fields.append(rst);

    // Comments (any additional exchange information)
    ExchangeField comments;
    comments.name = "Comments";
    comments.hint = "Any exchange info";
    comments.autoFill = false;
    comments.maxLength = 100;
    fields.append(comments);

    return fields;
}

QList<TableColumn> GeneralLoggingContest::getTableColumns() const {
    return {
        TableColumn("RST", "RST", 60, TableColumn::Alignment::Center),
        TableColumn("Comments", "Comments", 200, TableColumn::Alignment::Left)
    };
}

QString GeneralLoggingContest::formatSentExchange(int serialNumber, const QString& rst) const {
    Q_UNUSED(serialNumber);
    Q_UNUSED(rst);
    return "{RST} {COMMENTS}";  // Replaced by actual values from settings
}

bool GeneralLoggingContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    Q_UNUSED(exchange);
    Q_UNUSED(errorMsg);
    // All exchanges are valid for general logging - it's free-form
    return true;
}

void GeneralLoggingContest::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QString trimmed = exchange.trimmed();

    if (trimmed.isEmpty()) {
        return;  // Empty exchange is fine
    }

    // Try to parse RST from the beginning if it looks like an RST
    // RST format: 2-3 digits (59, 599, etc.) or RS format (59)
    QRegularExpression rstPattern("^(\\d{2,3})\\s*(.*)$");
    QRegularExpressionMatch match = rstPattern.match(trimmed);

    if (match.hasMatch()) {
        QString rst = match.captured(1);
        QString remainder = match.captured(2);

        // Validate RST looks reasonable (not something like "999" or "12345")
        bool validRst = false;
        if (rst.length() == 2) {
            // RS format: first digit 1-5, second digit 1-9
            int r = rst.mid(0, 1).toInt();
            int s = rst.mid(1, 1).toInt();
            validRst = (r >= 1 && r <= 5) && (s >= 1 && s <= 9);
        } else if (rst.length() == 3) {
            // RST format: first two digits as RS, third digit (tone) 1-9
            int r = rst.mid(0, 1).toInt();
            int s = rst.mid(1, 1).toInt();
            int t = rst.mid(2, 1).toInt();
            validRst = (r >= 1 && r <= 5) && (s >= 1 && s <= 9) && (t >= 1 && t <= 9);
        }

        if (validRst) {
            qso.rstReceived = rst;
            if (!remainder.isEmpty()) {
                qso.notes = remainder.trimmed();
            }
        } else {
            // Doesn't look like valid RST, treat entire exchange as notes
            qso.notes = trimmed;
        }
    } else {
        // No RST pattern found, entire exchange is notes
        qso.notes = trimmed;
    }
}

} // namespace TR4QT

// Register the contest
REGISTER_CONTEST(TR4QT::GeneralLoggingContest, "GENERAL")
