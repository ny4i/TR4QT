/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "DockManager.h"
#include "../../core/Constants.h"
#include "../../logging/LogMacros.h"
#include <QApplication>

namespace TR4QT {

DockManager::DockManager(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
    // Enable dock nesting and animated docks
    m_mainWindow->setDockNestingEnabled(true);
    m_mainWindow->setAnimated(true);

    // Allow tabbed docks
    m_mainWindow->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    m_mainWindow->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
    m_mainWindow->setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);

    LOG_INFO("DockManager", "DockManager initialized with TDI support");
}

DockManager::~DockManager()
{
    // Dock widgets are owned by mainWindow, no cleanup needed
}

QDockWidget* DockManager::createDock(DockId id, QWidget* widget, const QString& title,
                                      Qt::DockWidgetArea area)
{
    if (m_docks.contains(id)) {
        LOG_WARN("DockManager", QString("Dock already exists: %1").arg(dockIdToString(id)));
        return m_docks[id];
    }

    auto* dock = new QDockWidget(title, m_mainWindow);
    dock->setObjectName(QString("dock_%1").arg(dockIdToString(id)));
    dock->setWidget(widget);

    // Allow all dock features
    dock->setFeatures(QDockWidget::DockWidgetClosable |
                      QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);

    // Set allowed areas based on dock type
    switch (id) {
        case DockId::Radio1:
        case DockId::Radio2:
            dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
            break;
        case DockId::DXCluster:
        case DockId::BandMap:
            dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
            break;
        case DockId::Panadapter:
            dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
            break;
        case DockId::Multipliers:
        case DockId::Statistics:
            dock->setAllowedAreas(Qt::AllDockWidgetAreas);
            break;
    }

    // Add to main window
    m_mainWindow->addDockWidget(area, dock);
    m_docks[id] = dock;
    m_defaultAreas[id] = area;

    // Connect visibility signal
    connect(dock, &QDockWidget::visibilityChanged,
            this, &DockManager::onDockVisibilityChanged);
    connect(dock, &QDockWidget::dockLocationChanged,
            this, &DockManager::onDockLocationChanged);

    LOG_DEBUG("DockManager", QString("Created dock: %1 in area %2")
              .arg(title).arg(static_cast<int>(area)));

    return dock;
}

QDockWidget* DockManager::dock(DockId id) const
{
    return m_docks.value(id, nullptr);
}

void DockManager::setDockVisible(DockId id, bool visible)
{
    if (auto* d = dock(id)) {
        d->setVisible(visible);
    }
}

bool DockManager::isDockVisible(DockId id) const
{
    if (auto* d = dock(id)) {
        return d->isVisible();
    }
    return false;
}

void DockManager::toggleDock(DockId id)
{
    setDockVisible(id, !isDockVisible(id));
}

void DockManager::saveLayout(const QString& name)
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("DockLayouts");
    settings.beginGroup(name);

    // Save main window state (includes all dock positions)
    settings.setValue("windowState", m_mainWindow->saveState());
    settings.setValue("geometry", m_mainWindow->saveGeometry());

    // Save individual dock visibility
    for (auto it = m_docks.begin(); it != m_docks.end(); ++it) {
        QString key = QString("visible_%1").arg(dockIdToString(it.key()));
        settings.setValue(key, it.value()->isVisible());
    }

    settings.endGroup();
    settings.endGroup();

    LOG_INFO("DockManager", QString("Saved layout: %1").arg(name));
}

void DockManager::restoreLayout(const QString& name)
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("DockLayouts");
    settings.beginGroup(name);

    if (!settings.contains("windowState")) {
        LOG_WARN("DockManager", QString("Layout not found: %1").arg(name));
        settings.endGroup();
        settings.endGroup();
        return;
    }

    m_restoringLayout = true;

    // Restore main window state
    m_mainWindow->restoreState(settings.value("windowState").toByteArray());
    m_mainWindow->restoreGeometry(settings.value("geometry").toByteArray());

    // Restore individual dock visibility
    for (auto it = m_docks.begin(); it != m_docks.end(); ++it) {
        QString key = QString("visible_%1").arg(dockIdToString(it.key()));
        if (settings.contains(key)) {
            it.value()->setVisible(settings.value(key).toBool());
        }
    }

    settings.endGroup();
    settings.endGroup();

    m_restoringLayout = false;

    LOG_INFO("DockManager", QString("Restored layout: %1").arg(name));
    emit layoutRestored(name);
}

