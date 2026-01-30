#!/usr/bin/env python3
"""
Unit tests for headless server validation functions.

These tests validate the ADIF and Cabrillo parsing/validation logic
without requiring a running server.

Usage:
    python3 scripts/test_headless_validators.py
"""

import os
import sys
import tempfile
import unittest

# Import the validation functions from the main test script
# We'll define them inline here to avoid import issues
import re

# Copy of TEST_QSOS for validation
TEST_QSOS = [
    {"callsign": "W1AW", "exchange": "59 05", "band": "20M", "freq": 14200000, "zone": 5},
    {"callsign": "K3LR", "exchange": "59 03", "band": "20M", "freq": 14250000, "zone": 3},
    {"callsign": "DL1A", "exchange": "59 14", "band": "40M", "freq": 7250000, "zone": 14},
]


def validate_adif_content(adif_content: str, expected_qsos: list) -> tuple[bool, list]:
    """
    Validate ADIF content against expected QSOs.
    Returns (is_valid, list_of_errors).
    """
    errors = []

    # Count QSOs
    qso_count = adif_content.count("<EOR>")
    if qso_count != len(expected_qsos):
        errors.append(f"QSO count mismatch: expected {len(expected_qsos)}, got {qso_count}")

    # Verify each callsign and key fields
    for qso in expected_qsos:
        callsign = qso["callsign"]
        band = qso["band"].lower()

        # Extract QSO record
        pattern = f"<CALL:\\d+>{callsign}.*?<EOR>"
        match = re.search(pattern, adif_content, re.IGNORECASE | re.DOTALL)

        if not match:
            errors.append(f"{callsign}: not found in ADIF")
            continue

        record = match.group(0)

        # Check MODE is SSB (not CW)
        if "<MODE:3>SSB" not in record and "<MODE:2>PH" not in record:
            if "<MODE:2>CW" in record:
                errors.append(f"{callsign}: wrong mode (CW instead of SSB)")
            else:
                errors.append(f"{callsign}: missing MODE field")

        # Check BAND
        band_pattern = f"<BAND:\\d+>{band}"
        if not re.search(band_pattern, record, re.IGNORECASE):
            errors.append(f"{callsign}: wrong or missing band (expected {band})")

        # Check CQZ
        if "<CQZ:" not in record:
            errors.append(f"{callsign}: missing CQZ field")

        # Check RST_RCVD
        if "<RST_RCVD:" not in record:
            errors.append(f"{callsign}: missing RST_RCVD field")

    return len(errors) == 0, errors


def validate_cabrillo_content(cabrillo: str, expected_qsos: list,
                               expected_headers: dict = None) -> tuple[bool, list]:
    """
    Validate Cabrillo content against expected QSOs and headers.
    Returns (is_valid, list_of_errors).
    """
    errors = []
    lines = cabrillo.split('\n')
    qso_lines = [line for line in lines if line.startswith('QSO:')]

    # Check QSO count
    if len(qso_lines) != len(expected_qsos):
        errors.append(f"QSO count mismatch: expected {len(expected_qsos)}, got {len(qso_lines)}")

    # Check headers if provided
    if expected_headers:
        for header, expected_value in expected_headers.items():
            found = False
            for line in lines:
                if line.startswith(header):
                    found = True
                    if expected_value:
                        actual_value = line.split(header, 1)[1].strip()
                        if actual_value != expected_value:
                            errors.append(f"Header {header}: expected '{expected_value}', got '{actual_value}'")
                    break
            if not found:
                errors.append(f"Missing header: {header}")

    # Verify QSO lines
    for qso in expected_qsos:
        callsign = qso["callsign"]
        found = False
        for line in qso_lines:
            if callsign in line:
                found = True
                parts = line.split()
                if len(parts) >= 3:
                    mode = parts[2]
                    if mode not in ("PH", "CW", "RY"):  # Valid Cabrillo modes
                        errors.append(f"{callsign}: invalid mode '{mode}'")
                else:
                    errors.append(f"{callsign}: malformed QSO line")
                break
        if not found:
            errors.append(f"{callsign}: not found in Cabrillo")

    return len(errors) == 0, errors


