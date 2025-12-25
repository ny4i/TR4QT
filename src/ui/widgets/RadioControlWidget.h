#ifndef RADIOCONTROLWIDGET_H
#define RADIOCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QProgressBar>
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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void applyTheme();
    void updateRitWidgetStyle();
    void updateXitWidgetStyle();
    void updateSMeter(int signalStrength);
    QString dbmToSMeter(int dbm) const;

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

    // S-meter widgets
    QProgressBar* m_sMeterBar;      // Visual signal strength bar
    QLabel* m_sMeterLabel;          // S-meter value text (e.g., "S7", "S9+10")

    // Control widgets
    QFrame* m_ritWidget;        // Clickable frame with RIT label + offset
    QLabel* m_ritTitleLabel;    // "RIT" text
    QLabel* m_ritOffsetLabel;   // Offset value

    QFrame* m_xitWidget;        // Clickable frame with XIT label + offset
    QLabel* m_xitTitleLabel;    // "XIT" text
    QLabel* m_xitOffsetLabel;   // Offset value

    QPushButton* m_splitButton; // SPLIT remains a button

    int m_radioNumber;
    RadioState m_currentState;
};

} // namespace TR4QT

#endif // RADIOCONTROLWIDGET_H
