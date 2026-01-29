#include "BandSummaryModel.h"
#include "../../core/Types.h"

namespace TR4QT {

// ============================================================================
// BandSummaryModel
// ============================================================================

BandSummaryModel::BandSummaryModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int BandSummaryModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_stats.size();
}

int BandSummaryModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant BandSummaryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_stats.size()) {
        return QVariant();
    }

    const BandModeStats& stats = m_stats[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColBand:
                return bandToString(stats.band);
            case ColMode:
                return modeToString(stats.mode);
            case ColQsos:
                return stats.qsoCount;
            case ColMults:
                return stats.multipliers;
            case ColPoints:
                return stats.points;
            case ColScore:
                return stats.score;
            case ColBestHr:
                return stats.bestHourRate;
            case ColThisHr:
                return stats.thisHourRate;
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() >= ColQsos) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant BandSummaryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case ColBand: return "Band";
        case ColMode: return "Mode";
        case ColQsos: return "QSOs";
        case ColMults: return "Mults";
        case ColPoints: return "Points";
        case ColScore: return "Score";
        case ColBestHr: return "Best Hr";
        case ColThisHr: return "This Hr";
    }

    return QVariant();
}

Qt::ItemFlags BandSummaryModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void BandSummaryModel::updateStats(const QVector<BandModeStats>& stats) {
    beginResetModel();
    m_stats = stats;

    // Sort by band then mode
    std::sort(m_stats.begin(), m_stats.end(),
              [](const BandModeStats& a, const BandModeStats& b) {
                  if (a.band != b.band) {
                      return static_cast<int>(a.band) < static_cast<int>(b.band);
                  }
                  return static_cast<int>(a.mode) < static_cast<int>(b.mode);
              });

    endResetModel();
}

void BandSummaryModel::updateMultipliers(BandType band, ModeType mode, int multipliers) {
    for (int i = 0; i < m_stats.size(); ++i) {
        if (m_stats[i].band == band && m_stats[i].mode == mode) {
            m_stats[i].multipliers = multipliers;
            QModelIndex idx = index(i, ColMults);
            emit dataChanged(idx, idx);
            return;
        }
    }
}

void BandSummaryModel::updatePoints(BandType band, ModeType mode, int points, int score) {
    for (int i = 0; i < m_stats.size(); ++i) {
        if (m_stats[i].band == band && m_stats[i].mode == mode) {
            m_stats[i].points = points;
            m_stats[i].score = score;
            QModelIndex startIdx = index(i, ColPoints);
            QModelIndex endIdx = index(i, ColScore);
            emit dataChanged(startIdx, endIdx);
            return;
        }
    }
}

void BandSummaryModel::clear() {
    beginResetModel();
    m_stats.clear();
    endResetModel();
}

int BandSummaryModel::getTotalQsos() const {
    int total = 0;
    for (const auto& s : m_stats) {
        total += s.qsoCount;
    }
    return total;
}

int BandSummaryModel::getTotalMults() const {
    int total = 0;
    for (const auto& s : m_stats) {
        total += s.multipliers;
    }
    return total;
}

int BandSummaryModel::getTotalPoints() const {
    int total = 0;
    for (const auto& s : m_stats) {
        total += s.points;
    }
    return total;
}

int BandSummaryModel::getTotalScore() const {
    int total = 0;
    for (const auto& s : m_stats) {
        total += s.score;
    }
    return total;
}

// ============================================================================
// OperatorStatsModel
// ============================================================================

OperatorStatsModel::OperatorStatsModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int OperatorStatsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_stats.size();
}

int OperatorStatsModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant OperatorStatsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_stats.size()) {
        return QVariant();
    }

    const OperatorStats& stats = m_stats[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColOperator:
                return stats.callsign;
            case ColQsos:
                return stats.totalQsos;
            case ColLast60Min:
                return stats.last60MinQsos;
            case ColPeakRate:
                return stats.peakRate;
            case ColOnAirTime: {
                int hours = stats.onAirMinutes / 60;
                int mins = stats.onAirMinutes % 60;
                return QString("%1:%2").arg(hours).arg(mins, 2, 10, QChar('0'));
            }
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() >= ColQsos) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant OperatorStatsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case ColOperator: return "Operator";
        case ColQsos: return "QSOs";
        case ColLast60Min: return "60-min";
        case ColPeakRate: return "Peak";
        case ColOnAirTime: return "On-Air";
    }

    return QVariant();
}

void OperatorStatsModel::updateStats(const QVector<OperatorStats>& stats) {
    beginResetModel();
    m_stats = stats;

    // Sort by QSO count descending
    std::sort(m_stats.begin(), m_stats.end(),
              [](const OperatorStats& a, const OperatorStats& b) {
                  return a.totalQsos > b.totalQsos;
              });

    endResetModel();
}

void OperatorStatsModel::clear() {
    beginResetModel();
    m_stats.clear();
    endResetModel();
}

// ============================================================================
// StationStatsModel
// ============================================================================

StationStatsModel::StationStatsModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int StationStatsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_stats.size();
}

int StationStatsModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant StationStatsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_stats.size()) {
        return QVariant();
    }

    const StationStats& stats = m_stats[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColStation:
                return stats.stationId;
            case ColQsos:
                return stats.qsoCount;
            case ColLast60MinRate:
                return stats.last60MinRate;
            case ColDominantBand:
                return bandToString(stats.dominantBand);
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColQsos || index.column() == ColLast60MinRate) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant StationStatsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case ColStation: return "Station";
        case ColQsos: return "QSOs";
        case ColLast60MinRate: return "60-min";
        case ColDominantBand: return "Band";
    }

    return QVariant();
}

void StationStatsModel::updateStats(const QVector<StationStats>& stats) {
    beginResetModel();
    m_stats = stats;

    // Sort by station ID
    std::sort(m_stats.begin(), m_stats.end(),
              [](const StationStats& a, const StationStats& b) {
                  return a.stationId < b.stationId;
              });

    endResetModel();
}

void StationStatsModel::clear() {
    beginResetModel();
    m_stats.clear();
    endResetModel();
}

} // namespace TR4QT
