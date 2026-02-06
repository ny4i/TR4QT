#!/bin/bash
# Build TR4QT Linux AppImage on ARM64 Pi
# Usage: ./scripts/build-linux-appimage.sh [--clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
SHARED_DIR="/mnt/shared/TR4QT/linuxBuild"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== TR4QT Linux AppImage Builder ===${NC}"

# Check for clean build flag
if [ "$1" == "--clean" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Configure if needed
if [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo -e "${YELLOW}Configuring CMake...${NC}"
    cmake -B "$BUILD_DIR" "$PROJECT_DIR"
fi

# Build
echo -e "${YELLOW}Building TR4QT...${NC}"
cmake --build "$BUILD_DIR" -j$(nproc)

# Check build succeeded
if [ ! -f "$BUILD_DIR/src/tr4qt" ]; then
    echo -e "${RED}Build failed - executable not found${NC}"
    exit 1
fi

echo -e "${GREEN}Build complete${NC}"

# Create AppImage
echo -e "${YELLOW}Creating AppImage...${NC}"

# Clean up old AppImage artifacts
rm -rf "$PROJECT_DIR/AppDir" "$PROJECT_DIR"/TR4QT*.AppImage

# Create AppDir structure
mkdir -p "$PROJECT_DIR/AppDir/usr/bin"
mkdir -p "$PROJECT_DIR/AppDir/usr/share/applications"
mkdir -p "$PROJECT_DIR/AppDir/usr/share/icons/hicolor/256x256/apps"

cp "$BUILD_DIR/src/tr4qt" "$PROJECT_DIR/AppDir/usr/bin/"

cat > "$PROJECT_DIR/AppDir/usr/share/applications/tr4qt.desktop" << 'EOF'
[Desktop Entry]
Name=TR4QT
Exec=tr4qt
Icon=tr4qt
Type=Application
Categories=Utility;HamRadio;
EOF

# Copy icon if exists, otherwise create placeholder
if [ -f "$PROJECT_DIR/resources/icons/tr4qt.png" ]; then
    cp "$PROJECT_DIR/resources/icons/tr4qt.png" "$PROJECT_DIR/AppDir/usr/share/icons/hicolor/256x256/apps/"
else
    touch "$PROJECT_DIR/AppDir/usr/share/icons/hicolor/256x256/apps/tr4qt.png"
fi

ln -sf usr/share/applications/tr4qt.desktop "$PROJECT_DIR/AppDir/tr4qt.desktop"
ln -sf usr/share/icons/hicolor/256x256/apps/tr4qt.png "$PROJECT_DIR/AppDir/tr4qt.png"

# Download linuxdeploy if not present
ARCH=$(uname -m)
if [ "$ARCH" == "aarch64" ]; then
    LINUXDEPLOY_ARCH="aarch64"
else
    LINUXDEPLOY_ARCH="x86_64"
fi

if [ ! -f "$PROJECT_DIR/linuxdeploy" ]; then
    echo -e "${YELLOW}Downloading linuxdeploy...${NC}"
    wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${LINUXDEPLOY_ARCH}.AppImage" \
        -O "$PROJECT_DIR/linuxdeploy"
    chmod +x "$PROJECT_DIR/linuxdeploy"
fi

if [ ! -f "$PROJECT_DIR/linuxdeploy-plugin-qt" ]; then
    echo -e "${YELLOW}Downloading linuxdeploy Qt plugin...${NC}"
    wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${LINUXDEPLOY_ARCH}.AppImage" \
        -O "$PROJECT_DIR/linuxdeploy-plugin-qt"
    chmod +x "$PROJECT_DIR/linuxdeploy-plugin-qt"
fi

# Set QMAKE path for Qt6
export QMAKE=/usr/lib/qt6/bin/qmake

# Run linuxdeploy
cd "$PROJECT_DIR"
./linuxdeploy --appdir AppDir --executable build/src/tr4qt --plugin qt --output appimage

# Find the created AppImage
APPIMAGE=$(ls -1 TR4QT*.AppImage 2>/dev/null | head -1)

if [ -z "$APPIMAGE" ]; then
    echo -e "${RED}AppImage creation failed${NC}"
    exit 1
fi

echo -e "${GREEN}AppImage created: $APPIMAGE${NC}"

# Copy to shared location if available
if [ -d "/mnt/shared" ]; then
    mkdir -p "$SHARED_DIR"
    cp "$APPIMAGE" "$SHARED_DIR/"
    echo -e "${GREEN}Copied to: $SHARED_DIR/$APPIMAGE${NC}"
fi

# Print summary
echo ""
echo -e "${GREEN}=== Build Summary ===${NC}"
echo "  Executable: $BUILD_DIR/src/tr4qt"
echo "  AppImage:   $PROJECT_DIR/$APPIMAGE"
if [ -d "/mnt/shared" ]; then
    echo "  Shared:     $SHARED_DIR/$APPIMAGE"
fi
echo ""
ls -lh "$APPIMAGE"
