#!/bin/bash
# TR4QT Qt Module Validation Script
# Ensures Qt modules are consistently declared across:
#   1. CMakeLists.txt (find_package)
#   2. .github/workflows/build.yml (deployment)
#   3. scripts/verify-deployment.sh (verification)
#
# Usage:
#   ./scripts/validate-qt-modules.sh
#
# Exit codes:
#   0 = All modules consistent
#   1 = Inconsistencies found

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

ERRORS=0

echo "======================================"
echo "TR4QT Qt Module Validation"
echo "======================================"
echo ""

# Base Qt modules (always installed, don't need explicit deployment)
BASE_MODULES=(
    "Core"
    "Gui"
    "Widgets"
    "Network"
    "Sql"
    "PrintSupport"
    "Concurrent"
    "Test"
    "OpenGL"
    "Xml"
)

# Extract Qt modules from CMakeLists.txt
echo -e "${BLUE}1. Extracting modules from CMakeLists.txt...${NC}"
# Use awk to extract only lines between "COMPONENTS" and closing ")"
CMAKE_MODULES=$(awk '
    /find_package.*Qt6.*COMPONENTS/ { in_components=1; next }
    in_components && /^[[:space:]]*\)/ { exit }
    in_components && NF > 0 {
        gsub(/^[[:space:]]+/, "");  # Remove leading whitespace
        gsub(/#.*/, "");             # Remove comments
        if (length($0) > 0) print $0
    }
' CMakeLists.txt | sort)

if [ -z "$CMAKE_MODULES" ]; then
    echo -e "${RED}ERROR: Could not extract Qt modules from CMakeLists.txt${NC}"
    exit 1
fi

echo -e "${GREEN}Found modules in CMakeLists.txt:${NC}"
for module in $CMAKE_MODULES; do
    echo "  - $module"
done
echo ""

# Check if module is a base module (doesn't need explicit deployment)
is_base_module() {
    local module="$1"
    for base in "${BASE_MODULES[@]}"; do
        if [ "$base" = "$module" ]; then
            return 0
        fi
    done
    return 1
}

# Extract deployed Qt DLLs from build.yml
echo -e "${BLUE}2. Checking Windows deployment in .github/workflows/build.yml...${NC}"
DEPLOYED_DLLS=$(grep "cp.*Qt6.*\.dll.*DEST" .github/workflows/build.yml | \
    grep -v "^[[:space:]]*#" | \
    sed 's/.*Qt6/Qt6/' | \
    sed 's/\.dll.*//' | \
    sed 's/Qt6//' | \
    sort | \
    uniq)

echo -e "${GREEN}Deployed DLLs in build.yml:${NC}"
for dll in $DEPLOYED_DLLS; do
    echo "  - Qt6${dll}.dll"
done
echo ""

# Extract verified modules from verify-deployment.sh
echo -e "${BLUE}3. Checking verification in scripts/verify-deployment.sh...${NC}"
VERIFIED_MODULES=$(grep 'check_file "Qt6.*\.dll"' scripts/verify-deployment.sh | \
    sed 's/.*"Qt6//' | \
    sed 's/\.dll".*//' | \
    sort | \
    uniq)

echo -e "${GREEN}Verified modules in verify-deployment.sh:${NC}"
for module in $VERIFIED_MODULES; do
    echo "  - Qt6${module}.dll"
done
echo ""

# Validate: Each CMake module should be deployed (unless it's a base module)
echo "======================================"
echo "Validation Results"
echo "======================================"
echo ""

echo -e "${BLUE}Checking CMakeLists.txt → Deployment consistency...${NC}"
for module in $CMAKE_MODULES; do
    if is_base_module "$module"; then
        echo -e "  ${GREEN}✓${NC} $module (base module, no explicit deployment needed)"
        continue
    fi

    # Check if deployed
    if ! echo "$DEPLOYED_DLLS" | grep -q "^${module}$"; then
        echo -e "  ${RED}✗ ERROR${NC}: $module declared in CMakeLists.txt but NOT deployed in build.yml"
        echo -e "    ${YELLOW}Fix:${NC} Add to .github/workflows/build.yml:"
        echo -e "    ${YELLOW}     cp \"\$QT_BIN/Qt6${module}.dll\" \"\$DEST/\"${NC}"
        ERRORS=$((ERRORS + 1))
    else
        echo -e "  ${GREEN}✓${NC} $module is deployed"
    fi

    # Check if verified
    if ! echo "$VERIFIED_MODULES" | grep -q "^${module}$"; then
        echo -e "  ${RED}✗ ERROR${NC}: $module declared in CMakeLists.txt but NOT verified in verify-deployment.sh"
        echo -e "    ${YELLOW}Fix:${NC} Add to scripts/verify-deployment.sh:"
        echo -e "    ${YELLOW}     check_file \"Qt6${module}.dll\" \"Qt ${module}\" true${NC}"
        ERRORS=$((ERRORS + 1))
    else
        echo -e "  ${GREEN}✓${NC} $module is verified"
    fi
done
echo ""

# Check for deployed modules not in CMakeLists.txt (might be stale)
echo -e "${BLUE}Checking for stale deployments...${NC}"
for dll in $DEPLOYED_DLLS; do
    if ! echo "$CMAKE_MODULES" | grep -q "^${dll}$"; then
        if is_base_module "$dll"; then
            # Base module - okay to deploy explicitly even if not needed
            echo -e "  ${GREEN}✓${NC} Qt6${dll}.dll (base module, explicit deployment is fine)"
        else
            echo -e "  ${YELLOW}⚠ WARNING${NC}: Qt6${dll}.dll deployed but NOT in CMakeLists.txt"
            echo -e "    May be stale. Consider removing from .github/workflows/build.yml"
        fi
    fi
done
echo ""

# Check for verified modules not in CMakeLists.txt (might be stale)
echo -e "${BLUE}Checking for stale verifications...${NC}"
for module in $VERIFIED_MODULES; do
    if ! echo "$CMAKE_MODULES" | grep -q "^${module}$"; then
        if is_base_module "$module"; then
            # Base module - okay to verify
            echo -e "  ${GREEN}✓${NC} Qt6${module}.dll (base module)"
        else
            echo -e "  ${YELLOW}⚠ WARNING${NC}: Qt6${module}.dll verified but NOT in CMakeLists.txt"
            echo -e "    May be stale. Consider removing from scripts/verify-deployment.sh"
        fi
    fi
done
echo ""

# Final summary
echo "======================================"
echo "Summary"
echo "======================================"
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ ALL CHECKS PASSED${NC}"
    echo "Qt modules are consistent across CMakeLists.txt, deployment, and verification."
    exit 0
else
    echo -e "${RED}✗ ${ERRORS} ERROR(S) FOUND${NC}"
    echo ""
    echo "Please fix the errors above to ensure consistent Qt module deployment."
    echo ""
    echo "Quick reference:"
    echo "  1. CMakeLists.txt = Source of truth (what the app needs)"
    echo "  2. build.yml = Must deploy all non-base modules"
    echo "  3. verify-deployment.sh = Must verify all deployed modules"
    exit 1
fi
