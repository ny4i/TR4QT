# Qt/C++ Pre-Commit Hook Best Practices

This document describes a comprehensive set of pre-commit checks for Qt/C++ projects. These checks prevent common bugs and API misuse before code is committed.

## Installation

```bash
# Copy the sample hook to your git hooks directory
cp scripts/pre-commit.sample .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Overview

The pre-commit hook performs **11 automated checks** categorized as:
- **Critical Errors**: Block commit (must fix or use `--no-verify`)
- **Warnings**: Allow commit but flag for review

## Critical Errors (Block Commit)

### 1. setParent(nullptr) Creates Top-Level Windows

**Problem**: In Qt, calling `setParent(nullptr)` promotes a widget to a top-level window. This is a dangerous API design that "fails open" (shows unwanted UI) instead of "failing closed".

**What it catches**:
```cpp
widget->setParent(nullptr);  // ❌ Creates floating window!
```

**Why it matters**:
- Causes intermittent blank windows appearing on screen
- Windows show UI fragments like labels with "Both:", "0", etc.
- Race condition - only appears when paint events occur during orphaning
- Happens on both macOS and Windows

**What to do instead**:
```cpp
// ✅ Delete the widget
widget->deleteLater();

// ✅ Hide the widget
widget->hide();

// ✅ If you REALLY want top-level, be explicit
widget->setParent(nullptr);
widget->setWindowFlags(Qt::Window);
widget->show();
```

**Real-world bug**: TR4QT had blank floating windows appearing intermittently because `BandSummaryGrid::rebuildGrid()` was setting `parent = nullptr` when clearing layouts.

---

### 2. Missing Q_OBJECT Macro

**Problem**: Classes with signals/slots MUST have the `Q_OBJECT` macro or the Meta-Object Compiler (MOC) won't generate the necessary code.

**What it catches**:
```cpp
class MyWidget : public QWidget {
    // ❌ Missing Q_OBJECT!
signals:
    void somethingChanged();
};
```

**Why it matters**:
- Signals won't work (won't compile or will crash at runtime)
- Slots won't be callable
- QObject::connect() will fail silently or with cryptic errors
- Metaclass information unavailable (needed for QVariant, etc.)

**What to do**:
```cpp
class MyWidget : public QWidget {
    Q_OBJECT  // ✅ Add this!
signals:
    void somethingChanged();
};
```

---

### 3. Sensitive Data in Code

**Problem**: API keys, passwords, and tokens should never be committed to version control.

**What it catches**:
```cpp
QString apiKey = "sk-abc123def456";  // ❌ Don't commit secrets!
QString password = "myPassword123";  // ❌ Security risk!
```

**Why it matters**:
- Security breach if repository is public or leaked
- Difficult to rotate credentials (they're in git history forever)
- Violates security best practices
- May expose production systems

**What to do instead**:
```cpp
// ✅ Use environment variables
QString apiKey = qEnvironmentVariable("API_KEY");

// ✅ Use configuration files (add to .gitignore)
QSettings settings("config.ini");
QString apiKey = settings.value("api_key").toString();

// ✅ Use Qt Keychain for sensitive data
QKeychain::ReadPasswordJob job("MyApp");
```

---

### 4. Dangerous Pointer Dereference

**Problem**: Dereferencing pointers without nullptr checks can cause crashes.

**What it catches** (as warning):
```cpp
*myPointer->someMethod();  // ⚠️ Is myPointer valid?
```

**Why it matters**:
- Segmentation faults / access violations
- Undefined behavior
- Difficult to debug crashes

**What to do**:
```cpp
// ✅ Check before dereferencing
if (myPointer != nullptr) {
    myPointer->someMethod();
}

// ✅ Use Q_ASSERT in debug builds
Q_ASSERT(myPointer != nullptr);
myPointer->someMethod();

// ✅ Use smart pointers
std::unique_ptr<MyClass> ptr;
if (ptr) {
    ptr->someMethod();
}
```

---

## Warnings (Non-Blocking)

### 5. QWidget Created Without Parent

**What it catches**:
```cpp
QWidget* widget = new QWidget();  // ⚠️ No parent!
QDialog* dialog = new QDialog();  // ⚠️ Might be intentional
```

**Why it matters**:
- Creates top-level window (may be unintentional)
- Memory leak if not manually deleted
- Widget won't be destroyed when parent is destroyed

**What to consider**:
- For child widgets: Always specify parent
- For dialogs: Usually intentional, but consider ownership
- For main windows: Intentionally top-level

---

### 6. Debug Statements Left in Code

**What it catches**:
```cpp
qDebug() << "Testing something";  // ⚠️ Leftover debug code
```

**Why it matters**:
- Performance impact (I/O operations)
- Clutters output
- May leak sensitive information in production
- Should use proper logging framework

**What to do**:
```cpp
// ✅ Use proper logging with levels
LOG_DEBUG("Tag", "Message");
LOG_INFO("Tag", "Message");

