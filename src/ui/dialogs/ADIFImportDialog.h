#ifndef ADIFIMPORTDIALOG_H
#define ADIFIMPORTDIALOG_H

#include <QDialog>
#include <QList>
#include "../../models/QSO.h"

class QLabel;
class QTextEdit;
class QPushButton;
class QProgressBar;
class QCheckBox;

namespace TR4QT {

class CountryFile;

/**
 * Unified ADIF import dialog
 *
 * Single dialog that handles:
 * - File selection
 * - Import progress/status
 * - Results summary (imported count, failed count, warnings)
 *
 * Separate from MainWindow - no god class anti-pattern.
 */
class ADIFImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ADIFImportDialog(CountryFile* countryFile, QWidget* parent = nullptr);
    ~ADIFImportDialog() = default;

    /**
     * Get the imported QSOs (call after dialog accepted)
     */
    QList<QSO> getImportedQSOs() const { return m_importedQSOs; }

    /**
     * Check if user wants to rescore contest after import
     */
    bool shouldRescore() const;

private slots:
    void onSelectFileClicked();
    void onImportClicked();

private:
    void setupUI();
    void updateStatus(const QString& message);
    void showResults(int imported, int failed, const QStringList& warnings,
                     int validationErrors, int validationWarnings);

    // UI elements
    QLabel* m_fileLabel;
    QPushButton* m_selectFileButton;
    QPushButton* m_importButton;
    QPushButton* m_closeButton;
    QCheckBox* m_autoCorrectCheckBox;
    QCheckBox* m_rescoreCheckBox;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QTextEdit* m_resultsText;

    // Data
    QString m_selectedFile;
    QList<QSO> m_importedQSOs;
    QString m_validationDetails;  // Detailed validation errors for display
    CountryFile* m_countryFile;   // Pointer to MainWindow's CountryFile (not owned)
};

} // namespace TR4QT

#endif // ADIFIMPORTDIALOG_H
