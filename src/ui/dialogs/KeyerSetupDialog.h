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

#ifndef KEYERSETUPDIALOG_H
#define KEYERSETUPDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>

namespace TR4QT {

class KeyerController;
class IambicKeyer;
class RadioController;
class SidetoneGenerator;

/**
 * CW Keyer Setup Dialog
 *
 * Diagnostic/setup dialog for verifying CW keyer hardware before contest operation.
 * Shows live paddle indicators, provides sidetone playback, and allows adjusting
 * keyer parameters (WPM, mode, WinKeyer settings).
 *
 * This is NOT for contest operation - it's for hardware setup and verification.
 */
class KeyerSetupDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeyerSetupDialog(KeyerController* keyerController,
                              IambicKeyer* iambicKeyer,
                              RadioController* radioController,
                              QWidget* parent = nullptr);
    ~KeyerSetupDialog() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onConnectionStatusChanged(bool connected);
    void onPaddleStateChanged(bool dit, bool dah);
    void onWpmSliderChanged(int value);
    void onVolumeSliderChanged(int value);
    void onPitchSliderChanged(int value);
    void onSidetoneOnlyToggled(bool checked);
    void onKeyerModeChanged();
    void onSwapPaddlesToggled(bool checked);
    void onWeightingChanged(int value);
    void onLeadInChanged(int value);
    void onTailTimeChanged(int value);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateConnectionUI(bool connected);
    void updateWinKeyerVisibility();

    // Borrowed pointers (not owned)
    KeyerController* m_keyerController;
    IambicKeyer* m_iambicKeyer;
    RadioController* m_radioController;

    // Owned
    SidetoneGenerator* m_sidetone;

    // Paddle indicators
    QWidget* m_ditIndicator;
    QWidget* m_dahIndicator;

    // Controls
    QCheckBox* m_sidetoneOnlyCheckbox;
    QSlider* m_wpmSlider;
    QLabel* m_wpmLabel;
    QSlider* m_volumeSlider;
    QLabel* m_volumeLabel;
    QSlider* m_pitchSlider;
    QLabel* m_pitchLabel;

    // Keyer mode
    QRadioButton* m_iambicRadio;
    QRadioButton* m_straightKeyRadio;
    QRadioButton* m_modeARadio;
    QRadioButton* m_modeBRadio;
    QCheckBox* m_swapPaddlesCheckbox;

    // WinKeyer settings
    QGroupBox* m_winKeyerGroup;
    QSlider* m_weightingSlider;
    QLabel* m_weightingLabel;
    QSpinBox* m_leadInSpin;
    QSpinBox* m_tailTimeSpin;

    // Buttons
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;

    // Sidetone-only mode (practice, no radio keying)
    bool m_sidetoneOnly = true;

    // Paddle indicator size
    static constexpr int INDICATOR_SIZE = 24;
};

} // namespace TR4QT

#endif // KEYERSETUPDIALOG_H
