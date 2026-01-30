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
