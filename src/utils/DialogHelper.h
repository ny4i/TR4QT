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

#ifndef DIALOGHELPER_H
#define DIALOGHELPER_H

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace TR4QT {

/**
 * DialogHelper - Centralized dialog message handling with automatic logging
 *
 * All dialog messages in the application MUST go through these helpers.
 * This ensures consistent logging of user interactions for debugging.
 *
 * Benefits:
 * - All dialogs automatically logged at INFO level
 * - User responses captured in logs
 * - Easier remote debugging of user issues
 * - Single place to modify dialog behavior
 * - Consistent dialog styling/formatting
 *
 * Usage:
 *   Instead of: QMessageBox::question(this, "Title", "Message");
 *   Use:        DialogHelper::question(this, "Title", "Message");
 */
class DialogHelper {
public:
    /**
     * Show a question dialog (Yes/No or custom buttons)
     * Logs the message and user's response
     */
    static QMessageBox::StandardButton question(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton
    );

    /**
     * Show an information dialog (OK button)
     * Logs the message
     */
    static void information(
        QWidget* parent,
        const QString& title,
        const QString& text
    );

    /**
     * Show an information dialog with expandable detailed text
     * Logs the message and detailed text
     */
    static void informationWithDetails(
        QWidget* parent,
        const QString& title,
        const QString& text,
        const QString& detailedText,
        const QString& styleSheet = QString()
    );

    /**
     * Show a warning dialog (OK or custom buttons)
     * Logs the message and user's response
     */
    static QMessageBox::StandardButton warning(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton
    );

    /**
     * Show a critical error dialog (OK button)
     * Logs the message
     */
    static void critical(
        QWidget* parent,
        const QString& title,
        const QString& text
    );

    /**
     * Show an "About" dialog (simple informational, no icon)
     * Logs the message
     */
    static void about(
        QWidget* parent,
        const QString& title,
        const QString& text
    );

private:
    /**
     * Core message box display with logging and optional text selection
     * All public methods delegate to this for consistent behavior
     */
    static QMessageBox::StandardButton showMessageBox(
        QMessageBox::Icon icon,
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons,
        QMessageBox::StandardButton defaultButton,
        bool textSelectable = true
    );

    /**
     * Convert StandardButton to string for logging
     */
    static QString buttonToString(QMessageBox::StandardButton button);
};

} // namespace TR4QT

#endif // DIALOGHELPER_H
