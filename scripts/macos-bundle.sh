#!/bin/bash
# macOS App Bundle Script - HYBRID APPROACH
# Uses macdeployqt for Qt (handles Qt dependencies), then adds what it misses

set -e  # Exit on error

APP_BUNDLE="./build/src/tr4qt.app"
FRAMEWORKS="$APP_BUNDLE/Contents/Frameworks"
PLUGINS="$APP_BUNDLE/Contents/PlugIns"
HAMLIB_INSTALL="$HOME/projects/Hamlib/hamlib_install"
QT_PREFIX="/opt/homebrew/opt/qtbase"

echo "==> Hybrid macOS App Bundling"
echo "==> App bundle: $APP_BUNDLE"

# Step 1: Run macdeployqt to bundle Qt frameworks and handle their dependencies
echo "==> Step 1: Running macdeployqt..."
MACDEPLOYQT="$(brew --prefix qt@6)/bin/macdeployqt"
if [ ! -f "$MACDEPLOYQT" ]; then
    echo "    ERROR: macdeployqt not found at $MACDEPLOYQT"
    exit 1
fi

$MACDEPLOYQT "$APP_BUNDLE" -verbose=1
echo "    ✓ macdeployqt completed"

# Step 2: Remove OpenSSL TLS plugin (macdeployqt may have copied it)
echo "==> Step 2: Removing OpenSSL TLS plugin (has Homebrew dependencies)..."
if [ -f "$PLUGINS/tls/libqopensslbackend.dylib" ]; then
    rm "$PLUGINS/tls/libqopensslbackend.dylib"
    echo "    Removed libqopensslbackend.dylib (requires Homebrew OpenSSL)"
fi

# Verify only macOS-native TLS backends remain
# - libqsecuretransportbackend.dylib: macOS native TLS (recommended)
# - libqcertonlybackend.dylib: certificate-only backend
echo "    Remaining TLS plugins:"
ls "$PLUGINS/tls/" 2>/dev/null | sed 's/^/      /' || echo "      (none)"

# Step 3: Copy Hamlib library
echo "==> Step 3: Copying Hamlib library..."
if [ -f "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" ]; then
    cp "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" "$FRAMEWORKS/"
    echo "    Copied libhamlib.5.dylib"
else
    echo "    ERROR: Hamlib library not found at $HAMLIB_INSTALL/lib/libhamlib.5.dylib"
    exit 1
fi

# Step 4: Copy libusb (required by Hamlib for USB radio support)
echo "==> Step 4: Copying libusb..."
if [ -f "/opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib" ]; then
    cp "/opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib" "$FRAMEWORKS/"
    echo "    Copied libusb-1.0.0.dylib"
else
    echo "    WARNING: libusb not found (USB radio support will not work)"
fi

# Step 4b: Copy QtDBus (macdeployqt doesn't bundle it, but QtGui needs it)
echo "==> Step 4b: Copying QtDBus framework..."
if [ -d "$QT_PREFIX/lib/QtDBus.framework" ]; then
    rm -rf "$FRAMEWORKS/QtDBus.framework"
    cp -R "$QT_PREFIX/lib/QtDBus.framework" "$FRAMEWORKS/"
    rm -rf "$FRAMEWORKS/QtDBus.framework/Headers"
    rm -rf "$FRAMEWORKS/QtDBus.framework/Versions/A/Headers"
    echo "    Copied QtDBus.framework"

    # Also copy libdbus (QtDBus depends on it)
    if [ -f "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" ]; then
        cp "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" "$FRAMEWORKS/"
        echo "    Copied libdbus-1.3.dylib"
    fi
fi

# Step 4c: Copy missing transitive dependencies (macdeployqt doesn't follow complete dependency chains)
echo "==> Step 4c: Copying missing transitive dependencies..."
# libbrotlicommon (required by libbrotlidec, which macdeployqt bundles but doesn't follow)
if [ -f "/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib" ]; then
    cp "/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib" "$FRAMEWORKS/"
    echo "    Copied libbrotlicommon.1.dylib"
