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
#include <QMenu>
#include <QPaintEvent>
#include <QSharedPointer>
#include <QVBoxLayout>

class Avatar;
class FlatButton;
class Label;
class LeaveRoomDialog;
class Menu;
class OverlayModal;
class RoomSettings;

static const QString URL_HTML = "<a href=\"\\1\" style=\"color: #333333\">\\1</a>";
static const QRegExp URL_REGEX("((?:https?|ftp)://\\S+)");

class TopRoomBar : public QWidget
{
        Q_OBJECT
public:
        TopRoomBar(QWidget *parent = 0);
        ~TopRoomBar();

        void updateRoomAvatar(const QImage &avatar_image);
        void updateRoomAvatar(const QIcon &icon);
        void updateRoomName(const QString &name);
        void updateRoomTopic(QString topic);
        void updateRoomAvatarFromName(const QString &name);
        void setRoomSettings(QSharedPointer<RoomSettings> settings);

        void reset();

signals:
        void leaveRoom();

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private slots:
        void closeLeaveRoomDialog(bool leaving);

private:
        QHBoxLayout *topLayout_;
        QVBoxLayout *textLayout_;

        QLabel *nameLabel_;
        Label *topicLabel_;

        QSharedPointer<RoomSettings> roomSettings_;

        QMenu *menu_;
        QAction *toggleNotifications_;
        QAction *leaveRoom_;

        FlatButton *settingsBtn_;

        QSharedPointer<OverlayModal> leaveRoomModal_;
        QSharedPointer<LeaveRoomDialog> leaveRoomDialog_;

        Avatar *avatar_;

        int buttonSize_;

        QString roomName_;
        QString roomTopic_;
};
