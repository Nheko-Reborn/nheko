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
#include <QSharedPointer>
#include <QVBoxLayout>

class Avatar;
class FlatButton;
class Label;
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
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private:
        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        QLabel *nameLabel_;
        Label *topicLabel_;

        Menu *menu_;
        QAction *leaveRoom_;
        QAction *roomSettings_;
        QAction *inviteUsers_;

        FlatButton *settingsBtn_;

        Avatar *avatar_;

        int buttonSize_;

        QString roomName_;
        QString roomTopic_;

        QColor borderColor_;
};
