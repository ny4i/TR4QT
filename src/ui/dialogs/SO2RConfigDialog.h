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

#ifndef SO2RCONFIGDIALOG_H
#define SO2RCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

namespace TR4QT {

/**
 * SO2R Configuration Dialog
 *
 * Allows the user to configure SO2R (Single Operator Two Radio) settings:
 * - Enable/disable SO2R mode
 * - Assign radio profiles to Radio 1 and Radio 2 slots
 *
 * When SO2R is enabled, both configured radios will connect simultaneously
 * and the user can toggle between them using Alt+R.
 */
class SO2RConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit SO2RConfigDialog(QWidget* parent = nullptr);

    /**
     * Get whether SO2R mode is enabled
     */
    bool isSO2REnabled() const;

    /**
     * Get the profile name assigned to Radio 1
     */
    QString getRadio1Profile() const;

    /**
     * Get the profile name assigned to Radio 2
     */
    QString getRadio2Profile() const;

private slots:
    void onSO2REnabledChanged(bool enabled);
    void onAccepted();

private:
    void setupUI();
    void loadCurrentSettings();
    void populateProfileComboBoxes();
    void updateUIState();

    QCheckBox* m_so2rEnabledCheckbox;
    QComboBox* m_radio1ProfileCombo;
    QComboBox* m_radio2ProfileCombo;
    QLabel* m_radio1Label;
    QLabel* m_radio2Label;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
};

} // namespace TR4QT

#endif // SO2RCONFIGDIALOG_H
