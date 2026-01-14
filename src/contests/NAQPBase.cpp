#include "NAQPBase.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include "../exchanges/SmartExchangeParser.h"
#include <QRegularExpression>

namespace TR4QT {

bool NAQPBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() < 2) {
        errorMsg = "Exchange must be: Name State (e.g., 'JOHN FL' or 'FL JOHN')";
        return false;
    }

    // Use smart parser to extract fields (order-agnostic)
    QSO tempQSO;
    parseReceivedExchange(exchange, tempQSO);

    // Validate we got both fields
    if (tempQSO.operatorName.isEmpty()) {
        errorMsg = "Missing name in exchange";
        return false;
    }

    if (tempQSO.state.isEmpty()) {
        errorMsg = "Missing state/province in exchange";
        return false;
    }

    // Validate state is in the allowed list (US states + Canadian provinces)
    // NOTE: This uses looksLikeState() which checks getStatesAndProvinces(),
    // NOT isValidSection() which checks ARRL sections (EMA, WMA, etc.)
    if (!SmartExchangeParser::looksLikeState(tempQSO.state)) {
        errorMsg = QString("Invalid state/province: %1").arg(tempQSO.state);
        return false;
    }

    return true;
}

void NAQPBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    // Use SmartExchangeParser for order-agnostic field detection
    // This allows both "JOHN FL" and "FL JOHN" to parse correctly
    QList<ExchangeField> expectedFields = getReceivedExchangeFields();
    QMap<QString, QString> parsed = SmartExchangeParser::parse(
        exchange,
        expectedFields,
        const_cast<NAQPBase*>(this)
    );

    // Populate QSO fields from parsed result
    qso.operatorName = parsed.value("Name");
    qso.state = parsed.value("State");

    // Format exchangeReceived (NAQP does not include RST)
    formatExchangeReceived(exchange, qso);
}

} // namespace TR4QT
