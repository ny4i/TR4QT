#!/bin/bash
# macOS App Bundle Post-Build Script
# Bundles Qt frameworks, copies dependencies, and signs the app

set -e  # Exit on error

APP_BUNDLE="./build/src/tr4qt.app"
HAMLIB_INSTALL="$HOME/projects/Hamlib/hamlib_install"

echo "==> macOS App Bundling Script"
echo "==> App bundle: $APP_BUNDLE"

# Step 1: Run macdeployqt to bundle Qt frameworks
echo "==> Step 1: Running macdeployqt..."
/opt/homebrew/bin/macdeployqt "$APP_BUNDLE" -verbose=1

# Step 2: Copy Hamlib library
echo "==> Step 2: Copying Hamlib library..."
if [ -f "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" ]; then
    cp "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" "$APP_BUNDLE/Contents/Frameworks/"
    echo "    Copied libhamlib.5.dylib"
else
    echo "    Warning: Hamlib library not found at $HAMLIB_INSTALL/lib/libhamlib.5.dylib"
fi

# Step 3: Fix library paths to use @rpath
echo "==> Step 3: Fixing library paths..."
cd "$APP_BUNDLE/Contents/MacOS"

# Fix Hamlib path
if otool -L tr4qt | grep -q "/Users/toms/projects"; then
    install_name_tool -change "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" \
        "@rpath/libhamlib.5.dylib" tr4qt
    echo "    Fixed Hamlib path"
fi

# Fix Qt framework paths (only if they're still absolute paths)
for framework in QtSql QtHttpServer QtPrintSupport QtConcurrent QtWebSockets \
                 QtNetwork QtWidgets QtGui QtCore; do
    old_path="/opt/homebrew/opt/qtbase/lib/${framework}.framework/Versions/A/${framework}"
    if otool -L tr4qt | grep -q "$old_path"; then
        install_name_tool -change "$old_path" \
            "@rpath/${framework}.framework/Versions/A/${framework}" tr4qt
        echo "    Fixed $framework path"
    fi
done

# Fix QtHttpServer path (different Homebrew location)
old_path="/opt/homebrew/opt/qthttpserver/lib/QtHttpServer.framework/Versions/A/QtHttpServer"
if otool -L tr4qt | grep -q "$old_path"; then
    install_name_tool -change "$old_path" \
        "@rpath/QtHttpServer.framework/Versions/A/QtHttpServer" tr4qt
    echo "    Fixed QtHttpServer path"
fi

# Fix QtWebSockets path (different Homebrew location)
old_path="/opt/homebrew/opt/qtwebsockets/lib/QtWebSockets.framework/Versions/A/QtWebSockets"
if otool -L tr4qt | grep -q "$old_path"; then
    install_name_tool -change "$old_path" \
        "@rpath/QtWebSockets.framework/Versions/A/QtWebSockets" tr4qt
    echo "    Fixed QtWebSockets path"
fi

cd - > /dev/null

# Step 4: Fix Hamlib library ID
echo "==> Step 4: Fixing library IDs..."
if [ -f "$APP_BUNDLE/Contents/Frameworks/libhamlib.5.dylib" ]; then
    install_name_tool -id "@rpath/libhamlib.5.dylib" \
        "$APP_BUNDLE/Contents/Frameworks/libhamlib.5.dylib"
    echo "    Fixed libhamlib.5.dylib ID"
fi

# Step 5: Sign all libraries and frameworks
echo "==> Step 5: Code signing..."

# Sign all dylibs
find "$APP_BUNDLE/Contents/Frameworks" -name "*.dylib" -exec codesign -s - -f {} \; 2>/dev/null
echo "    Signed all dylibs"

# Sign all frameworks
for framework in "$APP_BUNDLE/Contents/Frameworks"/*.framework; do
    if [ -d "$framework" ]; then
        codesign -s - -f "$framework" 2>/dev/null
    fi
done
echo "    Signed all frameworks"

# Sign all plugins
find "$APP_BUNDLE/Contents/PlugIns" -name "*.dylib" -exec codesign -s - -f {} \; 2>/dev/null
echo "    Signed all plugins"

# Sign the executable
codesign -s - -f "$APP_BUNDLE/Contents/MacOS/tr4qt"
echo "    Signed executable"

# Sign the entire app bundle
codesign -s - -f "$APP_BUNDLE"
echo "    Signed app bundle"

# Step 6: Verify signature
echo "==> Step 6: Verifying signature..."
if codesign -vvv "$APP_BUNDLE" 2>&1 | grep -q "valid on disk"; then
    echo "    ✓ App bundle signature is valid"
else
    echo "    ✗ Warning: App bundle signature verification failed"
fi

echo "==> Done! App bundle is ready at: $APP_BUNDLE"
echo ""
echo "To run the app:"
echo "  $APP_BUNDLE/Contents/MacOS/tr4qt"
echo ""
echo "To create a DMG:"
echo "  hdiutil create -volname \"TR4QT\" -srcfolder build/src/tr4qt.app -ov -format UDZO tr4qt.dmg"
