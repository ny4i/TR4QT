# Email Logs to Support - Implementation Pattern

## Overview

A user-friendly, privacy-respecting pattern for allowing users to send application logs to support via email. This pattern is cross-platform, App Store compliant, and works with any email client.

**Key Principle**: Never try to automatically send emails. Give users complete control and transparency.

---

## Why This Pattern?

### What Doesn't Work

❌ **mailto: URLs with logs in body**
- Size limits (2000-5000 characters max)
- Forces truncation, loses critical data

❌ **Automatic email sending via SMTP**
- Requires user's email credentials (security risk)
- Complex configuration (server, port, auth method)
- Users uncomfortable giving passwords to apps
- Gmail/iCloud require app-specific passwords
- Often flagged as spam

❌ **mailto: URLs with attachments**
- Not supported by the protocol
- No way to auto-attach files via mailto:

### What Works ✅

**Save logs to Desktop → Show in file manager → User drags to email**

This approach:
- ✅ Works with ANY email client (Gmail, Outlook, Apple Mail, etc.)
- ✅ No configuration needed
- ✅ No credential storage
- ✅ User sees exactly what's being sent
- ✅ Cross-platform (macOS, Windows, Linux)
- ✅ App Store compliant
- ✅ No size limits

---

## Implementation Guide

### 1. Collect Logs from Current Session

Extract logs from the most recent "Program Startup" banner forward (not entire log file):

```cpp
QString logs = Logger::instance().getLastLogLines();
```

**Why?** Current session logs are what's needed for debugging the immediate issue.

### 2. Collect System Information

Include non-sensitive diagnostic info:

```cpp
QString systemInfo = QString(
    "App Version: %1\n"
    "Platform: %2 %3\n"
    "Qt Version: %4\n"
    "Radio Model (Configured): %5\n"
    "Connection Type: %6\n"
    "%7"  // Connection details (if serial)
    "Poll Interval: %8 ms\n"
    "Radio Connected: %9\n"
    "\n"
).arg(APP_VERSION)
 .arg(QSysInfo::productType())
 .arg(QSysInfo::productVersion())
 .arg(QT_VERSION_STR)
 .arg(configuredRadio)
 .arg(connectionType)
 .arg(connectionDetails.isEmpty() ? "" : connectionDetails + "\n")
 .arg(radioConfig.pollInterval)
 .arg(m_radio->isConnected() ? "Yes" : "No");
```

**Privacy Rule**: Sanitize IP addresses!
```cpp
if (radioConfig.port.contains(':')) {
    connectionType = "Network (TCP)";
    // Don't include actual IP:port for privacy
} else if (!radioConfig.port.isEmpty()) {
    connectionType = "Serial";
    connectionDetails = QString("Port: %1, Baud: %2, %3%4%5")
        .arg(radioConfig.port)
        .arg(radioConfig.baudRate)
        .arg(radioConfig.dataBits)
        .arg(radioConfig.parity == 0 ? "N" : radioConfig.parity == 1 ? "O" : "E")
        .arg(radioConfig.stopBits);
}
```

### 3. Show Preview Dialog (Transparency)

**Critical**: User must see what's being sent BEFORE file creation.

```cpp
QMessageBox preview;
preview.setWindowTitle("Email Logs to Support - Preview");
preview.setIcon(QMessageBox::Question);
preview.setText(
    QString("This will create a zip file with your support logs (%1 characters).\n\n"
            "Click 'Show Details' below to review what will be included.\n\n"
            "The zip file will be saved to your Desktop for you to attach to an email.")
    .arg(logContent.length()));
preview.setDetailedText(logContent);
preview.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
preview.setDefaultButton(QMessageBox::Ok);

// Auto-expand "Show Details" so user can see content immediately
foreach (QAbstractButton *button, preview.buttons()) {
    if (preview.buttonRole(button) == QMessageBox::ActionRole) {
        button->click();
        break;
    }
}

if (preview.exec() != QMessageBox::Ok) {
    return;  // User cancelled
}
```

