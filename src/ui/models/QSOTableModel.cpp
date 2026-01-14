#include "QSOTableModel.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/AppSettings.h"
#include "../../core/Types.h"
#include <QBrush>
#include <QFont>
#include <QColor>
#include <QLocale>
#include <QMutexLocker>

namespace TR4QT {

QSOTableModel::QSOTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &QSOTableModel::onThemeChanged);
}

int QSOTableModel::rowCount(const QModelIndex& parent) const {
    QMutexLocker locker(&m_mutex);
    if (parent.isValid()) {
        return 0;
    }
    return m_qsos.size();
}

int QSOTableModel::columnCount(const QModelIndex& parent) const {
    QMutexLocker locker(&m_mutex);
    if (parent.isValid()) {
        return 0;
    }
    // Return actual visible column count based on contest's exchange fields
    // Fixed columns: Band, Date, UTC, NR, Callsign (5 before exchange)
    // Exchange columns: variable (1-5)
    // Fixed columns: Pts, M, Id, $, D, Freq, Op (7 after exchange)
    return 5 + m_visibleExchangeColumns + 7;
}

QVariant QSOTableModel::data(const QModelIndex& index, int role) const {
    QMutexLocker locker(&m_mutex);
    if (!index.isValid() || index.row() >= m_qsos.size()) {
        return QVariant();
    }

    const QSO& qso = m_qsos.at(index.row());
    int col = index.column();

    // Display role - show data
    if (role == Qt::DisplayRole) {
        // Map physical column to logical column (handle variable exchange columns)
        int logicalCol = col;
        if (col >= ColExch1) {
            // After exchange start, adjust for hidden exchange columns
            if (col < ColExch1 + m_visibleExchangeColumns) {
                // This is an exchange column, use as-is
                logicalCol = col;
            } else {
                // This is a fixed column after exchange, adjust for hidden columns
                logicalCol = col + (MAX_EXCHANGE_COLUMNS - m_visibleExchangeColumns);
            }
        }

        switch (logicalCol) {
        case ColBand:
            // Format: "20CW", "15LSB" (remove M from band, e.g., "15M" -> "15")
            return QString("%1%2")
                .arg(bandToString(qso.band).remove('M'))
                .arg(modeToString(qso.mode));
        case ColDate:
            // Format: "11-30-2025" (month-day-year, 4-digit year)
            return qso.timestamp.toUTC().toString("MM-dd-yyyy");
        case ColUTC:
            // Format: "22:45" (time only)
            return qso.timestamp.toUTC().toString("HH:mm");
        case ColQSOs:
            // Sequential number (1-based from row index)
            return index.row() + 1;
        case ColCallsign:
            return qso.callsign;
        case ColExch1:
        case ColExch2:
        case ColExch3:
        case ColExch4:
        case ColExch5:
            // Contest-dependent exchange fields
            return getExchangeFieldValue(qso, logicalCol - ColExch1);
        case ColPts:
            // QSO points
            return qso.qsoPoints > 0 ? QString::number(qso.qsoPoints) : QString();
        case ColM:
            // Multiplier type indicators (TR4W style)
            // x = DXCC country, d = section/state, z = zone, p = prefix
            // Combined in order: x, d, z, p (e.g., "xz" for CQ WW)
            return getMultiplierIndicators(qso);
        case ColId:
            // Computer ID (network station identifier)
            return AppSettings::instance().getComputerID();
        case ColMult:
            // $ indicator (Search & Pounce - not implemented yet)
            return QString();  // TODO: implement S&P indicator
        case ColDupe:
            // D indicator for duplicate QSO
            return qso.isDupe ? "D" : QString();
        case ColFreq:
            // Frequency in kHz with 1 decimal
            return formatFrequency(qso.frequency);
        case ColOp:
            // Operator callsign
            return qso.operatorCall;
        default:
            return QVariant();
        }
    }

    // Foreground role - text color
    if (role == Qt::ForegroundRole) {
        if (qso.isDupe) {
            ThemeManager& theme = ThemeManager::instance();
            return QBrush(theme.color(ColorRole::DupeText));
        }
        return QVariant();  // Default color
    }

    // Background role - background color
    if (role == Qt::BackgroundRole) {
        if (qso.isMultiplier) {
            ThemeManager& theme = ThemeManager::instance();
            return QBrush(theme.color(ColorRole::NewMultiplierBackground));
        }
        return QVariant();  // Default background
    }

    // Font role - make new mults bold
    if (role == Qt::FontRole) {
        if (qso.isMultiplier) {
            QFont font;
            font.setBold(true);
            return font;
        }
        return QVariant();
    }

    return QVariant();
}

