#include "EditQSODialog.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

namespace TR4QT {

EditQSODialog::EditQSODialog(const QSO& qso, ContestBase* contest, QWidget* parent)
    : QDialog(parent)
    , m_qso(qso)
    , m_contest(contest)
{
    setupUI();
    loadQSOData();
}

void EditQSODialog::setupUI() {
    setWindowTitle("Edit QSO");
    setMinimumWidth(600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Basic Information Group
    QGroupBox* basicGroup = new QGroupBox("Basic Information", this);
    QGridLayout* basicLayout = new QGridLayout(basicGroup);

    basicLayout->addWidget(new QLabel("GUID:"), 0, 0);
    m_guidEdit = new QLineEdit(this);
    m_guidEdit->setReadOnly(true);
    m_guidEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
    basicLayout->addWidget(m_guidEdit, 0, 1);

    basicLayout->addWidget(new QLabel("Timestamp (UTC):"), 1, 0);
    m_timestampEdit = new QDateTimeEdit(this);
    m_timestampEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_timestampEdit->setCalendarPopup(true);
    basicLayout->addWidget(m_timestampEdit, 1, 1);

    basicLayout->addWidget(new QLabel("Callsign:"), 2, 0);
    m_callsignEdit = new QLineEdit(this);
    basicLayout->addWidget(m_callsignEdit, 2, 1);

    basicLayout->addWidget(new QLabel("Frequency (Hz):"), 3, 0);
    m_frequencyEdit = new QLineEdit(this);
    basicLayout->addWidget(m_frequencyEdit, 3, 1);

    basicLayout->addWidget(new QLabel("Mode:"), 4, 0);
    m_modeCombo = new QComboBox(this);
    populateModeCombo();
    basicLayout->addWidget(m_modeCombo, 4, 1);

    basicLayout->addWidget(new QLabel("Band:"), 5, 0);
    m_bandCombo = new QComboBox(this);
    populateBandCombo();
    basicLayout->addWidget(m_bandCombo, 5, 1);

    mainLayout->addWidget(basicGroup);

    // Exchange Information Group
    QGroupBox* exchangeGroup = new QGroupBox("Exchange Information", this);
    QGridLayout* exchangeLayout = new QGridLayout(exchangeGroup);

    exchangeLayout->addWidget(new QLabel("RST Sent:"), 0, 0);
    m_rstSentEdit = new QLineEdit(this);
    exchangeLayout->addWidget(m_rstSentEdit, 0, 1);

    exchangeLayout->addWidget(new QLabel("RST Received:"), 0, 2);
    m_rstReceivedEdit = new QLineEdit(this);
    exchangeLayout->addWidget(m_rstReceivedEdit, 0, 3);

    exchangeLayout->addWidget(new QLabel("Exchange Sent:"), 1, 0);
    m_exchangeSentEdit = new QLineEdit(this);
    exchangeLayout->addWidget(m_exchangeSentEdit, 1, 1);

    exchangeLayout->addWidget(new QLabel("Exchange Received:"), 1, 2);
    m_exchangeReceivedEdit = new QLineEdit(this);
    exchangeLayout->addWidget(m_exchangeReceivedEdit, 1, 3);

    mainLayout->addWidget(exchangeGroup);

    // Geographic Information Group
    QGroupBox* geoGroup = new QGroupBox("Geographic Information", this);
    QGridLayout* geoLayout = new QGridLayout(geoGroup);

    geoLayout->addWidget(new QLabel("DXCC Entity:"), 0, 0);
    m_dxccEntityEdit = new QLineEdit(this);
    m_dxccEntityEdit->setReadOnly(true);
    m_dxccEntityEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
    geoLayout->addWidget(m_dxccEntityEdit, 0, 1);

    geoLayout->addWidget(new QLabel("DXCC Prefix:"), 0, 2);
    m_dxccPrefixEdit = new QLineEdit(this);
    m_dxccPrefixEdit->setReadOnly(true);
    m_dxccPrefixEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
    geoLayout->addWidget(m_dxccPrefixEdit, 0, 3);

    geoLayout->addWidget(new QLabel("CQ Zone:"), 1, 0);
    m_cqZoneSpinBox = new QSpinBox(this);
    m_cqZoneSpinBox->setRange(0, 40);
    geoLayout->addWidget(m_cqZoneSpinBox, 1, 1);

    geoLayout->addWidget(new QLabel("ITU Zone:"), 1, 2);
    m_ituZoneSpinBox = new QSpinBox(this);
    m_ituZoneSpinBox->setRange(0, 90);
    geoLayout->addWidget(m_ituZoneSpinBox, 1, 3);

    geoLayout->addWidget(new QLabel("Continent:"), 2, 0);
    m_continentEdit = new QLineEdit(this);
    geoLayout->addWidget(m_continentEdit, 2, 1);

    geoLayout->addWidget(new QLabel("State/Province:"), 2, 2);
    m_stateEdit = new QLineEdit(this);
    geoLayout->addWidget(m_stateEdit, 2, 3);

    geoLayout->addWidget(new QLabel("County:"), 3, 0);
    m_countyEdit = new QLineEdit(this);
    geoLayout->addWidget(m_countyEdit, 3, 1);

    geoLayout->addWidget(new QLabel("ARRL Section:"), 3, 2);
    m_arrlSectionEdit = new QLineEdit(this);
    geoLayout->addWidget(m_arrlSectionEdit, 3, 3);

    geoLayout->addWidget(new QLabel("Contest Class:"), 4, 0);
    m_contestClassEdit = new QLineEdit(this);
    geoLayout->addWidget(m_contestClassEdit, 4, 1);

    mainLayout->addWidget(geoGroup);

    // Scoring and Metadata Group
    QGroupBox* metaGroup = new QGroupBox("Scoring and Metadata", this);
    QGridLayout* metaLayout = new QGridLayout(metaGroup);

    metaLayout->addWidget(new QLabel("QSO Points:"), 0, 0);
    m_qsoPointsSpinBox = new QSpinBox(this);
    m_qsoPointsSpinBox->setRange(0, 999);
    m_qsoPointsSpinBox->setReadOnly(true);
    m_qsoPointsSpinBox->setStyleSheet("QSpinBox { background-color: #f0f0f0; }");
    metaLayout->addWidget(m_qsoPointsSpinBox, 0, 1);

    m_isDupeCheckBox = new QCheckBox("Is Duplicate", this);
    m_isDupeCheckBox->setEnabled(false);
    metaLayout->addWidget(m_isDupeCheckBox, 0, 2);

    m_isMultiplierCheckBox = new QCheckBox("Is Multiplier", this);
    m_isMultiplierCheckBox->setEnabled(false);
    metaLayout->addWidget(m_isMultiplierCheckBox, 0, 3);

    metaLayout->addWidget(new QLabel("Serial Number:"), 1, 0);
    m_serialNumberSpinBox = new QSpinBox(this);
    m_serialNumberSpinBox->setRange(0, 99999);
    metaLayout->addWidget(m_serialNumberSpinBox, 1, 1);

    metaLayout->addWidget(new QLabel("Operator:"), 1, 2);
    m_operatorCallEdit = new QLineEdit(this);
    metaLayout->addWidget(m_operatorCallEdit, 1, 3);

    metaLayout->addWidget(new QLabel("Notes:"), 2, 0);
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    metaLayout->addWidget(m_notesEdit, 2, 1, 1, 3);

    mainLayout->addWidget(metaGroup);

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditQSODialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void EditQSODialog::populateModeCombo() {
    m_modeCombo->addItem("None", static_cast<int>(ModeType::None));
    m_modeCombo->addItem("CW", static_cast<int>(ModeType::CW));
    m_modeCombo->addItem("CWR", static_cast<int>(ModeType::CWR));
    m_modeCombo->addItem("USB", static_cast<int>(ModeType::USB));
    m_modeCombo->addItem("LSB", static_cast<int>(ModeType::LSB));
    m_modeCombo->addItem("FM", static_cast<int>(ModeType::FM));
    m_modeCombo->addItem("AM", static_cast<int>(ModeType::AM));
    m_modeCombo->addItem("RTTY", static_cast<int>(ModeType::RTTY));
    m_modeCombo->addItem("RTTYR", static_cast<int>(ModeType::RTTYR));
    m_modeCombo->addItem("PSK", static_cast<int>(ModeType::PSK));
    m_modeCombo->addItem("PSKR", static_cast<int>(ModeType::PSKR));
    m_modeCombo->addItem("FT8", static_cast<int>(ModeType::FT8));
    m_modeCombo->addItem("FT4", static_cast<int>(ModeType::FT4));
    m_modeCombo->addItem("DATA", static_cast<int>(ModeType::DATA));
    m_modeCombo->addItem("DATAR", static_cast<int>(ModeType::DATAR));
}

void EditQSODialog::populateBandCombo() {
    m_bandCombo->addItem("None", static_cast<int>(BandType::None));
    m_bandCombo->addItem("160m", static_cast<int>(BandType::Band160M));
    m_bandCombo->addItem("80m", static_cast<int>(BandType::Band80M));
    m_bandCombo->addItem("60m", static_cast<int>(BandType::Band60M));
    m_bandCombo->addItem("40m", static_cast<int>(BandType::Band40M));
    m_bandCombo->addItem("30m", static_cast<int>(BandType::Band30M));
    m_bandCombo->addItem("20m", static_cast<int>(BandType::Band20M));
    m_bandCombo->addItem("17m", static_cast<int>(BandType::Band17M));
    m_bandCombo->addItem("15m", static_cast<int>(BandType::Band15M));
    m_bandCombo->addItem("12m", static_cast<int>(BandType::Band12M));
    m_bandCombo->addItem("10m", static_cast<int>(BandType::Band10M));
    m_bandCombo->addItem("6m", static_cast<int>(BandType::Band6M));
    m_bandCombo->addItem("2m", static_cast<int>(BandType::Band2M));
}

void EditQSODialog::loadQSOData() {
    // Basic fields
    m_guidEdit->setText(m_qso.guid);
    m_timestampEdit->setDateTime(m_qso.timestamp);
    m_callsignEdit->setText(m_qso.callsign);
    // Format frequency as integer (never use scientific notation)
    m_frequencyEdit->setText(QString::number(static_cast<qint64>(m_qso.frequency)));

    // Set mode combo
    int modeIndex = m_modeCombo->findData(static_cast<int>(m_qso.mode));
    if (modeIndex >= 0) {
        m_modeCombo->setCurrentIndex(modeIndex);
    }

    // Set band combo
    int bandIndex = m_bandCombo->findData(static_cast<int>(m_qso.band));
    if (bandIndex >= 0) {
        m_bandCombo->setCurrentIndex(bandIndex);
    }

    // Exchange fields
    m_rstSentEdit->setText(m_qso.rstSent);
    m_rstReceivedEdit->setText(m_qso.rstReceived);
    m_exchangeSentEdit->setText(m_qso.exchangeSent);
    m_exchangeReceivedEdit->setText(m_qso.exchangeReceived);

    // Geographic fields
    m_dxccEntityEdit->setText(m_qso.dxccEntity);
    m_dxccPrefixEdit->setText(m_qso.dxccPrefix);
    m_cqZoneSpinBox->setValue(m_qso.cqZone);
    m_ituZoneSpinBox->setValue(m_qso.ituZone);
    m_continentEdit->setText(m_qso.continent);
    m_stateEdit->setText(m_qso.state);
    m_countyEdit->setText(m_qso.county);
    m_arrlSectionEdit->setText(m_qso.arrlSection);
    m_contestClassEdit->setText(m_qso.contestClass);

    // Scoring fields (read-only)
    m_qsoPointsSpinBox->setValue(m_qso.qsoPoints);
    m_isDupeCheckBox->setChecked(m_qso.isDupe);
    m_isMultiplierCheckBox->setChecked(m_qso.isMultiplier);

    // Metadata fields
    m_serialNumberSpinBox->setValue(m_qso.serialNumber);
    m_operatorCallEdit->setText(m_qso.operatorCall);
    m_notesEdit->setPlainText(m_qso.notes);
}

QSO EditQSODialog::getEditedQSO() const {
    QSO editedQSO = m_qso;  // Start with original QSO (preserves ID, etc.)

    // Update with edited values
    editedQSO.timestamp = m_timestampEdit->dateTime();
    editedQSO.callsign = m_callsignEdit->text().trimmed().toUpper();
    editedQSO.frequency = m_frequencyEdit->text().toLongLong();
    editedQSO.mode = static_cast<ModeType>(m_modeCombo->currentData().toInt());
    editedQSO.band = static_cast<BandType>(m_bandCombo->currentData().toInt());

    editedQSO.rstSent = m_rstSentEdit->text();
    editedQSO.rstReceived = m_rstReceivedEdit->text();
    editedQSO.exchangeSent = m_exchangeSentEdit->text();
    editedQSO.exchangeReceived = m_exchangeReceivedEdit->text();

    // Geographic fields (DXCC entity/prefix are read-only, don't update)
    editedQSO.cqZone = m_cqZoneSpinBox->value();
    editedQSO.ituZone = m_ituZoneSpinBox->value();
    editedQSO.continent = m_continentEdit->text();
    editedQSO.state = m_stateEdit->text();
    editedQSO.county = m_countyEdit->text();
    editedQSO.arrlSection = m_arrlSectionEdit->text();
    editedQSO.contestClass = m_contestClassEdit->text();

    // Scoring fields are read-only, don't update

    // Metadata
    editedQSO.serialNumber = m_serialNumberSpinBox->value();
    editedQSO.operatorCall = m_operatorCallEdit->text();
    editedQSO.notes = m_notesEdit->toPlainText();

    return editedQSO;
}

void EditQSODialog::onAccept() {
    // Validate exchange if we have a contest
    if (m_contest) {
        QString exchange = m_exchangeReceivedEdit->text().trimmed();

        if (!exchange.isEmpty()) {
            QString errorMsg;
            if (!m_contest->validateReceivedExchange(exchange, errorMsg)) {
                QMessageBox::warning(this, "Invalid Exchange",
                    QString("The exchange is not valid for this contest:\n\n%1\n\n"
                            "Please correct the exchange before saving.")
                        .arg(errorMsg));
                m_exchangeReceivedEdit->setFocus();
                m_exchangeReceivedEdit->selectAll();
                return;  // Don't accept the dialog
            }
        }
    }

    accept();
}

} // namespace TR4QT
