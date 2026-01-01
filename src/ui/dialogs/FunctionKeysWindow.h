#ifndef FUNCTIONKEYSWINDOW_H
#define FUNCTIONKEYSWINDOW_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QTimer>

namespace TR4QT {

/**
 * Function Keys Reference Window
 *
 * Displays F1-F12 CW macro key mappings in a non-modal window.
 * Shows the function key, macro label, and CW text for each of the 12 macros.
 * Auto-refreshes every 2 seconds to stay in sync with macro edits.
 *
 * Usage:
 * - Window → Function Keys Reference
 * - Keep open during operation for quick reference
 * - Changes in Send Morse dialog automatically reflected
 */
class FunctionKeysWindow : public QDialog {
    Q_OBJECT

public:
    explicit FunctionKeysWindow(QWidget* parent = nullptr);
    ~FunctionKeysWindow() override = default;

private slots:
    void refreshMacros();

private:
    void setupUI();
    void loadMacros();
    QString getFunctionKeyLabel(int index) const;

    QTableWidget* m_macroTable;
    QPushButton* m_refreshButton;
    QTimer* m_autoRefreshTimer;
};

} // namespace TR4QT

#endif // FUNCTIONKEYSWINDOW_H
