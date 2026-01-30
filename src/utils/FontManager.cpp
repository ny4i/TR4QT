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

#include "FontManager.h"
#include "../core/Constants.h"

namespace TR4QT {

FontManager& FontManager::instance()
{
    static FontManager instance;
    return instance;
}

QFont FontManager::monospaceFont(int pointSize) const
{
    QFont font("Monospace", pointSize);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

QFont FontManager::courierFont(int pointSize) const
{
    QFont font("Courier", pointSize);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

QFont FontManager::entryFont() const
{
    return monospaceFont(DEFAULT_ENTRY_FONT_SIZE);
}

QFont FontManager::tableFont() const
{
    return monospaceFont(DEFAULT_TABLE_FONT_SIZE);
}

QFont FontManager::gridFont() const
{
    return monospaceFont(DEFAULT_GRID_FONT_SIZE);
}

QFont FontManager::miscFont() const
{
    return monospaceFont(DEFAULT_MISC_DISPLAY_FONT_SIZE);
}

} // namespace TR4QT
