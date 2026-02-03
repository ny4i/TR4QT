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
 * @file PlaceholderActions.cpp
 * @brief Implementation of PlaceholderActions
 */

#include "PlaceholderActions.h"
#include <QWidget>
#include "../../utils/DialogHelper.h"
#include "../../logging/LogMacros.h"

namespace TR4QT {

const PlaceholderActions::ActionInfo& PlaceholderActions::getActionInfo(Action action) {
    static const ActionInfo infos[] = {
        // Window menu
        {"Swap Mult View", "Alt+G", "This will toggle between different multiplier display modes."},
        {"Missing Mults Report", "Ctrl+O", "This will show a report of multipliers still needed."},

        // Edit menu
        {"View/Edit Log", "Ctrl+L", "This will show all logged QSOs in a table for viewing and editing."},
        {"Clear Dupes", "Ctrl+K", "This will remove duplicate QSOs from the log."},
        {"Note", "Ctrl+N", "This will allow adding notes to the log."},
        {"Recall Last Entry", "Ctrl+R", "This will recall the last deleted log entry."},

        // Tools menu
        {"WK Mode", "Alt+A", "This will re-initialize the WinKeyer for CW keying."},
        {"Backup Log", "Alt+F", "This will create a backup of the current log."},
        {"Initialize", "Alt+W", "This will initialize/reset contest parameters."},

        // Operating menu
        {"Auto CQ", "Alt+Q", "This will enable automatic CQ sending."},
        {"Auto CQ Resume", "Alt+C", "This will resume automatic CQ after an interruption."},
        {"Kill CW", "Alt+K", "This will immediately stop CW transmission."},
        {"Dupe Check", "Alt+D", "This will check if the entered callsign is a duplicate."},
        {"Search Log", "Alt+L", "This will search the log for a specific callsign."},
        {"Delete Last QSO", "Alt+Y", "This will delete the most recent QSO from the log."},
        {"Inc Number", "Alt+I", "This will increment the serial number."},
        {"Initial Exchange", "Alt+Z", "This will set/reset the initial exchange information."},
        {"Toggle Sidetone", "Alt+=", "This will turn CW sidetone on/off."},
        {"Toggle Autosend", "Alt+-", "This will enable/disable automatic sending."},

        // Band menu
        {"Toggle Rigs", "Alt+R", "This will switch between radios in SO2R mode."},
        {"Edit SO2R", "Alt+E", "This will configure SO2R (two-radio) settings."}
    };

    return infos[static_cast<int>(action)];
}

void PlaceholderActions::showNotImplemented(Action action, QWidget* parent) {
    const ActionInfo& info = getActionInfo(action);

    LOG_DEBUG("PlaceholderActions", QString("%1 (%2) - Not yet implemented")
        .arg(info.name).arg(info.shortcut));

    DialogHelper::information(parent, "Not Implemented",
        QString("%1 will be implemented in a future version.\n\n%2")
            .arg(info.name)
            .arg(info.description));
}

} // namespace TR4QT
