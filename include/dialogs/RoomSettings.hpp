#pragma once

#include <QFrame>
#include <QImage>

#include "Cache.h"

class FlatButton;
class TextField;
class Avatar;
class QPixmap;
class QLayout;
class QLabel;

template<class T>
class QSharedPointer;

class TopSection : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)

public:
        TopSection(const RoomInfo &info, const QImage &img, QWidget *parent = nullptr)
          : QWidget{parent}
          , info_{std::move(info)}
        {
                textColor_ = palette().color(QPalette::Text);
                avatar_    = QPixmap::fromImage(img.scaled(
                  AvatarSize, AvatarSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }

        QSize sizeHint() const override
        {
                QFont font;
                font.setPixelSize(18);
                return QSize(200, AvatarSize + QFontMetrics(font).ascent() + 6 * Padding);
        }

        QColor textColor() const { return textColor_; }
        void setTextColor(QColor &color) { textColor_ = color; }

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        static constexpr int AvatarSize = 72;
        static constexpr int Padding    = 5;

        RoomInfo info_;
        QPixmap avatar_;
        QColor textColor_;
};

namespace dialogs {

class RoomSettings : public QFrame
{
        Q_OBJECT
public:
        RoomSettings(const QString &room_id, QWidget *parent = nullptr);

signals:
        void closing();

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        static constexpr int AvatarSize = 64;

        void setAvatar(const QImage &img) { avatarImg_ = img; }

        // Button section
        FlatButton *saveBtn_;
        FlatButton *cancelBtn_;

        RoomInfo info_;
        QString room_id_;
        QImage avatarImg_;
};

} // dialogs
