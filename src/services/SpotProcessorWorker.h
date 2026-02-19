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
#include <QMap>
#include <QSet>
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
 * Configuration snapshot passed from the main thread to the worker.
 * Captures all settings the worker needs so it never touches AppSettings directly.
 */
struct SpotProcessorConfig {
    // Colors for spot classification
    QColor dupeColor{128, 128, 128};       // Default gray
    QColor multiplierColor{255, 0, 0};     // Default red
    bool lotwLookupEnabled{false};

    // Display colors (band colors for frequency formatting)
    QColor band160mColor{102, 51, 153};
    QColor band80mColor{153, 76, 0};
    QColor band40mColor{204, 0, 102};
    QColor band20mColor{0, 102, 204};
    QColor band15mColor{0, 153, 0};
    QColor band10mColor{204, 102, 0};
    QColor bandDefaultColor{102, 102, 102};

    // Text formatting colors
    QColor spotterColor{102, 102, 102};    // Gray for spotter callsign
    QColor timestampColor{153, 153, 153};  // Light gray for timestamp
    QColor commentColor{51, 51, 51};       // Dark gray for comment
    QColor splitIndicatorColor{0, 206, 209}; // Cyan for split indicator
    QColor defaultCallColor{0, 0, 0};      // Black for unclassified spots

    // Alternating row backgrounds
    QColor evenRowBackground{255, 255, 255};
    QColor oddRowBackground{248, 248, 248};
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
 * All configuration (colors, settings) is passed in via SpotProcessorConfig
 * from the main thread - the worker never accesses AppSettings directly.
 *
 * Worked callsigns and multipliers are cached in memory to minimize SQL queries.
 * The cache is rebuilt when the contest context changes, and individual entries
 * are added via addWorkedCallsign() when a QSO is logged.
 */
class SpotProcessorWorker : public QObject {
    Q_OBJECT

public:
    explicit SpotProcessorWorker(QObject* parent = nullptr);
    ~SpotProcessorWorker() override;

    /**
     * Initialize database connections and load country file.
     * Must be called from worker thread.
     * @param countryFilePath Path to cty.dat (captured from main thread)
     */
    void initDatabase(const QString& contestDbPath, const QString& globalDbPath,
                      const QString& countryFilePath);

    /**
     * Update display/color configuration.
     * Thread-safe: called via queued connection from main thread.
     */
    void setConfig(const SpotProcessorConfig& config);

    /**
     * Update contest context for dupe/multiplier checking.
     * Rebuilds in-memory caches from database.
     * Thread-safe: called via queued connection from main thread.
     */
    void setContestContext(int contestDbId,
                           const QList<MultiplierDefinition>& multDefs);

    /**
     * Add a single worked callsign to the dupe cache (called when QSO logged).
     * Avoids full cache rebuild for each new QSO.
     */
    void addWorkedCallsign(const QString& callsign, const QString& band, const QString& mode);

    /**
     * Add a single multiplier to the cache (called when new mult logged).
     */
    void addWorkedMultiplier(const QString& multType, const QString& multValue, const QString& band);

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
     * Determine spot color based on cached dupe/multiplier status
     */
    QColor getSpotColor(const QString& callsign, double frequency);

    /**
     * Parse split/QSX/UP/DOWN from comment, returning listen frequency in Hz.
     * Returns 0 if no split info found.
     */
    double parseSplitFrequency(const QString& comment, double spotFrequencyHz);

    /**
     * Check LOTW user status via cached global DB
     */
    bool checkLotwUser(const QString& callsign);

    /**
     * Get band color from config
     */
    QColor getBandColor(BandType band) const;

    /**
     * Extract multiplier value from QSO data for a given type
     */
    static QString extractMultiplierValue(const QSO& qso, MultiplierType type);

    /**
     * Rebuild dupe cache from database
     */
    void rebuildDupeCache();

    /**
     * Rebuild multiplier cache from database
     */
    void rebuildMultiplierCache();

    // Worker-thread database connections
    QSqlDatabase m_contestDb;
    QSqlDatabase m_globalDb;
    QString m_contestDbPath;
    QString m_globalDbPath;

    // Contest context (set from main thread, read from worker)
    int m_contestDbId{-1};
    QList<MultiplierDefinition> m_multDefs;

    // In-memory dupe cache: key = "CALLSIGN|BAND|MODE" for PerBandMode
    QSet<QString> m_dupeCache;

    // In-memory multiplier cache: multType -> (band -> set of values)
    // For AllBands scope, band key is empty string
    QMap<QString, QMap<QString, QSet<QString>>> m_multiplierCache;

    // Configuration snapshot (set from main thread, read from worker)
    SpotProcessorConfig m_config;

    // Country file (worker's own instance - required for thread safety)
    CountryFile m_countryFile;

    // Spot row counter for alternating backgrounds
    int m_spotRowCount{0};

    static constexpr const char* CONTEST_CONN_NAME = "tr4qt_spot_worker";
    static constexpr const char* GLOBAL_CONN_NAME = "tr4qt_spot_worker_global";

    // Display formatting constants
    static constexpr int SPLIT_INDICATOR_WIDTH = 2;
    static constexpr int SPOTTER_FIELD_WIDTH = 12;
    static constexpr int FREQUENCY_FIELD_WIDTH = 10;
    static constexpr int CALLSIGN_INDENT = 3;
    static constexpr int CALLSIGN_FIELD_WIDTH = 12;
    static constexpr int TIMESTAMP_FIELD_WIDTH = 5;  // "1234Z"
};

} // namespace TR4QT

Q_DECLARE_METATYPE(TR4QT::ProcessedSpot)
Q_DECLARE_METATYPE(TR4QT::SpotProcessorConfig)

#endif // SPOTPROCESSORWORKER_H
