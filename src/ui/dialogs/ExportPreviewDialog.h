#ifndef EXPORTPREVIEWDIALOG_H
#define EXPORTPREVIEWDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

namespace TR4QT {

/**
 * Dialog for previewing export data before saving to file
 * 
 * Shows the export content (ADIF, Cabrillo, etc.) in a text editor
 * with options to copy to clipboard or save to file.
 */
class ExportPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportPreviewDialog(const QString& title,
                                const QString& content,
                                const QString& fileFilter,
                                const QString& defaultFileName,
                                QWidget* parent = nullptr);
    ~ExportPreviewDialog() override = default;

    /**
     * Get the file path if user chose to save
     * Empty string if user cancelled or only copied
     */
    QString getSaveFilePath() const { return m_saveFilePath; }

    /**
     * Check if user saved the file
     */
    bool wasSaved() const { return !m_saveFilePath.isEmpty(); }

private slots:
    void onCopyToClipboard();
    void onSaveToFile();

private:
    void setupUI();

    QString m_content;
    QString m_fileFilter;
    QString m_defaultFileName;
    QString m_saveFilePath;

    QTextEdit* m_textEdit;
    QLabel* m_infoLabel;
    QPushButton* m_copyButton;
    QPushButton* m_saveButton;
    QPushButton* m_closeButton;
};

} // namespace TR4QT

#endif // EXPORTPREVIEWDIALOG_H
