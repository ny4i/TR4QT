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

    QMessageBox::StandardButton result = static_cast<QMessageBox::StandardButton>(msgBox.exec());

    // Log the user's response
    QString buttonName = buttonToString(result);
    QString responseLog = QString("[%1] User clicked: %2").arg(title, buttonName);

    switch (icon) {
        case QMessageBox::Critical:
            LOG_ERROR("MessageBox", responseLog);
            break;
        case QMessageBox::Warning:
            LOG_WARN("MessageBox", responseLog);
            break;
        case QMessageBox::Information:
        case QMessageBox::Question:
            LOG_INFO("MessageBox", responseLog);
            break;
        default:
            LOG_DEBUG("MessageBox", responseLog);
            break;
    }

    return result;
}

QString SelectableMessageBox::buttonToString(QMessageBox::StandardButton button)
{
    switch (button) {
        case QMessageBox::Ok:       return "OK";
        case QMessageBox::Cancel:   return "Cancel";
        case QMessageBox::Yes:      return "Yes";
        case QMessageBox::No:       return "No";
        case QMessageBox::Abort:    return "Abort";
        case QMessageBox::Retry:    return "Retry";
        case QMessageBox::Ignore:   return "Ignore";
        case QMessageBox::Close:    return "Close";
        case QMessageBox::Discard:  return "Discard";
        case QMessageBox::Apply:    return "Apply";
        case QMessageBox::Reset:    return "Reset";
        case QMessageBox::Save:     return "Save";
        case QMessageBox::SaveAll:  return "SaveAll";
        case QMessageBox::Open:     return "Open";
        case QMessageBox::YesToAll: return "YesToAll";
        case QMessageBox::NoToAll:  return "NoToAll";
        default:                    return "Unknown";
    }
}

} // namespace TR4QT
