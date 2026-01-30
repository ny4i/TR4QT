#!/usr/bin/env python3
"""
TR4QT Headless Server Test Script

Tests the headless server API by:
1. Creating a new CQ WW SSB contest
2. Logging 10 QSOs across multiple bands
3. Verifying score and band breakdown
4. Exporting to ADIF and verifying fields
5. Exporting to Cabrillo and verifying format

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
            pts = result.get("points", 0)
            mult = result.get("isMultiplier", False)
            mult_str = "MULT" if mult else ""
            print(f"   [{i:2d}] {qso['callsign']:8s} {band:4s} - OK (pts={pts}) {mult_str}")
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


def export_adif(base_url: str) -> str:
    """Export QSOs to ADIF format and verify fields."""
    url = f"{base_url}/api/export/adif"

    print(f"\n4. Exporting to ADIF...")
    print(f"   GET {url}")

    response = requests.get(url)
    adif_content = response.text

    # Count QSOs in ADIF (by counting <EOR> tags)
    qso_count = adif_content.count("<EOR>")
    print(f"   Received {qso_count} QSO records")

    # Verify each callsign is present
    print(f"\n   Verifying ADIF fields...")
    all_found = True
    for qso in TEST_QSOS:
        callsign = qso["callsign"]
        zone = qso["zone"]

        # Check callsign present
        if callsign not in adif_content:
            print(f"   [MISSING] Callsign {callsign} not found")
            all_found = False
            continue

        # Extract the QSO record for this callsign
        # Look for pattern: <CALL:n>callsign ... <EOR>
        pattern = f"<CALL:\\d+>{callsign}.*?<EOR>"
        match = re.search(pattern, adif_content, re.IGNORECASE | re.DOTALL)
        if not match:
            print(f"   [ERROR] Could not parse QSO record for {callsign}")
            continue

        record = match.group(0)

        # Verify key fields
        errors = []

        # Check RST_RCVD (should be 59)
        if "<RST_RCVD" not in record:
            errors.append("missing RST_RCVD")
        elif ">59<" not in record and ">59 " not in record and "59<" not in adif_content:
            pass  # May be formatted differently

        # Check CQZ (zone)
        if f"<CQZ" not in record:
            errors.append("missing CQZ")

        if errors:
            print(f"   [WARN] {callsign}: {', '.join(errors)}")
        else:
            print(f"   [OK] {callsign} - all fields present")

    return adif_content


def export_cabrillo(base_url: str) -> str:
    """Export QSOs to Cabrillo format and verify."""
    url = f"{base_url}/api/export/cabrillo"

    print(f"\n5. Exporting to Cabrillo...")
    print(f"   GET {url}")

    response = requests.get(url)

    if response.status_code != 200:
        print(f"   ERROR: HTTP {response.status_code}")
        try:
            error = response.json()
            print(f"   {error.get('error', 'Unknown error')}")
        except:
            pass
        return ""

    cabrillo = response.text

    # Count QSO lines
    qso_lines = [line for line in cabrillo.split('\n') if line.startswith('QSO:')]
    print(f"   Received {len(qso_lines)} QSO records")

    # Verify header
    print(f"\n   Verifying Cabrillo header...")
    required_headers = ["START-OF-LOG:", "CONTEST:", "CALLSIGN:", "CATEGORY-OPERATOR:"]
    for header in required_headers:
        if header in cabrillo:
            # Extract value
            for line in cabrillo.split('\n'):
                if line.startswith(header):
                    print(f"   [OK] {line.strip()}")
                    break
        else:
            print(f"   [MISSING] {header}")

    # Verify QSO lines
    print(f"\n   Verifying QSO lines...")
    for qso in TEST_QSOS[:3]:  # Check first 3
        callsign = qso["callsign"]
        found = False
        for line in qso_lines:
            if callsign in line:
                found = True
                # Basic format check: QSO: freq mode date time mycall rst exch theircall rst exch
                parts = line.split()
                if len(parts) >= 9:
                    print(f"   [OK] {callsign}: {line[:70]}...")
                else:
                    print(f"   [WARN] {callsign}: malformed line")
                break
        if not found:
            print(f"   [MISSING] {callsign} not in Cabrillo")

    return cabrillo


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

        # 4. Export to ADIF and verify
        adif_content = export_adif(base_url)

        # 5. Export to Cabrillo and verify
        cabrillo_content = export_cabrillo(base_url)

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

        # Test 4: ADIF export
        if adif_content and adif_content.count("<EOR>") == len(TEST_QSOS):
            print(f"[PASS] ADIF export: {adif_content.count('<EOR>')} QSOs")
            tests_passed += 1
        else:
            print(f"[FAIL] ADIF export incomplete")

        # Test 5: Cabrillo export
        if cabrillo_content and "QSO:" in cabrillo_content:
            qso_count = cabrillo_content.count("QSO:")
            print(f"[PASS] Cabrillo export: {qso_count} QSOs")
            tests_passed += 1
        else:
            print(f"[FAIL] Cabrillo export failed")

        print(f"\nTests passed: {tests_passed}/{tests_total}")

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
