#!/bin/bash
# Version Check Script
# Validates that build environment versions match expected versions
# Exit code 0 = all good, 1 = warning, 2 = critical mismatch

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Color codes for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Expected versions (single source of truth)
EXPECTED_QT_VERSION="6.10.1"
EXPECTED_HAMLIB_VERSION="4.6.5"

# Tolerance: How many minor versions behind before we warn
QT_MINOR_VERSION_TOLERANCE=1  # Warn if >1 minor version behind (e.g., 6.8.x when expecting 6.10.x)

echo "=== TR4QT Version Validation ==="
echo ""

# Function to parse semantic version
parse_version() {
    echo "$1" | sed 's/[^0-9.]*\([0-9.]*\).*/\1/'
}

# Function to compare versions (returns 0 if equal, 1 if v1 < v2, 2 if v1 > v2)
version_compare() {
    if [[ "$1" == "$2" ]]; then
        return 0
    fi

    local IFS=.
    local i ver1=($1) ver2=($2)

    # Fill empty positions with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++)); do
        ver1[i]=0
    done

    for ((i=0; i<${#ver1[@]}; i++)); do
        if [[ -z ${ver2[i]} ]]; then
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]})); then
            return 2
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]})); then
            return 1
        fi
    done
    return 0
}

# Check Qt version
echo "Checking Qt version..."
DETECTED_QT_VERSION=""

if command -v qmake >/dev/null 2>&1; then
    DETECTED_QT_VERSION=$(qmake -query QT_VERSION)
elif [ -n "$Qt6_DIR" ]; then
    # CI environment - parse from Qt6_DIR
    DETECTED_QT_VERSION=$(echo "$Qt6_DIR" | grep -oP '\d+\.\d+\.\d+' | head -1)
elif command -v brew >/dev/null 2>&1; then
    # macOS - check Homebrew
    if brew list qt@6 >/dev/null 2>&1; then
        DETECTED_QT_VERSION=$(brew list --versions qt@6 | awk '{print $2}')
    fi
fi

HAS_WARNINGS=0
HAS_ERRORS=0

if [ -z "$DETECTED_QT_VERSION" ]; then
    echo -e "${YELLOW}⚠ WARNING: Could not detect Qt version${NC}"
    echo "  Please verify Qt is installed and qmake is in PATH"
    HAS_WARNINGS=1
else
    echo "  Expected: $EXPECTED_QT_VERSION"
    echo "  Detected: $DETECTED_QT_VERSION"

    # Parse major.minor version
    EXPECTED_MAJOR=$(echo "$EXPECTED_QT_VERSION" | cut -d. -f1)
    EXPECTED_MINOR=$(echo "$EXPECTED_QT_VERSION" | cut -d. -f2)
    DETECTED_MAJOR=$(echo "$DETECTED_QT_VERSION" | cut -d. -f1)
    DETECTED_MINOR=$(echo "$DETECTED_QT_VERSION" | cut -d. -f2)

    if [ "$DETECTED_QT_VERSION" = "$EXPECTED_QT_VERSION" ]; then
        echo -e "${GREEN}✓ Qt version matches expected version${NC}"
    else
        # Check if major version matches
        if [ "$EXPECTED_MAJOR" != "$DETECTED_MAJOR" ]; then
            echo -e "${RED}✗ CRITICAL: Qt major version mismatch!${NC}"
            echo "  This will likely cause build or runtime failures"
            echo "  Action: Update CI to use Qt $EXPECTED_QT_VERSION or update local environment"
            HAS_ERRORS=1
        else
            # Same major version - check minor version difference
            MINOR_DIFF=$((EXPECTED_MINOR - DETECTED_MINOR))
            if [ $MINOR_DIFF -lt 0 ]; then
                MINOR_DIFF=$((0 - MINOR_DIFF))
            fi

            if [ $MINOR_DIFF -gt $QT_MINOR_VERSION_TOLERANCE ]; then
                echo -e "${YELLOW}⚠ WARNING: Qt minor version differs by $MINOR_DIFF (tolerance: $QT_MINOR_VERSION_TOLERANCE)${NC}"
                echo "  This may cause subtle compatibility issues"
                echo "  Recommendation: Update to Qt $EXPECTED_QT_VERSION"
                HAS_WARNINGS=1
            else
                echo -e "${GREEN}✓ Qt version close enough (within tolerance)${NC}"
            fi
        fi
    fi
fi

echo ""

# Check Hamlib version
echo "Checking Hamlib version..."
DETECTED_HAMLIB_VERSION=""

if [ -f "$HAMLIB_ROOT/lib/libhamlib.so" ] || [ -f "$HAMLIB_ROOT/lib/libhamlib.dylib" ] || [ -f "$HAMLIB_ROOT/bin/libhamlib-4.dll" ]; then
    # Try to parse from HAMLIB_ROOT path
    DETECTED_HAMLIB_VERSION=$(echo "$HAMLIB_ROOT" | grep -oP '\d+\.\d+\.\d+' | head -1)
fi

if [ -z "$DETECTED_HAMLIB_VERSION" ] && command -v rigctl >/dev/null 2>&1; then
    # Try rigctl --version
    DETECTED_HAMLIB_VERSION=$(rigctl --version 2>&1 | grep -oP 'Hamlib \K\d+\.\d+\.\d+' | head -1)
fi

if [ -z "$DETECTED_HAMLIB_VERSION" ]; then
    echo -e "${YELLOW}⚠ WARNING: Could not detect Hamlib version${NC}"
    echo "  Please verify Hamlib is installed"
    HAS_WARNINGS=1
else
    echo "  Expected: $EXPECTED_HAMLIB_VERSION"
    echo "  Detected: $DETECTED_HAMLIB_VERSION"

    if [ "$DETECTED_HAMLIB_VERSION" = "$EXPECTED_HAMLIB_VERSION" ]; then
        echo -e "${GREEN}✓ Hamlib version matches expected version${NC}"
    else
        echo -e "${YELLOW}⚠ WARNING: Hamlib version mismatch${NC}"
        echo "  This may cause radio control compatibility issues"
        HAS_WARNINGS=1
    fi
fi

echo ""
echo "=== Summary ==="

if [ $HAS_ERRORS -gt 0 ]; then
    echo -e "${RED}✗ CRITICAL version mismatches detected${NC}"
    echo "Build may fail or produce incompatible binaries"
    exit 2
elif [ $HAS_WARNINGS -gt 0 ]; then
    echo -e "${YELLOW}⚠ Version warnings detected${NC}"
    echo "Build may succeed but could have subtle issues"
    exit 1
else
    echo -e "${GREEN}✓ All versions validated successfully${NC}"
    exit 0
fi
