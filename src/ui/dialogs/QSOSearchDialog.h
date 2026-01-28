/**
 * QSOSearchDialog - Modal dialog for QSO search criteria
 *
 * Allows user to search QSOs by callsign, operator, and time range.
 * All fields are optional (empty = no filter on that field).
 */

#ifndef QSOSEARCHDIALOG_H
#define QSOSEARCHDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QCheckBox>
#include "../../data/QSORepository.h"

namespace TR4QT {

class QSOSearchDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @param currentContestId Current active contest ID (-1 if none)
     * @param parent Parent widget
     */
    explicit QSOSearchDialog(int currentContestId, QWidget* parent = nullptr);

    /**
     * Get the search criteria entered by the user
     */
    QSOSearchCriteria getCriteria() const;

private:
    void setupUI();

    int m_currentContestId;

    QLineEdit* m_callsignEdit;
    QLineEdit* m_operatorEdit;
    QDateTimeEdit* m_startTimeEdit;
    QDateTimeEdit* m_endTimeEdit;
    QCheckBox* m_searchAllContestsCheckBox;
};

}  // namespace TR4QT

#endif  // QSOSEARCHDIALOG_H
