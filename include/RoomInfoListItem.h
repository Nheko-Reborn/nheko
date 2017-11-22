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
#include <QSharedPointer>
#include <QWidget>

#include "RoomState.h"

class Menu;
class RippleOverlay;
class RoomSettings;

struct DescInfo
{
        QString username;
        QString userid;
        QString body;
        QString timestamp;
};

class RoomInfoListItem : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor highlightedBackgroundColor READ highlightedBackgroundColor WRITE
                     setHighlightedBackgroundColor)
        Q_PROPERTY(
          QColor hoverBackgroundColor READ hoverBackgroundColor WRITE setHoverBackgroundColor)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

        Q_PROPERTY(QColor titleColor READ titleColor WRITE setTitleColor)
        Q_PROPERTY(QColor subtitleColor READ subtitleColor WRITE setSubtitleColor)

        Q_PROPERTY(
          QColor highlightedTitleColor READ highlightedTitleColor WRITE setHighlightedTitleColor)
        Q_PROPERTY(QColor highlightedSubtitleColor READ highlightedSubtitleColor WRITE
                     setHighlightedSubtitleColor)

public:
        RoomInfoListItem(QSharedPointer<RoomSettings> settings,
                         RoomState state,
                         QString room_id,
                         QWidget *parent = 0);

        ~RoomInfoListItem();

        void updateUnreadMessageCount(int count);
        void clearUnreadMessageCount();
        void setState(const RoomState &state);

        bool isPressed() const { return isPressed_; }
        RoomState state() const { return state_; }
        int unreadMessageCount() const { return unreadMsgCount_; }

        void setAvatar(const QImage &avatar_image);
        void setDescriptionMessage(const DescInfo &info);

        inline QColor highlightedBackgroundColor() const { return highlightedBackgroundColor_; }
        inline QColor hoverBackgroundColor() const { return hoverBackgroundColor_; }
        inline QColor backgroundColor() const { return backgroundColor_; }

        inline QColor highlightedTitleColor() const { return highlightedTitleColor_; }
        inline QColor highlightedSubtitleColor() const { return highlightedSubtitleColor_; }

        inline QColor titleColor() const { return titleColor_; }
        inline QColor subtitleColor() const { return subtitleColor_; }

        inline void setHighlightedBackgroundColor(QColor &color)
        {
                highlightedBackgroundColor_ = color;
        }
        inline void setHoverBackgroundColor(QColor &color) { hoverBackgroundColor_ = color; }
        inline void setBackgroundColor(QColor &color) { backgroundColor_ = color; }

        inline void setHighlightedTitleColor(QColor &color) { highlightedTitleColor_ = color; }
        inline void setHighlightedSubtitleColor(QColor &color)
        {
                highlightedSubtitleColor_ = color;
        }

        inline void setTitleColor(QColor &color) { titleColor_ = color; }
        inline void setSubtitleColor(QColor &color) { subtitleColor_ = color; }

signals:
        void clicked(const QString &room_id);
        void leaveRoom(const QString &room_id);

public slots:
        void setPressedState(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        QString notificationText();

        const int Padding  = 7;
        const int IconSize = 48;

        RippleOverlay *ripple_overlay_;

        RoomState state_;

        QString roomId_;
        QString roomName_;

        DescInfo lastMsgInfo_;

        QPixmap roomAvatar_;

        Menu *menu_;
        QAction *toggleNotifications_;
        QAction *leaveRoom_;

        QSharedPointer<RoomSettings> roomSettings_;

        bool isPressed_ = false;

        int maxHeight_;
        int unreadMsgCount_ = 0;

        QColor highlightedBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

        QColor highlightedTitleColor_;
        QColor highlightedSubtitleColor_;

        QColor titleColor_;
        QColor subtitleColor_;
};
