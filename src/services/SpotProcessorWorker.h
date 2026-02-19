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

#ifndef SPOTPROCESSORWORKER_H
#define SPOTPROCESSORWORKER_H

#include <QObject>
#include <QThread>
#include <QColor>
#include <QList>
#include <QSqlDatabase>
#include "../core/Types.h"
#include "../ui/widgets/BandMapWidget.h"
#include "../utils/CountryFile.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

/**
 * Format range for character-based text formatting in DX cluster display.
 * Shared between SpotProcessorWorker and DXClusterWindow.
 */
struct SpotFormatRange {
    int start;
    int length;
    QColor color;
    bool bold;
};

/**
 * Fully processed spot ready for display on the main thread.
 * Contains everything needed: formatted text for DX cluster window
 * and enriched Spot struct for band map.
 */
struct ProcessedSpot {
    // For DX cluster text display
    QString displayText;
    QList<SpotFormatRange> formats;
    bool isSplit{false};
    double spotFrequency{0};      // Hz (TX frequency)
    double listenFrequency{0};    // Hz (RX frequency, 0 if not split)
    QString callsign;

    // For band map
    Spot bandMapSpot;
};

/**
 * Worker that processes DX cluster spots off the main thread.
 *
 * Runs in a QThread. Receives raw spot data, does all heavy work:
 * - SQLite dupe/multiplier queries (own DB connection)
 * - Country/zone lookup
 * - LOTW user check
 * - Split frequency parsing
 * - Text formatting with colors
 *
 * Emits processed results back to the main thread for display.
 */
class SpotProcessorWorker : public QObject {
    Q_OBJECT

public:
    explicit SpotProcessorWorker(QObject* parent = nullptr);
    ~SpotProcessorWorker() override;

    /**
     * Initialize database connections (must be called from worker thread)
     */
    void initDatabase(const QString& contestDbPath, const QString& globalDbPath);

    /**
     * Update contest context for dupe/multiplier checking.
     * Thread-safe: copies the data needed, doesn't hold pointers.
     */
    void setContestContext(int contestDbId,
                           const QList<MultiplierDefinition>& multDefs);

public slots:
    /**
     * Process a raw spot (called from worker thread via queued connection)
     */
    void processSpot(const QString& callsign, double frequency,
                     const QString& spotter, const QString& comment,
                     const QString& timestamp);

signals:
    /**
     * Emitted when a spot has been fully processed
     */
    void spotProcessed(const TR4QT::ProcessedSpot& result);

private:
    /**
     * Determine spot color based on dupe/multiplier status
     */
    QColor getSpotColor(const QString& callsign, double frequency);

    /**
     * Parse split frequency from comment (QSX/UP/DOWN)
     */
    double parseSplitInfo(const QString& comment, double spotFrequency);

    /**
     * Check LOTW user status
     */
    bool checkLotwUser(const QString& callsign);

    /**
     * Parse QSX frequency from comment
     */
    freq_t parseQSX(const QString& comment, freq_t spotFrequency);

    /**
     * Parse UP offset from comment
     */
    freq_t parseUP(const QString& comment, freq_t spotFrequency);

    /**
     * Extract multiplier value from QSO data for a given type
     */
    static QString extractMultiplierValue(const QSO& qso, MultiplierType type);

    // Worker-thread database connections
    QSqlDatabase m_contestDb;
    QSqlDatabase m_globalDb;
    QString m_contestDbPath;
    QString m_globalDbPath;

    // Contest context (set from main thread, read from worker)
    int m_contestDbId{-1};
    QList<MultiplierDefinition> m_multDefs;

    // Country file (worker's own instance)
    CountryFile m_countryFile;

    // Spot row counter for alternating backgrounds
    int m_spotRowCount{0};

    static constexpr const char* CONTEST_CONN_NAME = "tr4qt_spot_worker";
    static constexpr const char* GLOBAL_CONN_NAME = "tr4qt_spot_worker_global";
};

} // namespace TR4QT

Q_DECLARE_METATYPE(TR4QT::ProcessedSpot)

#endif // SPOTPROCESSORWORKER_H
