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

private slots:
    void onRitClicked();
    void onXitClicked();
    void onSplitClicked();
    void onModeContextMenu(const QPoint& pos);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUI();
    void applyTheme();
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
    RadioController* m_radioController{nullptr};  // Reference to radio controller (for mode menu)
};

} // namespace TR4QT

#endif // RADIOCONTROLWIDGET_H
