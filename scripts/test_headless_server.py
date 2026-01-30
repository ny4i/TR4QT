#!/usr/bin/env python3
"""
TR4QT Headless Server Test Script

Tests the headless server API by:
1. Creating a new CQ WW SSB contest
2. Logging 10 QSOs across multiple bands
3. Verifying score and band breakdown
4. Exporting to ADIF, saving to /tmp, reading back and verifying
5. Exporting to Cabrillo, saving to /tmp, reading back and verifying

Usage:
    # Start the server first:
    ./build/src/tr4qt_server --port 14142

    # Then run this script:
    python3 scripts/test_headless_server.py

    # Or specify a different port:
    python3 scripts/test_headless_server.py --port 14143
"""

import argparse
import json
import os
import requests
import sys
import re
from datetime import datetime, timezone

# Test data - 10 QSOs across multiple bands with varied callsigns
# Format: callsign, exchange (RST + zone), band, expected_points
TEST_QSOS = [
    # 20M QSOs (3 contacts)
    {"callsign": "W1AW", "exchange": "59 05", "band": "20M", "freq": 14200000, "zone": 5},
    {"callsign": "K3LR", "exchange": "59 03", "band": "20M", "freq": 14250000, "zone": 3},
    {"callsign": "N1MM", "exchange": "59 05", "band": "20M", "freq": 14300000, "zone": 5},
    # 40M QSOs (3 contacts)
    {"callsign": "VE3EJ", "exchange": "59 04", "band": "40M", "freq": 7200000, "zone": 4},
    {"callsign": "DL1A", "exchange": "59 14", "band": "40M", "freq": 7250000, "zone": 14},
    {"callsign": "JA1YPA", "exchange": "59 25", "band": "40M", "freq": 7150000, "zone": 25},
    # 80M QSOs (2 contacts)
    {"callsign": "ZL1BYZ", "exchange": "59 32", "band": "80M", "freq": 3750000, "zone": 32},
    {"callsign": "PY2SEX", "exchange": "59 11", "band": "80M", "freq": 3800000, "zone": 11},
    # 15M QSOs (2 contacts)
    {"callsign": "VK3IO", "exchange": "59 30", "band": "15M", "freq": 21300000, "zone": 30},
    {"callsign": "G3PQA", "exchange": "59 14", "band": "15M", "freq": 21350000, "zone": 14},
]

# Expected band breakdown after logging all QSOs
EXPECTED_BAND_COUNTS = {
    "20M": 3,
    "40M": 3,
    "80M": 2,
    "15M": 2,
}

# Export directory
EXPORT_DIR = "/tmp"


def create_contest(base_url: str) -> dict:
    """Create a new CQ WW SSB contest."""
    url = f"{base_url}/api/contest/create"
    data = {
        "contestType": "CQWW_SSB",
        "callsign": "K1TEST",
        "exchangeSent": "59 05",
        "mode": "SSB",
        "category": "SINGLE-OP",
        "powerClass": "HIGH"
    }

    print(f"\n1. Creating CQ WW SSB contest...")
    print(f"   POST {url}")
    print(f"   Callsign: K1TEST, Exchange: 59 05")

    response = requests.post(url, json=data)
    result = response.json()

    if result.get("success"):
        print(f"   SUCCESS: Contest created")
        print(f"   - Contest ID: {result.get('contestDbId')}")
        print(f"   - Database: {result.get('databasePath')}")
    else:
        print(f"   FAILED: {result.get('error')}")

    return result


def log_qso(base_url: str, qso_data: dict) -> dict:
    """Log a single QSO with frequency."""
    url = f"{base_url}/api/log-qso"
    data = {
        "callsign": qso_data["callsign"],
        "exchange": qso_data["exchange"],
        "frequency": qso_data["freq"]
    }

    response = requests.post(url, json=data)
    return response.json()


