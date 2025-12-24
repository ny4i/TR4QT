#ifndef RADIOCONTROLWIDGET_H
#define RADIOCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "../../radio/RadioInterface.h"

namespace TR4QT {

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
    ~RadioControlWidget() override = default;

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

private slots:
    void onRitClicked();
    void onXitClicked();
    void onSplitClicked();

private:
    void setupUI();

    // Display labels
    QLabel* m_titleLabel;
    QLabel* m_vfoALabel;
    QLabel* m_vfoAFreqLabel;
    QLabel* m_vfoBLabel;
    QLabel* m_vfoBFreqLabel;
    QLabel* m_modeLabel;

    // Control buttons
    QPushButton* m_ritButton;
    QPushButton* m_xitButton;
    QPushButton* m_splitButton;

    int m_radioNumber;
    RadioState m_currentState;
};

} // namespace TR4QT

#endif // RADIOCONTROLWIDGET_H