class TestADIFValidation(unittest.TestCase):
    """Test ADIF validation functions."""

    def test_valid_adif(self):
        """Test validation of correct ADIF content."""
        adif = """ADIF Export from TR4QT v3.40.26
<ADIF_VER:5>3.1.4
<EOH>

<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>

<CALL:4>K3LR <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>

<CALL:4>DL1A <BAND:3>40m <MODE:3>SSB <CQZ:2>14 <RST_RCVD:2>59 <EOR>
"""
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertTrue(is_valid, f"Validation failed: {errors}")
        self.assertEqual(len(errors), 0)

    def test_missing_callsign(self):
        """Test detection of missing callsign."""
        adif = """<EOH>
<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>
<CALL:4>K3LR <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>
"""
        # Missing DL1A
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("DL1A" in e and "not found" in e for e in errors))

    def test_wrong_mode_cw(self):
        """Test detection of wrong mode (CW instead of SSB)."""
        adif = """<EOH>
<CALL:4>W1AW <BAND:3>20m <MODE:2>CW <CQZ:1>5 <RST_RCVD:2>59 <EOR>
<CALL:4>K3LR <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>
<CALL:4>DL1A <BAND:3>40m <MODE:3>SSB <CQZ:2>14 <RST_RCVD:2>59 <EOR>
"""
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("W1AW" in e and "CW instead of SSB" in e for e in errors))

    def test_missing_cqz(self):
        """Test detection of missing CQZ field."""
        adif = """<EOH>
<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <RST_RCVD:2>59 <EOR>
<CALL:4>K3LR <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>
<CALL:4>DL1A <BAND:3>40m <MODE:3>SSB <CQZ:2>14 <RST_RCVD:2>59 <EOR>
"""
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("W1AW" in e and "CQZ" in e for e in errors))

    def test_qso_count_mismatch(self):
        """Test detection of QSO count mismatch."""
        adif = """<EOH>
<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>
"""
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("QSO count mismatch" in e for e in errors))


class TestCabrilloValidation(unittest.TestCase):
    """Test Cabrillo validation functions."""

    def test_valid_cabrillo(self):
        """Test validation of correct Cabrillo content."""
        cabrillo = """START-OF-LOG: 3.0
CONTEST: CQ-WW-SSB
CALLSIGN: K1TEST
CATEGORY-MODE: SSB
CATEGORY-OPERATOR: SINGLE-OP
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
QSO: 14250 PH 2026-01-30 0049 K1TEST 59 5 K3LR 59 3
QSO:  7250 PH 2026-01-30 0049 K1TEST 59 5 DL1A 59 14
END-OF-LOG:
"""
        headers = {
            "CONTEST:": "CQ-WW-SSB",
            "CALLSIGN:": "K1TEST",
            "CATEGORY-MODE:": "SSB",
        }
        is_valid, errors = validate_cabrillo_content(cabrillo, TEST_QSOS, headers)
        self.assertTrue(is_valid, f"Validation failed: {errors}")

    def test_missing_callsign(self):
        """Test detection of missing callsign in QSO lines."""
        cabrillo = """START-OF-LOG: 3.0
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
QSO: 14250 PH 2026-01-30 0049 K1TEST 59 5 K3LR 59 3
END-OF-LOG:
"""
        # Missing DL1A
        is_valid, errors = validate_cabrillo_content(cabrillo, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("DL1A" in e for e in errors))

    def test_wrong_header_value(self):
        """Test detection of wrong header value."""
        cabrillo = """START-OF-LOG: 3.0
CONTEST: CQ-WW-CW
CALLSIGN: K1TEST
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
QSO: 14250 PH 2026-01-30 0049 K1TEST 59 5 K3LR 59 3
QSO:  7250 PH 2026-01-30 0049 K1TEST 59 5 DL1A 59 14
END-OF-LOG:
"""
        headers = {"CONTEST:": "CQ-WW-SSB"}  # Expect SSB, got CW
        is_valid, errors = validate_cabrillo_content(cabrillo, TEST_QSOS, headers)
        self.assertFalse(is_valid)
        self.assertTrue(any("CONTEST:" in e and "CQ-WW-SSB" in e for e in errors))

    def test_missing_header(self):
        """Test detection of missing header."""
        cabrillo = """START-OF-LOG: 3.0
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
QSO: 14250 PH 2026-01-30 0049 K1TEST 59 5 K3LR 59 3
QSO:  7250 PH 2026-01-30 0049 K1TEST 59 5 DL1A 59 14
END-OF-LOG:
"""
        headers = {"CONTEST:": "CQ-WW-SSB"}  # Missing CONTEST header
        is_valid, errors = validate_cabrillo_content(cabrillo, TEST_QSOS, headers)
        self.assertFalse(is_valid)
        self.assertTrue(any("Missing header" in e for e in errors))

    def test_qso_count_mismatch(self):
        """Test detection of QSO count mismatch."""
        cabrillo = """START-OF-LOG: 3.0
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
END-OF-LOG:
"""
        is_valid, errors = validate_cabrillo_content(cabrillo, TEST_QSOS)
        self.assertFalse(is_valid)
        self.assertTrue(any("QSO count mismatch" in e for e in errors))


