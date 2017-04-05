#ifndef UI_THEME_H
#define UI_THEME_H

#include <QColor>
#include <QHash>
#include <QObject>

namespace ui
{
enum AvatarType {
	Icon,
	Image,
	Letter
};

// Default font size.
const int FontSize = 16;

// Default avatar size. Width and height.
const int AvatarSize = 40;

enum ButtonPreset {
	FlatPreset,
	CheckablePreset
};

enum RippleStyle {
	CenteredRipple,
	PositionedRipple,
	NoRipple
};

enum OverlayStyle {
	NoOverlay,
	TintedOverlay,
	GrayOverlay
};

enum Role {
	Default,
	Primary,
	Secondary
};

enum ButtonIconPlacement {
	LeftIcon,
	RightIcon
};

enum ProgressType {
	DeterminateProgress,
	IndeterminateProgress
};

enum Color {
	Black,
	BrightWhite,
	FadedWhite,
	MediumWhite,
	DarkGreen,
	LightGreen,
	BrightGreen,
	Gray,
	Red,
	Blue,
	Transparent
};

}  // namespace ui

class Theme : public QObject
{
	Q_OBJECT
public:
	explicit Theme(QObject *parent = 0);
	~Theme();

	QColor getColor(const QString &key) const;

	void setColor(const QString &key, const QColor &color);
	void setColor(const QString &key, ui::Color &color);

private:
	QColor rgba(int r, int g, int b, qreal a) const;

	QHash<QString, QColor> colors_;
};

#endif  // UI_THEME_H
