#include "ContestMetadata.h"
#include <QRegularExpression>
#include <QDate>

namespace TR4QT {

/**
 * Calculate the next occurrence of a floating date
 * Supports rules like:
 * - "2nd Saturday", "3rd Saturday", "4th Saturday"
 * - "Last Saturday", "Last Sunday"
 * - "Last full weekend" (starts Friday)
 * - "1st full weekend", "2nd full weekend", etc.
 */
QDate FloatingDate::calculateNextOccurrence(const QDate& fromDate) const {
    if (!isValid()) {
        return QDate();  // Invalid
    }

    // Parse the rule to extract ordinal and day/weekend
    QRegularExpression re(R"((1st|2nd|3rd|4th|Last)\s+(Saturday|Sunday|full weekend))",
                         QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(rule);

    if (!match.hasMatch()) {
        return QDate();  // Rule not recognized
    }

    QString ordinal = match.captured(1).toLower();
    QString target = match.captured(2).toLower();

    // Start with the first day of the target month
    int year = fromDate.year();
    QDate firstOfMonth(year, month, 1);

    // If we're past this month's occurrence, look at next year
    QDate occurrence = calculateOccurrenceInMonth(firstOfMonth, ordinal, target);
    if (occurrence.isValid() && occurrence <= fromDate) {
        // Try next year
        firstOfMonth = QDate(year + 1, month, 1);
        occurrence = calculateOccurrenceInMonth(firstOfMonth, ordinal, target);
    }

    return occurrence;
}

/**
 * Calculate occurrence within a specific month
 */
QDate FloatingDate::calculateOccurrenceInMonth(const QDate& firstOfMonth,
                                              const QString& ordinal,
                                              const QString& target) const {
    if (target == "full weekend") {
        return calculateFullWeekend(firstOfMonth, ordinal);
    } else {
        // Single day (Saturday or Sunday)
        Qt::DayOfWeek targetDay = (target == "saturday") ? Qt::Saturday : Qt::Sunday;
        return calculateWeekday(firstOfMonth, ordinal, targetDay);
    }
}

/**
 * Calculate Nth occurrence of a weekday in a month
 * Examples: "2nd Saturday", "3rd Sunday", "Last Saturday"
 */
QDate FloatingDate::calculateWeekday(const QDate& firstOfMonth,
                                    const QString& ordinal,
                                    Qt::DayOfWeek targetDay) const {
    if (ordinal == "last") {
        // Find last occurrence by starting from end of month
        QDate lastOfMonth(firstOfMonth.year(), firstOfMonth.month(),
                         firstOfMonth.daysInMonth());

        // Walk backwards to find the last occurrence of targetDay
        while (lastOfMonth.dayOfWeek() != targetDay) {
            lastOfMonth = lastOfMonth.addDays(-1);
        }
        return lastOfMonth;
    } else {
        // Find Nth occurrence (1st, 2nd, 3rd, 4th)
        int targetOccurrence = (ordinal == "1st") ? 1 :
                              (ordinal == "2nd") ? 2 :
                              (ordinal == "3rd") ? 3 :
                              (ordinal == "4th") ? 4 : 0;

        if (targetOccurrence == 0) {
            return QDate();  // Invalid ordinal
        }

        QDate current = firstOfMonth;
        int occurrenceCount = 0;

        // Walk through the month
        while (current.month() == firstOfMonth.month()) {
            if (current.dayOfWeek() == targetDay) {
                occurrenceCount++;
                if (occurrenceCount == targetOccurrence) {
                    return current;
                }
            }
            current = current.addDays(1);
        }

        return QDate();  // Not found (month doesn't have that many occurrences)
    }
}

/**
 * Calculate "full weekend" (Saturday-Sunday)
 * Ham radio contests use "full weekend" to mean Saturday-Sunday.
 * A "full weekend" starts on Saturday, and both Saturday and Sunday
 * must fall within the target month. If the weekend would extend into the
 * next month, use the previous weekend instead.
 *
 * Example: If a month ends on Saturday, the "4th full weekend" would be
 * the previous weekend, NOT a partial weekend extending into next month.
 */
QDate FloatingDate::calculateFullWeekend(const QDate& firstOfMonth,
                                        const QString& ordinal) const {
    if (ordinal == "last") {
        // Last full weekend = last Saturday of month where Sunday is also in month
        QDate lastOfMonth(firstOfMonth.year(), firstOfMonth.month(),
                         firstOfMonth.daysInMonth());

        // Find last Saturday
        while (lastOfMonth.dayOfWeek() != Qt::Saturday) {
            lastOfMonth = lastOfMonth.addDays(-1);
        }

        QDate saturday = lastOfMonth;
        QDate sunday = saturday.addDays(1);  // Saturday + 1 = Sunday

        // Verify the complete weekend is within the month
        if (sunday.month() != firstOfMonth.month()) {
            // Weekend extends into next month, use previous weekend
            saturday = saturday.addDays(-7);
        }

        return saturday;
    } else {
        // Nth full weekend = Nth Saturday where complete weekend is in month
        int targetOccurrence = (ordinal == "1st") ? 1 :
                              (ordinal == "2nd") ? 2 :
                              (ordinal == "3rd") ? 3 :
                              (ordinal == "4th") ? 4 : 0;

        if (targetOccurrence == 0) {
            return QDate();
        }

        QDate current = firstOfMonth;
        int saturdayCount = 0;

        while (current.month() == firstOfMonth.month()) {
            if (current.dayOfWeek() == Qt::Saturday) {
                saturdayCount++;
                if (saturdayCount == targetOccurrence) {
                    // Found the Nth Saturday, verify complete weekend is in month
                    QDate saturday = current;
                    QDate sunday = saturday.addDays(1);  // Saturday + 1 = Sunday

                    if (sunday.month() != firstOfMonth.month()) {
                        // Weekend extends into next month, use previous weekend
                        saturday = saturday.addDays(-7);
                    }

                    return saturday;
                }
            }
            current = current.addDays(1);
        }

        return QDate();
    }
}

} // namespace TR4QT
