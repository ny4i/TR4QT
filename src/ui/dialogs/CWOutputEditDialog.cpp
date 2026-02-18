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

#include "CWOutputEditDialog.h"
#include "../../utils/DialogHelper.h"
#include "../../logging/LogMacros.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSerialPortInfo>

namespace TR4QT {

namespace {
    constexpr int CW_OUTPUT_DIALOG_MIN_WIDTH = 450;
}

CWOutputEditDialog::CWOutputEditDialog(QWidget* parent)
    : QDialog(parent)
    , m_isEditMode(false)
{
    setWindowTitle("Add CW Output");
    setModal(true);
    setMinimumWidth(CW_OUTPUT_DIALOG_MIN_WIDTH);
    setupUI();
}

CWOutputEditDialog::CWOutputEditDialog(const CWOutputProfile& profile, QWidget* parent)
    : QDialog(parent)
    , m_originalProfile(profile)
    , m_isEditMode(true)
{
    setWindowTitle(QString("Edit CW Output: %1").arg(profile.name));
    setModal(true);
    setMinimumWidth(CW_OUTPUT_DIALOG_MIN_WIDTH);
    setupUI();
    loadProfileIntoUI(profile);
}

void CWOutputEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === Profile Name ===
    QGroupBox* nameGroup = new QGroupBox("CW Output Name", this);
    QFormLayout* nameLayout = new QFormLayout(nameGroup);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("e.g., WinKeyer COM5, DTR on COM3, Radio CAT");
    nameLayout->addRow("Name:", m_nameEdit);
    mainLayout->addWidget(nameGroup);

    // === Output Type ===
    QGroupBox* typeGroup = new QGroupBox("Output Type", this);
    QFormLayout* typeLayout = new QFormLayout(typeGroup);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Radio CAT (KY command)", static_cast<int>(CWSenderFactory::Backend::Hamlib));
    m_typeCombo->addItem("WinKeyer", static_cast<int>(CWSenderFactory::Backend::KeyerDevice));
    m_typeCombo->addItem("DTR/RTS", static_cast<int>(CWSenderFactory::Backend::DtrRts));
    m_typeCombo->setToolTip(
        "Radio CAT: Send CW via Hamlib KY command through radio\n"
        "WinKeyer: K1EL hardware Morse generator (sends text)\n"
        "DTR/RTS: Toggle serial port line for CW keying");
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CWOutputEditDialog::onTypeChanged);
    typeLayout->addRow("Type:", m_typeCombo);
    mainLayout->addWidget(typeGroup);

    // === WinKeyer Settings ===
    m_winKeyerGroup = new QGroupBox("WinKeyer Hardware", this);
    QFormLayout* winKeyerLayout = new QFormLayout(m_winKeyerGroup);

    // Port selection with refresh
    QHBoxLayout* winKeyerPortLayout = new QHBoxLayout();
    m_winKeyerPortCombo = new QComboBox(this);
    m_winKeyerPortCombo->setEditable(true);
    m_winKeyerPortCombo->setToolTip("Serial port for WinKeyer device");
    m_winKeyerRefreshPortsButton = new QPushButton("Refresh", this);
    m_winKeyerRefreshPortsButton->setMaximumWidth(80);
    connect(m_winKeyerRefreshPortsButton, &QPushButton::clicked,
            this, &CWOutputEditDialog::onRefreshWinKeyerPorts);
    winKeyerPortLayout->addWidget(m_winKeyerPortCombo, 1);
    winKeyerPortLayout->addWidget(m_winKeyerRefreshPortsButton);
    winKeyerLayout->addRow("Port:", winKeyerPortLayout);

    // WinKeyer-specific settings
    m_weightingSpin = new QSpinBox(this);
    m_weightingSpin->setRange(CWProfileDefaults::WINKEYER_WEIGHTING_MIN,
                              CWProfileDefaults::WINKEYER_WEIGHTING_MAX);
    m_weightingSpin->setValue(CWProfileDefaults::WINKEYER_WEIGHTING_NORMAL);
    m_weightingSpin->setToolTip("WinKeyer weighting (10-90, 50=normal)");
    winKeyerLayout->addRow("Weighting:", m_weightingSpin);

    m_leadInSpin = new QSpinBox(this);
    m_leadInSpin->setRange(CWProfileDefaults::WINKEYER_TIMING_MIN,
                           CWProfileDefaults::WINKEYER_TIMING_MAX);
    m_leadInSpin->setValue(0);
    m_leadInSpin->setSuffix(" x10ms");
    m_leadInSpin->setToolTip("PTT lead-in time (x10ms)");
    winKeyerLayout->addRow("Lead-in:", m_leadInSpin);

    m_tailTimeSpin = new QSpinBox(this);
    m_tailTimeSpin->setRange(CWProfileDefaults::WINKEYER_TIMING_MIN,
                             CWProfileDefaults::WINKEYER_TIMING_MAX);
    m_tailTimeSpin->setValue(0);
    m_tailTimeSpin->setSuffix(" x10ms");
    m_tailTimeSpin->setToolTip("PTT tail time after last element (x10ms)");
    winKeyerLayout->addRow("Tail Time:", m_tailTimeSpin);

    mainLayout->addWidget(m_winKeyerGroup);

    // === DTR/RTS Settings ===
    m_dtrRtsGroup = new QGroupBox("DTR/RTS Keying", this);
    QFormLayout* dtrLayout = new QFormLayout(m_dtrRtsGroup);

    QHBoxLayout* dtrPortLayout = new QHBoxLayout();
    m_dtrRtsPortCombo = new QComboBox(this);
    m_dtrRtsPortCombo->setEditable(true);
    m_dtrRtsPortCombo->setToolTip("Serial port for DTR/RTS CW keying\n(separate from radio CAT port)");
    m_dtrRtsRefreshPortsButton = new QPushButton("Refresh", this);
    m_dtrRtsRefreshPortsButton->setMaximumWidth(80);
    connect(m_dtrRtsRefreshPortsButton, &QPushButton::clicked,
            this, &CWOutputEditDialog::onRefreshDtrRtsPorts);
    dtrPortLayout->addWidget(m_dtrRtsPortCombo, 1);
    dtrPortLayout->addWidget(m_dtrRtsRefreshPortsButton);
    dtrLayout->addRow("Port:", dtrPortLayout);

    m_dtrRtsPinCombo = new QComboBox(this);
    m_dtrRtsPinCombo->addItem("DTR", 0);
    m_dtrRtsPinCombo->addItem("RTS", 1);
    m_dtrRtsPinCombo->setToolTip("Which serial port line to toggle for CW keying");
    dtrLayout->addRow("Keying Pin:", m_dtrRtsPinCombo);

    mainLayout->addWidget(m_dtrRtsGroup);

    // === Buttons ===
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            DialogHelper::warning(this, "Validation", "Please enter a name for the CW output profile.");
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Initial port population
    onRefreshWinKeyerPorts();
    onRefreshDtrRtsPorts();

    // Show/hide based on default type
    updateVisibility();
}

