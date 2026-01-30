#!/usr/bin/env python3
"""
TR4QT Headless Server Test Script

Tests the headless server API by:
1. Creating a new contest
2. Logging 10 QSOs
3. Exporting to ADIF and verifying the content

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
import time
from datetime import datetime, timezone

# Test data - 10 QSOs with varied callsigns
TEST_QSOS = [
    {"callsign": "W1AW", "exchange": "599 05"},
    {"callsign": "K3LR", "exchange": "599 03"},
    {"callsign": "N1MM", "exchange": "599 05"},
    {"callsign": "VE3EJ", "exchange": "599 04"},
    {"callsign": "DL1A", "exchange": "599 14"},
    {"callsign": "JA1YPA", "exchange": "599 25"},
    {"callsign": "ZL1BYZ", "exchange": "599 32"},
    {"callsign": "PY2SEX", "exchange": "599 11"},
    {"callsign": "VK3IO", "exchange": "599 30"},
    {"callsign": "G3PQA", "exchange": "599 14"},
]


def create_contest(base_url: str) -> dict:
    """Create a new CQ WW CW contest."""
    url = f"{base_url}/api/contest/create"
    data = {
        "contestType": "CQWW_CW",
        "callsign": "K1TEST",
        "exchangeSent": "599 05",
        "mode": "CW",
        "category": "SINGLE-OP",
        "powerClass": "HIGH"
    }

    print(f"\n1. Creating contest...")
    print(f"   POST {url}")
    print(f"   Data: {json.dumps(data)}")

    response = requests.post(url, json=data)
    result = response.json()

    if result.get("success"):
        print(f"   SUCCESS: Contest created")
        print(f"   - Contest ID: {result.get('contestDbId')}")
        print(f"   - Serial Number: {result.get('serialNumber')}")
        print(f"   - Database: {result.get('databasePath')}")
    else:
        print(f"   FAILED: {result.get('error')}")

    return result


def log_qso(base_url: str, callsign: str, exchange: str) -> dict:
    """Log a single QSO."""
    url = f"{base_url}/api/log-qso"
    data = {
        "callsign": callsign,
        "exchange": exchange
    }

    response = requests.post(url, json=data)
    return response.json()


def log_all_qsos(base_url: str) -> list:
    """Log all test QSOs."""
    print(f"\n2. Logging {len(TEST_QSOS)} QSOs...")

    results = []
    for i, qso in enumerate(TEST_QSOS, 1):
        result = log_qso(base_url, qso["callsign"], qso["exchange"])
        results.append(result)

        if result.get("success"):
            print(f"   [{i:2d}] {qso['callsign']:8s} - OK (pts={result.get('points', 0)}, mult={result.get('isMultiplier', False)})")
        else:
            print(f"   [{i:2d}] {qso['callsign']:8s} - FAILED: {result.get('error')}")

    return results


def get_contest_status(base_url: str) -> dict:
    """Get current contest status."""
    url = f"{base_url}/api/contest/status"

    print(f"\n3. Checking contest status...")
    print(f"   GET {url}")

    response = requests.get(url)
    result = response.json()

    if result.get("active"):
        print(f"   Contest: {result.get('contestName')}")
        print(f"   QSO Count: {result.get('qsoCount')}")
        print(f"   Points: {result.get('totalPoints')}")
        print(f"   Multipliers: {result.get('totalMultipliers')}")
        print(f"   Score: {result.get('score')}")
    else:
        print(f"   No active contest")

    return result


def export_adif(base_url: str, output_file: str = None) -> str:
    """Export QSOs to ADIF format."""
    url = f"{base_url}/api/export/adif"

    print(f"\n4. Exporting to ADIF...")
    print(f"   GET {url}")

    response = requests.get(url)
    adif_content = response.text

    # Count QSOs in ADIF (by counting <EOR> tags)
    qso_count = adif_content.count("<EOR>")
    print(f"   Received {qso_count} QSO records in ADIF format")

    if output_file:
        with open(output_file, 'w') as f:
            f.write(adif_content)
        print(f"   Saved to: {output_file}")

    return adif_content


def verify_adif(adif_content: str, expected_callsigns: list) -> bool:
    """Verify ADIF content contains expected callsigns."""
    print(f"\n5. Verifying ADIF content...")

    all_found = True
    for callsign in expected_callsigns:
        # Look for callsign in ADIF format: <CALL:n>callsign
        if f">{callsign}<" in adif_content or f">{callsign}\n" in adif_content or callsign in adif_content:
            print(f"   [OK] Found {callsign}")
        else:
            print(f"   [MISSING] {callsign} not found in ADIF")
            all_found = False

    return all_found


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
    parser.add_argument("--output", "-o", default="test_export.adi", help="ADIF output file (default: test_export.adi)")
    args = parser.parse_args()

    base_url = f"http://{args.host}:{args.port}"

    print("=" * 60)
    print("TR4QT Headless Server API Test")
    print("=" * 60)
    print(f"Server: {base_url}")
    print(f"Output: {args.output}")

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

        # 2. Log QSOs
        qso_results = log_all_qsos(base_url)
        successful_qsos = sum(1 for r in qso_results if r.get("success"))
        print(f"\n   Logged {successful_qsos}/{len(TEST_QSOS)} QSOs successfully")

        # 3. Check status
        status = get_contest_status(base_url)

        # 4. Export to ADIF
        adif_content = export_adif(base_url, args.output)

        # 5. Verify ADIF
        expected_callsigns = [qso["callsign"] for qso in TEST_QSOS]
        verified = verify_adif(adif_content, expected_callsigns)

        # 6. Close contest
        close_contest(base_url)

        # Summary
        print("\n" + "=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        print(f"QSOs logged: {successful_qsos}/{len(TEST_QSOS)}")
        print(f"ADIF export: {'PASS' if adif_content else 'FAIL'}")
        print(f"Verification: {'PASS' if verified else 'FAIL'}")

        if successful_qsos == len(TEST_QSOS) and verified:
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
