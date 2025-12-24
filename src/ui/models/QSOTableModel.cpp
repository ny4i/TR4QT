#include "QSOTableModel.h"
#include "../../utils/ThemeManager.h"
#include "../../core/Types.h"
#include <QBrush>
#include <QFont>
#include <QColor>
#include <QLocale>

namespace TR4QT {

QSOTableModel::QSOTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &QSOTableModel::onThemeChanged);
}

int QSOTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_qsos.size();
}

int QSOTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return ColCount;
}

QVariant QSOTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_qsos.size()) {
        return QVariant();
    }

    const QSO& qso = m_qsos.at(index.row());

    // Display role - show data
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColBand:
            // Format: "20CW", "15LSB" (remove M from band, e.g., "15M" -> "15")
            return QString("%1%2")
                .arg(bandToString(qso.band).remove('M'))
                .arg(modeToString(qso.mode));
        case ColDate:
            // Format: "30-11-..." (day-month)
            return qso.timestamp.toUTC().toString("dd-MM-yy");
        case ColUTC:
            // Format: "22:45" (time only)
            return qso.timestamp.toUTC().toString("HH:mm");
        case ColQSOs:
            // Sequential number (1-based from row index)
            return index.row() + 1;
        case ColCallsign:
            return qso.callsign;
        case ColExch1:
            // Contest-dependent exchange field 1
            return getExchangeFieldValue(qso, 0);
        case ColExch2:
            // Contest-dependent exchange field 2
            return getExchangeFieldValue(qso, 1);
        case ColPts:
            // QSO points
            return qso.qsoPoints > 0 ? QString::number(qso.qsoPoints) : QString();
        case ColM:
            // Markers (will be populated when we implement multiplier logic)
            // x = new mult on this band, z = new mult all-time
            return QString();  // TODO: implement markers
        case ColMult:
            // $ indicator for multipliers
            return qso.isMultiplier ? "$" : QString();
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
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case ColBand:       return "Band";
        case ColDate:       return "Date";
        case ColUTC:        return "UTC";
        case ColQSOs:       return "NR";
        case ColCallsign:   return "Callsign";
        case ColExch1:      return getExchangeFieldHeader(0);
        case ColExch2:      return getExchangeFieldHeader(1);
        case ColPts:        return "Pts";
        case ColM:          return "M";
        case ColMult:       return "$";
        case ColFreq:       return "Freq";
        case ColOp:         return "Op";
        default:            return QVariant();
        }
    }

    // No vertical header (row numbers not shown in TR4W)
    return QVariant();
}

void QSOTableModel::setQSOs(const QList<QSO>& qsos) {
    beginResetModel();
    m_qsos = qsos;
    endResetModel();
}

void QSOTableModel::addQSO(const QSO& qso) {
    beginInsertRows(QModelIndex(), m_qsos.size(), m_qsos.size());
    m_qsos.append(qso);
    endInsertRows();
}

void QSOTableModel::updateQSO(int row, const QSO& qso) {
    if (row < 0 || row >= m_qsos.size()) {
        return;
    }

    m_qsos[row] = qso;
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
}

void QSOTableModel::removeQSO(int row) {
    if (row < 0 || row >= m_qsos.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_qsos.removeAt(row);
    endRemoveRows();
}

void QSOTableModel::clear() {
    beginResetModel();
    m_qsos.clear();
    endResetModel();
}

QSO QSOTableModel::getQSO(int row) const {
    if (row < 0 || row >= m_qsos.size()) {
        return QSO();
    }
    return m_qsos.at(row);
}

QString QSOTableModel::formatFrequency(freq_t freq) const {
    // Convert Hz to kHz for display
    double freqKhz = freq / 1000.0;

    // Format with 1 decimal place
    return QString::number(freqKhz, 'f', 1);
}

void QSOTableModel::setContestExchangeFields(const QList<ExchangeField>& fields) {
    m_exchangeFields = fields;

    // Notify views that headers have changed
    emit headerDataChanged(Qt::Horizontal, ColExch1, ColExch2);
}

QString QSOTableModel::getExchangeFieldHeader(int fieldIndex) const {
    if (fieldIndex >= 0 && fieldIndex < m_exchangeFields.size()) {
        // Use field name for header (e.g., "Zone", "Class", "Section")
        // Abbreviate for TR4W-style compactness
        QString name = m_exchangeFields[fieldIndex].name;
        if (name == "Zone") return "Zn";
        if (name == "ITU Zone") return "ITU";
        if (name == "Class") return "CL";
        if (name == "Section") return "QTH";
        if (name == "Serial") return "#";
        // Default: return first 3 chars uppercase
        return name.left(3).toUpper();
    }
    // Default headers if no contest set
    return fieldIndex == 0 ? "DX" : "Zn";
}

QString QSOTableModel::getExchangeFieldValue(const QSO& qso, int fieldIndex) const {
    if (fieldIndex >= 0 && fieldIndex < m_exchangeFields.size()) {
        QString fieldName = m_exchangeFields[fieldIndex].name;

        // Map field name to QSO data
        if (fieldName == "Zone") {
            return qso.cqZone > 0 ? QString::number(qso.cqZone) : QString();
        } else if (fieldName == "ITU Zone") {
            return qso.ituZone > 0 ? QString::number(qso.ituZone) : QString();
        } else if (fieldName == "Class") {
            // Class is stored in parsed exchange
            return qso.parsedExchange.value("Class", QString());
        } else if (fieldName == "Section") {
            // Section is stored in state field for WFD
            return qso.state;
        } else if (fieldName == "Serial") {
            // Serial number from parsed exchange
            return qso.parsedExchange.value("Serial", QString());
        } else {
            // Try to find in parsed exchange
            return qso.parsedExchange.value(fieldName, QString());
        }
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
    // Refresh all cells when theme changes
    if (!m_qsos.isEmpty()) {
        QModelIndex topLeft = index(0, 0);
        QModelIndex bottomRight = index(m_qsos.size() - 1, ColCount - 1);
        emit dataChanged(topLeft, bottomRight, {Qt::ForegroundRole, Qt::BackgroundRole});
    }
}

} // namespace TR4QT
