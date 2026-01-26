#include "WindowManager.h"
#include "../widgets/DXClusterWindow.h"
#include "../widgets/BandMapWidget.h"
#include "../widgets/RadioControlWidget.h"
#include "../widgets/MultiplierWidget.h"
#include "../widgets/StatisticsWindow.h"
#include "../NativeMapViewer.h"
#include "../dialogs/GraylineMapDialog.h"
#include <QTimer>
#include <QWidget>

namespace TR4QT {

WindowManager::WindowManager(QObject* parent)
    : QObject(parent)
    , m_dxClusterWindow(nullptr)
    , m_bandMapWindow(nullptr)
    , m_radioControlWindow(nullptr)
    , m_multiplierWindow(nullptr)
    , m_statisticsWindow(nullptr)
    , m_sectionsMapViewer(nullptr)
    , m_statesMapViewer(nullptr)
    , m_worldMapViewer(nullptr)
    , m_graylineMapDialog(nullptr)
    , m_inRaiseAllWindows(false)
{
}

void WindowManager::setWindows(const Config& config) {
    m_dxClusterWindow = config.dxClusterWindow;
    m_bandMapWindow = config.bandMapWindow;
    m_radioControlWindow = config.radioControlWindow;
    m_multiplierWindow = config.multiplierWindow;
    m_statisticsWindow = config.statisticsWindow;
    m_sectionsMapViewer = config.sectionsMapViewer;
    m_statesMapViewer = config.statesMapViewer;
    m_worldMapViewer = config.worldMapViewer;
    m_graylineMapDialog = config.graylineMapDialog;
}

// Template helper for show/raise/activate pattern
template<typename T>
void WindowManager::showAndRaise(T* window) {
    if (!window) return;

    window->show();
    window->raise();
    window->activateWindow();
    emit windowVisibilityChanged();
}

// Show window methods (overloaded for type safety)
void WindowManager::showWindow(DXClusterWindow* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(BandMapWidget* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(RadioControlWidget* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(MultiplierWidget* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(StatisticsWindow* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(NativeMapViewer* window) {
    showAndRaise(window);
}

void WindowManager::showWindow(GraylineMapDialog* window) {
    showAndRaise(window);
}

// Geometry restoration methods
void WindowManager::restoreWindowGeometry(DXClusterWindow* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(BandMapWidget* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(RadioControlWidget* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(MultiplierWidget* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(StatisticsWindow* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(NativeMapViewer* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

void WindowManager::restoreWindowGeometry(GraylineMapDialog* window, const QByteArray& geometry) {
    if (window && !geometry.isEmpty()) {
        window->restoreGeometry(geometry);
    }
}

// Visibility queries
bool WindowManager::isVisible(DXClusterWindow* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(BandMapWidget* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(RadioControlWidget* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(MultiplierWidget* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(StatisticsWindow* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(NativeMapViewer* window) const {
    return window && window->isVisible();
}

bool WindowManager::isVisible(GraylineMapDialog* window) const {
    return window && window->isVisible();
}

void WindowManager::raiseAllWindows() {
    // Prevent infinite recursion - raising windows triggers WindowActivate events
    // which would call this function again
    if (m_inRaiseAllWindows) {
        return;
    }

    m_inRaiseAllWindows = true;

    // Raise all child windows that are visible
    // Only call raise(), not activateWindow(), to prevent WindowActivate event loops on Windows
    if (m_dxClusterWindow && m_dxClusterWindow->isVisible()) {
        m_dxClusterWindow->raise();
    }

    if (m_bandMapWindow && m_bandMapWindow->isVisible()) {
        m_bandMapWindow->raise();
    }

    if (m_radioControlWindow && m_radioControlWindow->isVisible()) {
        m_radioControlWindow->raise();
    }

    if (m_multiplierWindow && m_multiplierWindow->isVisible()) {
        m_multiplierWindow->raise();
    }

    if (m_statisticsWindow && m_statisticsWindow->isVisible()) {
        m_statisticsWindow->raise();
    }

    if (m_sectionsMapViewer && m_sectionsMapViewer->isVisible()) {
        m_sectionsMapViewer->raise();
    }

    if (m_statesMapViewer && m_statesMapViewer->isVisible()) {
        m_statesMapViewer->raise();
    }

    if (m_worldMapViewer && m_worldMapViewer->isVisible()) {
        m_worldMapViewer->raise();
    }

    if (m_graylineMapDialog && m_graylineMapDialog->isVisible()) {
        m_graylineMapDialog->raise();
    }

    // Use a timer to reset the flag after queued events are processed
    // This prevents re-entry from WindowActivate events triggered by raise()
    const int RESET_DELAY_MS = 100;
    QTimer::singleShot(RESET_DELAY_MS, this, [this]() {
        m_inRaiseAllWindows = false;
    });
}

}  // namespace TR4QT
