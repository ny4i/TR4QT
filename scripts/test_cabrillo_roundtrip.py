#!/usr/bin/env python3
"""
Test Cabrillo import/export round-trip via TR4QT headless server API.

This script:
1. Parses a Cabrillo file to extract QSOs
2. Creates a new contest database via headless server API
3. Inserts each QSO via the API
4. Exports to Cabrillo format
5. Compares the QSO sections between original and exported files

Usage:
    ./scripts/test_cabrillo_roundtrip.py [cabrillo_file]

    Default: tests/fixtures/cabrillo/sample_cqww_ssb.cbr
"""

import sys
import os
import re
import json
import time
import subprocess
import tempfile
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Tuple

# Configuration
SERVER_PORT = 14142  # Use different port to avoid conflicts
SERVER_HOST = "127.0.0.1"
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"

# Paths
SCRIPT_DIR = Path(__file__).parent
PROJECT_DIR = SCRIPT_DIR.parent
BUILD_DIR = PROJECT_DIR / "build"
SERVER_BINARY = BUILD_DIR / "src" / "tr4qt_server"
DEFAULT_CABRILLO = PROJECT_DIR / "tests" / "ne4c.log"


@dataclass
class CabrilloQSO:
    """Represents a QSO from a Cabrillo file."""
    frequency: int      # kHz
    mode: str           # PH, CW, RY
    date: str           # YYYY-MM-DD
    time: str           # HHMM
    sent_call: str
    sent_rst: str
    sent_exch: str
    rcvd_call: str
    rcvd_rst: str
    rcvd_exch: str

    def to_api_request(self, contest_mode: str) -> dict:
        """Convert to API request format for log-qso endpoint."""
        # Map Cabrillo mode to TR4QT mode
        mode_map = {"PH": "SSB", "CW": "CW", "RY": "RTTY"}
        mode = mode_map.get(self.mode, contest_mode)

        # Parse datetime
        datetime_str = f"{self.date}T{self.time[:2]}:{self.time[2:]}:00Z"

        return {
            "callsign": self.rcvd_call,
            "exchange": f"{self.rcvd_rst} {self.rcvd_exch}",
            "frequency": self.frequency * 1000,  # Convert kHz to Hz
            "mode": mode,
            "datetime": datetime_str
        }


@dataclass
class CabrilloHeader:
    """Represents Cabrillo header fields."""
    contest: str = ""
    callsign: str = ""
    location: str = ""
    category_band: str = "ALL"
    category_mode: str = "SSB"
    category_operator: str = "SINGLE-OP"
    category_power: str = "HIGH"
    claimed_score: int = 0
    exchange_sent: str = ""


def parse_cabrillo(filepath: Path) -> Tuple[CabrilloHeader, List[CabrilloQSO]]:
    """Parse a Cabrillo file and extract header and QSOs."""
    header = CabrilloHeader()
    qsos = []

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            if line.startswith("CONTEST:"):
                header.contest = line.split(":", 1)[1].strip()
            elif line.startswith("CALLSIGN:"):
                header.callsign = line.split(":", 1)[1].strip()
            elif line.startswith("LOCATION:"):
                header.location = line.split(":", 1)[1].strip()
            elif line.startswith("CATEGORY-BAND:"):
                header.category_band = line.split(":", 1)[1].strip()
            elif line.startswith("CATEGORY-MODE:"):
                header.category_mode = line.split(":", 1)[1].strip()
            elif line.startswith("CATEGORY-OPERATOR:"):
                header.category_operator = line.split(":", 1)[1].strip()
            elif line.startswith("CATEGORY-POWER:"):
                header.category_power = line.split(":", 1)[1].strip()
            elif line.startswith("CLAIMED-SCORE:"):
                try:
                    header.claimed_score = int(line.split(":", 1)[1].strip())
                except ValueError:
                    pass
            elif line.startswith("QSO:"):
                qso = parse_qso_line(line, header.contest)
                if qso:
                    # Capture exchange sent from first QSO
                    if not header.exchange_sent:
                        if "ARRL-SS" in header.contest:
                            # SS: extract only PREC CHECK SECTION (not serial)
                            # sent_exch = "1 M 62 WWA" -> exchange_sent = "M 62 WWA"
                            parts = qso.sent_exch.split()
                            if len(parts) >= 4:
                                header.exchange_sent = " ".join(parts[1:])  # Skip serial
                            else:
                                header.exchange_sent = qso.sent_exch
                        elif qso.sent_rst:
                            header.exchange_sent = f"{qso.sent_rst} {qso.sent_exch}"
                        else:
                            header.exchange_sent = qso.sent_exch
                    qsos.append(qso)

    return header, qsos


