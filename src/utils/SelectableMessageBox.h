#ifndef SELECTABLEMESSAGEBOX_H
#define SELECTABLEMESSAGEBOX_H

#include <QMessageBox>
#include <QWidget>
#include <QString>

namespace TR4QT {

/**
 * Utility wrapper for QMessageBox that makes text selectable/copyable
 *
 * All message boxes created through this class will have text that can be
 * selected with the mouse and copied with Ctrl+C (Cmd+C on macOS).
 *
 * Additionally, all messages are automatically logged to help with debugging:
 * - critical() messages → LOG_ERROR
 * - warning() messages → LOG_WARN
 * - information() messages → LOG_INFO
 * - question() messages → LOG_INFO
 *
 * This is especially useful for error messages containing technical details,
 * file paths, or other information users may want to copy for support requests.
 *
 * Usage:
 *   // Instead of:
 *   QMessageBox::warning(this, "Error", "File not found: /path/to/file");
 *
 *   // Use:
 *   SelectableMessageBox::warning(this, "Error", "File not found: /path/to/file");
 *   // This will show the dialog AND log: [WARN] [MessageBox] [Error] File not found: /path/to/file
 */
class SelectableMessageBox {
public:
    /**
     * Show warning dialog with selectable text
     */
    static QMessageBox::StandardButton warning(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    /**
     * Show critical/error dialog with selectable text
     */
    static QMessageBox::StandardButton critical(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    /**
     * Show information dialog with selectable text
     */
    static QMessageBox::StandardButton information(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    /**
     * Show question dialog with selectable text
     */
    static QMessageBox::StandardButton question(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

private:
    /**
     * Create and configure a message box with selectable text
     */
    static QMessageBox::StandardButton showMessageBox(
        QMessageBox::Icon icon,
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons,
        QMessageBox::StandardButton defaultButton);

    /**
     * Convert StandardButton to string for logging
     */
    static QString buttonToString(QMessageBox::StandardButton button);
};

} // namespace TR4QT

#endif // SELECTABLEMESSAGEBOX_H
