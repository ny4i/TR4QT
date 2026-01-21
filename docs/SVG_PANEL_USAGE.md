# SVG Panel Widget Usage Guide

## Overview
The `SvgPanelWidget` class allows runtime control of SVG elements (LEDs, buttons, etc.) by manipulating the SVG DOM and changing element colors without modifying the original file.

## Current KPA1500 Panel Status

### ✅ Already Named LEDs
The following LEDs are already properly named in `kpa1500_panel.svg`:

**SWR Bargraph (10 LEDs):**
- `led_SWR_1` through `led_SWR_10`

**Status LEDs:**
- `led_ATU_IN` - ATU in circuit
- `led_ATU_BYP` - ATU bypassed
- `led_TX` - Transmit
- `led_ANT1` - Antenna 1 selected
- `led_ANT2` - Antenna 2 selected
- `led_STBY` - Standby mode
- `led_OPER` - Operate mode
- `led_FAULT` - Fault condition
- `led_OVR` - Overdrive/overload

### ⚠️ Need Renaming: Power Meter LEDs
The power meter LEDs currently have generic IDs and need to be renamed in Inkscape:

| Current ID | Recommended New ID | Represents |
|------------|-------------------|------------|
| rect28 | led_PWR_01 | 0W (lowest) |
| rect29 | led_PWR_02 | ~60W |
| ... | ... | ... |
| rect58 | led_PWR_31 | 1800W (highest) |

**To rename in Inkscape:**
1. Open `kpa1500_panel.svg` in Inkscape
2. Select a rectangle (LED)
3. Object → Object Properties (Shift+Ctrl+O)
4. Change the ID field
5. Save the file

## Code Example: Basic Usage

```cpp
#include "ui/widgets/SvgPanelWidget.h"

// Create widget with SVG path
QString svgPath = "/path/to/kpa1500_panel.svg";
SvgPanelWidget* panel = new SvgPanelWidget(svgPath, this);

// Turn on LED (green by default)
panel->setLedOn("led_OPER");

// Turn on LED with custom color
panel->setLedOn("led_FAULT", QColor("#ff0000"));  // Red

// Turn off LED (returns to dark gray #202020)
panel->setLedOff("led_OPER");

// Set custom color
panel->setElementColor("led_TX", QColor("#ffaa00"));  // Orange

// Animate SWR meter
for (int i = 1; i <= 10; i++) {
    panel->setLedOn(QString("led_SWR_%1").arg(i), QColor("#00ff00"));
}

// Reset all colors to default
panel->resetAllColors();
```

## Code Example: KPA1500 Power Meter

```cpp
// Display power level on bargraph
void updatePowerMeter(int watts) {
    const int MAX_WATTS = 1800;
    const int NUM_LEDS = 31;  // Power meter has 31 LEDs

    // Calculate how many LEDs to light
    int ledsToLight = (watts * NUM_LEDS) / MAX_WATTS;

    // Light up LEDs from 01 to ledsToLight
    for (int i = 1; i <= NUM_LEDS; i++) {
        QString ledId = QString("led_PWR_%1").arg(i, 2, 10, QChar('0'));

        if (i <= ledsToLight) {
            // Determine color based on power level
            QColor color;
            if (watts < 600) {
                color = QColor("#00ff00");  // Green (0-600W)
            } else if (watts < 1200) {
                color = QColor("#ffff00");  // Yellow (600-1200W)
            } else {
                color = QColor("#ff0000");  // Red (1200-1800W)
            }
            m_panel->setLedOn(ledId, color);
        } else {
            m_panel->setLedOff(ledId);
        }
    }
}
```

## Code Example: Amplifier Status

