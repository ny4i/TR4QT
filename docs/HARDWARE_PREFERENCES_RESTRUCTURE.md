# Hardware Preferences Tab Restructuring

## Problem Statement

The "Radio" preferences tab has become too tall after adding KPA1500 amplifier settings, causing the OK/Cancel buttons to be hidden off-screen. There is no scrollbar to access them.

Additionally, as more hardware is added (rotator, WinKeyer, antenna switches, etc.), the single Radio tab will become increasingly cluttered and difficult to navigate.

## Recommended Solution

**Restructure the "Radio" category into a "Hardware" category with sub-tabs:**

```
Preferences Window
├── Station
├── Hardware  ← Renamed from "Radio"
│   ├── Radio Tab        ← All current radio settings
│   ├── Amplifier Tab    ← KPA1500 settings (currently in Radio tab)
│   ├── Rotator Tab      ← Placeholder for future
│   ├── WinKeyer Tab     ← Placeholder for future
│   └── (Future tabs)    ← Antenna switches, external ATUs, etc.
├── DX Cluster
├── SCP
└── (other categories)
```

## Benefits

1. **Scalable Architecture**: Easy to add new hardware types without cluttering existing tabs
2. **Better Organization**: Each hardware type gets its own focused space
3. **Familiar Pattern**: Tab-within-category is common in preferences UIs (macOS System Preferences, Firefox, etc.)
4. **Solves Height Issue**: Each tab can grow independently without affecting others
5. **Future-Proof**: Prepared for rotator, WinKeyer, antenna switches, external ATUs, etc.

## Implementation Details

### Files to Modify

1. **PreferencesDialog.h** - Update method signatures and add members
2. **PreferencesDialog.cpp** - Implement restructured tabs
3. No changes needed to AppSettings or other backend code (only UI reorganization)

### Step-by-Step Implementation

#### 1. Update Category List (PreferencesDialog.cpp)

**Current:**
```cpp
m_categoryList->addItem("Radio");
```

**New:**
```cpp
m_categoryList->addItem("Hardware");
```

#### 2. Replace createRadioTab() with createHardwareTab()

**PreferencesDialog.h - Update method declaration:**
```cpp
// OLD:
QWidget* createRadioTab();

// NEW:
QWidget* createHardwareTab();
```

**PreferencesDialog.cpp - Update setupUI():**
```cpp
// OLD:
m_settingsStack->addWidget(createRadioTab());

// NEW:
m_settingsStack->addWidget(createHardwareTab());
```

#### 3. Implement createHardwareTab() with Sub-Tabs

**New implementation in PreferencesDialog.cpp:**

```cpp
QWidget* PreferencesDialog::createHardwareTab() {
    QWidget* hardwareTab = new QWidget(this);
    hardwareTab->setAutoFillBackground(true);
    QVBoxLayout* layout = new QVBoxLayout(hardwareTab);

    // Create tab widget for hardware sub-categories
    QTabWidget* hardwareTabs = new QTabWidget(hardwareTab);

    // Add sub-tabs
    hardwareTabs->addTab(createRadioSettingsWidget(), "Radio");
    hardwareTabs->addTab(createAmplifierSettingsWidget(), "Amplifier");

    // Future hardware tabs (add as placeholders or when implementing)
    // hardwareTabs->addTab(createRotatorSettingsWidget(), "Rotator");
    // hardwareTabs->addTab(createWinKeyerSettingsWidget(), "WinKeyer");

    layout->addWidget(hardwareTabs);
    return hardwareTab;
}
```

#### 4. Extract Radio Settings into Separate Widget

**Create new method in PreferencesDialog.cpp:**

```cpp
QWidget* PreferencesDialog::createRadioSettingsWidget() {
    QWidget* radioWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(radioWidget);

    // MOVE ALL EXISTING RADIO SETTINGS CODE HERE
    // This is the current content of createRadioTab()
    // Everything from "Radio Configuration" group through all radio settings

    // Radio Configuration Group
    QGroupBox* radioConfigGroup = new QGroupBox("Radio Configuration");
    QFormLayout* radioConfigLayout = new QFormLayout();

    // ... (all existing radio UI code) ...

    layout->addWidget(radioConfigGroup);
    layout->addStretch();

    return radioWidget;
}
```

#### 5. Extract Amplifier Settings into Separate Widget

**Create new method in PreferencesDialog.cpp:**

```cpp
QWidget* PreferencesDialog::createAmplifierSettingsWidget() {
    QWidget* amplifierWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(amplifierWidget);

    // MOVE AMPLIFIER SETTINGS FROM RADIO TAB HERE
    // Find the "KPA1500 Amplifier" group box in current createRadioTab()
    // and move it here

    // Amplifier Configuration Group
    QGroupBox* amplifierGroup = new QGroupBox("KPA1500 Amplifier");
    QFormLayout* amplifierLayout = new QFormLayout();

    // Enabled checkbox
    m_amplifierEnabledCheck = new QCheckBox("Enable KPA1500 Monitoring");
    amplifierLayout->addRow("Enabled:", m_amplifierEnabledCheck);

    // IP Address
    m_amplifierIpEdit = new QLineEdit();
    m_amplifierIpEdit->setPlaceholderText("192.168.1.100");
    amplifierLayout->addRow("IP Address:", m_amplifierIpEdit);

    // Port
    m_amplifierPortSpin = new QSpinBox();
    m_amplifierPortSpin->setRange(1, 65535);
    m_amplifierPortSpin->setValue(1500);
    amplifierLayout->addRow("UDP Port:", m_amplifierPortSpin);

    amplifierGroup->setLayout(amplifierLayout);
    layout->addWidget(amplifierGroup);

    // Add informational label
    QLabel* infoLabel = new QLabel(
        "The KPA1500 amplifier is monitored via UDP during transmit. "
        "Forward power and operating status (Operate/Standby) are polled "
        "to dynamically scale the TX power meter."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    layout->addWidget(infoLabel);

    layout->addStretch();

    return amplifierWidget;
}
```

