#pragma once

#include <QCommonStyle>

#include "Theme.h"

class ThemeManager : public QCommonStyle
{
	Q_OBJECT

public:
	inline static ThemeManager &instance();

	void setTheme(Theme *theme);
	QColor themeColor(const QString &key) const;

private:
	ThemeManager();

	ThemeManager(ThemeManager const &);
	void operator=(ThemeManager const &);

	Theme *theme_;
};

inline ThemeManager &
ThemeManager::instance()
{
	static ThemeManager instance;
	return instance;
}
