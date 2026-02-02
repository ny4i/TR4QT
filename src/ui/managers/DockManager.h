/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef DOCKMANAGER_H
#define DOCKMANAGER_H

#include <QObject>
#include <QDockWidget>
#include <QMainWindow>
#include <QMap>
#include <QSettings>

namespace TR4QT {

/**
 * @brief Manages dockable tool panels in TDI (Tabbed Document Interface) mode
 *
 * This class wraps existing widgets as QDockWidgets and provides:
 * - Dock area management (left, right, bottom)
 * - Layout save/restore
 * - Preset layouts (Contest, Minimal, Multi-monitor)
 * - Pop-out support for multi-monitor users
 */
class DockManager : public QObject {
    Q_OBJECT

public:
    explicit DockManager(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~DockManager() override;

    /**
     * @brief Dock position identifiers
     */
    enum class DockId {
        Radio1,
        Radio2,
        DXCluster,
        BandMap,
        Panadapter,
        Multipliers,
        Statistics
    };
    Q_ENUM(DockId)

    /**
     * @brief Preset layout configurations
     */
    enum class LayoutPreset {
        Default,        // All docks visible in default positions
        Minimal,        // Only Radio + Entry area
        Contest,        // Optimized for contest operation
        MultiMonitor    // Pop out visualization windows
    };
    Q_ENUM(LayoutPreset)

    /**
     * @brief Create a dock widget for an existing widget
     * @param id Dock identifier for save/restore
     * @param widget The widget to wrap (takes ownership)
     * @param title Window title for the dock
     * @param area Initial dock area
     * @return The created QDockWidget (owned by mainWindow)
     */
    QDockWidget* createDock(DockId id, QWidget* widget, const QString& title,
                            Qt::DockWidgetArea area);

    /**
     * @brief Get a dock widget by ID
     */
    QDockWidget* dock(DockId id) const;

    /**
     * @brief Show/hide a dock widget
     */
    void setDockVisible(DockId id, bool visible);
    bool isDockVisible(DockId id) const;

    /**
     * @brief Toggle dock visibility
     */
    void toggleDock(DockId id);

    /**
     * @brief Save current layout to settings
     */
    void saveLayout(const QString& name = "default");

    /**
     * @brief Restore layout from settings
     */
    void restoreLayout(const QString& name = "default");

    /**
     * @brief Apply a preset layout
     */
    void applyPreset(LayoutPreset preset);

    /**
     * @brief Reset to default layout
     */
    void resetToDefault();

    /**
     * @brief Get list of saved layout names
     */
    QStringList savedLayouts() const;

    /**
     * @brief Delete a saved layout
     */
    void deleteLayout(const QString& name);

    /**
     * @brief Pop out a dock as floating window
     */
    void popOut(DockId id);

    /**
     * @brief Dock a floating window back
     */
    void dockBack(DockId id);

signals:
    /**
     * @brief Emitted when dock visibility changes
     */
    void dockVisibilityChanged(DockId id, bool visible);

    /**
     * @brief Emitted when layout is restored
     */
    void layoutRestored(const QString& name);

private slots:
    void onDockVisibilityChanged(bool visible);
    void onDockLocationChanged(Qt::DockWidgetArea area);

private:
    void setupDefaultLayout();
    QString dockIdToString(DockId id) const;
    DockId stringToDockId(const QString& str) const;

    QMainWindow* m_mainWindow;
    QMap<DockId, QDockWidget*> m_docks;
    QMap<DockId, Qt::DockWidgetArea> m_defaultAreas;
    bool m_restoringLayout{false};
};

} // namespace TR4QT

#endif // DOCKMANAGER_H
