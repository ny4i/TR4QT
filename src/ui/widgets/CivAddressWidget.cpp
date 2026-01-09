#include "CivAddressWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QButtonGroup>

namespace TR4QT {

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
    m_civAddressEdit->setToolTip("Enter CI-V address in hex (without 0x prefix)\n"
                                   "Common values:\n"
                                   "  IC-7300: 94\n"
                                   "  IC-7610: 98\n"
                                   "  IC-9700: A2\n"
                                   "  IC-7760: 7C");

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

    // Map known Icom radios to their default CI-V addresses
    QString civAddress;
    if (hamlibModelId == 3078) {  // IC-7610
        civAddress = "98";
    } else if (hamlibModelId == 3092) {  // IC-7760
        civAddress = "7C";
    } else if (hamlibModelId == 3073) {  // IC-7300
        civAddress = "94";
    } else if (hamlibModelId == 3093) {  // IC-9700
        civAddress = "A2";
    }

    // If we found a known address, set it
    if (!civAddress.isEmpty()) {
        m_civCustomRadio->setChecked(true);
        m_civAddressEdit->setText(civAddress);
        m_civAddressEdit->setEnabled(true);
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
