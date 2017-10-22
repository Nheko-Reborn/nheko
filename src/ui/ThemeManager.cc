#include <QFontDatabase>

#include "ThemeManager.h"

ThemeManager::ThemeManager() { setTheme(new Theme); }

void
ThemeManager::setTheme(Theme *theme)
{
        theme_ = theme;
        theme_->setParent(this);
}

QColor
ThemeManager::themeColor(const QString &key) const
{
        Q_ASSERT(theme_);
        return theme_->getColor(key);
}
