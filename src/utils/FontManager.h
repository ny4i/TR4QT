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

#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <QFont>

namespace TR4QT {

/**
 * FontManager - Centralized font creation
 *
 * Singleton that provides consistent monospace font creation.
 * Replaces scattered QFont("Monospace", N) / QFont("Courier", N) calls
 * with a single source of truth that always sets styleHint and fixedPitch.
 *
 * Usage:
 *   QFont font = FontManager::instance().monospaceFont(12);
 *   QFont entry = FontManager::instance().entryFont();
 */
class FontManager {
public:
    /**
     * Get singleton instance
     */
    static FontManager& instance();

    /**
     * Create a monospace font with the given point size.
     * Sets styleHint(Monospace) and fixedPitch(true).
     * @param pointSize Font size in points
     */
    QFont monospaceFont(int pointSize) const;

    /**
     * Create a Courier-family font with the given point size.
     * Sets styleHint(Monospace) and fixedPitch(true).
     * @param pointSize Font size in points
     */
    QFont courierFont(int pointSize) const;

    /**
     * Monospace font at DEFAULT_ENTRY_FONT_SIZE (for callsign/exchange entry)
     */
    QFont entryFont() const;

    /**
     * Monospace font at DEFAULT_TABLE_FONT_SIZE (for QSO table)
     */
    QFont tableFont() const;

    /**
     * Monospace font at DEFAULT_GRID_FONT_SIZE (for band summary grid)
     */
    QFont gridFont() const;

    /**
     * Monospace font at DEFAULT_MISC_DISPLAY_FONT_SIZE (for misc displays)
     */
    QFont miscFont() const;

private:
    FontManager() = default;
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
};

} // namespace TR4QT

#endif // FONTMANAGER_H
