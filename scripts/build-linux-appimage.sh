#!/bin/bash
# Build TR4QT Linux AppImage on ARM64 Pi using Bookworm chroot
# Usage: ./scripts/build-linux-appimage.sh [--clean]
#
# Requirements:
#   - Bookworm chroot at /opt/bookworm
#   - appimagetool extracted at ~/TR4QT/squashfs-root/
#   - AppDir-bookworm directory with native wrapper binary

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Paths
CHROOT_PATH="/opt/bookworm"
PROJECT_PATH="/home/pi/TR4QT"
BUILD_DIR="build-bookworm"
APPDIR="AppDir-bookworm"
APPIMAGETOOL="squashfs-root/usr/bin/appimagetool"

echo -e "${GREEN}=== TR4QT Linux AppImage Builder (Bookworm Chroot) ===${NC}"

# Check for required components
if [ ! -d "$CHROOT_PATH" ]; then
    echo -e "${RED}Error: Bookworm chroot not found at $CHROOT_PATH${NC}"
    exit 1
fi

if [ ! -f "$HOME/TR4QT/$APPIMAGETOOL" ]; then
    echo -e "${RED}Error: appimagetool not found at $HOME/TR4QT/$APPIMAGETOOL${NC}"
    echo -e "${YELLOW}Extract it first with: ./appimagetool --appimage-extract${NC}"
    exit 1
fi

if [ ! -d "$HOME/TR4QT/$APPDIR" ]; then
    echo -e "${RED}Error: AppDir-bookworm not found at $HOME/TR4QT/$APPDIR${NC}"
    exit 1
fi

# Check and mount chroot bind mounts if needed
echo -e "${YELLOW}Checking chroot bind mounts...${NC}"
for mount_point in proc sys dev dev/pts; do
    if ! mountpoint -q "$CHROOT_PATH/$mount_point" 2>/dev/null; then
        echo -e "${YELLOW}Mounting $mount_point...${NC}"
        sudo mount --bind /$mount_point "$CHROOT_PATH/$mount_point"
    fi
done

# Bind mount project directory if needed
if ! mountpoint -q "$CHROOT_PATH$PROJECT_PATH" 2>/dev/null; then
    echo -e "${YELLOW}Mounting project directory...${NC}"
    sudo mount --bind "$PROJECT_PATH" "$CHROOT_PATH$PROJECT_PATH"
fi

echo -e "${GREEN}Chroot mounts ready${NC}"

# Pull latest code
echo -e "${YELLOW}Pulling latest code...${NC}"
cd "$HOME/TR4QT"
git pull

# Check for clean build flag
if [ "$1" == "--clean" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    sudo chroot "$CHROOT_PATH" /bin/bash -c "cd $PROJECT_PATH && rm -rf $BUILD_DIR"
fi

# Build in chroot
echo -e "${YELLOW}Building TR4QT in Bookworm chroot...${NC}"
sudo chroot "$CHROOT_PATH" /bin/bash -c "cd $PROJECT_PATH && cmake --build $BUILD_DIR -j8"

# Check build succeeded
if [ ! -f "$HOME/TR4QT/$BUILD_DIR/src/tr4qt" ]; then
    echo -e "${RED}Build failed - executable not found at $BUILD_DIR/src/tr4qt${NC}"
    exit 1
fi

echo -e "${GREEN}Build complete${NC}"

# Update AppDir with new binary
echo -e "${YELLOW}Updating AppDir with new binary...${NC}"
cp "$HOME/TR4QT/$BUILD_DIR/src/tr4qt" "$HOME/TR4QT/$APPDIR/usr/bin/tr4qt.bin"

# Create AppImage
echo -e "${YELLOW}Creating AppImage...${NC}"
cd "$HOME/TR4QT"
rm -f TR4QT-aarch64.AppImage

ARCH=aarch64 ./$APPIMAGETOOL -n "$APPDIR" TR4QT-aarch64.AppImage

# Check AppImage creation succeeded
if [ ! -f "TR4QT-aarch64.AppImage" ]; then
    echo -e "${RED}AppImage creation failed${NC}"
    exit 1
fi

echo -e "${GREEN}AppImage created: TR4QT-aarch64.AppImage${NC}"

# Print summary
echo ""
echo -e "${GREEN}=== Build Summary ===${NC}"
echo "  Chroot:     $CHROOT_PATH"
echo "  Build dir:  $BUILD_DIR"
echo "  AppDir:     $APPDIR"
echo "  AppImage:   $HOME/TR4QT/TR4QT-aarch64.AppImage"
echo ""
ls -lh TR4QT-aarch64.AppImage
echo ""
echo -e "${YELLOW}To test: ./TR4QT-aarch64.AppImage${NC}"
