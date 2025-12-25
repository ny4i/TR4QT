#ifndef EDITQSODIALOG_H
#define EDITQSODIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include "../../models/QSO.h"

namespace TR4QT {

/**
 * Dialog for editing QSO records
 *
 * Allows editing all fields in a QSO, including:
 * - Basic info (callsign, timestamp, frequency)
 * - Exchange data (RST, contest exchange)
 * - Geographic info (DXCC, zones, state)
 * - Metadata (serial number, operator, notes)
 */
class EditQSODialog : public QDialog {
    Q_OBJECT

public:
    explicit EditQSODialog(const QSO& qso, QWidget* parent = nullptr);
    ~EditQSODialog() override = default;

    /**
     * Get the edited QSO with all field changes
     */
    QSO getEditedQSO() const;

private slots:
    void onAccept();

private:
    void setupUI();
    void loadQSOData();
    void populateModeCombo();
    void populateBandCombo();

    // Original QSO being edited
    QSO m_qso;

    // Basic fields
    QDateTimeEdit* m_timestampEdit;
    QLineEdit* m_callsignEdit;
    QLineEdit* m_frequencyEdit;
    QComboBox* m_modeCombo;
    QComboBox* m_bandCombo;

    // Exchange fields
    QLineEdit* m_rstSentEdit;
    QLineEdit* m_rstReceivedEdit;
    QLineEdit* m_exchangeSentEdit;
    QLineEdit* m_exchangeReceivedEdit;

    // Geographic fields
    QLineEdit* m_dxccEntityEdit;      // Read-only
    QLineEdit* m_dxccPrefixEdit;      // Read-only
    QSpinBox* m_cqZoneSpinBox;
    QSpinBox* m_ituZoneSpinBox;
    QLineEdit* m_continentEdit;
    QLineEdit* m_stateEdit;

    // Scoring fields (read-only)
    QSpinBox* m_qsoPointsSpinBox;
    QCheckBox* m_isDupeCheckBox;
    QCheckBox* m_isMultiplierCheckBox;

    // Metadata fields
    QSpinBox* m_serialNumberSpinBox;
    QLineEdit* m_operatorCallEdit;
    QTextEdit* m_notesEdit;
};

} // namespace TR4QT

#endif // EDITQSODIALOG_H
