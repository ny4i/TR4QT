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

#include "MacroEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>

namespace TR4QT {

MacroEditDialog::MacroEditDialog(int macroIndex, const QString& currentLabel,
                                 const QString& currentText, QWidget* parent)
    : QDialog(parent)
    , m_macroIndex(macroIndex)
{
    setupUI(currentLabel, currentText);
    setWindowTitle(QString("Edit Macro %1").arg(macroIndex + 1));
    setMinimumWidth(350);
}

void MacroEditDialog::setupUI(const QString& currentLabel, const QString& currentText) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout for label and CW text
    QFormLayout* formLayout = new QFormLayout();

    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setText(currentLabel);
    m_labelEdit->setPlaceholderText("Button label (e.g., CQ)");
    m_labelEdit->setMaxLength(10);  // Keep labels short for button display
    formLayout->addRow("Button Label:", m_labelEdit);

    m_cwTextEdit = new QLineEdit(this);
    m_cwTextEdit->setText(currentText);
    m_cwTextEdit->setPlaceholderText("CW text to send (e.g., CQ TEST)");
    formLayout->addRow("CW Text:", m_cwTextEdit);

    mainLayout->addLayout(formLayout);

    // Help text
    QLabel* helpLabel = new QLabel(
        "The button label is what appears on the button.\n"
        "The CW text is what gets sent when you click the button.",
        this);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 11px; }");
    helpLabel->setWordWrap(true);
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    QPushButton* cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);
}

QString MacroEditDialog::getLabel() const {
    return m_labelEdit->text().trimmed();
}

QString MacroEditDialog::getCWText() const {
    return m_cwTextEdit->text().trimmed();
}

} // namespace TR4QT
