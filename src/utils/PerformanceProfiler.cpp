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

#include "PerformanceProfiler.h"
#include <QMutexLocker>
#include <QTextStream>

namespace TR4QT {

void PerformanceProfiler::recordTiming(const QString& radioType, const QString& functionName, qint64 elapsedMs)
{
    QMutexLocker locker(&m_mutex);

    QString key = makeKey(radioType, functionName);

    if (!m_stats.contains(key)) {
        FunctionStats stats;
        stats.functionName = functionName;
        stats.radioType = radioType;
        m_stats[key] = stats;
    }

    m_stats[key].addSample(elapsedMs);
}

FunctionStats PerformanceProfiler::getStats(const QString& radioType, const QString& functionName) const
{
    QMutexLocker locker(&m_mutex);

    QString key = makeKey(radioType, functionName);
    return m_stats.value(key);
}

QVector<FunctionStats> PerformanceProfiler::getAllStats() const
{
    QMutexLocker locker(&m_mutex);

    QVector<FunctionStats> result;
    for (const auto& stats : m_stats.values()) {
        result.append(stats);
    }
    return result;
}

void PerformanceProfiler::clear()
{
    QMutexLocker locker(&m_mutex);
    m_stats.clear();
}

QString PerformanceProfiler::generateReport() const
{
    QMutexLocker locker(&m_mutex);

    QString report;
    QTextStream stream(&report);

    stream << "================================================================================\n";
    stream << "                    RADIO PERFORMANCE COMPARISON REPORT\n";
    stream << "================================================================================\n\n";

    // Group by function name
    QHash<QString, QVector<FunctionStats>> byFunction;
    for (const auto& stats : m_stats.values()) {
        byFunction[stats.functionName].append(stats);
    }

    // Sort function names
    QStringList functions = byFunction.keys();
    functions.sort();

    for (const QString& funcName : functions) {
        stream << "Function: " << funcName << "\n";
        stream << QString(80, '-') << "\n";

        const auto& statsForFunc = byFunction[funcName];

        // Find K4Direct and Hamlib stats
        const FunctionStats* k4Stats = nullptr;
        const FunctionStats* hamlibStats = nullptr;

        for (const auto& stats : statsForFunc) {
            if (stats.radioType == "K4Direct") {
                k4Stats = &stats;
            } else if (stats.radioType == "Hamlib") {
                hamlibStats = &stats;
            }
        }

        // Display stats
        if (k4Stats) {
            stream << QString("  K4 Direct:  %1 calls, avg: %2 ms, min: %3 ms, max: %4 ms\n")
                      .arg(k4Stats->callCount, 6)
                      .arg(k4Stats->avgTime(), 8, 'f', 2)
                      .arg(k4Stats->minTime, 6)
                      .arg(k4Stats->maxTime, 6);
        } else {
            stream << "  K4 Direct:  No data\n";
        }

        if (hamlibStats) {
            stream << QString("  Hamlib:     %1 calls, avg: %2 ms, min: %3 ms, max: %4 ms\n")
                      .arg(hamlibStats->callCount, 6)
                      .arg(hamlibStats->avgTime(), 8, 'f', 2)
                      .arg(hamlibStats->minTime, 6)
                      .arg(hamlibStats->maxTime, 6);
        } else {
            stream << "  Hamlib:     No data\n";
        }

        // Calculate speedup if both exist
        if (k4Stats && hamlibStats && hamlibStats->avgTime() > 0) {
            double speedup = hamlibStats->avgTime() / k4Stats->avgTime();
            QString winner = speedup > 1.0 ? "K4 Direct" : "Hamlib";
            stream << QString("  Speedup:    %1x faster with %2\n")
                      .arg(qAbs(speedup), 0, 'f', 2)
                      .arg(winner);
        }

        stream << "\n";
    }

    stream << "================================================================================\n";

    return report;
}

} // namespace TR4QT
