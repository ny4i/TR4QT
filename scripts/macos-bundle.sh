#!/bin/bash
# macOS App Bundle Script - EXPLICIT DEPLOYMENT (no macdeployqt)
# Bundles Qt frameworks, copies dependencies, and signs the app

set -e  # Exit on error

APP_BUNDLE="./build/src/tr4qt.app"
FRAMEWORKS="$APP_BUNDLE/Contents/Frameworks"
PLUGINS="$APP_BUNDLE/Contents/PlugIns"
HAMLIB_INSTALL="$HOME/projects/Hamlib/hamlib_install"
QT_PREFIX="/opt/homebrew/opt/qtbase"
QT_HTTPSERVER="/opt/homebrew/opt/qthttpserver"
QT_WEBSOCKETS="/opt/homebrew/opt/qtwebsockets"

echo "==> Explicit macOS App Bundling (no macdeployqt)"
echo "==> App bundle: $APP_BUNDLE"

# Step 1: Copy Qt Frameworks explicitly
echo "==> Step 1: Copying Qt frameworks..."
mkdir -p "$FRAMEWORKS"

# List of Qt frameworks we need
QT_FRAMEWORKS=(
    "QtCore"
    "QtGui"
    "QtWidgets"
    "QtNetwork"
    "QtSql"
    "QtPrintSupport"
    "QtConcurrent"
)

for framework in "${QT_FRAMEWORKS[@]}"; do
    echo "    Copying $framework.framework..."
    rm -rf "$FRAMEWORKS/$framework.framework"
    cp -R "$QT_PREFIX/lib/$framework.framework" "$FRAMEWORKS/"
    # Keep only the main binary, remove headers/resources we don't need
    rm -rf "$FRAMEWORKS/$framework.framework/Headers"
    rm -rf "$FRAMEWORKS/$framework.framework/Versions/A/Headers"
done

# QtHttpServer (different location)
echo "    Copying QtHttpServer.framework..."
rm -rf "$FRAMEWORKS/QtHttpServer.framework"
cp -R "$QT_HTTPSERVER/lib/QtHttpServer.framework" "$FRAMEWORKS/"
rm -rf "$FRAMEWORKS/QtHttpServer.framework/Headers"
rm -rf "$FRAMEWORKS/QtHttpServer.framework/Versions/A/Headers"

# QtWebSockets (different location)
echo "    Copying QtWebSockets.framework..."
rm -rf "$FRAMEWORKS/QtWebSockets.framework"
cp -R "$QT_WEBSOCKETS/lib/QtWebSockets.framework" "$FRAMEWORKS/"
rm -rf "$FRAMEWORKS/QtWebSockets.framework/Headers"
rm -rf "$FRAMEWORKS/QtWebSockets.framework/Versions/A/Headers"

# QtDBus (needed by some Qt internals, not bundled by macdeployqt)
echo "    Copying QtDBus.framework..."
rm -rf "$FRAMEWORKS/QtDBus.framework"
cp -R "$QT_PREFIX/lib/QtDBus.framework" "$FRAMEWORKS/"
rm -rf "$FRAMEWORKS/QtDBus.framework/Headers"
rm -rf "$FRAMEWORKS/QtDBus.framework/Versions/A/Headers"

# Step 2: Copy Qt Plugins explicitly
echo "==> Step 2: Copying Qt plugins..."

# Platforms plugin (required)
mkdir -p "$PLUGINS/platforms"
echo "    Copying platforms/libqcocoa.dylib..."
cp "$QT_PREFIX/share/qt/plugins/platforms/libqcocoa.dylib" "$PLUGINS/platforms/"

# Styles plugin
mkdir -p "$PLUGINS/styles"
echo "    Copying styles/libqmacstyle.dylib..."
cp "$QT_PREFIX/share/qt/plugins/styles/libqmacstyle.dylib" "$PLUGINS/styles/"

# SQL drivers plugin (CRITICAL - for database access)
mkdir -p "$PLUGINS/sqldrivers"
echo "    Copying sqldrivers/libqsqlite.dylib..."
cp "$QT_PREFIX/share/qt/plugins/sqldrivers/libqsqlite.dylib" "$PLUGINS/sqldrivers/"

