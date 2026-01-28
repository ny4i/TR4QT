/**
 * QSOSearchPanel - Collapsible panel showing QSO search results
 *
 * Displays search results in a QTableView below the main QSO log.
 * Has a header bar with result count and close button.
 * Starts hidden; shown when a search is executed.
 */

#ifndef QSOSEARCHPANEL_H
#define QSOSEARCHPANEL_H

#include <QWidget>
#include <QTableView>
#include <QLabel>
#include <QPushButton>
#include "../../models/QSO.h"

namespace TR4QT {

class QSOTableModel;
class ContestBase;

class QSOSearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit QSOSearchPanel(QWidget* parent = nullptr);

    /**
     * Display search results in the panel
     * @param results List of QSOs matching search criteria
     */
    void setResults(const QList<QSO>& results);

    /**
     * Configure exchange columns for current contest
     * @param contest Active contest (for column headers)
     */
    void setContest(ContestBase* contest);

    /**
     * Copy font and column widths from the source table view
     * so search results match the main QSO log appearance.
     * @param sourceTable The main QSO table view to copy from
     */
    void syncAppearance(const QTableView* sourceTable);

    /**
     * Clear results and hide panel
     */
    void clear();

signals:
    /**
     * Emitted when user clicks the close button or presses ESC
     */
    void closeRequested();

    /**
     * Emitted when user double-clicks a result row
     * @param qso The QSO that was double-clicked
     */
    void qsoSelected(const QSO& qso);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUI();

    QLabel* m_headerLabel;
    QPushButton* m_closeButton;
    QTableView* m_tableView;
    QSOTableModel* m_tableModel;
};

}  // namespace TR4QT

#endif  // QSOSEARCHPANEL_H
