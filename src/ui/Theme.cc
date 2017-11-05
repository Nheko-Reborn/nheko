#include <QDebug>

#include "Theme.h"

Theme::Theme(QObject *parent)
  : QObject(parent)
{
        setColor("Black", ui::Color::Black);

        setColor("BrightWhite", ui::Color::BrightWhite);
        setColor("FadedWhite", ui::Color::FadedWhite);
        setColor("MediumWhite", ui::Color::MediumWhite);

        setColor("BrightGreen", ui::Color::BrightGreen);
        setColor("DarkGreen", ui::Color::DarkGreen);
        setColor("LightGreen", ui::Color::LightGreen);

        setColor("Gray", ui::Color::Gray);
        setColor("Red", ui::Color::Red);
        setColor("Blue", ui::Color::Blue);

        setColor("Transparent", ui::Color::Transparent);
}

Theme::~Theme() {}

QColor
Theme::rgba(int r, int g, int b, qreal a) const
{
        QColor color(r, g, b);
        color.setAlphaF(a);

        return color;
}

QColor
Theme::getColor(const QString &key) const
{
        if (!colors_.contains(key)) {
                qWarning() << "Color with key" << key << "could not be found";
                return QColor();
        }

        return colors_.value(key);
}

void
Theme::setColor(const QString &key, const QColor &color)
{
        colors_.insert(key, color);
}

void
Theme::setColor(const QString &key, ui::Color color)
{
        static const QColor palette[] = {
          QColor("#171919"),

          QColor("#EBEBEB"),
          QColor("#C9C9C9"),
          QColor("#929292"),

          QColor("#1C3133"),
          QColor("#577275"),
          QColor("#46A451"),

          QColor("#5D6565"),
          QColor("#E22826"),
          QColor("#81B3A9"),

          rgba(0, 0, 0, 0),
        };

        colors_.insert(key, palette[static_cast<int>(color)]);
}
