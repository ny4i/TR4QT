#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QByteArray>

// Forward declaration (not in TR4QT namespace)
class NativeMapViewer;

namespace TR4QT {

// Forward declarations
class DXClusterWindow;
class BandMapWidget;
class RadioControlWidget;
class MultiplierWidget;
class StatisticsWindow;
class GraylineMapDialog;

/**
 * @brief Manages lifecycle and visibility of auxiliary windows
 *
 * WindowManager centralizes window management logic that was previously
 * scattered throughout MainWindow. It handles:
 * - Showing/hiding windows on demand
 * - Restoring window geometry from settings
 * - Raising all windows to front
 * - Tracking which windows are currently visible
 *
 * Window creation remains in MainWindow (for signal/slot connections),
 * but lifecycle management is delegated to WindowManager.
 */
class WindowManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Configuration for WindowManager
     *
     * Pass window pointers to WindowManager after creating them in MainWindow.
     * WindowManager does not own these pointers (MainWindow retains ownership).
     */
    struct Config {
        DXClusterWindow* dxClusterWindow = nullptr;
        BandMapWidget* bandMapWindow = nullptr;
        RadioControlWidget* radioControlWindow = nullptr;
        MultiplierWidget* multiplierWindow = nullptr;
        StatisticsWindow* statisticsWindow = nullptr;
        NativeMapViewer* sectionsMapViewer = nullptr;
        NativeMapViewer* statesMapViewer = nullptr;
        GraylineMapDialog* graylineMapDialog = nullptr;
    };

    explicit WindowManager(QObject* parent = nullptr);
    ~WindowManager() override = default;

    // Configuration
    void setWindows(const Config& config);

    // Window visibility management
    void showWindow(DXClusterWindow* window);
    void showWindow(BandMapWidget* window);
    void showWindow(RadioControlWidget* window);
    void showWindow(MultiplierWidget* window);
    void showWindow(StatisticsWindow* window);
    void showWindow(NativeMapViewer* window);
    void showWindow(GraylineMapDialog* window);

    // Geometry restoration
    void restoreWindowGeometry(DXClusterWindow* window, const QByteArray& geometry);
    void restoreWindowGeometry(BandMapWidget* window, const QByteArray& geometry);
    void restoreWindowGeometry(RadioControlWidget* window, const QByteArray& geometry);
    void restoreWindowGeometry(MultiplierWidget* window, const QByteArray& geometry);
    void restoreWindowGeometry(StatisticsWindow* window, const QByteArray& geometry);
    void restoreWindowGeometry(NativeMapViewer* window, const QByteArray& geometry);
    void restoreWindowGeometry(GraylineMapDialog* window, const QByteArray& geometry);

    // Multi-window operations
    void raiseAllWindows();

    // Visibility queries
    bool isVisible(DXClusterWindow* window) const;
    bool isVisible(BandMapWidget* window) const;
    bool isVisible(RadioControlWidget* window) const;
    bool isVisible(MultiplierWidget* window) const;
    bool isVisible(StatisticsWindow* window) const;
    bool isVisible(NativeMapViewer* window) const;
    bool isVisible(GraylineMapDialog* window) const;

signals:
    /**
     * @brief Emitted when any window's visibility changes
     *
     * MainWindow can connect to this signal to update Window menu checkmarks.
     */
    void windowVisibilityChanged();

private:
    // Window pointers (not owned by WindowManager)
    DXClusterWindow* m_dxClusterWindow;
    BandMapWidget* m_bandMapWindow;
    RadioControlWidget* m_radioControlWindow;
    MultiplierWidget* m_multiplierWindow;
    StatisticsWindow* m_statisticsWindow;
    NativeMapViewer* m_sectionsMapViewer;
    NativeMapViewer* m_statesMapViewer;
    GraylineMapDialog* m_graylineMapDialog;

    // Helper to show/raise/activate a window
    template<typename T>
    void showAndRaise(T* window);

    // Recursion guard for raiseAllWindows
    bool m_inRaiseAllWindows;
};

}  // namespace TR4QT

#endif // WINDOWMANAGER_H