// ✅ Or remove debug statements before committing
```

---

### 7. Empty TODO/FIXME

**What it catches**:
```cpp
// TODO:
// FIXME
```

**Why it matters**:
- Lacks context - future developers won't know what to do
- May be forgotten
- Doesn't track ownership

**What to do**:
```cpp
// ✅ Add description and owner
// TODO(username): Refactor this to use async API
// FIXME(username): Memory leak when dialog is canceled
// TODO(username): Link to issue tracker #123
```

---

### 8. Magic Numbers in Widget Sizing

**What it catches**:
```cpp
label->setMaximumWidth(250);   // ⚠️ Magic number
button->setFixedHeight(45);    // ⚠️ What does 45 mean?
```

**Why it matters**:
- Hard to maintain (what does the number represent?)
- Doesn't scale with font size changes
- Duplicate values hard to update consistently

**What to do**:
```cpp
// ✅ Use named constants
const int LABEL_MAX_WIDTH = 250;  // Width to fit longest expected text
label->setMaximumWidth(LABEL_MAX_WIDTH);

// ✅ Calculate from font metrics
const int BUTTON_HEIGHT = button->fontMetrics().height() + 20;
button->setFixedHeight(BUTTON_HEIGHT);
```

---

### 9. QString::toStdString() Locale Issues

**What it catches**:
```cpp
std::string str = qstring.toStdString();  // ⚠️ Locale-dependent!
```

**Why it matters**:
- `toStdString()` uses system locale, NOT UTF-8
- Can corrupt international characters
- Behavior varies between systems
- Hard to debug encoding issues

**What to do**:
```cpp
// ✅ Explicit UTF-8 conversion
std::string str = qstring.toUtf8().constData();

// ✅ Or use toLocal8Bit() if you really want locale
std::string str = qstring.toLocal8Bit().constData();
```

---

### 10. Deprecated Qt APIs

**What it catches**:
```cpp
QRegExp regex("pattern");         // ⚠️ Deprecated
int rand = qrand();                // ⚠️ Deprecated
QTime time = QTime::currentTime(); // ⚠️ Deprecated for timing
```

**Why it matters**:
- May be removed in future Qt versions
- Modern alternatives are faster/better
- Deprecated APIs may have security issues

**What to do**:
```cpp
// ✅ QRegularExpression (Perl-compatible regex)
QRegularExpression regex("pattern");

// ✅ QRandomGenerator (cryptographically secure option available)
int rand = QRandomGenerator::global()->bounded(100);

// ✅ QElapsedTimer (for measuring time intervals)
QElapsedTimer timer;
timer.start();
// ... do work ...
qint64 elapsed = timer.elapsed();
```

---

### 11. Missing Include Guards

**What it catches**:
```cpp
// myheader.h
#include <QString>
class MyClass { };  // ⚠️ No include guard!
```

**Why it matters**:
- Multiple inclusion causes redefinition errors
- Slows down compilation (header parsed multiple times)
- Can cause subtle bugs with inline functions

**What to do**:
```cpp
// ✅ Traditional include guards
#ifndef MYHEADER_H
#define MYHEADER_H

#include <QString>
class MyClass { };

#endif // MYHEADER_H

// ✅ Or use #pragma once (non-standard but widely supported)
#pragma once

#include <QString>
class MyClass { };
```

---

## Hook Usage

### Normal Operation
```bash
git add myfile.cpp
git commit -m "Add new feature"
# Hook runs automatically and checks for issues
```

### Bypass Hook (When Intentional)
```bash
git commit --no-verify -m "Commit despite warnings"
```

### Example Output

**With Errors**:
```
Running TR4QT pre-commit checks...

❌ ERROR: Found setParent(nullptr) which creates top-level windows!
src/ui/MyWidget.cpp:42:    widget->setParent(nullptr);

This is usually a bug. Did you mean to:
  - deleteLater() - Delete the widget
  - hide() - Hide the widget

════════════════════════════════════════
❌ COMMIT BLOCKED: 1 critical error(s) found
   Fix the errors above or use --no-verify to bypass
```

**With Warnings**:
```
Running TR4QT pre-commit checks...

⚠️  WARNING: Found qDebug() calls
src/ui/MyWidget.cpp:55:    qDebug() << "Test";