### 4. Save to Desktop (Easy Access)

**Don't use temp folders** - users can't find them.

```cpp
QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
if (desktopPath.isEmpty()) {
    desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
QString logFileName = QString("appname-logs-%1.txt").arg(timestamp);
QString zipFileName = QString("appname-logs-%1.zip").arg(timestamp);
QString logFilePath = QFileInfo(desktopPath, logFileName).filePath();
QString zipFilePath = QFileInfo(desktopPath, zipFileName).filePath();
```

### 5. Write Log Content

```cpp
QFile logFile(logFilePath);
if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    DialogHelper::critical(this, "Error",
        QString("Failed to create log file: %1\n\nError: %2")
        .arg(logFilePath)
        .arg(logFile.errorString()));
    return;
}

QTextStream out(&logFile);
out << logContent;
logFile.close();
```

### 6. Zip the File

Use platform-native zip commands:

```cpp
QProcess zipProcess;
zipProcess.setWorkingDirectory(desktopPath);

#ifdef Q_OS_WIN
    // Windows: PowerShell Compress-Archive
    zipProcess.start("powershell", QStringList()
        << "-Command"
        << QString("Compress-Archive -Path '%1' -DestinationPath '%2' -Force")
           .arg(logFileName).arg(zipFileName));
#else
    // macOS/Linux: zip command
    zipProcess.start("zip", QStringList() << "-j" << zipFileName << logFileName);
#endif

if (!zipProcess.waitForFinished(5000)) {
    DialogHelper::critical(this, "Error",
        "Failed to create zip file.\n\n"
        "Please manually attach the log file to your email:\n" + logFilePath);
    return;
}

if (zipProcess.exitCode() != 0) {
    DialogHelper::critical(this, "Error",
        QString("Zip command failed with exit code %1.\n\n"
                "Please manually attach the log file to your email:\n%2")
        .arg(zipProcess.exitCode())
        .arg(logFilePath));
    return;
}

// Delete uncompressed log file (keep only zip)
QFile::remove(logFilePath);
```

### 7. Show Success Dialog with Options

Give users flexible workflow options:

```cpp
QMessageBox instructions;
instructions.setWindowTitle("Support Logs Ready");
instructions.setIcon(QMessageBox::Information);
instructions.setText(
    QString("Support logs saved to your Desktop:\n\n"
            "%1\n\n"
            "What would you like to do?")
    .arg(zipFileName));

QPushButton* bothButton = instructions.addButton(
#ifdef Q_OS_MAC
    "Show in Finder & Open Email",
#else
    "Show in Explorer & Open Email",
#endif
    QMessageBox::AcceptRole);
QPushButton* revealButton = instructions.addButton(
#ifdef Q_OS_MAC
    "Show in Finder Only",
#else
    "Show in Explorer Only",
#endif
    QMessageBox::ActionRole);
QPushButton* emailButton = instructions.addButton("Open Email Only", QMessageBox::ActionRole);
QPushButton* closeButton = instructions.addButton("Close", QMessageBox::RejectRole);
instructions.setDefaultButton(bothButton);

int result = instructions.exec();
QAbstractButton* clicked = instructions.clickedButton();

bool shouldReveal = (clicked == revealButton || clicked == bothButton);
bool shouldEmail = (clicked == emailButton || clicked == bothButton);
```

### 8. Reveal File in File Manager

Platform-specific commands to show file:

```cpp
if (shouldReveal) {
#ifdef Q_OS_MAC
    // macOS: Open Finder and highlight file
    QProcess::startDetached("open", QStringList() << "-R" << zipFilePath);
#elif defined(Q_OS_WIN)
    // Windows: Open Explorer and select file
    QProcess::startDetached("explorer",
        QStringList() << "/select," << QDir::toNativeSeparators(zipFilePath));
#else
    // Linux: Open file manager at directory (can't select specific file universally)
    QDesktopServices::openUrl(QUrl::fromLocalFile(desktopPath));
#endif
    LOG_INFO("MainWindow", QString("Revealed support zip file: %1").arg(zipFilePath));
}
```

