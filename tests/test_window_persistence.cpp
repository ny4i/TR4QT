/**
 * Test for persistent window state functionality
 *
 * Validates that PersistentWindow template correctly persists
 * visibility and geometry via QSettings on show/hide events.
 *
 * Also validates the event filter visibility tracking pattern
 * used by MainWindow for map windows.
 *
 * IMPORTANT: All tests use an isolated QSettings file in a temp directory.
 * Production settings (plist) are never touched.
 */

#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QWidget>
#include <QCloseEvent>
#include <QShowEvent>
#include <QHideEvent>
#include "../src/ui/PersistentWindow.h"
#include "../src/logging/LogMacros.h"

using namespace TR4QT;

/**
 * VisibilityTracker - Reproduces the MainWindow event filter pattern.
 *
 * MainWindow tracks map window visibility via boolean flags + eventFilter
 * instead of isVisible() (which is unreliable during SIGTERM shutdown).
 * This class isolates that pattern for testing without MainWindow dependencies.
 */
class VisibilityTracker : public QObject {
    Q_OBJECT

public:
    bool trackedVisible{false};
    QWidget* trackedWindow{nullptr};

    void trackWindow(QWidget* window) {
        trackedWindow = window;
        window->installEventFilter(this);
    }

    void showWindow() {
        if (trackedWindow) {
            trackedWindow->show();
            trackedVisible = true;
        }
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Close && obj == trackedWindow) {
            trackedVisible = false;
        }
        return QObject::eventFilter(obj, event);
    }
};

class TestWindowPersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test PersistentWindow show/hide persistence
    void testPersistentWindow_ShowWritesVisible();
    void testPersistentWindow_HideWritesNotVisible();
    void testPersistentWindow_HideSavesGeometry();

    // Test settings roundtrip (main window only)
    void testMainWindowGeometry_Roundtrip();

    // Test event filter visibility tracking (map window fix)
    void testVisibilityTracker_DefaultFalse();
    void testVisibilityTracker_ShowSetsTrue();
    void testVisibilityTracker_CloseSetsFalse();
    void testVisibilityTracker_HideDoesNotSetFalse();

private:
    QTemporaryDir* m_tempDir;
};

void TestWindowPersistence::initTestCase() {
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    // Redirect ALL QSettings(APP_ORG, APP_NAME) to the temp directory.
    // This ensures tests never touch the production plist.
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_tempDir->path());
}

void TestWindowPersistence::cleanupTestCase() {
    delete m_tempDir;
}

void TestWindowPersistence::init() {
    // Clear test settings before each test
    QSettings settings(APP_ORG, APP_NAME);
    settings.clear();
}

void TestWindowPersistence::cleanup() {
}

void TestWindowPersistence::testPersistentWindow_ShowWritesVisible() {
    // Create a PersistentWindow and show it
    PersistentWindow<QWidget> window("Windows/TestWidget");
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.show();

    // Verify QSettings has Visible=true
    QSettings settings(APP_ORG, APP_NAME);
    QVERIFY(settings.contains("Windows/TestWidget/Visible"));
    QCOMPARE(settings.value("Windows/TestWidget/Visible").toBool(), true);

    window.close();
    qInfo() << "OK PersistentWindow show writes Visible=true to QSettings";
}

void TestWindowPersistence::testPersistentWindow_HideWritesNotVisible() {
    // hide() should NOT write Visible=false — only close() does.
    // This ensures shutdown (which hides windows) doesn't overwrite state.
    PersistentWindow<QWidget> window("Windows/TestWidget2");
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);

    window.show();
    QSettings settings(APP_ORG, APP_NAME);
    QCOMPARE(settings.value("Windows/TestWidget2/Visible").toBool(), true);

    window.hide();
    settings.sync();
    // Visible stays true after hide — only close writes false
    QCOMPARE(settings.value("Windows/TestWidget2/Visible").toBool(), true);

    window.close();
    settings.sync();
    QCOMPARE(settings.value("Windows/TestWidget2/Visible").toBool(), false);

    qInfo() << "OK PersistentWindow close writes Visible=false, hide does not";
}

void TestWindowPersistence::testPersistentWindow_HideSavesGeometry() {
    // Geometry is saved in closeEvent (canonical Qt pattern)
    PersistentWindow<QWidget> window("Windows/TestWidget3");
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);
    window.resize(400, 300);

    window.show();
    window.close();

    QSettings settings(APP_ORG, APP_NAME);
    QVERIFY(settings.contains("Windows/TestWidget3/Geometry"));
    QByteArray geometry = settings.value("Windows/TestWidget3/Geometry").toByteArray();
    QVERIFY(!geometry.isEmpty());

    qInfo() << "OK PersistentWindow close saves geometry to QSettings";
}

void TestWindowPersistence::testMainWindowGeometry_Roundtrip() {
    // Test geometry/state roundtrip directly via QSettings to avoid
    // coupling to the AppSettings singleton (which can't be redirected
    // to a temp dir after construction).
    QSettings settings(APP_ORG, APP_NAME);

    QByteArray testGeometry("test_geometry_data_12345");
    QByteArray testState("test_state_data_67890");

    settings.setValue("MainWindow.geometry", testGeometry);
    settings.setValue("MainWindow.state", testState);
    settings.sync();

    QByteArray loadedGeometry = settings.value("MainWindow.geometry").toByteArray();
    QByteArray loadedState = settings.value("MainWindow.state").toByteArray();

    QCOMPARE(loadedGeometry, testGeometry);
    QCOMPARE(loadedState, testState);

    qInfo() << "OK Main window geometry roundtrip works";
}

// --- Event filter visibility tracking tests ---

void TestWindowPersistence::testVisibilityTracker_DefaultFalse()
{
    VisibilityTracker tracker;
    QVERIFY(!tracker.trackedVisible);
}

void TestWindowPersistence::testVisibilityTracker_ShowSetsTrue()
{
    VisibilityTracker tracker;
    QWidget window;
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);

    tracker.trackWindow(&window);
    QVERIFY(!tracker.trackedVisible);

    tracker.showWindow();
    QVERIFY(tracker.trackedVisible);
}

void TestWindowPersistence::testVisibilityTracker_CloseSetsFalse()
{
    VisibilityTracker tracker;
    QWidget window;
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);

    tracker.trackWindow(&window);
    tracker.showWindow();
    QVERIFY(tracker.trackedVisible);

    window.close();
    QVERIFY(!tracker.trackedVisible);
}

void TestWindowPersistence::testVisibilityTracker_HideDoesNotSetFalse()
{
    VisibilityTracker tracker;
    QWidget window;
    window.setWindowFlags(Qt::Window);
    window.setAttribute(Qt::WA_DeleteOnClose, false);

    tracker.trackWindow(&window);
    tracker.showWindow();
    QVERIFY(tracker.trackedVisible);

    window.hide();
    QVERIFY(tracker.trackedVisible);
}

QTEST_MAIN(TestWindowPersistence)
#include "test_window_persistence.moc"