#### 6. Update PreferencesDialog.h Method Declarations

**Add new method declarations:**

```cpp
private:
    // Category tab creators
    QWidget* createStationTab();
    QWidget* createHardwareTab();          // NEW: Replaces createRadioTab()
    QWidget* createDXClusterTab();
    // ... other tabs ...

    // Hardware sub-tab creators
    QWidget* createRadioSettingsWidget();      // NEW: Radio settings
    QWidget* createAmplifierSettingsWidget();  // NEW: Amplifier settings
    // Future:
    // QWidget* createRotatorSettingsWidget();
    // QWidget* createWinKeyerSettingsWidget();
```

#### 7. Update loadSettings() and saveSettings()

**No changes needed!** The settings loading/saving code remains the same because:
- `m_amplifierEnabledCheck`, `m_amplifierIpEdit`, `m_amplifierPortSpin` still exist
- They're just organized differently in the UI
- AppSettings API hasn't changed

### Migration Checklist

- [ ] Update category list: "Radio" → "Hardware"
- [ ] Rename `createRadioTab()` → `createHardwareTab()`
- [ ] Create `createRadioSettingsWidget()` with existing radio UI code
- [ ] Create `createAmplifierSettingsWidget()` with amplifier UI code
- [ ] Update `createHardwareTab()` to create QTabWidget with sub-tabs
- [ ] Update method declarations in PreferencesDialog.h
- [ ] Test: Verify all settings load/save correctly
- [ ] Test: Verify OK/Cancel buttons are now visible

### Code Organization Pattern

```cpp
// Main Hardware Tab
createHardwareTab()
    └── QTabWidget
        ├── Tab 1: createRadioSettingsWidget()
        │   ├── Radio Configuration Group
        │   ├── Connection Settings Group
        │   ├── Hamlib Radio List Group
        │   └── Serial Port Settings Group
        │
        ├── Tab 2: createAmplifierSettingsWidget()
        │   └── KPA1500 Amplifier Group
        │
        └── Tab 3+: (Future hardware tabs)
```

### Testing Plan

1. **Functional Testing:**
   - Open Preferences → Hardware
   - Switch between Radio and Amplifier tabs
   - Verify all settings are visible and accessible
   - Change settings and click Apply - verify settings persist
   - Change settings and click Cancel - verify settings revert

2. **Visual Testing:**
   - Verify OK/Cancel buttons are visible on all tabs
   - Verify no scrollbars needed (each tab fits in window)
   - Verify tab layout looks clean and organized

3. **Regression Testing:**
   - Radio connection still works
   - Amplifier monitoring still works
   - All existing functionality unchanged

## Future Enhancements

When adding new hardware types, follow this pattern:

1. Create `createXxxSettingsWidget()` method
2. Add tab in `createHardwareTab()`: `hardwareTabs->addTab(createXxxSettingsWidget(), "Hardware Name")`
3. Add member variables for UI controls to PreferencesDialog.h
4. Update `loadSettings()` and `saveSettings()` for new hardware

**Example for Rotator:**
```cpp
QWidget* PreferencesDialog::createRotatorSettingsWidget() {
    QWidget* rotatorWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(rotatorWidget);

    QGroupBox* rotatorGroup = new QGroupBox("Rotator Configuration");
    QFormLayout* rotatorLayout = new QFormLayout();

    // Rotator type dropdown
    m_rotatorTypeCombo = new QComboBox();
    m_rotatorTypeCombo->addItem("None", 0);
    m_rotatorTypeCombo->addItem("Yaesu GS-232", 1);
    m_rotatorTypeCombo->addItem("HAMLib", 2);
    rotatorLayout->addRow("Type:", m_rotatorTypeCombo);

    // Serial port
    m_rotatorPortCombo = new QComboBox();
    rotatorLayout->addRow("Port:", m_rotatorPortCombo);

    rotatorGroup->setLayout(rotatorLayout);
    layout->addWidget(rotatorGroup);
    layout->addStretch();

    return rotatorWidget;
}
```

## Notes

- This is a UI-only refactoring - no backend changes needed
- Settings storage (AppSettings) remains unchanged
- All existing signal/slot connections remain unchanged
- Member variables for UI controls stay in PreferencesDialog.h (just organized differently)
- This pattern scales well for future hardware additions

## Estimated Implementation Time

- Basic restructure: 20-30 minutes
- Testing and polish: 10-15 minutes
- **Total: 30-45 minutes**

Much better than the quick hack (scrollbar), and provides long-term value!
