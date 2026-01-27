#include "DialogHelper.h"
#include "../logging/LogMacros.h"
#include <QLabel>

namespace TR4QT {

QMessageBox::StandardButton DialogHelper::question(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(
        QMessageBox::Question, parent, title, text, buttons, defaultButton, true);
}

void DialogHelper::information(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    showMessageBox(
        QMessageBox::Information, parent, title, text, QMessageBox::Ok, QMessageBox::NoButton, true);
}

void DialogHelper::informationWithDetails(
    QWidget* parent,
    const QString& title,
    const QString& text,
    const QString& detailedText,
    const QString& styleSheet)
{
    // Log the message with detailed text
    LOG_INFO("DialogHelper", QString("[Information] %1 - %2").arg(title, text));
    LOG_INFO("DialogHelper", QString("[Information] %1 - Details: %2").arg(title, detailedText));

    // Create the message box with detailed text
    QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
    msgBox.setDetailedText(detailedText);
    msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    // Apply custom stylesheet if provided (e.g., for larger dialog size)
    if (!styleSheet.isEmpty()) {
        msgBox.setStyleSheet(styleSheet);
    }

    msgBox.exec();

    LOG_INFO("DialogHelper", QString("[Information] %1 - User closed dialog").arg(title));
}

QMessageBox::StandardButton DialogHelper::warning(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    return showMessageBox(
        QMessageBox::Warning, parent, title, text, buttons, defaultButton, true);
}

void DialogHelper::critical(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    showMessageBox(
        QMessageBox::Critical, parent, title, text, QMessageBox::Ok, QMessageBox::NoButton, true);
}

void DialogHelper::about(
    QWidget* parent,
    const QString& title,
    const QString& text)
{
    // Log the about dialog
    LOG_INFO("DialogHelper", QString("[About] %1 - %2").arg(title, text));

    // Use Qt's built-in about() which has special formatting
    QMessageBox::about(parent, title, text);

    // Log that user closed the dialog
    LOG_INFO("DialogHelper", QString("[About] %1 - User closed dialog").arg(title));
}

QMessageBox::StandardButton DialogHelper::showMessageBox(
    QMessageBox::Icon icon,
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton,
    bool textSelectable)
{
    // Log the message to help with debugging
    QString iconType;
    switch (icon) {
        case QMessageBox::Critical:   iconType = "Critical"; break;
        case QMessageBox::Warning:    iconType = "Warning"; break;
        case QMessageBox::Information: iconType = "Information"; break;
        case QMessageBox::Question:   iconType = "Question"; break;
        default:                      iconType = "Dialog"; break;
    }

    QString logMessage = QString("[%1] %2 - %3").arg(iconType, title, text);

    switch (icon) {
        case QMessageBox::Critical:
            LOG_ERROR("DialogHelper", logMessage);
            break;
        case QMessageBox::Warning:
            LOG_WARN("DialogHelper", logMessage);
            break;
        case QMessageBox::Information:
        case QMessageBox::Question:
            LOG_INFO("DialogHelper", logMessage);
            break;
        default:
            LOG_DEBUG("DialogHelper", logMessage);
            break;
    }

    // Create and configure the message box
    QMessageBox msgBox(icon, title, text, buttons, parent);

    // Make the text selectable/copyable (default behavior)
    if (textSelectable) {
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

        // Also make the informative text selectable if present
        if (QLabel* label = msgBox.findChild<QLabel*>("qt_msgbox_informativelabel")) {
            label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        }
    }

    if (defaultButton != QMessageBox::NoButton) {
        msgBox.setDefaultButton(defaultButton);
    }

    QMessageBox::StandardButton result = static_cast<QMessageBox::StandardButton>(msgBox.exec());

    // Log the user's response
    QString buttonName = buttonToString(result);
    QString responseLog = QString("[%1] %2 - User clicked: %3").arg(iconType, title, buttonName);

    switch (icon) {
        case QMessageBox::Critical:
            LOG_ERROR("DialogHelper", responseLog);
            break;
        case QMessageBox::Warning:
            LOG_WARN("DialogHelper", responseLog);
            break;
        case QMessageBox::Information:
        case QMessageBox::Question:
            LOG_INFO("DialogHelper", responseLog);
            break;
        default:
            LOG_DEBUG("DialogHelper", responseLog);
            break;
    }

    return result;
}

QString DialogHelper::buttonToString(QMessageBox::StandardButton button)
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
