#include "CivAddressWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QMap>

namespace TR4QT {

// Map Hamlib model IDs to default CI-V addresses for known Icom radios
// Single source of truth - used for both tooltip and auto-configuration
static const QMap<int, QString> HAMLIB_MODEL_TO_DEFAULT_CIV = {
    {3073, "94"},  // IC-7300
    {3078, "98"},  // IC-7610
    {3092, "B2"},  // IC-7760 (0xB2 per Hamlib and IC-7760 manual)
    {3093, "A2"}   // IC-9700
};

CivAddressWidget::CivAddressWidget(QWidget* parent)
    : QWidget(parent)
    , m_civDefaultRadio(nullptr)
    , m_civCustomRadio(nullptr)
    , m_civAddressEdit(nullptr)
{
    setupUI();
}

void CivAddressWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // Add bottom margin to create space before the next QFormLayout row
    mainLayout->setContentsMargins(0, 0, 0, 15);
    mainLayout->setSpacing(8);  // Spacing between the two radio buttons

    // Set fixed height: 2 radio buttons (20px each) + spacing (8px) + bottom margin (15px) = 63px
    setFixedHeight(63);

    // Create button group for mutual exclusivity
    m_civButtonGroup = new QButtonGroup(this);

    // Default CI-V address radio button
    m_civDefaultRadio = new QRadioButton("Use Default CI-V Address", this);
    m_civDefaultRadio->setToolTip("Use radio's default CI-V address (0x00)\n"
                                   "Radio will respond to broadcast commands");
    m_civDefaultRadio->setChecked(true);
    m_civButtonGroup->addButton(m_civDefaultRadio);
    mainLayout->addWidget(m_civDefaultRadio);

    // Custom CI-V address radio button + text field
    QWidget* civCustomWidget = new QWidget(this);
    QHBoxLayout* civCustomLayout = new QHBoxLayout(civCustomWidget);
    civCustomLayout->setContentsMargins(0, 0, 0, 0);
    civCustomLayout->setSpacing(12);  // Increased spacing between radio button and text field

    m_civCustomRadio = new QRadioButton("Custom CI-V Address:", this);
    m_civCustomRadio->setToolTip("Specify a custom CI-V address (hex value)\n"
                                  "Used when multiple Icom radios share a bus");
    m_civButtonGroup->addButton(m_civCustomRadio);

    m_civAddressEdit = new QLineEdit(this);
    m_civAddressEdit->setPlaceholderText("e.g., 94");
    m_civAddressEdit->setMaximumWidth(80);
    m_civAddressEdit->setEnabled(false);  // Disabled by default
    // Build tooltip dynamically from HAMLIB_MODEL_TO_DEFAULT_CIV map
    QString tooltip = "Enter CI-V address in hex (without 0x prefix)\nCommon values:\n";
    tooltip += QString("  IC-7300: %1\n").arg(HAMLIB_MODEL_TO_DEFAULT_CIV.value(3073));
    tooltip += QString("  IC-7610: %1\n").arg(HAMLIB_MODEL_TO_DEFAULT_CIV.value(3078));
    tooltip += QString("  IC-9700: %1\n").arg(HAMLIB_MODEL_TO_DEFAULT_CIV.value(3093));
    tooltip += QString("  IC-7760: %1").arg(HAMLIB_MODEL_TO_DEFAULT_CIV.value(3092));
    m_civAddressEdit->setToolTip(tooltip);

    civCustomLayout->addWidget(m_civCustomRadio);
    civCustomLayout->addWidget(m_civAddressEdit);
    civCustomLayout->addStretch();

    mainLayout->addWidget(civCustomWidget);

    // Connect radio button signals
    connect(m_civDefaultRadio, &QRadioButton::toggled, this, &CivAddressWidget::onCivAddressModeChanged);
    connect(m_civCustomRadio, &QRadioButton::toggled, this, &CivAddressWidget::onCivAddressModeChanged);
}

void CivAddressWidget::setCivAddress(int address) {
    if (address == 0) {
        // Default/auto address
        m_civDefaultRadio->setChecked(true);
        m_civAddressEdit->clear();
        m_civAddressEdit->setEnabled(false);
    } else {
        // Custom address
        m_civCustomRadio->setChecked(true);
        m_civAddressEdit->setText(QString::number(address, 16).toUpper());
        m_civAddressEdit->setEnabled(true);
    }
}

int CivAddressWidget::getCivAddress() const {
    if (m_civDefaultRadio->isChecked()) {
        return 0;  // Default/auto
    } else {
        // Custom address - parse from text field
        QString civText = m_civAddressEdit->text().trimmed();
        if (civText.isEmpty()) {
            return 0;
        }

        // Remove "0x" prefix if present
        if (civText.startsWith("0x", Qt::CaseInsensitive)) {
            civText = civText.mid(2);
        }

        // Parse as hex
        bool ok = false;
        int address = civText.toInt(&ok, 16);
        if (!ok) {
            return 0;  // Invalid input, treat as default
        }

        return address;
    }
}

void CivAddressWidget::autoConfigureForRadio(int hamlibModelId) {
    // Only auto-configure for Icom radios (model IDs 3000-3999)
    if (hamlibModelId < 3000 || hamlibModelId >= 4000) {
        return;
    }

    // Look up CI-V address from HAMLIB_MODEL_TO_DEFAULT_CIV map (single source of truth)
    QString civAddress = HAMLIB_MODEL_TO_DEFAULT_CIV.value(hamlibModelId, QString());

    // For Icom network radios, the CI-V address is auto-discovered during connection.
    // We don't need to auto-select "Custom" - just populate the field for reference.
    // The user should keep "Use Default" selected to let discovery work.
    if (!civAddress.isEmpty()) {
        // Just update the text field, don't change radio button selection
        m_civAddressEdit->setText(civAddress);
        // Note: Leave radio button selection as-is (typically "Default")
    }
}

void CivAddressWidget::onCivAddressModeChanged() {
    bool useCustom = m_civCustomRadio->isChecked();
    m_civAddressEdit->setEnabled(useCustom);

    if (useCustom) {
        // Focus and select text when switching to custom mode
        m_civAddressEdit->setFocus();
        m_civAddressEdit->selectAll();
    } else {
        // Clear text when switching to default mode
        m_civAddressEdit->clear();
    }
}

QSize CivAddressWidget::sizeHint() const {
    // Return fixed height to match what we set in setupUI()
    // Height includes: 2 radio buttons + spacing + bottom margin
    int totalHeight = 63;  // Must match setFixedHeight() value

    // Width: Use the wider of the two radio button rows
    int spacing = 8;
    int defaultWidth = m_civDefaultRadio->sizeHint().width();
    int customWidth = m_civCustomRadio->sizeHint().width() +
                      spacing +
                      m_civAddressEdit->sizeHint().width();
    int totalWidth = qMax(defaultWidth, customWidth);

    return QSize(totalWidth, totalHeight);
}

} // namespace TR4QT
