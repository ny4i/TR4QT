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

#ifndef OPERATORDIALOG_H
#define OPERATORDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace TR4QT {

/**
 * Simple dialog for changing the current operator callsign
 * Invoked when user enters "OPON" in the callsign field
 */
class OperatorDialog : public QDialog {
    Q_OBJECT

public:
    explicit OperatorDialog(QWidget* parent = nullptr);
    ~OperatorDialog() override = default;

    /**
     * Get the entered operator callsign
     */
    QString getOperatorCallsign() const;

    /**
     * Set the initial operator callsign in the field
     */
    void setOperatorCallsign(const QString& callsign);

private:
    void setupUI();

    QLineEdit* m_operatorEdit;
};

} // namespace TR4QT

#endif // OPERATORDIALOG_H