class TestFileOperations(unittest.TestCase):
    """Test file save/load operations."""

    def test_save_and_load_adif(self):
        """Test saving ADIF to file and reading it back."""
        adif_content = """ADIF Export from TR4QT
<ADIF_VER:5>3.1.4
<EOH>

<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.adi', delete=False) as f:
            f.write(adif_content)
            filepath = f.name

        try:
            # Read back
            with open(filepath, 'r') as f:
                read_content = f.read()

            self.assertEqual(adif_content, read_content)
            self.assertIn("<CALL:4>W1AW", read_content)
            self.assertIn("<EOR>", read_content)
        finally:
            os.unlink(filepath)

    def test_save_and_load_cabrillo(self):
        """Test saving Cabrillo to file and reading it back."""
        cabrillo_content = """START-OF-LOG: 3.0
CONTEST: CQ-WW-SSB
CALLSIGN: K1TEST
QSO: 14200 PH 2026-01-30 0049 K1TEST 59 5 W1AW 59 5
END-OF-LOG:
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.cbr', delete=False) as f:
            f.write(cabrillo_content)
            filepath = f.name

        try:
            # Read back
            with open(filepath, 'r') as f:
                read_content = f.read()

            self.assertEqual(cabrillo_content, read_content)
            self.assertIn("START-OF-LOG:", read_content)
            self.assertIn("QSO:", read_content)
        finally:
            os.unlink(filepath)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and error handling."""

    def test_empty_adif(self):
        """Test handling of empty ADIF content."""
        is_valid, errors = validate_adif_content("", TEST_QSOS)
        self.assertFalse(is_valid)

    def test_empty_cabrillo(self):
        """Test handling of empty Cabrillo content."""
        is_valid, errors = validate_cabrillo_content("", TEST_QSOS)
        self.assertFalse(is_valid)

    def test_adif_with_extra_whitespace(self):
        """Test ADIF with extra whitespace is still valid."""
        adif = """

<EOH>

<CALL:4>W1AW <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>

<CALL:4>K3LR <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>

<CALL:4>DL1A <BAND:3>40m <MODE:3>SSB <CQZ:2>14 <RST_RCVD:2>59 <EOR>

        """
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertTrue(is_valid, f"Validation failed: {errors}")

    def test_case_insensitive_callsign(self):
        """Test that callsign matching is case-insensitive."""
        adif = """<EOH>
<CALL:4>w1aw <BAND:3>20m <MODE:3>SSB <CQZ:1>5 <RST_RCVD:2>59 <EOR>
<CALL:4>k3lr <BAND:3>20m <MODE:3>SSB <CQZ:1>3 <RST_RCVD:2>59 <EOR>
<CALL:4>dl1a <BAND:3>40m <MODE:3>SSB <CQZ:2>14 <RST_RCVD:2>59 <EOR>
"""
        is_valid, errors = validate_adif_content(adif, TEST_QSOS)
        self.assertTrue(is_valid, f"Validation failed: {errors}")


def run_tests():
    """Run all unit tests."""
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add test classes
    suite.addTests(loader.loadTestsFromTestCase(TestADIFValidation))
    suite.addTests(loader.loadTestsFromTestCase(TestCabrilloValidation))
    suite.addTests(loader.loadTestsFromTestCase(TestFileOperations))
    suite.addTests(loader.loadTestsFromTestCase(TestEdgeCases))

    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(run_tests())
