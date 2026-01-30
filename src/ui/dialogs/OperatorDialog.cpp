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

#include "OperatorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

namespace TR4QT {

OperatorDialog::OperatorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Change Operator");
    setupUI();

    // Set minimum width for comfortable input
    setMinimumWidth(400);
}

void OperatorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Add explanation label
    QLabel* label = new QLabel(
        "Enter the callsign of the current operator.\n"
        "All subsequent QSOs will be logged with this operator.",
        this
    );
    label->setWordWrap(true);
    mainLayout->addWidget(label);

    // Add spacing
    mainLayout->addSpacing(10);

    // Add operator input field
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLabel* operatorLabel = new QLabel("Operator Callsign:", this);
    m_operatorEdit = new QLineEdit(this);
    m_operatorEdit->setPlaceholderText("Enter callsign");
    m_operatorEdit->setMaxLength(15);  // Reasonable callsign length limit

    // Convert to uppercase as user types
    connect(m_operatorEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        int cursorPos = m_operatorEdit->cursorPosition();
        m_operatorEdit->setText(text.toUpper());
        m_operatorEdit->setCursorPosition(cursorPos);
    });

    inputLayout->addWidget(operatorLabel);
    inputLayout->addWidget(m_operatorEdit);
    mainLayout->addLayout(inputLayout);

    // Add spacing
    mainLayout->addSpacing(20);

    // Add OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Set focus to the operator edit field
    m_operatorEdit->setFocus();
}

QString OperatorDialog::getOperatorCallsign() const {
    return m_operatorEdit->text().trimmed().toUpper();
}

void OperatorDialog::setOperatorCallsign(const QString& callsign) {
    m_operatorEdit->setText(callsign.toUpper());
    m_operatorEdit->selectAll();  // Select all text for easy replacement
}

} // namespace TR4QT