def log_all_qsos(base_url: str) -> list:
    """Log all test QSOs across multiple bands."""
    print(f"\n2. Logging {len(TEST_QSOS)} QSOs across bands...")

    results = []
    for i, qso in enumerate(TEST_QSOS, 1):
        result = log_qso(base_url, qso)
        results.append(result)

        if result.get("success"):
            band = result.get("band", qso["band"])
            mode = result.get("mode", "?")
            pts = result.get("points", 0)
            mult = result.get("isMultiplier", False)
            mult_str = "MULT" if mult else ""
            print(f"   [{i:2d}] {qso['callsign']:8s} {band:4s} {mode:3s} - OK (pts={pts}) {mult_str}")
        else:
            print(f"   [{i:2d}] {qso['callsign']:8s} - FAILED: {result.get('error')}")

    successful = sum(1 for r in results if r.get("success"))
    print(f"\n   Logged {successful}/{len(TEST_QSOS)} QSOs successfully")
    return results


def check_score(base_url: str) -> dict:
    """Get and verify score with band breakdown."""
    url = f"{base_url}/api/contest/score"

    print(f"\n3. Checking score and band breakdown...")
    print(f"   GET {url}")

    response = requests.get(url)
    result = response.json()

    if not result.get("active"):
        print(f"   ERROR: No active contest")
        return result

    print(f"   Contest: {result.get('contestName')}")
    print(f"   Total QSOs: {result.get('totalQsos')}")
    print(f"   Total Points: {result.get('totalPoints')}")
    print(f"   Total Multipliers: {result.get('totalMultipliers')}")
    print(f"   Score: {result.get('score')}")

    # Verify band breakdown
    print(f"\n   Band Breakdown:")
    breakdown = result.get("bandBreakdown", [])
    band_counts = {}
    for entry in breakdown:
        band = entry.get("band", "?")
        mode = entry.get("mode", "?")
        qsos = entry.get("qsos", 0)
        pts = entry.get("points", 0)
        mults = entry.get("multipliers", 0)
        band_counts[band] = qsos
        print(f"     {band:4s} {mode:4s}: {qsos:3d} QSOs, {pts:4d} pts, {mults:2d} mults")

    # Verify expected band counts
    print(f"\n   Verifying band counts...")
    all_correct = True
    for band, expected in EXPECTED_BAND_COUNTS.items():
        actual = band_counts.get(band, 0)
        status = "OK" if actual == expected else "MISMATCH"
        if actual != expected:
            all_correct = False
        print(f"     {band}: expected {expected}, got {actual} - {status}")

    return result


def export_and_save_adif(base_url: str) -> tuple[str, str]:
    """Export QSOs to ADIF, save to /tmp, return (filepath, content)."""
    url = f"{base_url}/api/export/adif"

    print(f"\n4. Exporting to ADIF...")
    print(f"   GET {url}")

    response = requests.get(url)
    result = response.json()

    if not result.get("success"):
        print(f"   ERROR: {result.get('error', 'Unknown error')}")
        return "", ""

    filename = result.get("filename", "export.adi")
    content = result.get("content", "")
    filepath = os.path.join(EXPORT_DIR, filename)

    # Save to file
    with open(filepath, "w") as f:
        f.write(content)

    print(f"   Saved to: {filepath}")
    print(f"   Size: {len(content)} bytes")

    return filepath, content


def validate_adif_from_file(filepath: str) -> bool:
    """Read ADIF file from disk and validate contents."""
    print(f"\n   Validating ADIF from file: {filepath}")

    if not os.path.exists(filepath):
        print(f"   ERROR: File not found")
        return False

    with open(filepath, "r") as f:
        adif_content = f.read()

    # Count QSOs
    qso_count = adif_content.count("<EOR>")
    print(f"   Found {qso_count} QSO records in file")

    if qso_count != len(TEST_QSOS):
        print(f"   ERROR: Expected {len(TEST_QSOS)} QSOs, found {qso_count}")
        return False

    # Verify each callsign and key fields
    all_valid = True
    for qso in TEST_QSOS:
        callsign = qso["callsign"]
        zone = qso["zone"]
        band = qso["band"].lower()  # ADIF uses lowercase bands

        # Extract the QSO record for this callsign
        pattern = f"<CALL:\\d+>{callsign}.*?<EOR>"
        match = re.search(pattern, adif_content, re.IGNORECASE | re.DOTALL)

        if not match:
            print(f"   [FAIL] {callsign}: not found in file")
            all_valid = False
            continue

        record = match.group(0)
        errors = []

        # Check MODE is SSB (not CW)
        if "<MODE:3>SSB" not in record and "<MODE:2>PH" not in record:
            if "<MODE:2>CW" in record:
                errors.append("wrong mode (CW instead of SSB)")
            else:
                errors.append("missing MODE")

        # Check BAND
        band_pattern = f"<BAND:\\d+>{band}"
        if not re.search(band_pattern, record, re.IGNORECASE):
            errors.append(f"wrong band (expected {band})")

        # Check CQZ
        if "<CQZ:" not in record:
            errors.append("missing CQZ")

        # Check RST_RCVD
        if "<RST_RCVD:" not in record:
            errors.append("missing RST_RCVD")

        if errors:
            print(f"   [FAIL] {callsign}: {', '.join(errors)}")
            all_valid = False
        else:
            print(f"   [OK] {callsign}: all fields valid")

    return all_valid


