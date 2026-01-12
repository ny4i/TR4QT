#!/bin/bash
# TR4QT Architecture Health Check
# Validates architectural constraints and logs violations

set -e

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Thresholds
YELLOW_THRESHOLD=1000
RED_THRESHOLD=1500

# Output files
VIOLATIONS_FILE=".claude/VIOLATIONS.md"
METRICS_FILE=".claude/METRICS.md"

# Counters
VIOLATIONS_COUNT=0
WARNINGS_COUNT=0

echo "🔍 TR4QT Architecture Health Check"
echo "=================================="
echo ""

# Initialize violations file if doesn't exist
if [ ! -f "$VIOLATIONS_FILE" ]; then
    mkdir -p .claude
    cat > "$VIOLATIONS_FILE" << 'EOF'
# TR4QT Architecture Violations Log

This file logs all architecture rule violations.

Format:
- Date/Time
- Violation type
- File and details
- Action taken (blocked, overridden, etc.)

---

EOF
fi

# Initialize metrics file if doesn't exist
if [ ! -f "$METRICS_FILE" ]; then
    mkdir -p .claude
    cat > "$METRICS_FILE" << 'EOF'
# TR4QT Architecture Metrics

Track architecture health over time.

Format: Date | MainWindow LOC | God Classes | Test Coverage | Notes

---

EOF
fi

# Check 1: God Class Detection (File Size)
echo "📏 Check 1: File Size Limits"
echo "   Thresholds: Yellow=$YELLOW_THRESHOLD, Red=$RED_THRESHOLD"
echo ""

GOD_CLASSES=()

# Check MainWindow specifically
if [ -f "src/ui/MainWindow.cpp" ]; then
    LINES=$(wc -l < "src/ui/MainWindow.cpp" | tr -d ' ')

    if [ $LINES -ge $RED_THRESHOLD ]; then
        OVERAGE=$((LINES - RED_THRESHOLD))
        PERCENT=$(awk "BEGIN {printf \"%.1f\", ($LINES / $RED_THRESHOLD) * 100 - 100}")

        echo -e "   ${RED}❌ VIOLATION${NC}: src/ui/MainWindow.cpp"
        echo "      Lines: $LINES (limit: $RED_THRESHOLD)"
        echo "      Overage: $OVERAGE lines (${PERCENT}% over)"
        echo ""

        # Log violation
        {
            echo "## Violation: $(date '+%Y-%m-%d %H:%M:%S')"
            echo "- Type: God Class (File Size)"
            echo "- File: src/ui/MainWindow.cpp"
            echo "- Lines: $LINES"
            echo "- Limit: $RED_THRESHOLD"
            echo "- Overage: $OVERAGE lines (${PERCENT}% over)"
            echo "- Status: ❌ RED"
            echo ""
        } >> "$VIOLATIONS_FILE"

        GOD_CLASSES+=("src/ui/MainWindow.cpp:$LINES")
        VIOLATIONS_COUNT=$((VIOLATIONS_COUNT + 1))

    elif [ $LINES -ge $YELLOW_THRESHOLD ]; then
        REMAINING=$((RED_THRESHOLD - LINES))

        echo -e "   ${YELLOW}⚠️  WARNING${NC}: src/ui/MainWindow.cpp"
        echo "      Lines: $LINES (threshold: $YELLOW_THRESHOLD)"
        echo "      Remaining headroom: $REMAINING lines"
        echo ""

        WARNINGS_COUNT=$((WARNINGS_COUNT + 1))
    else
        echo -e "   ${GREEN}✅ PASS${NC}: src/ui/MainWindow.cpp ($LINES lines)"
        echo ""
    fi
else
    echo "   ⚠️  MainWindow.cpp not found (skipping check)"
    echo ""
fi