# TLS plugins (CRITICAL - for HTTPS connections)
mkdir -p "$PLUGINS/tls"
echo "    Copying TLS plugins..."
# Copy all available TLS backends (OpenSSL, SecureTransport, cert-only)
for tls_plugin in "$QT_PREFIX/share/qt/plugins/tls"/*.dylib; do
    if [ -f "$tls_plugin" ]; then
        plugin_name=$(basename "$tls_plugin")
        echo "      Copying tls/$plugin_name..."
        cp "$tls_plugin" "$PLUGINS/tls/"
    fi
done

# Step 3: Copy Hamlib library
echo "==> Step 3: Copying Hamlib library..."
if [ -f "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" ]; then
    cp "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" "$FRAMEWORKS/"
    echo "    Copied libhamlib.5.dylib"
else
    echo "    ERROR: Hamlib library not found at $HAMLIB_INSTALL/lib/libhamlib.5.dylib"
    exit 1
fi

# Step 4: Copy libdbus (required by QtDBus)
echo "==> Step 4: Copying libdbus..."
if [ -f "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" ]; then
    cp "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" "$FRAMEWORKS/"
    echo "    Copied libdbus-1.3.dylib"
fi

# Step 5: Fix library paths in executable
echo "==> Step 5: Fixing executable library paths..."
cd "$APP_BUNDLE/Contents/MacOS"

# Add correct rpath for app bundle (points to Frameworks directory)
install_name_tool -add_rpath "@executable_path/../Frameworks" tr4qt 2>/dev/null || true
echo "    Added @executable_path/../Frameworks to rpath"

# Fix Hamlib path
install_name_tool -change "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" \
    "@rpath/libhamlib.5.dylib" tr4qt

# Fix Qt framework paths
for framework in QtCore QtGui QtWidgets QtNetwork QtSql QtPrintSupport QtConcurrent; do
    install_name_tool -change "$QT_PREFIX/lib/${framework}.framework/Versions/A/${framework}" \
        "@rpath/${framework}.framework/Versions/A/${framework}" tr4qt
done

# Fix QtHttpServer path
install_name_tool -change "$QT_HTTPSERVER/lib/QtHttpServer.framework/Versions/A/QtHttpServer" \
    "@rpath/QtHttpServer.framework/Versions/A/QtHttpServer" tr4qt

# Fix QtWebSockets path
install_name_tool -change "$QT_WEBSOCKETS/lib/QtWebSockets.framework/Versions/A/QtWebSockets" \
    "@rpath/QtWebSockets.framework/Versions/A/QtWebSockets" tr4qt

echo "    Fixed all executable paths"
cd - > /dev/null

# Step 6: Fix library IDs
echo "==> Step 6: Fixing library IDs..."

# Fix Hamlib ID
install_name_tool -id "@rpath/libhamlib.5.dylib" \
    "$FRAMEWORKS/libhamlib.5.dylib"
echo "    Fixed libhamlib.5.dylib ID"

# Fix libdbus ID
if [ -f "$FRAMEWORKS/libdbus-1.3.dylib" ]; then
    install_name_tool -id "@rpath/libdbus-1.3.dylib" \
        "$FRAMEWORKS/libdbus-1.3.dylib"
    echo "    Fixed libdbus-1.3.dylib ID"
fi

# Fix QtDBus ID and its dependency on libdbus
install_name_tool -id "@rpath/QtDBus.framework/Versions/A/QtDBus" \
    "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus"
install_name_tool -change "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" \
    "@rpath/libdbus-1.3.dylib" \
    "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus"
echo "    Fixed QtDBus.framework ID and dependencies"

# Step 7: Fix Qt plugin rpaths
echo "==> Step 7: Fixing Qt plugin rpaths..."
for plugin in "$PLUGINS"/*/*.dylib; do
    if [ -f "$plugin" ]; then
        # Add the correct rpath if not already present
        if ! otool -l "$plugin" | grep -q "@loader_path/../../../Frameworks"; then
            install_name_tool -add_rpath "@loader_path/../../../Frameworks" "$plugin" 2>/dev/null || true
        fi
    fi
done
echo "    Fixed plugin rpaths"

# Step 8: Create qt.conf
echo "==> Step 8: Creating qt.conf..."
cat > "$APP_BUNDLE/Contents/Resources/qt.conf" <<EOF
[Paths]
Plugins = PlugIns
EOF
echo "    Created qt.conf"

# Step 9: Sign all libraries and frameworks
echo "==> Step 9: Code signing..."

# Sign all dylibs
find "$FRAMEWORKS" -name "*.dylib" -exec codesign -s - -f {} \; 2>/dev/null
echo "    Signed all dylibs"

# Sign all frameworks
for framework in "$FRAMEWORKS"/*.framework; do
    if [ -d "$framework" ]; then
        codesign -s - -f "$framework" 2>/dev/null
    fi
done
echo "    Signed all frameworks"

# Sign all plugins
find "$PLUGINS" -name "*.dylib" -exec codesign -s - -f {} \; 2>/dev/null
echo "    Signed all plugins"

# Sign the executable
codesign -s - -f "$APP_BUNDLE/Contents/MacOS/tr4qt"
echo "    Signed executable"

# Sign the entire app bundle
codesign -s - -f "$APP_BUNDLE"
echo "    Signed app bundle"

# Step 10: Verify signature
echo "==> Step 10: Verifying signature..."
if codesign -vvv "$APP_BUNDLE" 2>&1 | grep -q "valid on disk"; then
    echo "    ✓ App bundle signature is valid"
else
    echo "    ✗ Warning: App bundle signature verification failed"
fi

# Step 11: Verification
echo "==> Step 11: Deployment verification..."
echo "Qt Frameworks:"
ls -lh "$FRAMEWORKS"/*.framework | awk '{print "  " $9}'
echo "Qt Plugins:"
find "$PLUGINS" -name "*.dylib" | sed 's|.*/|  |'
echo "Other Libraries:"
ls -lh "$FRAMEWORKS"/*.dylib | awk '{print "  " $9}'

echo ""
echo "==> Done! App bundle is ready at: $APP_BUNDLE"
echo ""
echo "To run the app:"
echo "  $APP_BUNDLE/Contents/MacOS/tr4qt"
echo ""
echo "To create a DMG:"
echo "  hdiutil create -volname \"TR4QT\" -srcfolder build/src/tr4qt.app -ov -format UDZO tr4qt.dmg"
