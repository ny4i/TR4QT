#!/bin/bash
# run_tests.sh — Run CTest with production settings protected
#
# Protection mechanism: TestSettingsRedirect.cpp (linked into every test binary)
# calls QSettings::setPath() via a C++ static constructor BEFORE main() runs,
# redirecting all QSettings NativeFormat writes to /tmp/tr4qt-test-settings/.
# This means tests never touch ~/Library/Preferences/com.tr4qt.TR4QT.plist.
#
# Why this approach (not chmod):
# macOS cfprefsd ignores filesystem permissions entirely. chmod a-w on the
# plist has no effect — cfprefsd writes through it. QSettings::setPath()
# is the only reliable way to redirect writes at the application level.
#
# This script cleans up the temp settings dir before/after tests.
#
# Usage:
#   ./tests/run_tests.sh                     # from project root
#   ./tests/run_tests.sh --output-on-failure # pass args to ctest

set -uo pipefail

TEST_SETTINGS_DIR="/tmp/tr4qt-test-settings"
BUILD_DIR="$(cd "$(dirname "$0")/../build" && pwd)"

cleanup() {
    if [ -d "$TEST_SETTINGS_DIR" ]; then
        rm -rf "$TEST_SETTINGS_DIR"
        echo "[guard] Cleaned up test settings: $TEST_SETTINGS_DIR"
    fi
}

# Clean up on exit (trap guarantees cleanup even on Ctrl-C or failure)
trap cleanup EXIT

# Start fresh — remove any stale test settings from previous runs
cleanup

# Run tests, forwarding all arguments to ctest
cd "$BUILD_DIR" && ctest "$@"
TEST_EXIT=$?

# cleanup happens automatically via trap
exit $TEST_EXIT