# Check other large files
echo "   Checking other UI files..."
for file in src/ui/*.cpp; do
    if [ "$file" = "src/ui/MainWindow.cpp" ]; then
        continue # Already checked
    fi

    if [ -f "$file" ]; then
        LINES=$(wc -l < "$file" | tr -d ' ')

        if [ $LINES -ge $RED_THRESHOLD ]; then
            echo -e "   ${RED}❌ VIOLATION${NC}: $file ($LINES lines)"
            GOD_CLASSES+=("$file:$LINES")
            VIOLATIONS_COUNT=$((VIOLATIONS_COUNT + 1))
        elif [ $LINES -ge $YELLOW_THRESHOLD ]; then
            echo -e "   ${YELLOW}⚠️  WARNING${NC}: $file ($LINES lines)"
            WARNINGS_COUNT=$((WARNINGS_COUNT + 1))
        fi
    fi
done
echo ""

# Check 2: SQL in UI Classes
echo "📊 Check 2: SQL Queries in UI Classes"
echo ""

SQL_VIOLATIONS=$(grep -r "QSqlQuery\|db\.exec\|query\.exec" src/ui/ --include="*.cpp" --exclude="*Test.cpp" -n 2>/dev/null || true)

if [ -n "$SQL_VIOLATIONS" ]; then
    echo -e "   ${RED}❌ VIOLATION${NC}: SQL queries found in UI classes"
    echo ""
    echo "$SQL_VIOLATIONS" | head -10
    echo ""

    # Log violation
    {
        echo "## Violation: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "- Type: SQL in UI Class"
        echo "- Details: SQL queries found in src/ui/ directory"
        echo "- Lines:"
        echo "\`\`\`"
        echo "$SQL_VIOLATIONS" | head -10
        echo "\`\`\`"
        echo ""
    } >> "$VIOLATIONS_FILE"

    VIOLATIONS_COUNT=$((VIOLATIONS_COUNT + 1))
else
    echo -e "   ${GREEN}✅ PASS${NC}: No SQL in UI classes"
    echo ""
fi

# Check 3: Hardcoded Hex Colors (excluding ThemeManager)
echo "🎨 Check 3: Hardcoded Hex Colors"
echo ""

COLOR_VIOLATIONS=$(grep -r '= "#[0-9A-Fa-f]\{6\}' src/ --include="*.cpp" --exclude="ThemeManager.cpp" -n 2>/dev/null | grep -v "// Theme default" || true)

if [ -n "$COLOR_VIOLATIONS" ]; then
    echo -e "   ${RED}❌ VIOLATION${NC}: Hardcoded colors found (use ThemeManager)"
    echo ""
    echo "$COLOR_VIOLATIONS" | head -10
    echo ""

    # Log violation
    {
        echo "## Violation: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "- Type: Hardcoded Hex Colors"
        echo "- Details: Colors should use ThemeManager"
        echo "- Lines:"
        echo "\`\`\`"
        echo "$COLOR_VIOLATIONS" | head -10
        echo "\`\`\`"
        echo ""
    } >> "$VIOLATIONS_FILE"

    VIOLATIONS_COUNT=$((VIOLATIONS_COUNT + 1))
else
    echo -e "   ${GREEN}✅ PASS${NC}: No hardcoded colors (using ThemeManager)"
    echo ""
fi

# Check 4: setParent(nullptr) Calls
echo "🪟 Check 4: setParent(nullptr) Calls"
echo ""

SETPARENT_VIOLATIONS=$(grep -r 'setParent(nullptr)' src/ --include="*.cpp" -n 2>/dev/null || true)

if [ -n "$SETPARENT_VIOLATIONS" ]; then
    echo -e "   ${RED}❌ VIOLATION${NC}: setParent(nullptr) found (creates top-level windows)"
    echo ""
    echo "$SETPARENT_VIOLATIONS"
    echo ""

    # Log violation
    {
        echo "## Violation: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "- Type: setParent(nullptr)"
        echo "- Details: Creates unwanted top-level windows"
        echo "- Lines:"
        echo "\`\`\`"
        echo "$SETPARENT_VIOLATIONS"
        echo "\`\`\`"
        echo ""
    } >> "$VIOLATIONS_FILE"

    VIOLATIONS_COUNT=$((VIOLATIONS_COUNT + 1))
else
    echo -e "   ${GREEN}✅ PASS${NC}: No setParent(nullptr) calls"
    echo ""
fi

# Update metrics
MAINWINDOW_LOC=$(wc -l < "src/ui/MainWindow.cpp" 2>/dev/null | tr -d ' ' || echo "N/A")
GOD_CLASS_COUNT=${#GOD_CLASSES[@]}

{
    echo "$(date '+%Y-%m-%d') | $MAINWINDOW_LOC | $GOD_CLASS_COUNT | TBD | Automated check"
} >> "$METRICS_FILE"

# Summary
echo "=================================="
echo "📊 Summary"
echo "=================================="
echo ""
echo "   MainWindow: $MAINWINDOW_LOC lines"
echo "   God classes: $GOD_CLASS_COUNT"
echo "   Violations: $VIOLATIONS_COUNT"
echo "   Warnings: $WARNINGS_COUNT"
echo ""

if [ $VIOLATIONS_COUNT -gt 0 ]; then
    echo -e "${RED}❌ FAILED${NC}: $VIOLATIONS_COUNT architecture violations found"
    echo ""
    echo "See: .claude/VIOLATIONS.md for details"
    echo "See: .claude/CHECKPOINTS.md for resolution steps"
    echo ""
    exit 1
elif [ $WARNINGS_COUNT -gt 0 ]; then
    echo -e "${YELLOW}⚠️  WARNINGS${NC}: $WARNINGS_COUNT files approaching limits"
    echo ""
    echo "Consider refactoring soon to prevent future violations."
    echo ""
    exit 0
else
    echo -e "${GREEN}✅ PASSED${NC}: All architecture checks passed"
    echo ""
    exit 0
fi
