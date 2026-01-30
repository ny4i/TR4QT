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

#ifndef MACROEDITDIALOG_H
#define MACROEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace TR4QT {

/**
 * Dialog for editing a CW macro button
 * Allows user to set the button label and the CW text to send
 */
class MacroEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit MacroEditDialog(int macroIndex, const QString& currentLabel,
                            const QString& currentText, QWidget* parent = nullptr);

    QString getLabel() const;
    QString getCWText() const;

private:
    void setupUI(const QString& currentLabel, const QString& currentText);

    int m_macroIndex;
    QLineEdit* m_labelEdit;
    QLineEdit* m_cwTextEdit;
};

} // namespace TR4QT

#endif // MACROEDITDIALOG_H
