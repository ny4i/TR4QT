#include <QTest>
#include <QDate>
#include "../src/contests/ContestMetadata.h"

using namespace TR4QT;

class TestFloatingDates : public QObject {
    Q_OBJECT

private slots:
    void test_4th_full_weekend_january_2026() {
        // Winter Field Day - 4th full weekend of January
        FloatingDate fd(1, "4th full weekend");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 1, 24));  // Should be Saturday Jan 24, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_4th_full_weekend_june_2026() {
        // ARRL Field Day - 4th full weekend of June
        FloatingDate fd(6, "4th full weekend");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 6, 27));  // Should be Saturday Jun 27, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_4th_full_weekend_october_2026() {
        // CQ WW SSB - 4th full weekend of October
        FloatingDate fd(10, "4th full weekend");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 10, 24));  // Should be Saturday Oct 24, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_last_full_weekend_november_2026() {
        // CQ WW CW - Last full weekend of November
        FloatingDate fd(11, "Last full weekend");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 11, 28));  // Should be Saturday Nov 28, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_4th_saturday_march_2026() {
        // CQ WPX Phone - 4th Saturday of March
        FloatingDate fd(3, "4th Saturday");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 3, 28));  // Should be Saturday Mar 28, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_2nd_saturday_january_2026() {
        // NAQP CW - 2nd Saturday of January
        FloatingDate fd(1, "2nd Saturday");
        QDate result = fd.calculateNextOccurrence(QDate(2026, 1, 1));

        QCOMPARE(result, QDate(2026, 1, 10));  // Should be Saturday Jan 10, 2026
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }

    void test_next_year_rollover() {
        // Test that if we're past the date, it calculates next year
        FloatingDate fd(1, "4th full weekend");  // January
        QDate result = fd.calculateNextOccurrence(QDate(2026, 12, 1));  // December

        QCOMPARE(result.year(), 2027);  // Should roll to 2027
        QCOMPARE(result.month(), 1);    // January
        QCOMPARE(result.dayOfWeek(), Qt::Saturday);
    }
};

QTEST_MAIN(TestFloatingDates)
#include "test_floating_dates.moc"
