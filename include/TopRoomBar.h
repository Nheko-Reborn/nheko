/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QAction>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QSharedPointer>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>

class Avatar;
class FlatButton;
class Menu;
class OverlayModal;

class TopRoomBar : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
        TopRoomBar(QWidget *parent = 0);

        void updateRoomAvatar(const QImage &avatar_image);
        void updateRoomAvatar(const QIcon &icon);
        void updateRoomName(const QString &name);
        void updateRoomTopic(QString topic);
        void updateRoomAvatarFromName(const QString &name);

        void reset();

        QColor borderColor() const { return borderColor_; }
        void setBorderColor(QColor &color) { borderColor_ = color; }

signals:
        void inviteUsers(QStringList users);

protected:
        void mousePressEvent(QMouseEvent *) override
        {
                if (roomSettings_ != nullptr)
                        roomSettings_->trigger();
        }

        void paintEvent(QPaintEvent *) override
        {
                QStyleOption opt;
                opt.init(this);
                QPainter p(this);
                style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

                p.setPen(QPen(borderColor()));
                p.drawLine(QPointF(0, height()), QPointF(width(), height()));
        }

private:
        QHBoxLayout *topLayout_  = nullptr;
        QVBoxLayout *textLayout_ = nullptr;

        QLabel *nameLabel_  = nullptr;
        QLabel *topicLabel_ = nullptr;

        Menu *menu_;
        QAction *leaveRoom_    = nullptr;
        QAction *roomMembers_  = nullptr;
        QAction *roomSettings_ = nullptr;
        QAction *inviteUsers_  = nullptr;

        FlatButton *settingsBtn_;

        Avatar *avatar_;

        int buttonSize_;

        QColor borderColor_;
};
