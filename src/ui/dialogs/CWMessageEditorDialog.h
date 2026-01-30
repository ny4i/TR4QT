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

#ifndef CWMESSAGEEDITORDIALOG_H
#define CWMESSAGEEDITORDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

namespace TR4QT {

// Forward declarations
class RadioController;
class ContestBase;

/**
 * @brief CW Message Editor Dialog
 *
 * Allows editing of all TR4W-style CW messages with tabs for:
 * - CQ Mode (F1-F12)
 * - S&P Mode (F1-F12)
 * - Ctrl+F Keys (F1-F12, both modes)
 * - Alt+F Keys (F1-F12, both modes)
 * - Cut Numbers (SHORT_0 through SHORT_9)
 * - Auto-Send Messages (CQ_CW_EXCHANGE, S&P_CW_EXCHANGE, etc.)
 *
 * Features:
 * - Table editor with Key | Template | Caption columns
 * - Live preview showing template substitution
 * - Test Selected button to send message to radio
 * - Load TR4W Defaults button
 * - Template variable help reference
 */
class CWMessageEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit CWMessageEditorDialog(RadioController* radio, ContestBase* contest, QWidget* parent = nullptr);
    ~CWMessageEditorDialog() override = default;

    // Accept/Apply settings
    void accept() override;

private slots:
    void onApply();
    void onLoadDefaults();
    void onTestSelected();
    void onShowHelp();
    void onTableCellChanged(int row, int column);
    void onTableSelectionChanged();

private:
    void setupUI();
    void createTablesForAllModes();
    QTableWidget* createMessageTable();
    QTableWidget* createCutNumbersTable();
    QTableWidget* createAutoSendMessagesTable();
    void loadMessagesFromSettings();
    void loadCQMessages();
    void loadSPMessages();
    void loadCtrlFMessages();
    void loadAltFMessages();
    void loadCutNumbers();
    void loadAutoSendMessages();
    void saveMessagesToSettings();
    void saveCQMessages();
    void saveSPMessages();
    void saveCtrlFMessages();
    void saveAltFMessages();
    void saveCutNumbers();
    void saveAutoSendMessages();
    void updatePreview();
    QString getPreviewText(const QString& templateStr);

    // UI Components
    QTabWidget* m_tabWidget;

    // Tables for each mode
    QTableWidget* m_cqTable;
    QTableWidget* m_spTable;
    QTableWidget* m_ctrlFTable;
    QTableWidget* m_altFTable;
    QTableWidget* m_cutNumbersTable;
    QTableWidget* m_autoSendTable;

    // Preview
    QLabel* m_previewLabel;
    QLineEdit* m_previewText;

    // Buttons
    QPushButton* m_applyButton;
    QPushButton* m_loadDefaultsButton;
    QPushButton* m_testButton;
    QPushButton* m_helpButton;

    // Context for preview
    RadioController* m_radio;
    ContestBase* m_contest;
};

} // namespace TR4QT

#endif // CWMESSAGEEDITORDIALOG_H