def export_and_save_cabrillo(base_url: str) -> tuple[str, str]:
    """Export QSOs to Cabrillo, save to /tmp, return (filepath, content)."""
    url = f"{base_url}/api/export/cabrillo"

    print(f"\n5. Exporting to Cabrillo...")
    print(f"   GET {url}")

    response = requests.get(url)
    result = response.json()

    if not result.get("success"):
        print(f"   ERROR: {result.get('error', 'Unknown error')}")
        return "", ""

    filename = result.get("filename", "export.cbr")
    content = result.get("content", "")
    filepath = os.path.join(EXPORT_DIR, filename)

    # Save to file
    with open(filepath, "w") as f:
        f.write(content)

    print(f"   Saved to: {filepath}")
    print(f"   Size: {len(content)} bytes")

    return filepath, content


def validate_cabrillo_from_file(filepath: str) -> bool:
    """Read Cabrillo file from disk and validate contents."""
    print(f"\n   Validating Cabrillo from file: {filepath}")

    if not os.path.exists(filepath):
        print(f"   ERROR: File not found")
        return False

    with open(filepath, "r") as f:
        cabrillo = f.read()

    lines = cabrillo.split('\n')
    qso_lines = [line for line in lines if line.startswith('QSO:')]
    print(f"   Found {len(qso_lines)} QSO lines in file")

    if len(qso_lines) != len(TEST_QSOS):
        print(f"   ERROR: Expected {len(TEST_QSOS)} QSOs, found {len(qso_lines)}")
        return False

    # Verify header fields
    all_valid = True
    required_headers = {
        "START-OF-LOG:": None,
        "CONTEST:": "CQ-WW-SSB",
        "CALLSIGN:": "K1TEST",
        "CATEGORY-MODE:": "SSB",
        "CATEGORY-OPERATOR:": "SINGLE-OP",
        "CATEGORY-POWER:": "HIGH",
    }

    print(f"\n   Checking headers...")
    for header, expected_value in required_headers.items():
        found = False
        for line in lines:
            if line.startswith(header):
                found = True
                if expected_value:
                    actual_value = line.split(header, 1)[1].strip()
                    if actual_value != expected_value:
                        print(f"   [FAIL] {header} expected '{expected_value}', got '{actual_value}'")
                        all_valid = False
                    else:
                        print(f"   [OK] {line.strip()}")
                else:
                    print(f"   [OK] {line.strip()}")
                break
        if not found:
            print(f"   [FAIL] Missing header: {header}")
            all_valid = False

    # Verify QSO lines have correct mode (PH for SSB)
    print(f"\n   Checking QSO lines...")
    for qso in TEST_QSOS:
        callsign = qso["callsign"]
        found = False
        for line in qso_lines:
            if callsign in line:
                found = True
                # Check mode is PH (phone), not CW
                parts = line.split()
                if len(parts) >= 3:
                    mode = parts[2]  # QSO: freq mode ...
                    if mode != "PH":
                        print(f"   [FAIL] {callsign}: wrong mode '{mode}' (expected 'PH')")
                        all_valid = False
                    else:
                        print(f"   [OK] {callsign}: mode=PH, line valid")
                else:
                    print(f"   [FAIL] {callsign}: malformed QSO line")
                    all_valid = False
                break
        if not found:
            print(f"   [FAIL] {callsign}: not found in Cabrillo")
            all_valid = False

    return all_valid


