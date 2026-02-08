#!/bin/bash
# Post-CI Release Script
# Builds Raspberry Pi ARM64 AppImage and uploads to the GitHub release.
# macOS and Windows builds are handled by CI runners automatically.
#
# Usage: ./scripts/post-release.sh [version]
#   version: e.g., v3.40.62 (auto-detected from Constants.h if omitted)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

# --- Determine version ---
if [ -n "$1" ]; then
    VERSION="$1"
else
    VERSION="v$(grep 'APP_VERSION' src/core/Constants.h | sed 's/.*"\(.*\)".*/\1/')"
fi
echo "==> Post-release build for $VERSION"

# --- Verify tag exists ---
if ! git tag -l "$VERSION" | grep -q "$VERSION"; then
    echo "ERROR: Tag $VERSION does not exist. Create it first."
    exit 1
fi

# --- Verify release exists ---
if ! gh release view "$VERSION" > /dev/null 2>&1; then
    echo "WARNING: Release $VERSION does not exist yet. CI may still be running."
    echo "         Waiting for release to appear..."
    for i in $(seq 1 60); do
        if gh release view "$VERSION" > /dev/null 2>&1; then
            echo "         Release found."
            break
        fi
        if [ "$i" -eq 60 ]; then
            echo "ERROR: Release $VERSION not found after 30 minutes. Check CI."
            exit 1
        fi
        sleep 30
    done
fi

# ============================================================
# Raspberry Pi ARM64 AppImage
# ============================================================
echo ""
echo "============================================================"
echo "  Raspberry Pi ARM64 AppImage"
echo "============================================================"

PI_APPIMAGE="TR4QT-${VERSION}-aarch64.AppImage"

echo "==> Checking bench5 connectivity..."
if ! ssh -o ConnectTimeout=5 bench5 "echo ok" > /dev/null 2>&1; then
    echo "    ✗ Cannot reach bench5. Skipping Pi build."
    echo "    Run manually later:"
    echo "      ssh bench5 'cd ~/TR4QT && git pull && sudo chroot /opt/bookworm /bin/bash -c \"cd /home/pi/TR4QT && cmake --build build-bookworm -j4\" && cp build-bookworm/src/tr4qt AppDir-bookworm/usr/bin/tr4qt.bin && ARCH=aarch64 ./squashfs-root/usr/bin/appimagetool -n AppDir-bookworm $PI_APPIMAGE'"
    echo "      scp bench5:~/TR4QT/$PI_APPIMAGE ."
    echo "      gh release upload $VERSION $PI_APPIMAGE"
else
    echo "==> Updating repo on bench5..."
    ssh bench5 "cd ~/TR4QT && git pull"

    echo "==> Ensuring chroot mounts..."
    ssh bench5 "sudo mount --bind /proc /opt/bookworm/proc 2>/dev/null; sudo mount --bind /sys /opt/bookworm/sys 2>/dev/null; sudo mount --bind /dev /opt/bookworm/dev 2>/dev/null; sudo mount --bind /dev/pts /opt/bookworm/dev/pts 2>/dev/null; sudo mount --bind /home/pi/TR4QT /opt/bookworm/home/pi/TR4QT 2>/dev/null; echo 'Mounts ready'"

    echo "==> Building in Bookworm chroot..."
    ssh bench5 "sudo chroot /opt/bookworm /bin/bash -c 'cd /home/pi/TR4QT && cmake --build build-bookworm -j4'"

    echo "==> Creating AppImage..."
    ssh bench5 "cd ~/TR4QT && cp build-bookworm/src/tr4qt AppDir-bookworm/usr/bin/tr4qt.bin && ARCH=aarch64 ./squashfs-root/usr/bin/appimagetool -n AppDir-bookworm $PI_APPIMAGE"

    echo "==> Copying AppImage from bench5..."
    scp "bench5:~/TR4QT/$PI_APPIMAGE" .

    echo "==> Uploading Pi AppImage to release..."
    gh release upload "$VERSION" "$PI_APPIMAGE" --clobber

    echo "    ✓ Pi AppImage uploaded"
fi

# ============================================================
# SUMMARY
# ============================================================
echo ""
echo "============================================================"
echo "  RELEASE $VERSION COMPLETE"
echo "============================================================"
echo ""
echo "Assets:"
gh release view "$VERSION" --json assets --jq '.assets[] | "  \(.name) (\(.size / 1048576 | floor)MB)"'
echo ""
echo "URL: https://github.com/ny4i/TR4QT/releases/tag/$VERSION"