void CWOutputEditDialog::loadProfileIntoUI(const CWOutputProfile& profile)
{
    m_nameEdit->setText(profile.name);

    // Set type combo
    int typeIndex = m_typeCombo->findData(static_cast<int>(profile.type));
    if (typeIndex >= 0) m_typeCombo->setCurrentIndex(typeIndex);

    // WinKeyer settings — ports already populated in setupUI()
    m_winKeyerPortCombo->setCurrentText(profile.winKeyerPortName);
    m_weightingSpin->setValue(profile.weighting);
    m_leadInSpin->setValue(profile.leadInTime);
    m_tailTimeSpin->setValue(profile.tailTime);

    // DTR/RTS settings — ports already populated in setupUI()
    m_dtrRtsPortCombo->setCurrentText(profile.dtrRtsPortName);
    int pinIndex = m_dtrRtsPinCombo->findData(static_cast<int>(profile.dtrRtsPin));
    if (pinIndex >= 0) m_dtrRtsPinCombo->setCurrentIndex(pinIndex);

    updateVisibility();
}

CWOutputProfile CWOutputEditDialog::getCWOutputProfile() const
{
    CWOutputProfile profile;
    profile.name = m_nameEdit->text().trimmed();
    profile.type = static_cast<CWSenderFactory::Backend>(m_typeCombo->currentData().toInt());

    // Only populate fields relevant to the selected type
    switch (profile.type) {
    case CWSenderFactory::Backend::KeyerDevice:
        profile.winKeyerPortName = m_winKeyerPortCombo->currentText();
        profile.weighting = m_weightingSpin->value();
        profile.leadInTime = m_leadInSpin->value();
        profile.tailTime = m_tailTimeSpin->value();
        break;
    case CWSenderFactory::Backend::DtrRts:
        profile.dtrRtsPortName = m_dtrRtsPortCombo->currentText();
        profile.dtrRtsPin = static_cast<DtrRtsCWSender::Pin>(m_dtrRtsPinCombo->currentData().toInt());
        break;
    default:  // Hamlib: no extra fields
        break;
    }

    return profile;
}

void CWOutputEditDialog::onTypeChanged(int index)
{
    Q_UNUSED(index);
    updateVisibility();
}

void CWOutputEditDialog::onRefreshWinKeyerPorts()
{
    QString previousPort = m_winKeyerPortCombo->currentText();
    m_winKeyerPortCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    QList<QPair<QString, QString>> items;

    for (const auto& port : ports) {
        QString displayText = port.description().isEmpty()
            ? port.portName()
            : QString("%1 (%2)").arg(port.description(), port.portName());
        items.append({displayText, port.portName()});
    }

    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    for (const auto& item : items) {
        m_winKeyerPortCombo->addItem(item.first, item.second);
    }

    // Restore previous selection
    if (!previousPort.isEmpty()) {
        int idx = m_winKeyerPortCombo->findText(previousPort);
        if (idx >= 0) m_winKeyerPortCombo->setCurrentIndex(idx);
    }
}

void CWOutputEditDialog::onRefreshDtrRtsPorts()
{
    QString previousPort = m_dtrRtsPortCombo->currentText();
    m_dtrRtsPortCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    QList<QPair<QString, QString>> items;

    for (const auto& port : ports) {
        QString displayText = port.description().isEmpty()
            ? port.portName()
            : QString("%1 (%2)").arg(port.description(), port.portName());
        items.append({displayText, port.portName()});
    }

    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    for (const auto& item : items) {
        m_dtrRtsPortCombo->addItem(item.first, item.second);
    }

    // Restore previous selection
    if (!previousPort.isEmpty()) {
        int idx = m_dtrRtsPortCombo->findText(previousPort);
        if (idx >= 0) m_dtrRtsPortCombo->setCurrentIndex(idx);
    }
}

void CWOutputEditDialog::updateVisibility()
{
    int type = m_typeCombo->currentData().toInt();
    auto backend = static_cast<CWSenderFactory::Backend>(type);

    m_winKeyerGroup->setVisible(backend == CWSenderFactory::Backend::KeyerDevice);
    m_dtrRtsGroup->setVisible(backend == CWSenderFactory::Backend::DtrRts);
}

} // namespace TR4QT
