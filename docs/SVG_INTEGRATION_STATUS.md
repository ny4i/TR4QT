# SVG Panel Integration Status

## ✅ Completed

### 1. Core SVG Widget (`SvgPanelWidget`)
- **File**: `src/ui/widgets/SvgPanelWidget.{h,cpp}`
- **Features**:
  - Loads SVG into `QDomDocument` for runtime manipulation
  - Provides API: `setLedOn(id, color)`, `setLedOff(id)`, `setElementColor(id, color)`
  - Updates SVG fill colors by element ID and reloads renderer
  - Thread-safe (QMutex protected)
  - Scales SVG to fit widget while preserving aspect ratio
  - Fallback to placeholder if SVG not found

### 2. AmplifierControlWindow Integration
- **File**: `src/ui/windows/AmplifierControlWindow.{h,cpp}`
- **Changes**:
  - Constructor creates `SvgPanelWidget` instead of static QPixmap
  - SVG panel added to layout (fills entire window)
  - `onAmplifierStateUpdated()` now controls LEDs based on amplifier state:
    - `led_OPER` / `led_STBY` - Operate/Standby mode (green/amber)
    - `led_FAULT` - Fault condition (red)
    - `led_SWR_1` through `led_SWR_10` - SWR bargraph (green/yellow/red based on SWR)
  - Added helper method `updateSwrMeter(float swr)` for SWR bargraph control

### 3. Build Configuration
- **Files**: `CMakeLists.txt`, `src/CMakeLists.txt`, `.github/workflows/build.yml`
- **Changes**:
  - Added Qt6::Svg and Qt6::Xml modules to build
  - Updated Windows CI to include `qtsvg` module
  - Added `SvgPanelWidget.cpp` to source files

### 4. Documentation
- **Files**: `docs/SVG_PANEL_USAGE.md`, `docs/SVG_INTEGRATION_STATUS.md`
- **Content**:
  - Usage examples for SvgPanelWidget
  - List of existing LED IDs in kpa1500_panel.svg
  - Integration guide for AmplifierControlWindow
  - Todo list for power meter LED renaming

## ⚠️ Needs Work

### 1. Power Meter LEDs
**Issue**: Power meter LEDs have generic IDs (`rect28` through `rect58`) instead of semantic names.

**Solution**: Rename in Inkscape
| Current | Recommended | Value |
|---------|-------------|-------|
| rect28 | led_PWR_01 | 0W |
| rect29 | led_PWR_02 | ~60W |
| ... | ... | ... |
| rect58 | led_PWR_31 | 1800W |

**Instructions**:
1. Open `resources/images/kpa1500_panel.svg` in Inkscape
2. Select each rectangle (use XML Editor: Edit → XML Editor)
3. Change `id="rect28"` to `id="led_PWR_01"`, etc.
4. Save file

**Code Ready**: `updatePowerMeter()` method stub exists, just needs LED IDs to be renamed.

### 2. Button Click Regions
**Status**: Legacy button region code still exists but is not connected to SVG

**Options**:
- **Option A**: Remove button interaction (display-only panel)
- **Option B**: Implement SVG-based click detection:
  ```cpp
  void AmplifierControlWindow::mousePressEvent(QMouseEvent* event) {
      // Map click to SVG coordinates
      // Check which button was clicked
      // Send command to amplifier
  }
  ```

### 3. LCD Display Region
**Status**: Not implemented

**Possible Approach**:
- Option A: Keep LCD as part of SVG (static background)
- Option B: Overlay QLabel on top of SVG for dynamic text
- Option C: Dynamically update SVG text elements (complex)

### 4. Testing Required
- [ ] Test with real KPA1500 connected
- [ ] Verify LED colors match expected behavior
- [ ] Test SWR meter scaling (does 1.0-3.0 map correctly to 10 LEDs?)
- [ ] Test window resize (SVG should scale smoothly)
- [ ] Verify fallback path works in deployed app

## 📋 Testing Checklist

**When amplifier connects:**
- [ ] `led_OPER` lights green or `led_STBY` lights amber based on mode
- [ ] `led_FAULT` lights red if fault detected
- [ ] SWR bargraph updates as SWR changes (green → yellow → red)
- [ ] LEDs turn off when values return to normal

**Edge cases:**
- [ ] SVG file missing → falls back to placeholder
- [ ] Unknown LED ID → logs warning but doesn't crash
- [ ] Rapid state updates → renderer keeps up without flickering

## 🚀 Next Steps

**Priority 1 - Power Meter**:
1. Rename power LEDs in Inkscape (`rect28` → `led_PWR_01`, etc.)
2. Implement `updatePowerMeter(int watts)` method
3. Call from `onAmplifierStateUpdated()`
4. Test with real amplifier

**Priority 2 - Clean Up Legacy Code**:
- Remove `m_frontPanelImage`, `m_originalImageSize`, `m_aspectRatio` (no longer used)
- Remove `drawPowerMeter()`, `drawSwrMeter()`, etc. (replaced by SVG control)
- Remove `initializeButtonRegions()`, `initializeLedIndicators()` (not needed)
- Remove `paintEvent()` override (SVG widget handles painting)

**Priority 3 - Polish**:
- Add antenna selection LEDs (`led_ANT1`, `led_ANT2`)
- Add ATU bypass LEDs (`led_ATU_IN`, `led_ATU_BYP`)
- Add TX indicator when amplifier reports transmit state
- Fine-tune SWR color thresholds

## 📊 Code Statistics

**New Files**: 2
- `src/ui/widgets/SvgPanelWidget.h` (130 lines)
- `src/ui/widgets/SvgPanelWidget.cpp` (180 lines)

**Modified Files**: 5
- `src/ui/windows/AmplifierControlWindow.h` (+5 lines)
- `src/ui/windows/AmplifierControlWindow.cpp` (+85 lines, simplified constructor)
- `CMakeLists.txt` (+2 lines)
- `src/CMakeLists.txt` (+2 lines)
- `.github/workflows/build.yml` (+1 word)

**Total Addition**: ~400 lines of new code

## 🎯 Expected Behavior

**When you open Hardware → Amplifier Control:**
1. Window displays KPA1500 SVG front panel (line art, clean)
2. All LEDs start in "off" state (dark gray #202020)
3. When amplifier connects:
   - `led_OPER` or `led_STBY` lights up based on mode
   - `led_FAULT` lights if fault detected
   - SWR bargraph animates as SWR changes
   - Power bargraph will animate once LEDs are renamed
4. LEDs change color dynamically without flickering
5. Window can be resized smoothly (SVG scales)

**Log Messages to Expect:**
```
[INFO] SvgPanelWidget: Loaded SVG: /path/to/kpa1500_panel.svg (...)
[INFO] AmplifierControlWindow: Connection status changed: connected
[WARN] SvgPanelWidget: SVG element not found: led_PWR_01  (until renamed)
```

## 💡 Tips for Renaming LEDs in Inkscape

1. **Open SVG**: Inkscape → File → Open → `kpa1500_panel.svg`
2. **Open XML Editor**: Edit → XML Editor (Shift+Ctrl+X)
3. **Find Rectangle**: Click on a power meter LED in the canvas
4. **View in XML**: XML Editor shows selected element
5. **Change ID**: Double-click `id` attribute, change value
6. **Repeat**: Select next LED, change ID, etc.
7. **Save**: File → Save

**Pro Tip**: Use Find & Replace in a text editor for bulk renaming:
```bash
# Open SVG in VS Code or similar
# Find: id="rect28"
# Replace: id="led_PWR_01"
# Repeat for rect29-rect58
```
