#ifndef QSOTABLEMODEL_H
#define QSOTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "../../models/QSO.h"

namespace TR4QT {

/**
 * Table model for displaying QSO log
 *
 * Displays QSOs in a table with columns:
 * Time, Callsign, Frequency, Mode, RST Sent, RST Rcvd, Exchange, Mult, Points
 *
 * Supports color coding:
 * - Dupes shown in red
 * - New multipliers shown in green/bold
 *
 * Usage:
 *   QSOTableModel* model = new QSOTableModel(this);
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
        ColDX,            // Country/entity abbreviation
        ColZn,            // CQ Zone
        ColPts,           // QSO Points
        ColM,             // Markers (x = new mult on this band, z = new mult all-time)
        ColMult,          // $ indicator for multiplier
        ColFreq,          // Frequency in kHz
        ColOp,            // Operator callsign
        ColCount          // Number of columns
    };

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

    // Get total count
    int count() const { return m_qsos.size(); }

private:
    QList<QSO> m_qsos;

    QString formatFrequency(freq_t freq) const;
};

} // namespace TR4QT

#endif // QSOTABLEMODEL_H
