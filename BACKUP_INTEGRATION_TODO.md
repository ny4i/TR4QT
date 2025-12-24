# Backup Settings Integration TODO

## Phase 3: PreferencesDialog Integration (PENDING)

This was skipped to avoid conflicts with ongoing PreferencesDialog work. Once that work is merged, complete this phase:

### 1. Add Backup Settings to AppSettings

**File**: `src/utils/AppSettings.h`

Add these methods:
```cpp
// Backup settings
void setAutoBackupEnabled(bool enabled);
bool getAutoBackupEnabled() const;

void setAutoBackupInterval(int qsoCount);
int getAutoBackupInterval() const;

void setBackupDirectory(const QString& path);
QString getBackupDirectory() const;

void setMaxBackups(int count);
int getMaxBackups() const;
```

**File**: `src/utils/AppSettings.cpp`

Implement the methods using QSettings group "Backup/":
- `Backup/autoBackupEnabled` (default: false)
- `Backup/autoBackupInterval` (default: 50)
- `Backup/backupDirectory` (default: "~/.tr4qt/backups")
- `Backup/maxBackups` (default: 10)

### 2. Add Backup Tab to PreferencesDialog

**File**: `src/ui/dialogs/PreferencesDialog.h`

Add:
```cpp
QWidget* createBackupTab();  // New method

// Backup tab widgets
QCheckBox* m_autoBackupEnabledCheck;
QSpinBox* m_autoBackupIntervalSpin;
QLineEdit* m_backupDirectoryEdit;
QPushButton* m_browseBackupDirButton;
QSpinBox* m_maxBackupsSpin;
QLabel* m_backupInfoLabel;
```

**File**: `src/ui/dialogs/PreferencesDialog.cpp`

Create backup tab with:
- **Auto-backup Settings Group**:
  - Checkbox: "Enable automatic backups"
  - Spinner: "Backup every [50] QSOs" (range: 1-1000)
  - Spinner: "Keep [10] most recent backups" (range: 1-100)

- **Backup Location Group**:
  - Path field + Browse button: "Backup directory"
  - Default placeholder: "~/.tr4qt/backups"
  - Browse button handler: `onBrowseBackupDirectory()`

- **Info Section**:
  - Display last backup time, location, file size
  - Show next scheduled auto-backup (e.g., "in 23 QSOs")

**Pattern to follow**: See `createLoggingTab()` (lines 482-582) for reference

### 3. Update MainWindow to Load Settings

**File**: `src/ui/MainWindow.cpp`

Replace the hardcoded backup initialization (lines 66-72) with:
```cpp
// Load backup settings from preferences
BackupManager& backup = BackupManager::instance();
AppSettings& settings = AppSettings::instance();
backup.setAutoBackupEnabled(settings.getAutoBackupEnabled());
backup.setAutoBackupInterval(settings.getAutoBackupInterval());
backup.setBackupDirectory(settings.getBackupDirectory());
backup.setMaxBackups(settings.getMaxBackups());
```

### 4. Test

1. Open Preferences → Backup tab
2. Enable auto-backup
3. Set interval to 5 QSOs (for testing)
4. Set backup directory
5. Click OK/Apply
6. Log 5 QSOs and verify backup is created
7. Check backup rotation works (create more than max backups)
8. Verify settings persist across application restarts

## Current State

**What's Working:**
- ✅ Database persistence (QSOs saved to SQLite)
- ✅ BackupManager infrastructure (VACUUM INTO, rotation)
- ✅ Manual backup via Tools → Backup Log (Alt+F)
- ✅ Restore from backup (with double confirmation)
- ✅ Auto-backup hook in QSO logging flow

**What's Using Defaults:**
- ⏳ Auto-backup: **disabled by default** (user must enable via code)
- ⏳ Interval: 50 QSOs (hardcoded)
- ⏳ Directory: `~/.tr4qt/backups` (hardcoded)
- ⏳ Max backups: 10 (hardcoded)

## Files Involved

- `src/utils/AppSettings.h/cpp` - Add 4 getter/setter pairs
- `src/ui/dialogs/PreferencesDialog.h/cpp` - Add backup tab UI
- `src/ui/MainWindow.cpp` - Update lines 66-72 to load from settings

## Estimated Effort

~1-2 hours - straightforward UI integration following existing patterns.
