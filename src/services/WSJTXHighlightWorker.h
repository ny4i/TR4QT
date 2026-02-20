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

#ifndef WSJTXHIGHLIGHTWORKER_H
#define WSJTXHIGHLIGHTWORKER_H

#include <QColor>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include "../core/Types.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

/**
 * Highlight decision for a single callsign.
 */
struct WSJTXHighlightDecision {
    QString callsign;
    bool isDupe{false};
    bool isMultiplier{false};
    QColor bgColor;
    QColor fgColor;
};

/**
 * Worker that checks dupe/multiplier status for WSJT-X callsign highlighting.
 *
 * Modeled on SpotProcessorWorker — runs in a QThread owned by WSJTXController.
 * Has its own QSqlDatabase connection for thread safety.
 *
 * Maintains in-memory caches:
 *   - m_dupeCache: QSet<"CALLSIGN|BAND|MODE"> for quick dupe lookups
 *   - m_multiplierCache: multType → (band → values) for multiplier checks
 *
 * Caches are rebuilt on contest change and incrementally updated on QSO log.
 */
class WSJTXHighlightWorker : public QObject {
    Q_OBJECT

public:
    explicit WSJTXHighlightWorker(QObject* parent = nullptr);
    ~WSJTXHighlightWorker() override;

    /**
     * Initialize database connection. Must be called from worker thread.
     */
    void initDatabase(const QString& contestDbPath);

    /**
     * Update contest context — rebuilds caches.
     * Thread-safe: called via queued connection from main thread.
     */
    void setContestContext(int contestDbId,
                           const QList<MultiplierDefinition>& multDefs,
                           DuplicateCheckingRule dupeRule);

    /**
     * Add a single worked callsign to the dupe cache (called when QSO logged).
     */
    void addWorkedCallsign(const QString& callsign, const QString& band, const QString& mode);

    /**
     * Add a multiplier to the cache (called when new mult logged).
     */
    void addWorkedMultiplier(const QString& multType, const QString& multValue, const QString& band);

    /**
     * Set highlight colors (from ThemeManager, passed from main thread).
     */
    void setColors(const QColor& dupeBg, const QColor& dupeFg,
                   const QColor& multBg, const QColor& multFg);

public slots:
    /**
     * Check a callsign and emit highlight decision.
     * Called from main thread via queued signal.
     */
    void checkCallsign(const QString& callsign, quint64 frequencyHz, const QString& wsjtxMode);

signals:
    /**
     * Emitted with the highlighting result.
     */
    void highlightDecision(const TR4QT::WSJTXHighlightDecision& decision);

private:
    void rebuildDupeCache();
    void rebuildMultiplierCache();

    /**
     * Build dupe cache key from callsign/band/mode.
     */
    QString dupeKey(const QString& callsign, BandType band, ModeType mode) const;

    /**
     * Check if callsign is a dupe on the given band/mode.
     */
    bool isDupe(const QString& callsign, BandType band, ModeType mode) const;

    /**
     * Check if callsign provides a new multiplier.
     * Note: Without country lookup, this only checks callsign-based multipliers.
     * Full multiplier checking requires country data which is done at QSO log time.
     */
    bool isNewMultiplier(const QString& callsign, BandType band) const;

    // Database connection (worker thread only)
    QSqlDatabase m_contestDb;
    QString m_contestDbPath;

    // Contest context
    int m_contestDbId{-1};
    QList<MultiplierDefinition> m_multDefs;
    DuplicateCheckingRule m_dupeRule{DuplicateCheckingRule::PerBandMode};

    // In-memory caches (same pattern as SpotProcessorWorker)
    QSet<QString> m_dupeCache;  // "CALLSIGN|BAND|MODE"
    QMap<QString, QMap<QString, QSet<QString>>> m_multiplierCache;  // type → (band → values)

    // Highlight colors
    QColor m_dupeBg{255, 0, 0};        // Red background for dupes
    QColor m_dupeFg{255, 255, 255};    // White text for dupes
    QColor m_multBg{255, 255, 0};      // Yellow background for mults
    QColor m_multFg{0, 0, 0};          // Black text for mults

    static constexpr const char* DB_CONN_NAME = "tr4qt_wsjtx_highlight_worker";
};

} // namespace TR4QT

Q_DECLARE_METATYPE(TR4QT::WSJTXHighlightDecision)

#endif // WSJTXHIGHLIGHTWORKER_H
