#include "SelectableMessageBox.h"
#include "../logging/LogMacros.h"
#include <QLabel>

namespace TR4QT {

QMessageBox::StandardButton SelectableMessageBox::warning(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(QMessageBox::Warning, parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton SelectableMessageBox::critical(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(QMessageBox::Critical, parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton SelectableMessageBox::information(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(QMessageBox::Information, parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton SelectableMessageBox::question(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(QMessageBox::Question, parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton SelectableMessageBox::showMessageBox(
    QMessageBox::Icon icon,
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    // Log the message to help with debugging
    QString logMessage = QString("[%1] %2").arg(title).arg(text);

    switch (icon) {
        case QMessageBox::Critical:
            LOG_ERROR("MessageBox", logMessage);
            break;
        case QMessageBox::Warning:
            LOG_WARN("MessageBox", logMessage);
            break;
        case QMessageBox::Information:
            LOG_INFO("MessageBox", logMessage);
            break;
        case QMessageBox::Question:
            LOG_INFO("MessageBox", logMessage);
            break;
        default:
            LOG_DEBUG("MessageBox", logMessage);
            break;
    }

    QMessageBox msgBox(icon, title, text, buttons, parent);

    // Make the text selectable/copyable
    // This allows users to select text with mouse and copy with Ctrl+C (Cmd+C on macOS)
    msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    // Also make the informative text selectable if present
    if (QLabel* label = msgBox.findChild<QLabel*>("qt_msgbox_informativelabel")) {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    }

    if (defaultButton != QMessageBox::NoButton) {
        msgBox.setDefaultButton(defaultButton);
    }

    return static_cast<QMessageBox::StandardButton>(msgBox.exec());
}

} // namespace TR4QT
