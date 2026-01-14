#ifndef QSOTABLEMODEL_H
#define QSOTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QRecursiveMutex>
#include "../../models/QSO.h"
#include "../../contests/ContestBase.h"

namespace TR4QT {

/**
 * Table model for displaying QSO log
 *
 * Displays QSOs in a table with contest-dependent columns.
 * Exchange columns (Exch1, Exch2) adapt their headers based on the active contest.
 *
 * Examples:
 * - CQ WW:  Exch1 = "DX" (country), Exch2 = "Zn" (zone)
 * - CQ WPX: Exch1 = "DX" (country), Exch2 = "#" (serial number)
 * - WFD:    Exch1 = "CL" (class), Exch2 = "QTH" (section)
 *
 * Supports color coding:
 * - Dupes shown in red
 * - New multipliers shown in green/bold
 *
 * Thread Safety:
 * - All public methods are thread-safe (protected by QMutex)
 * - Safe to access from WebServer HTTP handlers or other threads
 * - Authoritative data source for QSO list
 *
 * Usage:
 *   QSOTableModel* model = new QSOTableModel(this);
 *   model->setContestExchangeFields(contest->getReceivedExchangeFields());
 *   tableView->setModel(model);
 *   model->addQSO(qso);  // Add new QSO to display
 */
class QSOTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColBand = 0,      // "20CW", "40SSB"
        ColDate,          // "30-11-..."
        ColUTC,           // "22:45"
        ColQSOs,          // Sequential number (1, 2, 3...)
        ColCallsign,      // "W1AW"
        ColExch1,         // Contest-dependent exchange field 1
        ColExch2,         // Contest-dependent exchange field 2
        ColExch3,         // Contest-dependent exchange field 3
        ColExch4,         // Contest-dependent exchange field 4
        ColExch5,         // Contest-dependent exchange field 5
        ColPts,           // QSO Points
        ColM,             // Markers (x = new mult on this band, z = new mult all-time)
        ColId,            // Computer ID (network station identifier)
        ColMult,          // $ indicator for multiplier
        ColDupe,          // D indicator for duplicate QSO
        ColFreq,          // Frequency in kHz
        ColOp,            // Operator callsign
        ColCount          // Total columns (with all 5 exchange fields visible)
    };

    static constexpr int MAX_EXCHANGE_COLUMNS = 5;

    explicit QSOTableModel(QObject* parent = nullptr);
    ~QSOTableModel() override = default;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Data management
    void setQSOs(const QList<QSO>& qsos);
    void addQSO(const QSO& qso);
    void updateQSO(int row, const QSO& qso);
    void removeQSO(int row);
    void clear();

    // Get QSO at row
    QSO getQSO(int row) const;

    // Get all QSOs (thread-safe copy)
    QList<QSO> getAllQSOs() const;

    // Get total count
    int count() const { return m_qsos.size(); }

    // Contest-dependent exchange fields and display
    void setTableColumns(const QList<TableColumn>& columns);
    void setContestExchangeFields(const QList<ExchangeField>& fields);  // Legacy compatibility

private slots:
    void onThemeChanged();

private:
    QList<QSO> m_qsos;
    QList<TableColumn> m_tableColumns;      // Contest table column definitions
    int m_visibleExchangeColumns{2};        // Number of exchange columns to show (1-5)
    mutable QRecursiveMutex m_mutex;        // Thread-safe access (recursive for Qt callbacks)

    QString formatFrequency(freq_t freq) const;
    QString getExchangeFieldHeader(int fieldIndex) const;
    QString getExchangeFieldValue(const QSO& qso, int fieldIndex) const;
    QString getMultiplierIndicators(const QSO& qso) const;
};

} // namespace TR4QT

#endif // QSOTABLEMODEL_H
