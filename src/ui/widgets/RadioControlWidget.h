/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RADIOCONTROLWIDGET_H
#define RADIOCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include "../../radio/RadioInterface.h"

namespace TR4QT {

// Forward declarations
class RadioController;
class SMeterWidget;

/**
 * Radio control widget - displays VFO frequencies and controls
 *
 * Shows:
 * - VFO A frequency (main)
 * - VFO B frequency (sub)
 * - Current mode (CW, SSB, FM, etc.)
 * - RIT/XIT/SPLIT toggle buttons
 *
 * Similar to TR4W's radio control window
 */
class RadioControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadioControlWidget(QWidget* parent = nullptr);
    ~RadioControlWidget() override;

    /**
     * Update display from radio state
     */
    void updateRadioState(const RadioState& state);

    /**
     * Clear display when radio disconnects
     */
    void clearDisplay();

    /**
     * Set radio number (1 or 2 for multi-radio setups)
     */
    void setRadioNumber(int number);

    /**
     * Set active state (shows visual indicator for TX focus)
     */
    void setActive(bool active);

    /**
     * Set maximum TX power for power meter scale
     * @param maxPowerWatts Maximum power in watts (110W for K4, 1800W for amplifier)
     */
    void setMaxPower(int maxPowerWatts);

    /**
     * Set radio controller reference (for mode menu)
     */
    void setRadioController(RadioController* controller);

signals:
    /**
     * User toggled RIT
     */
    void ritToggled(bool enabled);

    /**
     * User toggled XIT
     */
    void xitToggled(bool enabled);

    /**
     * User toggled SPLIT
     */
    void splitToggled(bool enabled);

    /**
     * User requested mode change (from right-click menu)
     */
    void modeChangeRequested(ModeType mode);

    /**
     * User requested CW speed change
     */
    void cwSpeedChangeRequested(int wpm);

private slots:
    void onRitClicked();
    void onXitClicked();
    void onSplitClicked();
    void onModeContextMenu(const QPoint& pos);
    void onWpmContextMenu(const QPoint& pos);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void applyTheme();
    void updateTitleLabel();
    void updateRitWidgetStyle();
    void updateXitWidgetStyle();

    // VFO widgets (for theme updates)
    QWidget* m_vfoAWidget;
    QWidget* m_vfoBWidget;

    // Display labels
    QLabel* m_titleLabel;
    QLabel* m_vfoALabel;
    QLabel* m_vfoAFreqLabel;
    QLabel* m_vfoBLabel;
    QLabel* m_vfoBFreqLabel;
    QLabel* m_modeLabel;
    QLabel* m_wpmLabel;
    SMeterWidget* m_sMeterWidget;  // S-meter display

    // Control widgets
    QFrame* m_ritWidget;        // Clickable frame with RIT label + offset
    QLabel* m_ritTitleLabel;    // "RIT" text
    QLabel* m_ritOffsetLabel;   // Offset value

    QFrame* m_xitWidget;        // Clickable frame with XIT label + offset
    QLabel* m_xitTitleLabel;    // "XIT" text
    QLabel* m_xitOffsetLabel;   // Offset value

    QPushButton* m_splitButton; // SPLIT remains a button

    int m_radioNumber;
    bool m_isActive{false};  // Active radio indicator for SO2R
    RadioState m_currentState;
    RadioController* m_radioController{nullptr};  // Reference to radio controller (for mode menu)
};

} // namespace TR4QT

#endif // RADIOCONTROLWIDGET_H
