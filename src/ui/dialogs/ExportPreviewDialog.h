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
