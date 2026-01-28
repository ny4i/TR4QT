/**
 * QSOSearchDialog - Implementation
 */

#include "QSOSearchDialog.h"
#include "../../core/Constants.h"
#include <QTimeZone>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

namespace TR4QT {

QSOSearchDialog::QSOSearchDialog(int currentContestId, QWidget* parent)
    : QDialog(parent)
    , m_currentContestId(currentContestId)
{
    setWindowTitle("Search QSOs");
    setupUI();
}

void QSOSearchDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();
    const int FORM_SPACING = 8;
    formLayout->setSpacing(FORM_SPACING);

    // Callsign (partial match)
    m_callsignEdit = new QLineEdit(this);
    m_callsignEdit->setPlaceholderText("Partial match (e.g., W1A)");
    m_callsignEdit->setMaxLength(SearchLimits::MAX_CALLSIGN_LENGTH);
    formLayout->addRow("Callsign:", m_callsignEdit);

    // Operator
    m_operatorEdit = new QLineEdit(this);
    m_operatorEdit->setPlaceholderText("Exact match");
    m_operatorEdit->setMaxLength(SearchLimits::MAX_CALLSIGN_LENGTH);
    formLayout->addRow("Operator:", m_operatorEdit);

    // Time range
    m_startTimeEdit = new QDateTimeEdit(this);
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_startTimeEdit->setCalendarPopup(true);
    m_startTimeEdit->setSpecialValueText("No limit");
    m_startTimeEdit->setDateTime(m_startTimeEdit->minimumDateTime());
    formLayout->addRow("From (UTC):", m_startTimeEdit);

    m_endTimeEdit = new QDateTimeEdit(this);
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_endTimeEdit->setCalendarPopup(true);
    m_endTimeEdit->setSpecialValueText("No limit");
    m_endTimeEdit->setDateTime(m_endTimeEdit->minimumDateTime());
    formLayout->addRow("To (UTC):", m_endTimeEdit);

    // Search all contests checkbox
    m_searchAllContestsCheckBox = new QCheckBox("Search all contests", this);
    m_searchAllContestsCheckBox->setChecked(false);
    formLayout->addRow("", m_searchAllContestsCheckBox);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(FORM_SPACING);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Search");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Focus callsign field
    m_callsignEdit->setFocus();

    setMinimumWidth(UIDefaults::SEARCH_DIALOG_MIN_WIDTH);
}

QSOSearchCriteria QSOSearchDialog::getCriteria() const {
    QSOSearchCriteria criteria;

    criteria.callsign = m_callsignEdit->text().trimmed().toUpper();
    criteria.operatorCall = m_operatorEdit->text().trimmed().toUpper();

    // Only set time if user changed from the "no limit" default
    if (m_startTimeEdit->dateTime() != m_startTimeEdit->minimumDateTime()) {
        criteria.startTime = m_startTimeEdit->dateTime();
        criteria.startTime.setTimeZone(QTimeZone::utc());
    }

    if (m_endTimeEdit->dateTime() != m_endTimeEdit->minimumDateTime()) {
        criteria.endTime = m_endTimeEdit->dateTime();
        criteria.endTime.setTimeZone(QTimeZone::utc());
    }

    // Scope to current contest unless "search all" is checked
    if (!m_searchAllContestsCheckBox->isChecked() && m_currentContestId >= 0) {
        criteria.contestId = m_currentContestId;
    }

    return criteria;
}

}  // namespace TR4QT
