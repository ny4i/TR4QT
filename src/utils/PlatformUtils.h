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

#pragma once

#include <QString>

namespace TR4QT {
namespace PlatformUtils {

// Returns the computer's NetBIOS name (or equivalent).
// Windows: Win32 GetComputerName API (true NetBIOS name)
// macOS: SCDynamicStore SMB/NetBIOSName, falls back to local hostname
// Linux: hostname without domain suffix
QString getNetBiosName();

}  // namespace PlatformUtils
}  // namespace TR4QT