### 9. Open Email Client (Optional)

```cpp
if (shouldEmail) {
    QString subject = QString("AppName Support Request - v%1 (%2)")
        .arg(APP_VERSION)
        .arg(QSysInfo::productType());

    QString body = QString(
        "Please describe your issue:\n\n\n\n"
        "---\n"
        "Logs attached: %1\n"
        "App Version: %2\n"
        "Platform: %3 %4")
        .arg(zipFileName)
        .arg(APP_VERSION)
        .arg(QSysInfo::productType())
        .arg(QSysInfo::productVersion());

    QString mailto = QString("mailto:support@example.com?subject=%1&body=%2")
        .arg(QUrl::toPercentEncoding(subject))
        .arg(QUrl::toPercentEncoding(body));

    if (!QDesktopServices::openUrl(QUrl(mailto))) {
        DialogHelper::critical(this, "Error",
            QString("Failed to open email client.\n\n"
                    "Please manually email the zip file to: support@example.com\n\n"
                    "The file is on your Desktop:\n%1").arg(zipFileName));
    } else {
        // Show reminder only if we didn't already show file manager
        if (!shouldReveal) {
            DialogHelper::information(this, "Don't Forget!",
                QString("Remember to attach the zip file from your Desktop:\n\n%1")
                .arg(zipFileName));
        }

        LOG_INFO("MainWindow", QString("Created support zip file: %1").arg(zipFilePath));
    }
}
```

---

## Required Qt Includes

```cpp
#include <QMessageBox>
#include <QAbstractButton>
#include <QProcess>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QDir>
```

---

## App Store Compliance

This pattern is **fully compliant** with Apple App Store and other app store policies:

### ✅ Requirements Met

1. **User-initiated**: User explicitly clicks menu item
2. **Transparent**: Preview dialog shows exact contents before file creation
3. **User-controlled**: User manually attaches and sends email
4. **No automatic transmission**: No network code, no auto-sending
5. **Privacy-preserving**: No PII collected, IP addresses sanitized

### ✅ Privacy Policy Language

If you have a privacy policy, mention:

> "TR4QT allows you to email support logs to our support team. When you use the 'Email Logs to Support' feature, the application saves a zip file to your Desktop containing recent application logs and basic system information (app version, operating system version, Qt version). This file is created locally on your device and is not transmitted until you manually attach it to an email and send it. The logs may contain technical diagnostic information but do not include personally identifiable information such as your name, email address, or location."

---

## User Experience Flow

```
User clicks: Help → Email Logs to Support...
  ↓
Preview Dialog appears:
  "This will create a zip file with your support logs (92,245 characters)"
  [Show Details] ← Auto-expanded, shows full log content
  [OK] [Cancel]
  ↓
User reviews content, clicks OK
  ↓
Success Dialog appears:
  "Support logs saved to your Desktop: appname-logs-2026-01-03_143022.zip"
  [Show in Finder & Open Email] ← Default
  [Show in Finder Only]
  [Open Email Only]
  [Close]
  ↓
User clicks "Show in Finder & Open Email"
  ↓
1. Finder opens, file highlighted (ready to drag)
2. Email client opens with pre-filled subject/body
3. User drags zip file to email attachment area
4. User clicks Send
```

---

## What NOT to Include

**Never collect sensitive information**:

❌ User's name or email address
❌ Passwords or API keys
❌ IP addresses or network configuration
❌ Personal data (contacts, calendar, photos)
❌ Location information
❌ User's callsign (ham radio apps) without explicit consent