def parse_qso_line(line: str, contest: str = "") -> Optional[CabrilloQSO]:
    """Parse a Cabrillo QSO line.

    Handles multiple formats:
    - CQ WW: QSO: freq mode date time sent_call sent_rst sent_zone rcvd_call rcvd_rst rcvd_zone
    - CQ WPX: QSO: freq mode date time sent_call sent_rst sent_serial rcvd_call rcvd_rst rcvd_serial [tx#]
    - ARRL SS: QSO: freq mode date time sent_call NR PREC CK SEC rcvd_call NR PREC CK SEC

    Example CQ WW:  QSO: 14250 PH 2025-10-25 1201 NE4C         59 05     W1AW          59 05
    Example CQ WPX: QSO: 14000 PH 2025-03-29 1723 NE4C         59 001    KC3MIO        59 001    1
    Example ARRL SS: QSO: 28500 PH 2024-11-16 2100 K7RI 0001 M 62 WWA NT5V 0001 U 69 NTX
    """
    # Remove "QSO:" prefix and split by whitespace
    parts = line[4:].split()

    try:
        # ARRL Sweepstakes has 14 fields (4 exchange fields each side)
        if "ARRL-SS" in contest and len(parts) >= 14:
            return CabrilloQSO(
                frequency=int(parts[0]),
                mode=parts[1],
                date=parts[2],
                time=parts[3],
                sent_call=parts[4],
                sent_rst="",  # SS doesn't use RST
                sent_exch=f"{parts[5]} {parts[6]} {parts[7]} {parts[8]}",  # NR PREC CK SEC
                rcvd_call=parts[9],
                rcvd_rst="",  # SS doesn't use RST
                rcvd_exch=f"{parts[10]} {parts[11]} {parts[12]} {parts[13]}"  # NR PREC CK SEC
            )

        # Standard format (CQ WW, CQ WPX, ARRL DX, etc.) - 10 fields minimum
        if len(parts) < 10:
            print(f"Warning: Skipping malformed QSO line: {line}")
            return None

        return CabrilloQSO(
            frequency=int(parts[0]),
            mode=parts[1],
            date=parts[2],
            time=parts[3],
            sent_call=parts[4],
            sent_rst=parts[5],
            sent_exch=parts[6],
            rcvd_call=parts[7],
            rcvd_rst=parts[8],
            rcvd_exch=parts[9]
            # parts[10] = TX# (ignored for CQ WPX)
        )
    except (ValueError, IndexError) as e:
        print(f"Warning: Error parsing QSO line: {line} - {e}")
        return None