```cpp
void updateAmplifierStatus(const AmplifierState& state) {
    // Operate/Standby
    if (state.operateMode) {
        m_panel->setLedOn("led_OPER", QColor("#00ff00"));
        m_panel->setLedOff("led_STBY");
    } else {
        m_panel->setLedOff("led_OPER");
        m_panel->setLedOn("led_STBY", QColor("#ffaa00"));
    }

    // Fault indicator
    if (state.faultDetected) {
        m_panel->setLedOn("led_FAULT", QColor("#ff0000"));
    } else {
        m_panel->setLedOff("led_FAULT");
    }

    // TX indicator
    if (state.transmitting) {
        m_panel->setLedOn("led_TX", QColor("#ff0000"));
    } else {
        m_panel->setLedOff("led_TX");
    }

    // Antenna selection
    m_panel->setLedOff("led_ANT1");
    m_panel->setLedOff("led_ANT2");
    if (state.antenna == 1) {
        m_panel->setLedOn("led_ANT1", QColor("#00ff00"));
    } else if (state.antenna == 2) {
        m_panel->setLedOn("led_ANT2", QColor("#00ff00"));
    }

    // SWR meter
    updateSwrMeter(state.swr);

    // Power meter
    updatePowerMeter(state.forwardPowerWatts);
}
```

## Integration with AmplifierControlWindow

The `AmplifierControlWindow` should be refactored to:

1. Replace the static QPixmap rendering with `SvgPanelWidget`
2. Connect amplifier state signals to LED update slots
3. Remove manual paintEvent drawing of meters (SVG handles it)

**Before (current):**
```cpp
// AmplifierControlWindow.cpp - OLD
void AmplifierControlWindow::paintEvent(QPaintEvent* event) {
    // Manually draw everything with QPainter
    drawPowerMeter(painter);
    drawSwrMeter(painter);
    drawLedIndicators(painter);
}
```

**After (with SvgPanelWidget):**
```cpp
// AmplifierControlWindow.cpp - NEW
AmplifierControlWindow::AmplifierControlWindow(...) {
    m_svgPanel = new SvgPanelWidget(":/svg/kpa1500_panel.svg", this);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_svgPanel);
    setLayout(layout);
}

void AmplifierControlWindow::onAmplifierStateUpdated(const AmplifierState& state) {
    updatePowerMeter(state.forwardPowerWatts);
    updateSwrMeter(state.swr);
    updateStatusLeds(state);
}
```

## Testing Without Full Integration

Create a simple test program to verify LED control works:

```cpp
// test_svg_panel.cpp
#include <QApplication>
#include <QTimer>
#include "ui/widgets/SvgPanelWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    SvgPanelWidget panel("/path/to/kpa1500_panel.svg");
    panel.resize(800, 300);
    panel.show();

    // Flash led_OPER every second
    QTimer* timer = new QTimer(&panel);
    bool ledState = false;
    QObject::connect(timer, &QTimer::timeout, [&]() {
        if (ledState) {
            panel.setLedOn("led_OPER", QColor("#00ff00"));
        } else {
            panel.setLedOff("led_OPER");
        }
        ledState = !ledState;
    });
    timer->start(1000);

    return app.exec();
}
```

## Next Steps

1. **Rename Power Meter LEDs** in Inkscape:
   - rect28 → led_PWR_01
   - rect29 → led_PWR_02
   - ... (through rect58 → led_PWR_31)

2. **Test LED Control** with existing IDs to verify the approach works

3. **Integrate into AmplifierControlWindow** replacing manual rendering

4. **Add Button Click Regions** (optional - for interactive buttons)

5. **Add LCD Display Region** (optional - for dynamic text)

## Troubleshooting

**Q: LED doesn't change color when I call setLedOn()**

A: Check the debug log for warnings like "SVG element not found". The LED may not have the expected ID in the SVG file.

**Q: SVG looks blurry**

A: The SVG is rendered at 2x resolution for Retina displays. If it still looks blurry, increase the rendering scale in `SvgPanelWidget::loadSvg()`.

**Q: Can I use this with other SVG files?**

A: Yes! The widget works with any SVG file. Just ensure elements you want to control have unique `id` attributes.