void DockManager::applyPreset(LayoutPreset preset)
{
    switch (preset) {
        case LayoutPreset::Default:
            resetToDefault();
            break;

        case LayoutPreset::Minimal:
            // Show only radio controls, hide everything else
            setDockVisible(DockId::Radio1, true);
            setDockVisible(DockId::Radio2, false);
            setDockVisible(DockId::DXCluster, false);
            setDockVisible(DockId::BandMap, false);
            setDockVisible(DockId::Panadapter, false);
            setDockVisible(DockId::Multipliers, false);
            setDockVisible(DockId::Statistics, false);
            break;

        case LayoutPreset::Contest:
            // Optimized for contest: radios, cluster, band map, panadapter
            setDockVisible(DockId::Radio1, true);
            setDockVisible(DockId::Radio2, true);
            setDockVisible(DockId::DXCluster, true);
            setDockVisible(DockId::BandMap, true);
            setDockVisible(DockId::Panadapter, true);
            setDockVisible(DockId::Multipliers, false);
            setDockVisible(DockId::Statistics, false);

            // Tab cluster and band map together on the right
            if (auto* clusterDock = dock(DockId::DXCluster)) {
                if (auto* bandMapDock = dock(DockId::BandMap)) {
                    m_mainWindow->tabifyDockWidget(clusterDock, bandMapDock);
                    clusterDock->raise();  // Show cluster tab first
                }
            }
            break;

        case LayoutPreset::MultiMonitor:
            // Pop out visualization windows for multi-monitor
            popOut(DockId::Panadapter);
            popOut(DockId::BandMap);
            setDockVisible(DockId::Radio1, true);
            setDockVisible(DockId::Radio2, true);
            setDockVisible(DockId::DXCluster, true);
            break;
    }

    LOG_INFO("DockManager", QString("Applied preset: %1").arg(static_cast<int>(preset)));
}

void DockManager::resetToDefault()
{
    // Restore all docks to their default areas
    for (auto it = m_docks.begin(); it != m_docks.end(); ++it) {
        QDockWidget* d = it.value();
        Qt::DockWidgetArea area = m_defaultAreas.value(it.key(), Qt::RightDockWidgetArea);

        // If floating, dock it back
        if (d->isFloating()) {
            d->setFloating(false);
        }

        // Move to default area
        m_mainWindow->addDockWidget(area, d);
        d->setVisible(true);
    }

    // Stack radios vertically on left
    if (auto* r1 = dock(DockId::Radio1)) {
        if (auto* r2 = dock(DockId::Radio2)) {
            m_mainWindow->splitDockWidget(r1, r2, Qt::Vertical);
        }
    }

    // Stack cluster and band map on right
    if (auto* cluster = dock(DockId::DXCluster)) {
        if (auto* bandMap = dock(DockId::BandMap)) {
            m_mainWindow->splitDockWidget(cluster, bandMap, Qt::Vertical);
        }
    }

    LOG_INFO("DockManager", "Reset to default layout");
}

QStringList DockManager::savedLayouts() const
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("DockLayouts");
    QStringList layouts = settings.childGroups();
    settings.endGroup();
    return layouts;
}

void DockManager::deleteLayout(const QString& name)
{
    QSettings settings(APP_ORG, APP_NAME);
    settings.beginGroup("DockLayouts");
    settings.remove(name);
    settings.endGroup();

    LOG_INFO("DockManager", QString("Deleted layout: %1").arg(name));
}

void DockManager::popOut(DockId id)
{
    if (auto* d = dock(id)) {
        d->setFloating(true);
        d->show();
        LOG_DEBUG("DockManager", QString("Popped out dock: %1").arg(dockIdToString(id)));
    }
}

void DockManager::dockBack(DockId id)
{
    if (auto* d = dock(id)) {
        d->setFloating(false);
        Qt::DockWidgetArea area = m_defaultAreas.value(id, Qt::RightDockWidgetArea);
        m_mainWindow->addDockWidget(area, d);
        LOG_DEBUG("DockManager", QString("Docked back: %1").arg(dockIdToString(id)));
    }
}

void DockManager::onDockVisibilityChanged(bool visible)
{
    if (m_restoringLayout) return;

    auto* d = qobject_cast<QDockWidget*>(sender());
    if (!d) return;

    // Find the dock ID
    for (auto it = m_docks.begin(); it != m_docks.end(); ++it) {
        if (it.value() == d) {
            emit dockVisibilityChanged(it.key(), visible);
            break;
        }
    }
}

void DockManager::onDockLocationChanged(Qt::DockWidgetArea area)
{
    auto* d = qobject_cast<QDockWidget*>(sender());
    if (!d) return;

    LOG_DEBUG("DockManager", QString("Dock %1 moved to area %2")
              .arg(d->objectName()).arg(static_cast<int>(area)));
}

QString DockManager::dockIdToString(DockId id) const
{
    switch (id) {
        case DockId::Radio1: return "radio1";
        case DockId::Radio2: return "radio2";
        case DockId::DXCluster: return "dxcluster";
        case DockId::BandMap: return "bandmap";
        case DockId::Panadapter: return "panadapter";
        case DockId::Multipliers: return "multipliers";
        case DockId::Statistics: return "statistics";
    }
    return "unknown";
}

DockManager::DockId DockManager::stringToDockId(const QString& str) const
{
    if (str == "radio1") return DockId::Radio1;
    if (str == "radio2") return DockId::Radio2;
    if (str == "dxcluster") return DockId::DXCluster;
    if (str == "bandmap") return DockId::BandMap;
    if (str == "panadapter") return DockId::Panadapter;
    if (str == "multipliers") return DockId::Multipliers;
    if (str == "statistics") return DockId::Statistics;
    return DockId::Radio1;  // Default
}

} // namespace TR4QT
