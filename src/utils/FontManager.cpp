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
