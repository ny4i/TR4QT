# TR4QT Coding Standards

## Number Formatting

### Scientific Notation - NEVER USE

**Rule:** Scientific notation must NEVER be used when displaying numbers to users.

**Rationale:**
- Ham radio operators expect to see frequencies as integers (e.g., "7074000" not "7.074e+06")
- Contest serial numbers, zones, and other numeric fields should always be displayed as plain integers
- Scientific notation is confusing and inappropriate for amateur radio applications

**Implementation:**

```cpp
// ❌ WRONG - May use scientific notation for large numbers
QString::number(frequency)

// ✅ CORRECT - Always formats as integer
QString::number(static_cast<qint64>(frequency))

// ✅ CORRECT - Explicit integer formatting
QString::number(frequency, 'f', 0)  // Fixed-point, 0 decimal places
```

**Examples:**

```cpp
// Frequency display (Hz)
freq_t freq = 7074000;
m_frequencyEdit->setText(QString::number(static_cast<qint64>(freq)));  // "7074000"

// Serial numbers
int serial = 123;
m_serialLabel->setText(QString::number(serial));  // "123" (safe, already int)

// Zone numbers
int zone = 14;
m_zoneEdit->setText(QString::number(zone));  // "14" (safe, already int)
```

**Where This Applies:**
- Frequency fields (VFO displays, frequency editors, band maps)
- Serial numbers (contest exchange)
- Zone numbers (CQ/ITU zones)
- QSO counts and statistics
- Any numeric display visible to the user

**Exceptions:**
- Internal calculations and logging (scientific notation acceptable)
- Debug output (scientific notation acceptable)
- Database storage (use native numeric types)

---

## Qt-Specific Guidelines

### QString Formatting

- Always specify format explicitly when converting numbers to strings for display
- Use `QLocale::c()` for consistent formatting regardless of system locale (if needed)
- For frequencies, prefer Hz (integer) over kHz/MHz (floating point) for precision

### Signal Strength

When displaying signal strength values:
```cpp
// S-meter values are in dBm (-127 to -13 typical range)
// Always display as "S9+20" format, never as "-53 dBm" to users
QString sMeter = SMeterWidget::dbmToSMeter(strength);  // "S9+20"
```

---

## File Organization

### Dialog Classes

Edit dialogs should:
1. Group fields logically (Basic, Exchange, Geographic, Scoring, Metadata)
2. Mark calculated/read-only fields clearly with gray background
3. Use appropriate input widgets (QSpinBox for numbers, QLineEdit for text, QDateTimeEdit for timestamps)
4. Validate input before accepting (TODO: add validation examples)

### Model-View Separation

- Table models should handle data formatting
- Views should focus on display and interaction
- Controllers (MainWindow) coordinate between models and views

---

## Amateur Radio Conventions

### Callsign Handling

```cpp
// Always normalize callsigns to uppercase
QString callsign = input.trimmed().toUpper();
```

### RST Reports

- Default to "599" for CW/Digital, "59" for Phone
- Allow manual override in edit dialogs
- Store as strings (e.g., "599", "579")

### Frequencies

- Store in Hz (freq_t = unsigned long long)
- Display in kHz or MHz only for human readability
- Never lose precision by converting to floating point prematurely

---

## Future Guidelines

(To be added as patterns emerge)

- Date/Time handling (always UTC for QSO timestamps)
- Multiplier validation rules
- Database transaction patterns
- Error handling and logging conventions
