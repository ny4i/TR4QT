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
