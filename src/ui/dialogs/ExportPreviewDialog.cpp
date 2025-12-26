#include "ExportPreviewDialog.h"
#include "../../utils/AppSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QFont>
#include <QTimer>
#include <QDir>

namespace TR4QT {

ExportPreviewDialog::ExportPreviewDialog(const QString& title,
                                       const QString& content,
                                       const QString& fileFilter,
                                       const QString& defaultFileName,
                                       QWidget* parent)
    : QDialog(parent)
    , m_content(content)
    , m_fileFilter(fileFilter)
    , m_defaultFileName(defaultFileName)
{
    setWindowTitle(title);
    setupUI();
    
    // Set the content
    m_textEdit->setPlainText(content);
    
    // Show line count and size
    int lineCount = content.count('\n') + 1;
    int charCount = content.length();
    m_infoLabel->setText(QString("%1 lines, %2 characters")
                        .arg(lineCount).arg(charCount));
}

void ExportPreviewDialog::setupUI() {
    resize(800, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Info label at top
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    mainLayout->addWidget(m_infoLabel);
    
    // Text edit for preview
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QTextEdit::NoWrap);

    // Use monospace font with size from appearance settings
    QFont monoFont("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    int fontSize = AppSettings::instance().getMiscDisplayFontSize();
    monoFont.setPointSize(fontSize);
    m_textEdit->setFont(monoFont);
    
    mainLayout->addWidget(m_textEdit);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_copyButton = new QPushButton("Copy to Clipboard", this);
    connect(m_copyButton, &QPushButton::clicked, this, &ExportPreviewDialog::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);
    
    m_saveButton = new QPushButton("Save to File...", this);
    m_saveButton->setDefault(true);
    connect(m_saveButton, &QPushButton::clicked, this, &ExportPreviewDialog::onSaveToFile);
    buttonLayout->addWidget(m_saveButton);
    
    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void ExportPreviewDialog::onCopyToClipboard() {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(m_content);
    
    m_infoLabel->setText(m_infoLabel->text() + " - Copied to clipboard!");
    
    // Show temporary confirmation
    QTimer::singleShot(2000, this, [this]() {
        int lineCount = m_content.count('\n') + 1;
        int charCount = m_content.length();
        m_infoLabel->setText(QString("%1 lines, %2 characters")
                            .arg(lineCount).arg(charCount));
    });
}

void ExportPreviewDialog::onSaveToFile() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Export File",
        QDir::homePath() + "/" + m_defaultFileName,
        m_fileFilter);
    
    if (fileName.isEmpty()) {
        return;  // User cancelled
    }
    
    // Save to file
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Save Error",
                            QString("Failed to open file for writing:\n%1")
                                .arg(file.errorString()));
        return;
    }
    
    QTextStream out(&file);
    out << m_content;
    file.close();
    
    m_saveFilePath = fileName;
    
    // Show success message
    QMessageBox::information(this, "Export Saved",
                           QString("Export saved successfully to:\n%1")
                               .arg(QFileInfo(fileName).fileName()));
    
    // Close the dialog
    accept();
}

} // namespace TR4QT
