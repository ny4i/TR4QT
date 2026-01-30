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

#ifndef PERFORMANCEPROFILER_H
#define PERFORMANCEPROFILER_H

// Conditional compilation for performance profiling
// Comment out this line to disable profiling (zero overhead)
// #define ENABLE_PERFORMANCE_PROFILING

#include <QString>
#include <QHash>
#include <QMutex>
#include <QElapsedTimer>
#include <QVector>

namespace TR4QT {

// Statistics for a profiled function
struct FunctionStats {
    QString functionName;
    QString radioType;  // "K4Direct" or "Hamlib"
    qint64 minTime{999999999};
    qint64 maxTime{0};
    qint64 totalTime{0};
    int callCount{0};

    double avgTime() const {
        return callCount > 0 ? static_cast<double>(totalTime) / callCount : 0.0;
    }

    void addSample(qint64 timeMs) {
        if (timeMs < minTime) minTime = timeMs;
        if (timeMs > maxTime) maxTime = timeMs;
        totalTime += timeMs;
        callCount++;
    }
};

// Global performance profiler (singleton)
class PerformanceProfiler {
public:
    static PerformanceProfiler& instance() {
        static PerformanceProfiler profiler;
        return profiler;
    }

    // Record a function timing
    void recordTiming(const QString& radioType, const QString& functionName, qint64 elapsedMs);

    // Get statistics for a specific function
    FunctionStats getStats(const QString& radioType, const QString& functionName) const;

    // Get all statistics
    QVector<FunctionStats> getAllStats() const;

    // Clear all statistics
    void clear();

    // Generate summary report
    QString generateReport() const;

private:
    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    mutable QMutex m_mutex;
    // Key: "RadioType::FunctionName", Value: Stats
    QHash<QString, FunctionStats> m_stats;

    QString makeKey(const QString& radioType, const QString& functionName) const {
        return radioType + "::" + functionName;
    }
};

// RAII timer for automatic function profiling
class ScopedTimer {
public:
    ScopedTimer(const QString& radioType, const QString& functionName)
        : m_radioType(radioType)
        , m_functionName(functionName)
    {
        m_timer.start();
    }

    ~ScopedTimer() {
        qint64 elapsed = m_timer.elapsed();
        PerformanceProfiler::instance().recordTiming(m_radioType, m_functionName, elapsed);
    }

private:
    QString m_radioType;
    QString m_functionName;
    QElapsedTimer m_timer;
};

// Convenience macro for timing functions
#ifdef ENABLE_PERFORMANCE_PROFILING
    #define PROFILE_FUNCTION(radioType) \
        ScopedTimer _profiler_timer(radioType, __FUNCTION__)
#else
    #define PROFILE_FUNCTION(radioType) do { } while(0)  // No-op when profiling disabled
#endif

} // namespace TR4QT

#endif // PERFORMANCEPROFILER_H