def start_server(db_path: Path) -> subprocess.Popen:
    """Start the headless server with a specific database path."""
    if not SERVER_BINARY.exists():
        raise FileNotFoundError(f"Server binary not found: {SERVER_BINARY}")

    # Kill any existing server on this port
    subprocess.run(["pkill", "-9", "tr4qt_server"], capture_output=True)
    time.sleep(0.5)

    # Start server with output redirected to /dev/null to prevent buffer blocking
    # (CRITICAL: using PIPE without reading causes server to block after ~48 requests)
    devnull = open(os.devnull, 'w')
    proc = subprocess.Popen(
        [str(SERVER_BINARY), "--port", str(SERVER_PORT), "--address", SERVER_HOST],
        stdout=devnull,
        stderr=devnull
    )

    # Wait for server to start
    for _ in range(50):
        time.sleep(0.1)
        try:
            result = subprocess.run(
                ["curl", "-s", "-m", "1", f"{BASE_URL}/api/contest/status"],
                capture_output=True,
                timeout=2
            )
            if result.returncode == 0:
                return proc
        except subprocess.TimeoutExpired:
            continue

    proc.kill()
    raise RuntimeError("Server failed to start within timeout")


def stop_server(proc: subprocess.Popen):
    """Stop the headless server."""
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def create_contest(header: CabrilloHeader) -> dict:
    """Create a contest via the API."""
    # Map Cabrillo contest ID to TR4QT contest type
    contest_map = {
        "CQ-WW-SSB": "CQWW_SSB",
        "CQ-WW-CW": "CQWW_CW",
        "CQ-WPX-SSB": "CQWPX_SSB",
        "CQ-WPX-CW": "CQWPX_CW",
        "ARRL-DX-SSB": "ARRL_DX_SSB",
        "ARRL-DX-CW": "ARRL_DX_CW",
        "ARRL-SS-SSB": "ARRL_SS_SSB",
        "ARRL-SS-CW": "ARRL_SS_CW",
        "WFD": "WFD"
    }

    contest_type = contest_map.get(header.contest, "CQWW_SSB")

    payload = {
        "contestType": contest_type,
        "callsign": header.callsign,
        "exchangeSent": header.exchange_sent,
        "mode": header.category_mode,
        "category": header.category_operator,
        "powerClass": header.category_power
    }

    payload_json = json.dumps(payload)
    result = subprocess.run(
        ["curl", "-s", "-m", "10", "-X", "POST",
         f"{BASE_URL}/api/contest/create",
         "-H", "Content-Type: application/json",
         "-d", payload_json],
        capture_output=True,
        text=True,
        timeout=15
    )

    if result.returncode != 0:
        raise RuntimeError(f"curl failed: {result.stderr}")

    return json.loads(result.stdout)


def log_qso(qso: CabrilloQSO, contest_mode: str) -> dict:
    """Log a QSO via the API using curl subprocess (bypasses Python HTTP issues)."""
    payload = qso.to_api_request(contest_mode)
    payload_json = json.dumps(payload)

    result = subprocess.run(
        ["curl", "-s", "-m", "30", "-X", "POST",
         f"{BASE_URL}/api/log-qso",
         "-H", "Content-Type: application/json",
         "-d", payload_json],
        capture_output=True,
        text=True,
        timeout=35
    )

    if result.returncode != 0:
        raise RuntimeError(f"curl failed: {result.stderr}")

    return json.loads(result.stdout)


def export_cabrillo() -> str:
    """Export contest to Cabrillo format via API."""
    result = subprocess.run(
        ["curl", "-s", "-m", "30", f"{BASE_URL}/api/export/cabrillo"],
        capture_output=True,
        text=True,
        timeout=35
    )

    if result.returncode != 0:
        raise RuntimeError(f"curl failed: {result.stderr}")

    data = json.loads(result.stdout)
    if not data.get("success"):
        raise RuntimeError(f"Export failed: {data.get('error', 'Unknown error')}")
    return data.get("content", "")


def get_score() -> dict:
    """Get score from API."""
    result = subprocess.run(
        ["curl", "-s", "-m", "30", f"{BASE_URL}/api/contest/score"],
        capture_output=True,
        text=True,
        timeout=35
    )

    if result.returncode != 0:
        raise RuntimeError(f"curl failed: {result.stderr}")

    return json.loads(result.stdout)


def extract_qso_lines(cabrillo_content: str) -> List[str]:
    """Extract just the QSO lines from Cabrillo content."""
    lines = []
    for line in cabrillo_content.split('\n'):
        line = line.strip()
        if line.startswith("QSO:"):
            lines.append(line)
    return lines


