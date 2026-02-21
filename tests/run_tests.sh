#!/bin/bash
# run_tests.sh — Run CTest with production settings protected
#
# On macOS, test processes that link AppSettings.cpp will instantiate
# AppSettings::instance(), which reads/writes the production plist.
# The singleton destructor calls forceSync(), overwriting the plist
# with partial/default data — wiping user configuration.
#
# This script makes the plist read-only before tests run and restores
# it after, using a trap to guarantee cleanup even on test failure,
# SIGINT (Ctrl-C), or crash.
#
# After restoring permissions, kills cfprefsd to flush macOS's
# preferences cache so the app sees the original (unmodified) plist.
#
# Usage:
#   ./tests/run_tests.sh                     # from project root
#   ./tests/run_tests.sh --output-on-failure # pass args to ctest

set -uo pipefail

PLIST="$HOME/Library/Preferences/com.tr4qt.TR4QT.plist"
BUILD_DIR="$(cd "$(dirname "$0")/../build" && pwd)"

unlock() {
    if [ -f "$PLIST" ]; then
        chmod u+w "$PLIST" 2>/dev/null
        echo "[guard] Restored write permission: $PLIST"
        # Flush cfprefsd cache so the app reads the real (unmodified) file
        pkill cfprefsd 2>/dev/null && echo "[guard] Flushed cfprefsd cache"
    fi
}

# Guarantee unlock runs on exit, interrupt, or termination
trap unlock EXIT

# Lock
if [ -f "$PLIST" ]; then
    chmod a-w "$PLIST"
    echo "[guard] Production plist is now READ-ONLY"
else
    echo "[guard] No plist found at $PLIST (first run?)"
fi

# Run tests, forwarding all arguments to ctest
cd "$BUILD_DIR" && ctest "$@"
TEST_EXIT=$?

# unlock happens automatically via trap
exit $TEST_EXIT
