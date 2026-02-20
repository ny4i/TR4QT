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

#include <QTest>
#include <QSignalSpy>
#include <QColor>
#include "services/WSJTXHighlightWorker.h"

using namespace TR4QT;

/**
 * Tests for WSJTXHighlightWorker dupe/multiplier cache logic.
 *
 * Tests run without a database (in-memory cache only).
 * We populate the cache manually and verify checkCallsign decisions.
 */
class TestWSJTXHighlightWorker : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<WSJTXHighlightDecision>("WSJTXHighlightDecision");
    }

    void testNoDupeWhenCacheEmpty()
    {
        WSJTXHighlightWorker worker;

        // Set contest context with no DB (caches remain empty)
        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);
        worker.checkCallsign("K3LR", 14074000, "FT8");

        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QCOMPARE(decision.callsign, QString("K3LR"));
        QVERIFY(!decision.isDupe);
        QVERIFY(!decision.isMultiplier);
    }

    void testDupeDetectedPerBandMode()
    {
        WSJTXHighlightWorker worker;

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        // Add K3LR as worked on 20m FT8
        worker.addWorkedCallsign("K3LR", "20M", "FT8");

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);

        // Same call, same band, same mode → dupe
        worker.checkCallsign("K3LR", 14074000, "FT8");
        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(decision.isDupe);
        QVERIFY(decision.bgColor.isValid());
    }

    void testNotDupeOnDifferentBand()
    {
        WSJTXHighlightWorker worker;

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        // K3LR worked on 20m FT8
        worker.addWorkedCallsign("K3LR", "20M", "FT8");

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);

        // Same call on 40m → NOT dupe
        worker.checkCallsign("K3LR", 7074000, "FT8");
        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(!decision.isDupe);
    }

    void testDupeAllBand()
    {
        WSJTXHighlightWorker worker;

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::AllBand);

        // K3LR worked on 20m
        worker.addWorkedCallsign("K3LR", "*", "*");

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);

        // Same call on ANY band → dupe
        worker.checkCallsign("K3LR", 7074000, "FT8");
        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(decision.isDupe);
    }

    void testNoHighlightWithoutContest()
    {
        WSJTXHighlightWorker worker;
        // No setContestContext called — contestDbId = -1

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);
        worker.checkCallsign("K3LR", 14074000, "FT8");

        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(!decision.isDupe);
        QVERIFY(!decision.isMultiplier);
    }

    void testRejectedModeSkipped()
    {
        WSJTXHighlightWorker worker;

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);

        // WSPR mode should be rejected
        worker.checkCallsign("K3LR", 14074000, "WSPR");
        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(!decision.isDupe);
        QVERIFY(!decision.isMultiplier);
    }

    void testCustomColors()
    {
        WSJTXHighlightWorker worker;

        QColor customDupeBg(128, 0, 0);
        QColor customDupeFg(255, 255, 0);
        QColor customMultBg(0, 255, 0);
        QColor customMultFg(0, 0, 0);

        worker.setColors(customDupeBg, customDupeFg, customMultBg, customMultFg);

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        worker.addWorkedCallsign("K3LR", "20M", "FT8");

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);
        worker.checkCallsign("K3LR", 14074000, "FT8");

        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(decision.isDupe);
        QCOMPARE(decision.bgColor, customDupeBg);
        QCOMPARE(decision.fgColor, customDupeFg);
    }

    void testCaseInsensitiveDupe()
    {
        WSJTXHighlightWorker worker;

        QList<MultiplierDefinition> multDefs;
        worker.setContestContext(1, multDefs, DuplicateCheckingRule::PerBandMode);

        // Add lowercase
        worker.addWorkedCallsign("k3lr", "20M", "FT8");

        QSignalSpy spy(&worker, &WSJTXHighlightWorker::highlightDecision);

        // Check uppercase → should still detect dupe
        worker.checkCallsign("K3LR", 14074000, "FT8");
        QCOMPARE(spy.count(), 1);
        auto decision = spy.at(0).at(0).value<WSJTXHighlightDecision>();
        QVERIFY(decision.isDupe);
    }
};

QTEST_GUILESS_MAIN(TestWSJTXHighlightWorker)
#include "test_wsjtx_highlight_worker.moc"