def normalize_qso_line(line: str) -> str:
    """Normalize a QSO line for comparison (handle whitespace variations)."""
    # Split by whitespace and rejoin with single spaces
    parts = line.split()
    return ' '.join(parts)


def normalize_exchange(exch: str) -> str:
    """Normalize exchange for comparison (strip leading zeros, whitespace)."""
    parts = exch.strip().split()
    normalized = []
    for part in parts:
        # Try to convert to int to strip leading zeros
        try:
            normalized.append(str(int(part)))
        except ValueError:
            normalized.append(part)
    return ' '.join(normalized)


def compare_qso_sections(original_qsos: List[CabrilloQSO], exported_content: str, contest: str = "") -> Tuple[bool, List[str]]:
    """Compare original QSOs with exported Cabrillo content.

    Comparison criteria (for round-trip testing):
    - Frequency must match
    - Mode must match
    - Date must match
    - Time must match
    - Callsign must match (case-insensitive)
    - Exchange must match (after normalization)

    Returns (success, list of differences)
    """
    exported_lines = extract_qso_lines(exported_content)
    differences = []

    if len(original_qsos) != len(exported_lines):
        differences.append(f"QSO count mismatch: original={len(original_qsos)}, exported={len(exported_lines)}")

    # Build a set of normalized QSO data for comparison
    # Compare by: frequency, mode, date, time, rcvd_call, normalized_rcvd_exch
    original_set = set()
    for qso in original_qsos:
        key = (qso.frequency, qso.mode, qso.date, qso.time, qso.rcvd_call.upper(), normalize_exchange(qso.rcvd_exch))
        original_set.add(key)

    exported_set = set()
    for line in exported_lines:
        qso = parse_qso_line(line, contest)
        if qso:
            key = (qso.frequency, qso.mode, qso.date, qso.time, qso.rcvd_call.upper(), normalize_exchange(qso.rcvd_exch))
            exported_set.add(key)

    # Find QSOs in original but not in exported
    missing = original_set - exported_set
    for key in sorted(missing):
        differences.append(f"Missing in export: freq={key[0]} mode={key[1]} date={key[2]} time={key[3]} call={key[4]} exch={key[5]}")

    # Find QSOs in exported but not in original
    extra = exported_set - original_set
    for key in sorted(extra):
        differences.append(f"Extra in export: freq={key[0]} mode={key[1]} date={key[2]} time={key[3]} call={key[4]} exch={key[5]}")

    return len(differences) == 0, differences