fi

# Step 4d: Fix ALL Qt frameworks bundled by macdeployqt
# CRITICAL: macdeployqt copies frameworks but doesn't always fix their IDs
echo "==> Step 4d: Fixing library IDs for ALL Qt frameworks..."
for framework in "$FRAMEWORKS"/*.framework; do
    if [ -d "$framework" ]; then
        framework_name=$(basename "$framework" .framework)
        binary="$framework/Versions/A/$framework_name"

        if [ -f "$binary" ]; then
            # Fix the framework's install ID to use @rpath
            install_name_tool -id "@rpath/$framework_name.framework/Versions/A/$framework_name" "$binary" 2>/dev/null || true

            # Fix any absolute Homebrew paths in its dependencies
            DEPS=$(otool -L "$binary" 2>/dev/null | grep -E "^\s+/opt/homebrew" | awk '{print $1}' || true)
            for dep in $DEPS; do
                dep_name=$(basename "$dep")
                # Check if this dependency is a framework
                if echo "$dep" | grep -q "\.framework"; then
                    # Extract framework name from path like /opt/homebrew/opt/qtsvg/lib/QtSvg.framework/Versions/A/QtSvg
                    dep_framework=$(echo "$dep" | sed -E 's|.*/([^/]+\.framework).*|\1|')
                    dep_framework_name=$(basename "$dep_framework" .framework)
                    install_name_tool -change "$dep" "@rpath/$dep_framework_name.framework/Versions/A/$dep_framework_name" "$binary" 2>/dev/null || true
                else
                    # It's a dylib
                    install_name_tool -change "$dep" "@rpath/$dep_name" "$binary" 2>/dev/null || true
                fi
            done
            echo "    Fixed $framework_name.framework"
        fi
    fi
done

# Step 5: Fix library IDs for manually added libraries
echo "==> Step 5: Fixing library IDs for manually added libraries..."

# Fix Hamlib ID
install_name_tool -id "@rpath/libhamlib.5.dylib" \
    "$FRAMEWORKS/libhamlib.5.dylib"
echo "    Fixed libhamlib.5.dylib ID"

# Fix libusb ID
if [ -f "$FRAMEWORKS/libusb-1.0.0.dylib" ]; then
    install_name_tool -id "@rpath/libusb-1.0.0.dylib" \
        "$FRAMEWORKS/libusb-1.0.0.dylib"
    echo "    Fixed libusb-1.0.0.dylib ID"
fi

# Fix Hamlib's dependency on libusb
if [ -f "$FRAMEWORKS/libhamlib.5.dylib" ] && [ -f "$FRAMEWORKS/libusb-1.0.0.dylib" ]; then
    install_name_tool -change "/opt/homebrew/opt/libusb/lib/libusb-1.0.0.dylib" \
        "@rpath/libusb-1.0.0.dylib" \
        "$FRAMEWORKS/libhamlib.5.dylib"
    echo "    Fixed libhamlib.5.dylib dependency on libusb"
fi

# Fix libbrotlicommon ID
if [ -f "$FRAMEWORKS/libbrotlicommon.1.dylib" ]; then
    install_name_tool -id "@rpath/libbrotlicommon.1.dylib" \
        "$FRAMEWORKS/libbrotlicommon.1.dylib"
    echo "    Fixed libbrotlicommon.1.dylib ID"
fi

# Fix libdbus ID
if [ -f "$FRAMEWORKS/libdbus-1.3.dylib" ]; then
    install_name_tool -id "@rpath/libdbus-1.3.dylib" \
        "$FRAMEWORKS/libdbus-1.3.dylib"
    echo "    Fixed libdbus-1.3.dylib ID"
fi

