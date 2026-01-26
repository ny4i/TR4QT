#!/bin/bash
# TR4QT Deployment Verification Script
# Checks that all required DLLs, plugins, and resources are present for deployment
#
# Usage:
#   ./scripts/verify-deployment.sh [build_dir]
#
# Example:
#   ./scripts/verify-deployment.sh build/src

set -e  # Exit on error (but check functions handle their own errors)

BUILD_DIR="${1:-build/src}"
ERRORS=0
WARNINGS=0

echo "======================================"
echo "TR4QT Deployment Verification"
echo "======================================"
echo ""
echo "Checking: $BUILD_DIR"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if file exists
check_file() {
    local file="$1"
    local description="$2"
    local is_critical="${3:-true}"

    if [ -f "$BUILD_DIR/$file" ]; then
        echo -e "${GREEN}✓${NC} $description: $file"
    else
        if [ "$is_critical" = "true" ]; then
            echo -e "${RED}✗ ERROR${NC}: $description missing: $file"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "${YELLOW}⚠ WARNING${NC}: $description missing (optional): $file"
            WARNINGS=$((WARNINGS + 1))
        fi
    fi
    # Always return 0 to prevent set -e from exiting script
    return 0
}

# Function to check directory exists and has files
check_directory() {
    local dir="$1"
    local description="$2"
    local pattern="${3:-*.*}"
    local is_critical="${4:-true}"

    if [ -d "$BUILD_DIR/$dir" ]; then
        local file_count=$(find "$BUILD_DIR/$dir" -name "$pattern" -type f 2>/dev/null | wc -l)
        if [ "$file_count" -gt 0 ]; then
            echo -e "${GREEN}✓${NC} $description: $dir/ ($file_count files)"
        else
            if [ "$is_critical" = "true" ]; then
                echo -e "${RED}✗ ERROR${NC}: $description has no files: $dir/"
                ERRORS=$((ERRORS + 1))
            else
                echo -e "${YELLOW}⚠ WARNING${NC}: $description has no files: $dir/"
                WARNINGS=$((WARNINGS + 1))
            fi
        fi
    else
        if [ "$is_critical" = "true" ]; then
            echo -e "${RED}✗ ERROR${NC}: $description directory missing: $dir/"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "${YELLOW}⚠ WARNING${NC}: $description directory missing: $dir/"
            WARNINGS=$((WARNINGS + 1))
        fi
    fi
    # Always return 0 to prevent set -e from exiting script
    return 0
}

echo "=== Executable ==="
check_file "tr4qt.exe" "Main executable" true || check_file "tr4qt" "Main executable (Unix)" true || true
echo ""

echo "=== Qt Core DLLs ==="
check_file "Qt6Core.dll" "Qt Core" true
check_file "Qt6Gui.dll" "Qt GUI" true
check_file "Qt6Widgets.dll" "Qt Widgets" true
check_file "Qt6Network.dll" "Qt Network" true
check_file "Qt6Sql.dll" "Qt SQL" true
echo ""

echo "=== Qt Additional DLLs ==="
check_file "Qt6HttpServer.dll" "Qt HTTP Server" true
check_file "Qt6WebSockets.dll" "Qt WebSockets" true
check_file "Qt6SerialPort.dll" "Qt Serial Port" true
check_file "Qt6PrintSupport.dll" "Qt Print Support" true
check_file "Qt6Concurrent.dll" "Qt Concurrent" true
check_file "Qt6Svg.dll" "Qt SVG" true
check_file "Qt6Xml.dll" "Qt XML" true
echo ""

echo "=== Qt Plugins ==="
check_directory "platforms" "Platform plugins (qwindows)" "*.dll" true
check_directory "sqldrivers" "SQL drivers (qsqlite)" "*.dll" true
check_directory "imageformats" "Image format plugins" "*.dll" false
check_directory "styles" "Style plugins" "*.dll" false
check_directory "tls" "TLS plugins" "*.dll" true
echo ""

echo "=== Runtime Libraries ==="
if [ -f "$BUILD_DIR/Qt6Core.dll" ]; then
    # Windows checks - try different GCC runtime variants
    if [ -f "$BUILD_DIR/libgcc_s_seh-1.dll" ]; then
        check_file "libgcc_s_seh-1.dll" "MinGW GCC runtime (SEH)" true
    elif [ -f "$BUILD_DIR/libgcc_s_dw2-1.dll" ]; then
        check_file "libgcc_s_dw2-1.dll" "MinGW GCC runtime (DW2)" true
    elif [ -f "$BUILD_DIR/libgcc_s_sjlj-1.dll" ]; then
        check_file "libgcc_s_sjlj-1.dll" "MinGW GCC runtime (SJLJ)" true
    else
        echo -e "${RED}✗ ERROR${NC}: No MinGW GCC runtime found"
        ERRORS=$((ERRORS + 1))
    fi

    check_file "libstdc++-6.dll" "MinGW C++ runtime" true
    check_file "libwinpthread-1.dll" "MinGW pthread" true
fi
echo ""

echo "=== Hamlib ==="
# Try different platforms
if [ -f "$BUILD_DIR/libhamlib-4.dll" ]; then
    check_file "libhamlib-4.dll" "Hamlib library (Windows)" true
elif [ -f "$BUILD_DIR/libhamlib.4.dylib" ]; then
    check_file "libhamlib.4.dylib" "Hamlib library (macOS)" true
elif [ -f "$BUILD_DIR/libhamlib.so.4" ]; then
    check_file "libhamlib.so.4" "Hamlib library (Linux)" true
else
    echo -e "${RED}✗ ERROR${NC}: Hamlib library not found"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo "======================================"
echo "Verification Summary"
echo "======================================"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ ALL CHECKS PASSED${NC}"
    echo "Deployment is ready for packaging."
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠ ${WARNINGS} WARNING(S)${NC}"
    echo "Deployment has non-critical issues but should work."
    exit 0
else
    echo -e "${RED}✗ ${ERRORS} ERROR(S), ${WARNINGS} WARNING(S)${NC}"
    echo ""
    echo "Deployment is INCOMPLETE and will NOT work correctly."
    echo "Please fix the errors above before packaging."
    exit 1
fi
