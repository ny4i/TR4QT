/**
 * QSOQueryService implementation
 *
 * Part of Phase 13 extraction from MainWindow.
 */

#include "QSOQueryService.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

QSet<QString> QSOQueryService::getWorkedCallsigns(const QList<QSO>& qsos) const {
    QSet<QString> workedCallsigns;

    for (const QSO& qso : qsos) {
        workedCallsigns.insert(qso.callsign.toUpper());
    }

    return workedCallsigns;
}

QList<BandType> QSOQueryService::getWorkedBandsForCallsign(const QList<QSO>& qsos,
                                                           const QString& callsign) const {
    QList<BandType> workedBands;

    if (callsign.isEmpty()) {
        return workedBands;
    }

    for (const QSO& qso : qsos) {
        if (qso.callsign.compare(callsign, Qt::CaseInsensitive) == 0) {
            if (!workedBands.contains(qso.band)) {
                workedBands.append(qso.band);
            }
        }
    }

    return workedBands;
}

QList<BandType> QSOQueryService::getWorkedBandsForMultiplier(const QList<QSO>& qsos,
                                                             const QString& multValue,
                                                             MultiplierType type,
                                                             ContestBase* contest) const {
    QList<BandType> workedBands;

    if (!contest || multValue.isEmpty()) {
        return workedBands;
    }

    for (const QSO& qso : qsos) {
        QString qsoMultValue = contest->getMultiplierValue(qso, type, QStringList());

        if (qsoMultValue.compare(multValue, Qt::CaseInsensitive) == 0) {
            if (!workedBands.contains(qso.band)) {
                workedBands.append(qso.band);
            }
        }
    }

    return workedBands;
}

int QSOQueryService::countQSOsInTimeWindow(const QList<QSO>& qsos,
                                            const QDateTime& startTime,
                                            const QDateTime& endTime) const {
    int count = 0;

    for (const QSO& qso : qsos) {
        if (qso.timestamp >= startTime && qso.timestamp <= endTime) {
            count++;
        }
    }

    return count;
}

int QSOQueryService::calculateRate(const QList<QSO>& qsos, int lookbackCount) const {
    if (qsos.size() < 2) {
        return 0;
    }

    int lookback = qMin(lookbackCount, qsos.size());
    const QSO& firstQSO = qsos.at(qsos.size() - lookback);
    const QSO& lastQSO = qsos.at(qsos.size() - 1);

    qint64 periodSecs = firstQSO.timestamp.secsTo(lastQSO.timestamp);
    if (periodSecs > 0) {
        return (lookback - 1) * 3600 / periodSecs;
    }

    return 0;
}

}  // namespace TR4QT
