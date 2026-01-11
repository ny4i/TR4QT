#include "NAQPBase.h"
#include "../models/QSO.h"
#include "../utils/ArrlSectionHelper.h"
#include <QRegularExpression>

namespace TR4QT {

bool NAQPBase::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() < 2) {
        errorMsg = "Exchange must be: Name State (e.g., 'JOHN FL')";
        return false;
    }

    // First part is name - any string is valid
    // Second part is state/province
    QString state = parts[1].toUpper();

    if (!Arrl::isValidSection(state)) {
        errorMsg = QString("Invalid state/province: %1").arg(state);
        return false;
    }

    return true;
}

void NAQPBase::parseReceivedExchange(const QString& exchange, QSO& qso) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() >= 2) {
        qso.operatorName = parts[0];
        qso.state = parts[1].toUpper();

        // If there are more parts, they're part of the name
        if (parts.size() > 2) {
            QStringList nameParts;
            for (int i = 0; i < parts.size() - 1; i++) {
                nameParts.append(parts[i]);
            }
            qso.operatorName = nameParts.join(" ");
            qso.state = parts.last().toUpper();
        }
    }

    // Format exchangeReceived (NAQP does not include RST)
    formatExchangeReceived(exchange, qso);
}

} // namespace TR4QT
