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

#include "FunctionKeysWindow.h"
#include "SendMorseDialog.h"
#include "../../utils/AppSettings.h"
#include "../../core/Constants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

namespace TR4QT {

FunctionKeysWindow::FunctionKeysWindow(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    loadMacros();

    // Auto-refresh every 2 seconds to stay in sync with macro edits
    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &FunctionKeysWindow::refreshMacros);
    m_autoRefreshTimer->start(2000);  // 2 seconds

    setWindowTitle("CW Function Keys Reference");
    resize(UIDefaults::FUNCTION_KEYS_WIDTH, UIDefaults::FUNCTION_KEYS_HEIGHT);
}

void FunctionKeysWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title label
    QLabel* titleLabel = new QLabel("CW Macro Function Key Mappings", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Instructions
    QLabel* instructionsLabel = new QLabel(
        "Press F1-F12 in the Send Morse dialog to transmit the corresponding macro.\n"
        "Right-click macro buttons in Send Morse dialog to edit.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; margin: 5px; }");
    mainLayout->addWidget(instructionsLabel);

    // Table widget
    m_macroTable = new QTableWidget(SendMorseDialog::MACRO_COUNT, 3, this);
    m_macroTable->setHorizontalHeaderLabels({"Function Key", "Label", "CW Text"});

    // Column widths
    m_macroTable->setColumnWidth(0, 100);  // Function Key
    m_macroTable->setColumnWidth(1, 120);  // Label
    m_macroTable->setColumnWidth(2, 350);  // CW Text

    // Make table read-only
    m_macroTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_macroTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_macroTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Alternating row colors for readability
    m_macroTable->setAlternatingRowColors(true);

    // Stretch last column
    m_macroTable->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(m_macroTable);

    // Refresh button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_refreshButton = new QPushButton("Refresh Now", this);
    m_refreshButton->setToolTip("Manually refresh the macro list");
    connect(m_refreshButton, &QPushButton::clicked, this, &FunctionKeysWindow::refreshMacros);
    buttonLayout->addWidget(m_refreshButton);

    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void FunctionKeysWindow::loadMacros() {
    AppSettings& settings = AppSettings::instance();

    for (int i = 0; i < SendMorseDialog::MACRO_COUNT; ++i) {
        // Get macro settings (with defaults if not set)
        QString label = settings.getMacroLabel(i);
        QString cwText = settings.getMacroCWText(i);

        // Use defaults if empty
        if (label.isEmpty()) {
            label = SendMorseDialog::DEFAULT_MACRO_LABELS[i];
        }
        if (cwText.isEmpty()) {
            cwText = SendMorseDialog::DEFAULT_MACRO_TEXTS[i];
        }

        // Function key label
        QTableWidgetItem* keyItem = new QTableWidgetItem(getFunctionKeyLabel(i));
        keyItem->setTextAlignment(Qt::AlignCenter);
        QFont keyFont = keyItem->font();
        keyFont.setBold(true);
        keyItem->setFont(keyFont);
        m_macroTable->setItem(i, 0, keyItem);

        // Macro label
        QTableWidgetItem* labelItem = new QTableWidgetItem(label);
        labelItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QFont labelFont = labelItem->font();
        labelFont.setBold(true);
        labelItem->setFont(labelFont);
        m_macroTable->setItem(i, 1, labelItem);

        // CW text
        QTableWidgetItem* cwItem = new QTableWidgetItem(cwText);
        cwItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_macroTable->setItem(i, 2, cwItem);
    }

    // Adjust row heights for readability
    for (int i = 0; i < SendMorseDialog::MACRO_COUNT; ++i) {
        m_macroTable->setRowHeight(i, 30);
    }
}

void FunctionKeysWindow::refreshMacros() {
    // Reload all macros from settings
    AppSettings& settings = AppSettings::instance();

    for (int i = 0; i < SendMorseDialog::MACRO_COUNT; ++i) {
        QString label = settings.getMacroLabel(i);
        QString cwText = settings.getMacroCWText(i);

        // Use defaults if empty
        if (label.isEmpty()) {
            label = SendMorseDialog::DEFAULT_MACRO_LABELS[i];
        }
        if (cwText.isEmpty()) {
            cwText = SendMorseDialog::DEFAULT_MACRO_TEXTS[i];
        }

        // Update table items
        if (m_macroTable->item(i, 1)) {
            m_macroTable->item(i, 1)->setText(label);
        }
        if (m_macroTable->item(i, 2)) {
            m_macroTable->item(i, 2)->setText(cwText);
        }
    }
}

QString FunctionKeysWindow::getFunctionKeyLabel(int index) const {
    if (index >= 0 && index < 12) {
        return QString("F%1").arg(index + 1);
    }
    return QString();
}

} // namespace TR4QT
