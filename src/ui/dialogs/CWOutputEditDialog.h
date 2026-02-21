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

#ifndef CWOUTPUTEDITDIALOG_H
#define CWOUTPUTEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QPushButton>
#include "../../cw/CWOutputProfile.h"

namespace TR4QT {

/**
 * Dialog for editing a single CW output profile's configuration.
 *
 * Output-only concerns: Radio CAT, WinKeyer, or DTR/RTS.
 * Paddle input (HaliKey) is configured separately in CW Settings → Paddle Input.
 *
 * Used by PreferencesDialog when adding or editing CW outputs.
 */
class CWOutputEditDialog : public QDialog {
    Q_OBJECT

public:
    /// Create dialog for adding a new CW output profile
    explicit CWOutputEditDialog(QWidget* parent = nullptr);

    /// Create dialog for editing an existing CW output profile
    explicit CWOutputEditDialog(const CWOutputProfile& profile, QWidget* parent = nullptr);

    ~CWOutputEditDialog() override = default;

    /// Get the configured CW output profile
    CWOutputProfile getCWOutputProfile() const;

private slots:
    void onTypeChanged(int index);
    void onRefreshWinKeyerPorts();
    void onRefreshDtrRtsPorts();

private:
    void setupUI();
    void loadProfileIntoUI(const CWOutputProfile& profile);
    void updateVisibility();

    // Profile name
    QLineEdit* m_nameEdit;

    // Output type selection
    QComboBox* m_typeCombo;

    // WinKeyer settings group (type == KeyerDevice)
    QGroupBox* m_winKeyerGroup;
    QComboBox* m_winKeyerPortCombo;
    QPushButton* m_winKeyerRefreshPortsButton;
    QSpinBox* m_weightingSpin;
    QSpinBox* m_leadInSpin;
    QSpinBox* m_tailTimeSpin;
    QSpinBox* m_potMinWpmSpin;
    QSpinBox* m_potMaxWpmSpin;

    // DTR/RTS settings group (type == DtrRts)
    QGroupBox* m_dtrRtsGroup;
    QComboBox* m_dtrRtsPortCombo;
    QPushButton* m_dtrRtsRefreshPortsButton;
    QComboBox* m_dtrRtsPinCombo;

    // Original profile (for edit mode)
    CWOutputProfile m_originalProfile;
    bool m_isEditMode{false};
};

} // namespace TR4QT

#endif // CWOUTPUTEDITDIALOG_H