# Fix QtDBus ID and its dependency on libdbus
if [ -f "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus" ]; then
    install_name_tool -id "@rpath/QtDBus.framework/Versions/A/QtDBus" \
        "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus"

    if [ -f "$FRAMEWORKS/libdbus-1.3.dylib" ]; then
        install_name_tool -change "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" \
            "@rpath/libdbus-1.3.dylib" \
            "$FRAMEWORKS/QtDBus.framework/Versions/A/QtDBus"
    fi
    echo "    Fixed QtDBus.framework ID and dependencies"
fi

# Step 6: Fix library paths in executable for manually added libraries
echo "==> Step 6: Fixing executable library paths..."
cd "$APP_BUNDLE/Contents/MacOS"

# CRITICAL: Remove Homebrew rpath (causes duplicate Qt loading)
# The build process adds /opt/homebrew/lib which makes dyld load system Qt
# This conflicts with bundled Qt and causes crashes
install_name_tool -delete_rpath "/opt/homebrew/lib" tr4qt 2>/dev/null || true
echo "    Removed /opt/homebrew/lib rpath"

# Add correct rpath for app bundle (points to Frameworks directory)
install_name_tool -add_rpath "@executable_path/../Frameworks" tr4qt 2>/dev/null || true
echo "    Added @executable_path/../Frameworks to rpath"

# Fix Hamlib path
install_name_tool -change "$HAMLIB_INSTALL/lib/libhamlib.5.dylib" \
    "@rpath/libhamlib.5.dylib" tr4qt
echo "    Fixed Hamlib path in executable"

cd - > /dev/null

