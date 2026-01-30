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

#ifndef FUNCTIONKEYSWINDOW_H
#define FUNCTIONKEYSWINDOW_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QTimer>

namespace TR4QT {

/**
 * Function Keys Reference Window
 *
 * Displays F1-F12 CW macro key mappings in a non-modal window.
 * Shows the function key, macro label, and CW text for each of the 12 macros.
 * Auto-refreshes every 2 seconds to stay in sync with macro edits.
 *
 * Usage:
 * - Window → Function Keys Reference
 * - Keep open during operation for quick reference
 * - Changes in Send Morse dialog automatically reflected
 */
class FunctionKeysWindow : public QDialog {
    Q_OBJECT

public:
    explicit FunctionKeysWindow(QWidget* parent = nullptr);
    ~FunctionKeysWindow() override = default;

private slots:
    void refreshMacros();

private:
    void setupUI();
    void loadMacros();
    QString getFunctionKeyLabel(int index) const;

    QTableWidget* m_macroTable;
    QPushButton* m_refreshButton;
    QTimer* m_autoRefreshTimer;
};

} // namespace TR4QT

#endif // FUNCTIONKEYSWINDOW_H
