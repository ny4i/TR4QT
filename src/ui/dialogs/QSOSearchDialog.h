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