⚠️  WARNING: Found magic numbers in widget sizing
src/ui/MyWidget.cpp:89:    setMaximumWidth(250);

════════════════════════════════════════
⚠️  2 warning(s) - please review
✓ Pre-commit checks passed (with warnings)
```

---

## Customization

The hook can be customized by editing `.git/hooks/pre-commit`:

**Disable Specific Checks**:
Comment out or remove the check you don't want:
```bash
# Check 6: Debug statements (DISABLED for this project)
# if echo "$STAGED_FILES" | xargs grep -n 'qDebug()' 2>/dev/null; then
#     ...
# fi
```

**Add Project-Specific Checks**:
```bash
# Check 12: Project-specific pattern
if echo "$STAGED_FILES" | xargs grep -n 'MyBadPattern' 2>/dev/null; then
    echo "❌ ERROR: Don't use MyBadPattern, use MyGoodPattern instead"
    ERRORS=$((ERRORS + 1))
fi
```

**Adjust Severity** (Error → Warning):
Change the `ERRORS` counter to `WARNINGS`:
```bash
# Make check 3 a warning instead of error
if echo "$STAGED_FILES" | xargs grep -nE '(password)' 2>/dev/null; then
    echo "⚠️  WARNING: Found password in code"
    WARNINGS=$((WARNINGS + 1))  # Changed from ERRORS
fi
```

---

## Benefits

### Caught at Commit Time
- Faster feedback than CI/CD
- Fixes bugs before they're shared
- Reduces code review burden
- Prevents broken code from being pushed

### Educational
- Teaches developers Qt best practices
- Explains WHY patterns are bad
- Suggests correct alternatives
- Documents common pitfalls

### Customizable
- Enable/disable checks per project
- Adjust severity (error vs warning)
- Add project-specific patterns
- Share across team via repository

---

## Applying to New Projects

When starting a new Qt/C++ project:

1. **Copy the hook**:
   ```bash
   mkdir -p scripts
   cp /path/to/tr4qt/scripts/pre-commit.sample scripts/
   cp scripts/pre-commit.sample .git/hooks/pre-commit
   chmod +x .git/hooks/pre-commit
   ```

2. **Customize for your project**:
   - Adjust which checks are errors vs warnings
   - Add project-specific patterns
   - Disable checks that don't apply

3. **Document in README**:
   ```markdown
   ## Development Setup

   Install pre-commit hooks:
   ```bash
   cp scripts/pre-commit.sample .git/hooks/pre-commit
   chmod +x .git/hooks/pre-commit
   ```
   ```

4. **Consider team adoption**:
   - Share hook via repository
   - Document bypass process (`--no-verify`)
   - Review warnings during code review

---

## Related Best Practices

### Static Analysis
Complement pre-commit hooks with static analysis:
- **clang-tidy**: C++ linting
- **cppcheck**: Static analysis
- **Qt Creator Code Model**: Built-in warnings
- **Compiler warnings**: Enable `-Wall -Wextra -Wpedantic`

### CI/CD Integration
Run same checks in CI pipeline:
```yaml
# .github/workflows/lint.yml
- name: Run pre-commit checks
  run: bash scripts/pre-commit.sample
```

### Code Review
- Pre-commit hooks catch mechanical issues
- Code review focuses on design/logic
- Reduces "nitpicking" comments
- Faster review cycles

---

## Troubleshooting

### Hook Not Running
```bash
# Verify hook is executable
ls -la .git/hooks/pre-commit
# Should show: -rwxr-xr-x

# Make executable if needed
chmod +x .git/hooks/pre-commit
```

### False Positives
```bash
# Bypass specific commit
git commit --no-verify -m "Intentional use of setParent"

# Or fix the pattern in the hook to be more specific
```

### Performance Issues
```bash
# Hook runs on staged files only, so it should be fast
# If slow, check for expensive grep patterns or large file lists
```

---

## Summary

This pre-commit hook prevents **11 common Qt/C++ bugs** before they're committed:

**Critical Errors**:
1. ✅ setParent(nullptr) top-level windows
2. ✅ Missing Q_OBJECT macro
3. ✅ Sensitive data in code
4. ✅ Dangerous pointer dereference

**Warnings**:
5. ✅ QWidget without parent
6. ✅ Debug statements (qDebug)
7. ✅ Empty TODO/FIXME
8. ✅ Magic numbers
9. ✅ QString::toStdString() locale issues
10. ✅ Deprecated Qt APIs
11. ✅ Missing include guards

**Result**: Cleaner code, fewer bugs, better developer experience.

---

*Document created from TR4QT project (https://github.com/ny4i/TR4QT)*
*These patterns are applicable to any Qt/C++ project*
