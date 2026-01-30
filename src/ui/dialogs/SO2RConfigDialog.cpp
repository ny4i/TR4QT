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

#include "SO2RConfigDialog.h"
#include "../../utils/AppSettings.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>

namespace TR4QT {

SO2RConfigDialog::SO2RConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("SO2R Configuration");
    setModal(true);
    setMinimumWidth(400);

    setupUI();
    loadCurrentSettings();
    updateUIState();
}

void SO2RConfigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // SO2R enabled checkbox
    m_so2rEnabledCheckbox = new QCheckBox("Enable SO2R (Single Operator Two Radio)");
    connect(m_so2rEnabledCheckbox, &QCheckBox::toggled,
            this, &SO2RConfigDialog::onSO2REnabledChanged);
    mainLayout->addWidget(m_so2rEnabledCheckbox);

    // Radio profile assignments group
    QGroupBox* profileGroup = new QGroupBox("Radio Assignments");
    QGridLayout* profileLayout = new QGridLayout(profileGroup);

    // Radio 1
    m_radio1Label = new QLabel("Radio 1:");
    m_radio1ProfileCombo = new QComboBox();
    m_radio1ProfileCombo->setMinimumWidth(250);
    profileLayout->addWidget(m_radio1Label, 0, 0);
    profileLayout->addWidget(m_radio1ProfileCombo, 0, 1);

    // Radio 2
    m_radio2Label = new QLabel("Radio 2:");
    m_radio2ProfileCombo = new QComboBox();
    m_radio2ProfileCombo->setMinimumWidth(250);
    profileLayout->addWidget(m_radio2Label, 1, 0);
    profileLayout->addWidget(m_radio2ProfileCombo, 1, 1);

    mainLayout->addWidget(profileGroup);

    // Instructions
    QLabel* instructions = new QLabel(
        "When SO2R is enabled, both radios will connect simultaneously.\n"
        "Use Alt+R to toggle which radio is active for logging.\n"
        "The active radio frequency is shown bright, standby is grayed."
    );
    instructions->setWordWrap(true);
    instructions->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(instructions);

    mainLayout->addStretch();

    // Button row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton("OK");
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &SO2RConfigDialog::onAccepted);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton("Cancel");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    populateProfileComboBoxes();
}

void SO2RConfigDialog::loadCurrentSettings()
{
    AppSettings& settings = AppSettings::instance();

    m_so2rEnabledCheckbox->setChecked(settings.isSO2REnabled());

    // Load radio profile assignments
    QString radio1Profile = settings.getSO2RRadioProfile(0);
    QString radio2Profile = settings.getSO2RRadioProfile(1);

    // Find and select the profiles in combo boxes (use findData since name is stored as data)
    int radio1Index = m_radio1ProfileCombo->findData(radio1Profile);
    if (radio1Index >= 0) {
        m_radio1ProfileCombo->setCurrentIndex(radio1Index);
    }

    int radio2Index = m_radio2ProfileCombo->findData(radio2Profile);
    if (radio2Index >= 0) {
        m_radio2ProfileCombo->setCurrentIndex(radio2Index);
    }
}

void SO2RConfigDialog::populateProfileComboBoxes()
{
    AppSettings& settings = AppSettings::instance();
    QList<RadioProfile> profiles = settings.loadRadioProfiles();

    m_radio1ProfileCombo->clear();
    m_radio2ProfileCombo->clear();

    // Add "None" option
    m_radio1ProfileCombo->addItem("(None)", QString());
    m_radio2ProfileCombo->addItem("(None)", QString());

    // Add all valid profiles
    for (const RadioProfile& profile : profiles) {
        if (profile.isValid()) {
            m_radio1ProfileCombo->addItem(profile.displayString(), profile.name);
            m_radio2ProfileCombo->addItem(profile.displayString(), profile.name);
        }
    }
}

void SO2RConfigDialog::updateUIState()
{
    bool enabled = m_so2rEnabledCheckbox->isChecked();

    m_radio1Label->setEnabled(enabled);
    m_radio1ProfileCombo->setEnabled(enabled);
    m_radio2Label->setEnabled(enabled);
    m_radio2ProfileCombo->setEnabled(enabled);
}

void SO2RConfigDialog::onSO2REnabledChanged(bool /* enabled */)
{
    updateUIState();
}

void SO2RConfigDialog::onAccepted()
{
    // Validate: if SO2R enabled, must have at least Radio 1 configured
    if (m_so2rEnabledCheckbox->isChecked()) {
        QString radio1 = m_radio1ProfileCombo->currentData().toString();
        QString radio2 = m_radio2ProfileCombo->currentData().toString();

        if (radio1.isEmpty() && radio2.isEmpty()) {
            QMessageBox::warning(this, "Invalid Configuration",
                "Please select at least one radio profile for SO2R operation.");
            return;
        }

        // Warn if only one radio configured
        if (radio1.isEmpty() || radio2.isEmpty()) {
            QMessageBox::StandardButton reply = QMessageBox::question(this,
                "Single Radio Configuration",
                "Only one radio is configured. SO2R works best with two radios.\n\n"
                "Continue anyway?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
        }

        // Warn if same profile selected for both
        if (!radio1.isEmpty() && radio1 == radio2) {
            QMessageBox::warning(this, "Invalid Configuration",
                "Radio 1 and Radio 2 must use different profiles.\n"
                "Please select a different profile for each radio.");
            return;
        }
    }

    // Save settings
    AppSettings& settings = AppSettings::instance();
    settings.setSO2REnabled(m_so2rEnabledCheckbox->isChecked());
    settings.setSO2RRadioProfile(0, m_radio1ProfileCombo->currentData().toString());
    settings.setSO2RRadioProfile(1, m_radio2ProfileCombo->currentData().toString());

    LOG_INFO("SO2RConfigDialog", QString("SO2R settings saved: enabled=%1, radio1='%2', radio2='%3'")
             .arg(m_so2rEnabledCheckbox->isChecked())
             .arg(m_radio1ProfileCombo->currentData().toString())
             .arg(m_radio2ProfileCombo->currentData().toString()));

    accept();
}

bool SO2RConfigDialog::isSO2REnabled() const
{
    return m_so2rEnabledCheckbox->isChecked();
}

QString SO2RConfigDialog::getRadio1Profile() const
{
    return m_radio1ProfileCombo->currentData().toString();
}

QString SO2RConfigDialog::getRadio2Profile() const
{
    return m_radio2ProfileCombo->currentData().toString();
}

} // namespace TR4QT