def main():
    # Get Cabrillo file path
    if len(sys.argv) > 1:
        cabrillo_file = Path(sys.argv[1])
    else:
        cabrillo_file = DEFAULT_CABRILLO

    if not cabrillo_file.exists():
        print(f"Error: Cabrillo file not found: {cabrillo_file}")
        sys.exit(1)

    print(f"=== Cabrillo Round-Trip Test ===")
    print(f"Input file: {cabrillo_file}")
    print()

    # Parse input Cabrillo file
    print("Step 1: Parsing input Cabrillo file...")
    header, qsos = parse_cabrillo(cabrillo_file)
    print(f"  Contest: {header.contest}")
    print(f"  Callsign: {header.callsign}")
    print(f"  Mode: {header.category_mode}")
    print(f"  Exchange: {header.exchange_sent}")
    print(f"  QSO count: {len(qsos)}")
    print()

    if len(qsos) == 0:
        print("Error: No QSOs found in input file")
        sys.exit(1)

    # Create temporary database
    with tempfile.TemporaryDirectory(prefix="tr4qt_test_") as tmpdir:
        db_path = Path(tmpdir) / "test_contest.db"
        export_path = Path(tmpdir) / "exported.cbr"

        print(f"Step 2: Starting headless server...")
        print(f"  Database: {db_path}")

        try:
            server_proc = start_server(db_path)
            print(f"  Server started on port {SERVER_PORT}")
        except Exception as e:
            print(f"Error starting server: {e}")
            sys.exit(1)

        try:
            # Create contest
            print()
            print("Step 3: Creating contest via API...")
            result = create_contest(header)
            print(f"  Contest created: ID={result.get('contestDbId')}")

            # Log QSOs
            print()
            print("Step 4: Logging QSOs via API...")
            success_count = 0
            fail_count = 0
            last_error = None

            for i, qso in enumerate(qsos, 1):
                try:
                    result = log_qso(qso, header.category_mode)
                    if result.get("success"):
                        success_count += 1
                        # Show progress every 10 QSOs or first/last
                        if i <= 3 or i == len(qsos) or i % 10 == 0:
                            print(f"  [{i}/{len(qsos)}] Logged: {qso.rcvd_call} on {qso.frequency} kHz")
                    else:
                        fail_count += 1
                        last_error = result.get('error', 'Unknown error')
                except Exception as e:
                    fail_count += 1
                    last_error = str(e)
                    # Only show first error
                    if fail_count == 1:
                        print(f"  [{i}/{len(qsos)}] First error: {qso.rcvd_call} - {e}")

            print(f"  Logged: {success_count}/{len(qsos)} QSOs" +
                  (f" ({fail_count} failed)" if fail_count > 0 else ""))

            # Get score and compare with claimed
            print()
            print("Step 5: Comparing scores...")
            score_data = get_score()
            calculated_score = score_data.get("score", 0)
            # Handle both old and new API field names
            total_points = score_data.get("totalPoints", score_data.get("points", 0))
            total_mults = score_data.get("totalMultipliers", score_data.get("multipliers", 0))
            total_qsos = score_data.get("totalQsos", score_data.get("qsos", 0))
            claimed_score = header.claimed_score
            score_diff = calculated_score - claimed_score
            score_pct = (calculated_score / claimed_score * 100) if claimed_score > 0 else 0
            print(f"  Claimed score:    {claimed_score:,}")
            print(f"  Calculated score: {calculated_score:,} ({total_points} pts × {total_mults} mults, {total_qsos} QSOs)")
            print(f"  Difference:       {score_diff:+,} ({score_pct:.1f}% of claimed)")

            # Export to Cabrillo
            print()
            print("Step 6: Exporting to Cabrillo format...")
            exported_content = export_cabrillo()

            # Save exported file
            with open(export_path, 'w') as f:
                f.write(exported_content)
            print(f"  Exported to: {export_path}")

            # Count exported QSOs
            exported_qso_lines = extract_qso_lines(exported_content)
            print(f"  Exported QSO count: {len(exported_qso_lines)}")

            # Compare QSO sections
            print()
            print("Step 7: Comparing QSO sections...")
            success, differences = compare_qso_sections(qsos, exported_content, header.contest)

            if success:
                print("  ✓ QSO sections match!")
            else:
                print("  ✗ QSO sections differ:")
                for diff in differences:
                    print(f"    - {diff}")

            # Print summary
            print()
            print("=== Summary ===")
            print(f"Input QSOs:    {len(qsos)}")
            print(f"Logged QSOs:   {success_count}")
            print(f"Exported QSOs: {len(exported_qso_lines)}")
            print(f"Round-trip:    {'PASS' if success else 'FAIL'}")

            # Show exported content for debugging
            print()
            print("=== Exported Cabrillo (QSO section) ===")
            for line in exported_qso_lines[:5]:  # Show first 5 QSOs
                print(f"  {line}")
            if len(exported_qso_lines) > 5:
                print(f"  ... and {len(exported_qso_lines) - 5} more")

            sys.exit(0 if success else 1)

        finally:
            print()
            print("Stopping server...")
            stop_server(server_proc)
            print("Done.")


if __name__ == "__main__":
    main()