QVariant QSOTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    QMutexLocker locker(&m_mutex);
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        // Map physical column to logical column (handle variable exchange columns)
        int logicalCol = section;
        if (section >= ColExch1) {
            if (section < ColExch1 + m_visibleExchangeColumns) {
                logicalCol = section;
            } else {
                logicalCol = section + (MAX_EXCHANGE_COLUMNS - m_visibleExchangeColumns);
            }
        }

        switch (logicalCol) {
        case ColBand:       return "Band";
        case ColDate:       return "Date";
        case ColUTC:        return "UTC";
        case ColQSOs:       return "NR";
        case ColCallsign:   return "Callsign";
        case ColExch1:
        case ColExch2:
        case ColExch3:
        case ColExch4:
        case ColExch5:
            return getExchangeFieldHeader(logicalCol - ColExch1);
        case ColPts:        return "Pts";
        case ColM:          return "M";
        case ColId:         return "Id";
        case ColMult:       return "$";
        case ColDupe:       return "D";
        case ColFreq:       return "Freq";
        case ColOp:         return "Op";
        default:            return QVariant();
        }
    }

    // No vertical header (row numbers not shown in TR4W)
    return QVariant();
}

void QSOTableModel::setQSOs(const QList<QSO>& qsos) {
    QMutexLocker locker(&m_mutex);
    beginResetModel();
    m_qsos = qsos;
    endResetModel();
}

void QSOTableModel::addQSO(const QSO& qso) {
    QMutexLocker locker(&m_mutex);
    int row = m_qsos.size();
    beginInsertRows(QModelIndex(), row, row);
    m_qsos.append(qso);
    endInsertRows();
}

