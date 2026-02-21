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

#ifndef PERSISTENTWINDOW_H
#define PERSISTENTWINDOW_H

#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>
#include <QDataStream>
#include "../core/Constants.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

/**
 * PersistentWindow<Base> — CRTP template for child window persistence.
 *
 * Follows the canonical Qt pattern: save state in closeEvent, restore
 * in constructor/show. The window is fully self-contained.
 *
 * - setVisible(true):  Restores geometry, writes Visible=true
 * - closeEvent:        Saves geometry, writes Visible=false
 *
 * No showEvent/hideEvent involvement for visibility. No global flags.
 * If the app crashes or gets SIGTERM'd, the last Visible=true persists
 * because only closeEvent (user action) writes Visible=false.
 */

/**
 * Run once at startup (before restoreChildWindows) to migrate old-style
 * per-window visibility/geometry keys to the new Windows/* namespace.
 */
inline void migrateWindowSettings()
{
    QSettings settings(APP_ORG, APP_NAME);

    // Skip if already migrated (any new-style key exists)
    if (settings.contains("Windows/BandMap/Visible")) {
        return;
    }

    struct Migration {
        const char* newKey;
        const char* oldVisibleKey;
        const char* oldGeometryKey;
    };

    static const Migration migrations[] = {
        { "Windows/BandMap",          "BandMapWindow/visible",          "BandMapWindow/geometry" },
        { "Windows/DXCluster",        "DXClusterWindow/visible",        "DXClusterWindow/geometry" },
        { "Windows/RadioControl",     "RadioControlWindow/visible",     "RadioControlWindow/geometry" },
        { "Windows/Radio2Control",    "Radio2ControlWindow/visible",    "Radio2ControlWindow/geometry" },
        { "Windows/Multipliers",      "MultipliersWindow/visible",      "MultipliersWindow/geometry" },
        { "Windows/Statistics",        "Windows/Statistics/Visible",     "Windows/Statistics/Geometry" },
        { "Windows/SectionsMap",       "MapViewer/Sections/Visible",    "MapViewer/Sections/Geometry" },
        { "Windows/StatesMap",         "MapViewer/States/Visible",      "MapViewer/States/Geometry" },
        { "Windows/WorldMap",          "MapViewer/DXCC/Visible",        "MapViewer/DXCC/Geometry" },
        { "Windows/GraylineMap",      "GraylineMapWindow/visible",      "GraylineMapWindow/geometry" },
        { "Windows/AmplifierControl", "AmplifierControlWindow/visible", "AmplifierControlWindow/geometry" },
        { "Windows/Panadapter",       "PanadapterWindow/visible",       "PanadapterWindow/geometry" },
    };

    bool anyMigrated = false;
    for (const auto& m : migrations) {
        if (settings.contains(m.oldVisibleKey)) {
            settings.setValue(QString(m.newKey) + "/Visible",
                             settings.value(m.oldVisibleKey).toBool());
            anyMigrated = true;
        }
        if (m.oldGeometryKey && settings.contains(m.oldGeometryKey)) {
            settings.setValue(QString(m.newKey) + "/Geometry",
                             settings.value(m.oldGeometryKey).toByteArray());
            anyMigrated = true;
        }
    }

    if (anyMigrated) {
        settings.sync();
        LOG_DEBUG("PersistentWindow", "Migrated old window settings to Windows/* namespace");
    }
}

template<typename Base>
class PersistentWindow : public Base {
public:
    explicit PersistentWindow(const QString& settingsKey,
                              QWidget* parent = nullptr,
                              const QString& oldKey = QString())
        : Base(parent)
        , m_settingsKey(settingsKey)
        , m_geometryRestored(false)
    {
        Q_UNUSED(oldKey);
        // Store key as dynamic property so shutdown code can access it
        // through a QWidget* pointer without knowing the template type.
        this->setProperty("persistentWindowKey", settingsKey);
    }

    /// Show the window first, then restore geometry after the WM has placed it.
    /// On macOS, restoreGeometry() before the window is mapped gets overridden
    /// by the window manager. Deferring with a short timer lets the WM finish
    /// its initial placement before we apply the saved geometry.
    void setVisible(bool visible) override {
        if (visible) {
            // Window is being shown — persist Visible=true.
            QSettings settings(APP_ORG, APP_NAME);
            settings.setValue(m_settingsKey + "/Visible", true);
        }
        // Let the WM map the window first.
        Base::setVisible(visible);

        if (visible && !m_geometryRestored) {
            m_geometryRestored = true;
            QSettings settings(APP_ORG, APP_NAME);
            QByteArray savedGeometry =
                settings.value(m_settingsKey + "/Geometry").toByteArray();
            if (!savedGeometry.isEmpty()) {
                // Deferred restore — gives macOS WM time to finish placement.
                const int GEOMETRY_RESTORE_DELAY_MS = 150;
                QTimer::singleShot(GEOMETRY_RESTORE_DELAY_MS, this, [this, savedGeometry]() {
                    QPoint posBefore = this->pos();
                    this->restoreGeometry(savedGeometry);

                    // Fallback: if restoreGeometry() had no effect, decode the
                    // blob and use move()/resize() directly.
                    if (this->pos() == posBefore) {
                        QDataStream stream(savedGeometry);
                        stream.setVersion(QDataStream::Qt_5_0);
                        quint32 magic; quint16 majV, minV;
                        qint32 fx, fy, fw, fh, nx, ny, nw, nh;
                        stream >> magic >> majV >> minV
                               >> fx >> fy >> fw >> fh
                               >> nx >> ny >> nw >> nh;
                        this->move(nx, ny);
                        this->resize(nw, nh);
                    }
                });
            }
        }
    }

protected:
    /// User explicitly closed this window — save state.
    void closeEvent(QCloseEvent* event) override {
        QSettings settings(APP_ORG, APP_NAME);
        settings.setValue(m_settingsKey + "/Visible", false);
        settings.setValue(m_settingsKey + "/Geometry", this->saveGeometry());

        Base::closeEvent(event);
    }

public:
    QString settingsKey() const { return m_settingsKey; }

private:
    QString m_settingsKey;
    bool m_geometryRestored;
};

/// Save geometry and hide all PersistentWindow children during shutdown.
/// Works through QWidget* pointers by reading the dynamic property set
/// in the constructor. Pass any list of QWidget* — nulls are skipped,
/// non-PersistentWindow widgets are skipped.
inline void saveAndHideAll(std::initializer_list<QWidget*> windows)
{
    QSettings settings(APP_ORG, APP_NAME);
    for (QWidget* w : windows) {
        if (!w) continue;
        QVariant keyVar = w->property("persistentWindowKey");
        if (keyVar.isValid() && w->isVisible()) {
            settings.setValue(keyVar.toString() + "/Geometry", w->saveGeometry());
        }
        w->hide();
    }
}

} // namespace TR4QT

#endif // PERSISTENTWINDOW_H