# Step 7: Fix Qt plugin rpaths (for TLS plugins we added)
echo "==> Step 7: Fixing TLS plugin rpaths..."
for plugin in "$PLUGINS"/tls/*.dylib; do
    if [ -f "$plugin" ]; then
        # Add the correct rpath if not already present
        if ! otool -l "$plugin" | grep -q "@loader_path/../../../Frameworks"; then
            install_name_tool -add_rpath "@loader_path/../../../Frameworks" "$plugin" 2>/dev/null || true
        fi
    fi
done
echo "    Fixed TLS plugin rpaths"

# Step 8: Code signing (CRITICAL - sign in dependency order)
echo "==> Step 8: Code signing..."
echo "    IMPORTANT: Sign dependencies before dependents, use --deep for thorough signing"

# Sign order:
# 1. Libraries (deepest dependencies first)
# 2. Frameworks
# 3. Plugins
# 4. Executable
# 5. App bundle

# 1. Sign all dylibs in Frameworks (dependencies)
echo "    Signing all dylibs in Frameworks..."
find "$FRAMEWORKS" -name "*.dylib" -type f -exec codesign --force --sign - {} \;

# 2. Sign all frameworks
echo "    Signing all frameworks..."
for framework in "$FRAMEWORKS"/*.framework; do
    if [ -d "$framework" ]; then
        codesign --force --sign - "$framework"
    fi
done

# 3. Sign all plugins
echo "    Signing all plugins..."
find "$PLUGINS" -name "*.dylib" -type f -exec codesign --force --sign - {} \;

# 4. Sign the executable
echo "    Signing executable..."
codesign --force --sign - "$APP_BUNDLE/Contents/MacOS/tr4qt"

# 5. Sign the entire app bundle
echo "    Signing app bundle..."
codesign --force --sign - "$APP_BUNDLE"

echo "    ✓ Code signing completed"

# Step 9: Verify signature
echo "==> Step 9: Verifying signature..."
if codesign -vvv "$APP_BUNDLE" 2>&1 | grep -q "valid on disk"; then
    echo "    ✓ App bundle signature is valid"
else
    echo "    ✗ Warning: App bundle signature verification failed"
fi

# Step 10: CRITICAL - Check for absolute Homebrew paths
echo ""
echo "==> Step 10: CRITICAL - Checking for absolute Homebrew paths..."
echo "    This check prevents deployment issues on Macs without Homebrew"
echo ""

# Check executable
echo "Checking executable..."
EXEC_PATHS=$(otool -L "$APP_BUNDLE/Contents/MacOS/tr4qt" | grep -E "^\s+/opt/homebrew" | grep -v "@rpath" || true)
if [ -n "$EXEC_PATHS" ]; then
    echo "    ✗ ERROR: Executable has absolute Homebrew paths:"
    echo "$EXEC_PATHS"
    exit 1
fi
echo "    ✓ Executable OK"

# Check all dylibs in Frameworks
echo "Checking dylibs..."
DYLIB_ERRORS=0
for lib in "$FRAMEWORKS"/*.dylib; do
    if [ -f "$lib" ]; then
        LIB_PATHS=$(otool -L "$lib" 2>/dev/null | grep -E "^\s+/opt/homebrew" | grep -v "@rpath" || true)
        if [ -n "$LIB_PATHS" ]; then
            echo "    ✗ ERROR: $(basename "$lib") has absolute Homebrew paths:"
            echo "$LIB_PATHS"
            DYLIB_ERRORS=1
        fi
    fi
done
if [ $DYLIB_ERRORS -eq 0 ]; then
    echo "    ✓ All dylibs OK"
else
    exit 1
fi

# Check all frameworks
echo "Checking frameworks..."
FRAMEWORK_ERRORS=0
for framework in "$FRAMEWORKS"/*.framework; do
    if [ -d "$framework" ]; then
        binary="$framework/Versions/A/$(basename "$framework" .framework)"
        if [ -f "$binary" ]; then
            FW_PATHS=$(otool -L "$binary" 2>/dev/null | grep -E "^\s+/opt/homebrew" | grep -v "@rpath" || true)
            if [ -n "$FW_PATHS" ]; then
                echo "    ✗ ERROR: $(basename "$framework") has absolute Homebrew paths:"
                echo "$FW_PATHS"
                FRAMEWORK_ERRORS=1
            fi
        fi
    fi
done
if [ $FRAMEWORK_ERRORS -eq 0 ]; then
    echo "    ✓ All frameworks OK"
else
    exit 1
fi

# Check all plugins
echo "Checking plugins..."
PLUGIN_ERRORS=0
for plugin in "$PLUGINS"/*/*.dylib; do
    if [ -f "$plugin" ]; then
        PLUGIN_PATHS=$(otool -L "$plugin" 2>/dev/null | grep -E "^\s+/opt/homebrew" | grep -v "@rpath" || true)
        if [ -n "$PLUGIN_PATHS" ]; then
            echo "    ✗ ERROR: $(basename "$plugin") has absolute Homebrew paths:"
            echo "$PLUGIN_PATHS"
            PLUGIN_ERRORS=1
        fi
    fi
done
if [ $PLUGIN_ERRORS -eq 0 ]; then
    echo "    ✓ All plugins OK"
else
    exit 1
fi

echo ""
echo "==> ✓ SUCCESS: No absolute Homebrew paths found!"
echo "    App bundle will work on Macs without Homebrew installed"
echo ""

# Step 11: Deployment verification
echo "==> Step 11: Deployment summary..."
echo "Qt Frameworks:"
ls -lh "$FRAMEWORKS"/*.framework 2>/dev/null | awk '{print "  " $9}' || echo "  (none)"
echo "Qt Plugins:"
find "$PLUGINS" -name "*.dylib" 2>/dev/null | sed 's|.*/|  |' || echo "  (none)"
echo "Other Libraries:"
ls -lh "$FRAMEWORKS"/*.dylib 2>/dev/null | awk '{print "  " $9}' || echo "  (none)"

echo ""
echo "==> Done! App bundle is ready at: $APP_BUNDLE"
echo ""
echo "To run the app:"
echo "  $APP_BUNDLE/Contents/MacOS/tr4qt"
echo ""
echo "To create a DMG:"
echo "  hdiutil create -volname \"TR4QT\" -srcfolder build/src/tr4qt.app -ov -format UDZO tr4qt.dmg"
