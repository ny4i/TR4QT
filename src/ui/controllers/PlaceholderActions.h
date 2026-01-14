/**
 * @file PlaceholderActions.h
 * @brief Handles placeholder "Not Implemented" menu actions
 *
 * Consolidates all placeholder stubs into a single utility to reduce
 * MainWindow line count. When features are implemented, move them
 * to appropriate controllers/services.
 */

#ifndef PLACEHOLDERACTIONS_H
#define PLACEHOLDERACTIONS_H

#include <QString>

class QWidget;

namespace TR4QT {

/**
 * @brief Utility for showing "Not Implemented" dialogs
 *
 * Consolidates all placeholder menu actions. Each action is identified
 * by an enum and displays a consistent "Not Implemented" message.
 */
class PlaceholderActions {
public:
    enum class Action {
        // Window menu
        SwapMultView,
        MissingMultsReport,

        // Edit menu
        ViewEditLog,
        ClearDupes,
        Note,
        RecallLast,

        // Tools menu
        WKMode,
        BackupLog,
        Initialize,

        // Operating menu
        AutoCQ,
        AutoCQResume,
        KillCW,
        DupeCheck,
        SearchLog,
        DeleteLastQSO,
        IncNumber,
        InitialExchange,
        ToggleSidetone,
        ToggleAutosend,

        // Band menu
        ToggleRigs,
        EditSO2R
    };

    /**
     * @brief Show "Not Implemented" dialog for the specified action
     * @param action The placeholder action
     * @param parent Parent widget for the dialog
     */
    static void showNotImplemented(Action action, QWidget* parent);

private:
    struct ActionInfo {
        const char* name;
        const char* shortcut;
        const char* description;
    };

    static const ActionInfo& getActionInfo(Action action);
};

} // namespace TR4QT

#endif // PLACEHOLDERACTIONS_H