void QSOTableModel::updateQSO(int row, const QSO& qso) {
    QMutexLocker locker(&m_mutex);
    if (row < 0 || row >= m_qsos.size()) {
        return;
    }

    m_qsos[row] = qso;
    emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

void QSOTableModel::removeQSO(int row) {
    QMutexLocker locker(&m_mutex);
    if (row < 0 || row >= m_qsos.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_qsos.removeAt(row);
    endRemoveRows();
}

void QSOTableModel::clear() {
    QMutexLocker locker(&m_mutex);
    beginResetModel();
    m_qsos.clear();
    endResetModel();
}

QSO QSOTableModel::getQSO(int row) const {
    QMutexLocker locker(&m_mutex);
    if (row < 0 || row >= m_qsos.size()) {
        return QSO();
    }
    return m_qsos.at(row);
}

QList<QSO> QSOTableModel::getAllQSOs() const {
    QMutexLocker locker(&m_mutex);
    return m_qsos;  // Returns copy (thread-safe)
}

QString QSOTableModel::formatFrequency(freq_t freq) const {
    // Convert Hz to kHz for display
    double freqKhz = freq / 1000.0;

    // Format with 1 decimal place
    return QString::number(freqKhz, 'f', 1);
}

void QSOTableModel::setTableColumns(const QList<TableColumn>& columns) {
    QMutexLocker locker(&m_mutex);
    // Set the table column definitions from the contest
    m_tableColumns = columns;
    m_visibleExchangeColumns = qMin(columns.size(), MAX_EXCHANGE_COLUMNS);

    // Notify views that the column structure has changed
    beginResetModel();
    endResetModel();
}

void QSOTableModel::setContestExchangeFields(const QList<ExchangeField>& fields) {
    // Legacy compatibility - convert ExchangeField to TableColumn
    QList<TableColumn> columns;
    for (const ExchangeField& field : fields) {
        // Create default table column from exchange field
        QString header = field.name;
        if (header == "Zone") header = "Zn";
        else if (header == "ITU Zone") header = "ITU";
        else if (header == "Class") header = "CL";
        else if (header == "Section") header = "QTH";
        else if (header == "Serial") header = "#";
        else header = header.left(3).toUpper();

        columns.append(TableColumn(field.name, header, 0, TableColumn::Alignment::Left));
    }

    setTableColumns(columns);
}

QString QSOTableModel::getExchangeFieldHeader(int fieldIndex) const {
    if (fieldIndex >= 0 && fieldIndex < m_tableColumns.size()) {
        // Use header text from TableColumn definition
        return m_tableColumns[fieldIndex].headerText;
    }
    // Default headers if no contest set
    return fieldIndex == 0 ? "DX" : "Zn";
}

QString QSOTableModel::getExchangeFieldValue(const QSO& qso, int fieldIndex) const {
    if (fieldIndex >= 0 && fieldIndex < m_tableColumns.size()) {
        QString fieldName = m_tableColumns[fieldIndex].fieldName;

        // Map field name to dedicated QSO fields
        // All exchange data is now stored in dedicated fields (no more parsedExchange)
        // TODO: Keep this mapping in sync with new contest exchange fields!
        //       When adding new contests, verify all exchange fields have mappings here.
        if (fieldName == "Zone") {
            return qso.cqZone > 0 ? QString::number(qso.cqZone) :
                   (!qso.ituZoneExchange.isEmpty() ? qso.ituZoneExchange : QString());
        } else if (fieldName == "ITU Zone") {
            return qso.ituZone > 0 ? QString::number(qso.ituZone) : QString();
        } else if (fieldName == "Serial") {
            return qso.serialNumberReceived > 0 ? QString::number(qso.serialNumberReceived) : QString();
        } else if (fieldName == "Section") {
            // Try ARRL section field first, then state
            if (!qso.arrlSection.isEmpty()) {
                return qso.arrlSection;
            } else if (!qso.state.isEmpty()) {
                return qso.state;
            }
            return QString();
        } else if (fieldName == "Class") {
            return qso.contestClass;
        } else if (fieldName == "Precedence") {
            return qso.precedence;
        } else if (fieldName == "Check") {
            return qso.check;
        } else if (fieldName == "Power") {
            return qso.power;
        } else if (fieldName == "Name") {
            return qso.operatorName;
        } else if (fieldName == "State") {
            return qso.state;
        } else if (fieldName == "State/Power") {
            // ARRL DX: state OR power
            return !qso.state.isEmpty() ? qso.state : qso.power;
        } else if (fieldName == "State/Serial") {
            // ARRL RTTY: state OR serial
            if (!qso.state.isEmpty()) {
                return qso.state;
            } else if (qso.serialNumberReceived > 0) {
                return QString::number(qso.serialNumberReceived);
            }
            return QString();
        } else if (fieldName == "Comments") {
            // General Logging: free-form comments/notes
            return qso.notes;
        } else if (fieldName == "RST") {
            // General Logging: RST received
            return qso.rstReceived;
        }

        // If no mapping found, return empty
        return QString();
    }

    // Default values if no contest set
    if (fieldIndex == 0) {
        // Default to DXCC prefix (country)
        return qso.dxccPrefix;
    } else if (fieldIndex == 1) {
        // Default to CQ Zone
        return qso.cqZone > 0 ? QString::number(qso.cqZone) : QString();
    }

    return QString();
}

void QSOTableModel::onThemeChanged() {
    QMutexLocker locker(&m_mutex);
    // Refresh all cells when theme changes
    if (!m_qsos.isEmpty()) {
        QModelIndex topLeft = index(0, 0);
        QModelIndex bottomRight = index(m_qsos.size() - 1, columnCount() - 1);
        emit dataChanged(topLeft, bottomRight, {Qt::ForegroundRole, Qt::BackgroundRole});
    }
}

QString QSOTableModel::getMultiplierIndicators(const QSO& qso) const {
    if (!qso.isMultiplier || qso.multipliers.isEmpty()) {
        return QString();
    }

    QString indicators;

    // Parse multiplier types from QStringList (format: "Type:Value")
    for (const QString& mult : qso.multipliers) {
        QStringList parts = mult.split(':');
        if (parts.size() != 2) continue;

        QString type = parts[0];

        // Convert to TR4W-style lowercase indicators
        // Order: x, d, z, p
        if (type == "Country" && !indicators.contains('x')) {
            indicators += 'x';
        } else if ((type == "Section" || type == "State") && !indicators.contains('d')) {
            indicators += 'd';
        } else if ((type == "CQZone" || type == "ITUZone") && !indicators.contains('z')) {
            indicators += 'z';
        } else if (type == "Prefix" && !indicators.contains('p')) {
            indicators += 'p';
        }
    }

    // Ensure correct order: x, d, z, p
    QString ordered;
    if (indicators.contains('x')) ordered += 'x';
    if (indicators.contains('d')) ordered += 'd';
    if (indicators.contains('z')) ordered += 'z';
    if (indicators.contains('p')) ordered += 'p';

    return ordered;
}

} // namespace TR4QT