**Exception**: Technical configuration that's not personally identifiable is OK:
- ✅ "Connection Type: Network (TCP)" (not the actual IP)
- ✅ "Serial Port: /dev/ttyUSB0" (standard port name, not sensitive)
- ✅ "Radio Model: Elecraft K4" (user's own configuration choice)

---

## Testing Checklist

- [ ] Preview dialog shows complete log content
- [ ] Preview dialog can be cancelled (doesn't create file)
- [ ] Zip file appears on Desktop with timestamp
- [ ] "Show in Finder" reveals and highlights file (macOS)
- [ ] "Show in Explorer" opens Explorer with file selected (Windows)
- [ ] "Open Email Client" opens default email app
- [ ] Email has correct subject and body
- [ ] All three button combinations work
- [ ] No IP addresses in system info section
- [ ] File can be dragged to email as attachment
- [ ] Works with Gmail, Outlook, Apple Mail, etc.

---

## Platform-Specific Notes

### macOS
- `open -R <file>` reveals file in Finder (highlights it)
- Desktop path: `~/Desktop`
- Works with Mail.app, Outlook, Gmail in browser

### Windows
- `explorer /select,<file>` opens Explorer with file selected
- Desktop path: `C:\Users\Username\Desktop`
- Works with Outlook, Thunderbird, Gmail in browser
- **Note**: Use `QDir::toNativeSeparators()` for Windows paths

### Linux
- No universal "reveal file" command
- Fallback: Open file manager at Desktop directory
- Desktop path: `~/Desktop` (usually)
- Works with Thunderbird, Evolution, web-based email

---

## Common Mistakes to Avoid

### ❌ Don't Save to Temp Directory
```cpp
// BAD: User can't find it
QString tempDir = QDir::temp().filePath("AppName-Support");
```

```cpp
// GOOD: User can easily find it
QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
```

### ❌ Don't Use mailto: for Large Content
```cpp
// BAD: Size limits, will be truncated
QString mailto = QString("mailto:support@example.com?body=%1")
    .arg(QUrl::toPercentEncoding(entireLogFile));
```

### ❌ Don't Auto-Send Without User Review
```cpp
// BAD: User doesn't know what's being sent
sendEmailViaSMTP(logContent);
```

```cpp
// GOOD: User reviews before sending
if (preview.exec() == QMessageBox::Ok) {
    createZipFile();
}
```

### ❌ Don't Include Sensitive Data
```cpp
// BAD: Exposes user's network
QString systemInfo = QString("IP Address: %1\n").arg(getLocalIPAddress());
```

```cpp
// GOOD: Generic connection type
QString systemInfo = QString("Connection Type: Network (TCP)\n");
```

---

## Advantages Over Alternatives

| Approach | Pros | Cons |
|----------|------|------|
| **Desktop Zip + mailto:** (This pattern) | ✅ Works with any email client<br>✅ No configuration<br>✅ No size limits<br>✅ User control<br>✅ Cross-platform | ⚠️ Requires user to attach file manually |
| **mailto: with logs in body** | ✅ Simple<br>✅ No file management | ❌ Size limits (2000-5000 chars)<br>❌ Truncation loses data |
| **SMTP implementation** | ✅ Fully automated | ❌ Requires credentials<br>❌ Complex setup<br>❌ Security risks<br>❌ Spam filters |
| **HTTP upload to server** | ✅ Automated | ❌ Requires backend server<br>❌ Privacy concerns<br>❌ Network dependency |

---

## Summary

This pattern provides the **best balance** of:
- ✅ User control and transparency
- ✅ Privacy protection
- ✅ Cross-platform compatibility
- ✅ No configuration needed
- ✅ App Store compliance
- ✅ Works with any email client

**Implementation time**: ~2 hours for full integration
**Maintenance**: Minimal (uses standard Qt APIs)
**User satisfaction**: High (transparent, easy to use)

---

## Example Menu Integration

```cpp
// In your main window menu setup:
QMenu* helpMenu = menuBar->addMenu("&Help");
QAction* aboutAction = helpMenu->addAction("&About");
connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

QAction* emailLogsAction = helpMenu->addAction("Email Logs to Support...");
connect(emailLogsAction, &QAction::triggered, this, &MainWindow::onEmailLogsToSupport);
```

---

## License

This pattern is released into the public domain. Use it in any project (commercial or open source) without attribution required.

---

**Questions or improvements?** Open an issue or PR at your project's repository.

**Version**: 1.0 (January 2026)
