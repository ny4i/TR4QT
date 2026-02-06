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

#ifndef WINDOWACTIVATIONHELPER_H
#define WINDOWACTIVATIONHELPER_H

#include <QObject>
#include <QWidget>
#include <QSet>
#include <QEvent>

namespace TR4QT {

/**
 * WindowActivationHelper - Manages window z-order and activation behavior
 *
 * Extracted from MainWindow::eventFilter as part of Issue #76.
 *
 * Responsibilities:
 * - Track child windows that should trigger "raise all" behavior
 * - Handle WindowActivate events on tracked windows
 * - Handle ApplicationActivate events (macOS)
 * - Provide raiseAllWindows() functionality
 *
 * Usage:
 *   m_windowHelper = new WindowActivationHelper(this);
 *   m_windowHelper->trackWindow(m_dxClusterWindow);
 *   m_windowHelper->trackWindow(m_bandMapWindow);
 *
 *   // In eventFilter:
 *   if (m_windowHelper->handleEvent(obj, event)) {
 *       return true;
 *   }
 */
class WindowActivationHelper : public QObject {
    Q_OBJECT

public:
    explicit WindowActivationHelper(QWidget* mainWindow, QObject* parent = nullptr);

    /**
     * Add a window to be tracked for activation events
     * When a tracked window is activated, all windows will be raised
     */
    void trackWindow(QWidget* window);

    /**
     * Remove a window from tracking
     */
    void untrackWindow(QWidget* window);

    /**
     * Handle events - call from MainWindow::eventFilter
     * @return true if event was handled and should not propagate
     */
    bool handleEvent(QObject* obj, QEvent* event);

    /**
     * Raise all visible top-level windows
     * @param activatedWindow Optional window to raise last (keeps it on top)
     */
    void raiseAllWindows(QWidget* activatedWindow = nullptr);

    /**
     * Enable/disable the "raise all windows on activate" behavior
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

private:
    QWidget* m_mainWindow;
    QSet<QWidget*> m_trackedWindows;
    bool m_enabled{true};
    bool m_inRaiseAllWindows{false};  // Guard against recursion
};

} // namespace TR4QT

#endif // WINDOWACTIVATIONHELPER_H