def close_contest(base_url: str) -> dict:
    """Close the active contest."""
    url = f"{base_url}/api/contest/close"

    print(f"\n6. Closing contest...")
    print(f"   POST {url}")

    response = requests.post(url)
    result = response.json()

    if result.get("success"):
        print(f"   Contest closed successfully")
    else:
        print(f"   Note: {result.get('error', 'No error message')}")

    return result


def main():
    parser = argparse.ArgumentParser(description="Test TR4QT headless server API")
    parser.add_argument("--host", default="localhost", help="Server host (default: localhost)")
    parser.add_argument("--port", type=int, default=14142, help="Server port (default: 14142)")
    args = parser.parse_args()

    base_url = f"http://{args.host}:{args.port}"

    print("=" * 70)
    print("TR4QT Headless Server API Test - Multi-Band CQ WW SSB")
    print("=" * 70)
    print(f"Server: {base_url}")
    print(f"Export directory: {EXPORT_DIR}")

    # Check server is running
    try:
        response = requests.get(f"{base_url}/api/contest/status", timeout=5)
    except requests.exceptions.ConnectionError:
        print(f"\nERROR: Cannot connect to server at {base_url}")
        print("Make sure the server is running:")
        print(f"  ./build/src/tr4qt_server --port {args.port}")
        return 1

    # Run test sequence
    try:
        # 1. Create contest
        create_result = create_contest(base_url)
        if not create_result.get("success"):
            print("\nFailed to create contest. Aborting.")
            return 1

        # 2. Log QSOs across bands
        qso_results = log_all_qsos(base_url)
        successful_qsos = sum(1 for r in qso_results if r.get("success"))

        # 3. Check score and band breakdown
        score_result = check_score(base_url)

        # 4. Export ADIF, save to file, read back and validate
        adif_filepath, adif_content = export_and_save_adif(base_url)
        adif_valid = validate_adif_from_file(adif_filepath) if adif_filepath else False

        # 5. Export Cabrillo, save to file, read back and validate
        cabrillo_filepath, cabrillo_content = export_and_save_cabrillo(base_url)
        cabrillo_valid = validate_cabrillo_from_file(cabrillo_filepath) if cabrillo_filepath else False

        # 6. Close contest
        close_contest(base_url)

        # Summary
        print("\n" + "=" * 70)
        print("TEST SUMMARY")
        print("=" * 70)

        tests_passed = 0
        tests_total = 5

        # Test 1: QSOs logged
        if successful_qsos == len(TEST_QSOS):
            print(f"[PASS] QSOs logged: {successful_qsos}/{len(TEST_QSOS)}")
            tests_passed += 1
        else:
            print(f"[FAIL] QSOs logged: {successful_qsos}/{len(TEST_QSOS)}")

        # Test 2: Score calculated
        if score_result.get("score", 0) > 0:
            print(f"[PASS] Score calculated: {score_result.get('score')}")
            tests_passed += 1
        else:
            print(f"[FAIL] Score not calculated")

        # Test 3: Band breakdown correct
        breakdown = score_result.get("bandBreakdown", [])
        band_counts = {e.get("band"): e.get("qsos") for e in breakdown}
        breakdown_correct = all(
            band_counts.get(band) == count
            for band, count in EXPECTED_BAND_COUNTS.items()
        )
        if breakdown_correct:
            print(f"[PASS] Band breakdown correct")
            tests_passed += 1
        else:
            print(f"[FAIL] Band breakdown incorrect")

        # Test 4: ADIF export and validation
        if adif_valid:
            print(f"[PASS] ADIF export: saved and validated from {adif_filepath}")
            tests_passed += 1
        else:
            print(f"[FAIL] ADIF export/validation failed")

        # Test 5: Cabrillo export and validation
        if cabrillo_valid:
            print(f"[PASS] Cabrillo export: saved and validated from {cabrillo_filepath}")
            tests_passed += 1
        else:
            print(f"[FAIL] Cabrillo export/validation failed")

        print(f"\nTests passed: {tests_passed}/{tests_total}")

        # Show export file locations
        print(f"\nExport files:")
        if adif_filepath:
            print(f"  ADIF:     {adif_filepath}")
        if cabrillo_filepath:
            print(f"  Cabrillo: {cabrillo_filepath}")

        if tests_passed == tests_total:
            print("\nAll tests PASSED!")
            return 0
        else:
            print("\nSome tests FAILED")
            return 1

    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
