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
