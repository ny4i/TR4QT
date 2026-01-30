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

/**
 * QSOSearchPanel - Implementation
 */

#include "QSOSearchPanel.h"
#include "../models/QSOTableModel.h"
#include "../../core/Constants.h"
#include "../../utils/ThemeManager.h"
#include "../../utils/FontManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>

namespace TR4QT {

QSOSearchPanel::QSOSearchPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    hide();  // Start hidden
}

void QSOSearchPanel::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar: result count + close button
    QWidget* headerBar = new QWidget(this);
    headerBar->setAutoFillBackground(true);
    QString headerBg = ThemeManager::instance().colorName(ColorRole::WindowBackground);
    QString headerFg = ThemeManager::instance().colorName(ColorRole::PrimaryText);
    QString borderColor = ThemeManager::instance().colorName(ColorRole::BorderColor);
    headerBar->setStyleSheet(
        QString("QWidget { background: %1; border-bottom: 1px solid %2; }")
            .arg(headerBg, borderColor));

    QHBoxLayout* headerLayout = new QHBoxLayout(headerBar);
    const int HEADER_MARGIN = 4;
    headerLayout->setContentsMargins(HEADER_MARGIN + 4, HEADER_MARGIN, HEADER_MARGIN, HEADER_MARGIN);

    m_headerLabel = new QLabel("Search Results", this);
    m_headerLabel->setStyleSheet(
        QString("QLabel { color: %1; font-weight: bold; border: none; }").arg(headerFg));
    headerLayout->addWidget(m_headerLabel);

    headerLayout->addStretch();

    m_closeButton = new QPushButton("X", this);
    const int CLOSE_BUTTON_SIZE = 20;
    m_closeButton->setFixedSize(CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE);
    m_closeButton->setFlat(true);
    m_closeButton->setStyleSheet(
        QString("QPushButton { color: %1; font-weight: bold; border: none; } "
                "QPushButton:hover { background: %2; }")
            .arg(headerFg)
            .arg(ThemeManager::instance().colorName(ColorRole::HoverHighlight)));
    connect(m_closeButton, &QPushButton::clicked, this, &QSOSearchPanel::closeRequested);
    headerLayout->addWidget(m_closeButton);

    layout->addWidget(headerBar);

    // Results table
    m_tableModel = new QSOTableModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_tableModel);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);

    // Force visible bottom border on header (consistent with main QSO table)
    m_tableView->horizontalHeader()->setStyleSheet(
        QString("QHeaderView::section { "
                "border: none; "
                "border-bottom: 2px solid %1; "
                "padding: 2px 4px; "
                "background: %2; "
                "}")
            .arg(ThemeManager::instance().colorName(ColorRole::BorderColor))
            .arg(ThemeManager::instance().colorName(ColorRole::WindowBackground)));

    connect(m_tableView, &QTableView::doubleClicked,
            this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QSO qso = m_tableModel->getQSO(index.row());
            emit qsoSelected(qso);
        }
    });

    layout->addWidget(m_tableView, 1);
}

void QSOSearchPanel::syncAppearance(const QTableView* sourceTable) {
    if (!sourceTable) return;

    // Match font
    m_tableView->setFont(sourceTable->font());

    // Match column widths
    const QHeaderView* srcHeader = sourceTable->horizontalHeader();
    QHeaderView* dstHeader = m_tableView->horizontalHeader();
    int cols = qMin(srcHeader->count(), dstHeader->count());
    for (int i = 0; i < cols; ++i) {
        // Skip the last column (stretch)
        if (srcHeader->sectionResizeMode(i) == QHeaderView::Stretch) {
            dstHeader->setSectionResizeMode(i, QHeaderView::Stretch);
        } else {
            dstHeader->setSectionResizeMode(i, QHeaderView::Interactive);
            m_tableView->setColumnWidth(i, sourceTable->columnWidth(i));
        }
    }
}

void QSOSearchPanel::setResults(const QList<QSO>& results) {
    m_tableModel->setQSOs(results);
    bool limitReached = (results.size() >= SearchLimits::MAX_SEARCH_RESULTS);
    m_headerLabel->setText(
        QString("Search Results: %1 QSO%2 found%3")
            .arg(results.size())
            .arg(results.size() != 1 ? "s" : "")
            .arg(limitReached ? " (limit reached - refine search)" : ""));
    show();
}

void QSOSearchPanel::setContest(ContestBase* contest) {
    if (contest) {
        m_tableModel->setContestExchangeFields(contest->getReceivedExchangeFields());
    }
}

void QSOSearchPanel::clear() {
    m_tableModel->clear();
    m_headerLabel->setText("Search Results");
    hide();
}

void QSOSearchPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit closeRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

}  // namespace TR4QT
