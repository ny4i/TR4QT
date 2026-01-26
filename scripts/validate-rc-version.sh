#!/bin/bash
#
# Validates tr4qt.rc version syntax to prevent the "dots instead of commas" error
# that broke Windows CI builds.
#
# The Windows resource file requires:
# - VER_FILEVERSION X,Y,Z,0 (commas, not dots)
# - VER_PRODUCTVERSION X,Y,Z,0 (commas, not dots)
# - VER_FILEVERSION_STR "X.Y.Z.0\0" (dots in string, with trailing \0)
# - VER_PRODUCTVERSION_STR "X.Y.Z\0" (dots in string, with trailing \0)

set -e

RC_FILE="resources/tr4qt.rc"

echo "=== Validating tr4qt.rc version syntax ==="

if [ ! -f "$RC_FILE" ]; then
    echo "ERROR: $RC_FILE not found"
    exit 1
fi

ERRORS=0

# Check VER_FILEVERSION format: should be X,Y,Z,0 (commas)
VER_FILEVERSION=$(grep '#define VER_FILEVERSION ' "$RC_FILE" | head -1)
if echo "$VER_FILEVERSION" | grep -qE '#define VER_FILEVERSION[[:space:]]+[0-9]+,[0-9]+,[0-9]+,[0-9]+'; then
    echo "✓ VER_FILEVERSION format correct (uses commas)"
else
    echo "✗ ERROR: VER_FILEVERSION has wrong format"
    echo "  Found: $VER_FILEVERSION"
    echo "  Expected format: #define VER_FILEVERSION X,Y,Z,0 (with commas)"
    ERRORS=$((ERRORS + 1))
fi

# Check VER_PRODUCTVERSION format: should be X,Y,Z,0 (commas)
VER_PRODUCTVERSION=$(grep '#define VER_PRODUCTVERSION ' "$RC_FILE" | head -1)
if echo "$VER_PRODUCTVERSION" | grep -qE '#define VER_PRODUCTVERSION[[:space:]]+[0-9]+,[0-9]+,[0-9]+,[0-9]+'; then
    echo "✓ VER_PRODUCTVERSION format correct (uses commas)"
else
    echo "✗ ERROR: VER_PRODUCTVERSION has wrong format"
    echo "  Found: $VER_PRODUCTVERSION"
    echo "  Expected format: #define VER_PRODUCTVERSION X,Y,Z,0 (with commas)"
    ERRORS=$((ERRORS + 1))
fi

# Check VER_FILEVERSION_STR format: should be "X.Y.Z.0\0" (dots in string)
VER_FILEVERSION_STR=$(grep '#define VER_FILEVERSION_STR' "$RC_FILE")
if echo "$VER_FILEVERSION_STR" | grep -qE '#define VER_FILEVERSION_STR[[:space:]]+"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+\\0"'; then
    echo "✓ VER_FILEVERSION_STR format correct"
else
    echo "✗ ERROR: VER_FILEVERSION_STR has wrong format"
    echo "  Found: $VER_FILEVERSION_STR"
    echo "  Expected format: #define VER_FILEVERSION_STR \"X.Y.Z.0\\0\""
    ERRORS=$((ERRORS + 1))
fi

# Check VER_PRODUCTVERSION_STR format: should be "X.Y.Z\0" (dots in string)
VER_PRODUCTVERSION_STR=$(grep '#define VER_PRODUCTVERSION_STR' "$RC_FILE")
if echo "$VER_PRODUCTVERSION_STR" | grep -qE '#define VER_PRODUCTVERSION_STR[[:space:]]+"[0-9]+\.[0-9]+\.[0-9]+\\0"'; then
    echo "✓ VER_PRODUCTVERSION_STR format correct"
else
    echo "✗ ERROR: VER_PRODUCTVERSION_STR has wrong format"
    echo "  Found: $VER_PRODUCTVERSION_STR"
    echo "  Expected format: #define VER_PRODUCTVERSION_STR \"X.Y.Z\\0\""
    ERRORS=$((ERRORS + 1))
fi

# Extract version numbers to check consistency
FILE_VER=$(echo "$VER_FILEVERSION" | grep -oE '[0-9]+,[0-9]+,[0-9]+' | head -1 | tr ',' '.')
PROD_VER=$(echo "$VER_PRODUCTVERSION" | grep -oE '[0-9]+,[0-9]+,[0-9]+' | head -1 | tr ',' '.')
FILE_STR_VER=$(echo "$VER_FILEVERSION_STR" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
PROD_STR_VER=$(echo "$VER_PRODUCTVERSION_STR" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)

echo ""
echo "=== Version consistency check ==="
echo "  VER_FILEVERSION:     $FILE_VER"
echo "  VER_PRODUCTVERSION:  $PROD_VER"
echo "  VER_FILEVERSION_STR: $FILE_STR_VER"
echo "  VER_PRODUCTVERSION_STR: $PROD_STR_VER"

if [ "$FILE_VER" != "$PROD_VER" ] || [ "$FILE_VER" != "$FILE_STR_VER" ] || [ "$FILE_VER" != "$PROD_STR_VER" ]; then
    echo "✗ ERROR: Version numbers are inconsistent"
    ERRORS=$((ERRORS + 1))
else
    echo "✓ All version numbers are consistent: $FILE_VER"
fi

echo ""
if [ $ERRORS -gt 0 ]; then
    echo "════════════════════════════════════════"
    echo "✗ FAILED: $ERRORS error(s) found in tr4qt.rc"
    echo ""
    echo "The Windows resource file syntax is strict:"
    echo "- VER_FILEVERSION must use COMMAS: 3,38,79,0"
    echo "- VER_PRODUCTVERSION must use COMMAS: 3,38,79,0"
    echo "- String versions use dots: \"3.38.79.0\\0\""
    echo ""
    echo "This check exists because using dots instead of commas"
    echo "causes cryptic rc.exe compilation errors on Windows."
    echo "════════════════════════════════════════"
    exit 1
fi

echo "✓ tr4qt.rc version syntax validated successfully"
