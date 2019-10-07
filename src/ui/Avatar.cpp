#include <QPainter>
#include <QSettings>

#include "AvatarProvider.h"
#include "Utils.h"
#include "ui/Avatar.h"

Avatar::Avatar(QWidget *parent, int size)
  : QWidget(parent)
  , size_(size)
{
        type_   = ui::AvatarType::Letter;
        letter_ = "A";

        QFont _font(font());
        _font.setPointSizeF(ui::FontSize);
        setFont(_font);

        QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        setSizePolicy(policy);
}

QColor
Avatar::textColor() const
{
        if (!text_color_.isValid())
                return QColor("black");

        return text_color_;
}

QColor
Avatar::backgroundColor() const
{
        if (!text_color_.isValid())
                return QColor("white");

        return background_color_;
}

int
Avatar::size() const
{
        return size_;
}

QSize
Avatar::sizeHint() const
{
        return QSize(size_ + 2, size_ + 2);
}

void
Avatar::setTextColor(const QColor &color)
{
        text_color_ = color;
}

void
Avatar::setBackgroundColor(const QColor &color)
{
        background_color_ = color;
}

void
Avatar::setLetter(const QString &letter)
{
        letter_ = letter;
        type_   = ui::AvatarType::Letter;
        update();
}

void
Avatar::setImage(const QString &avatar_url)
{
        AvatarProvider::resolve(avatar_url, size_, this, [this](QPixmap pm) {
                type_   = ui::AvatarType::Image;
                pixmap_ = pm;
                update();
        });
}

void
Avatar::setImage(const QString &room, const QString &user)
{
        AvatarProvider::resolve(room, user, size_, this, [this](QPixmap pm) {
                type_   = ui::AvatarType::Image;
                pixmap_ = pm;
                update();
        });
}

void
Avatar::setIcon(const QIcon &icon)
{
        icon_ = icon;
        type_ = ui::AvatarType::Icon;
        update();
}

void
Avatar::paintEvent(QPaintEvent *)
{
        bool rounded = QSettings().value("user/avatar_circles", true).toBool();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect r      = rect();
        const int hs = size_ / 2;

        if (type_ != ui::AvatarType::Image) {
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(backgroundColor());

                painter.setPen(Qt::NoPen);
                painter.setBrush(brush);
                rounded ? painter.drawEllipse(r.center(), hs, hs)
                        : painter.drawRoundedRect(r, 3, 3);
        }

        switch (type_) {
        case ui::AvatarType::Icon: {
                icon_.paint(&painter,
                            QRect((width() - hs) / 2, (height() - hs) / 2, hs, hs),
                            Qt::AlignCenter,
                            QIcon::Normal);
                break;
        }
        case ui::AvatarType::Image: {
                QPainterPath ppath;

                rounded ? ppath.addEllipse(width() / 2 - hs, height() / 2 - hs, size_, size_)
                        : ppath.addRoundedRect(r, 3, 3);

                painter.setClipPath(ppath);
                painter.drawPixmap(QRect(width() / 2 - hs, height() / 2 - hs, size_, size_),
                                   pixmap_);
                break;
        }
        case ui::AvatarType::Letter: {
                painter.setPen(textColor());
                painter.setBrush(Qt::NoBrush);
                painter.drawText(r.translated(0, -1), Qt::AlignCenter, letter_);
                break;
        }
        default:
                break;
        }
}
