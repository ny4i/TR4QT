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

#include "WindowActivationHelper.h"
#include "../../logging/LogMacros.h"
#include "../../utils/AppSettings.h"
#include <QApplication>

namespace TR4QT {

WindowActivationHelper::WindowActivationHelper(QWidget* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
}

void WindowActivationHelper::trackWindow(QWidget* window) {
    if (window) {
        m_trackedWindows.insert(window);
    }
}

void WindowActivationHelper::untrackWindow(QWidget* window) {
    m_trackedWindows.remove(window);
}

bool WindowActivationHelper::handleEvent(QObject* obj, QEvent* event) {
    if (!m_enabled) {
        return false;
    }

#ifdef Q_OS_MAC
    // macOS: Bring all windows to front when app is activated (if setting enabled)
    if (event->type() == QEvent::ApplicationActivate) {
        if (AppSettings::instance().getShowAllWindowsOnActivate()) {
            LOG_DEBUG("WindowActivationHelper", "ApplicationActivate - raising all windows");
            raiseAllWindows();
        }
        return false;  // Don't consume - let other handlers see it
    }
#endif

    // Handle WindowActivate on tracked child windows
    if (event->type() == QEvent::WindowActivate) {
        QWidget* widget = qobject_cast<QWidget*>(obj);
        if (widget && widget->isWindow() && m_trackedWindows.contains(widget)) {
            LOG_DEBUG("WindowActivationHelper",
                     QString("Child window activated: %1").arg(widget->windowTitle()));
            raiseAllWindows(widget);
            return false;  // Don't consume - window still needs to process
        }
    }

    return false;
}

void WindowActivationHelper::raiseAllWindows(QWidget* activatedWindow) {
    // Guard against recursion
    if (m_inRaiseAllWindows) {
        return;
    }
    m_inRaiseAllWindows = true;

    // Raise ALL top-level windows belonging to this application.
    // Order matters: raise all windows first, then re-raise the window
    // the user clicked on so it stays on top (not buried under MainWindow).
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevelWidgets) {
        if (widget && widget->isVisible() && !widget->isMinimized() && widget != activatedWindow) {
            widget->raise();
        }
    }

    // Re-raise the activated window last so it stays on top
    if (activatedWindow && activatedWindow->isVisible()) {
        activatedWindow->raise();
        activatedWindow->activateWindow();
    }

    // Ensure main window is accessible but not on top of the activated window
    if (m_mainWindow && m_mainWindow != activatedWindow) {
        m_mainWindow->raise();
        // Re-raise activated window one more time to ensure it's on top
        if (activatedWindow && activatedWindow->isVisible()) {
            activatedWindow->raise();
        }
    }

    m_inRaiseAllWindows = false;
}

void WindowActivationHelper::setEnabled(bool enabled) {
    m_enabled = enabled;
}

} // namespace TR4QT
