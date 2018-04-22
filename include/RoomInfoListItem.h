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
#include <QDateTime>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/responses.hpp>

class Menu;
class RippleOverlay;
struct RoomInfo;

struct DescInfo
{
        QString username;
        QString userid;
        QString body;
        QString timestamp;
        QDateTime datetime;
};

class RoomInfoListItem : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor highlightedBackgroundColor READ highlightedBackgroundColor WRITE
                     setHighlightedBackgroundColor)
        Q_PROPERTY(
          QColor hoverBackgroundColor READ hoverBackgroundColor WRITE setHoverBackgroundColor)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

        Q_PROPERTY(QColor avatarBgColor READ avatarBgColor WRITE setAvatarBgColor)
        Q_PROPERTY(QColor avatarFgColor READ avatarFgColor WRITE setAvatarFgColor)

        Q_PROPERTY(QColor bubbleBgColor READ bubbleBgColor WRITE setBubbleBgColor)
        Q_PROPERTY(QColor bubbleFgColor READ bubbleFgColor WRITE setBubbleFgColor)

        Q_PROPERTY(QColor titleColor READ titleColor WRITE setTitleColor)
        Q_PROPERTY(QColor subtitleColor READ subtitleColor WRITE setSubtitleColor)

        Q_PROPERTY(QColor timestampColor READ timestampColor WRITE setTimestampColor)
        Q_PROPERTY(QColor highlightedTimestampColor READ highlightedTimestampColor WRITE
                     setHighlightedTimestampColor)

        Q_PROPERTY(
          QColor highlightedTitleColor READ highlightedTitleColor WRITE setHighlightedTitleColor)
        Q_PROPERTY(QColor highlightedSubtitleColor READ highlightedSubtitleColor WRITE
                     setHighlightedSubtitleColor)

        Q_PROPERTY(QColor btnColor READ btnColor WRITE setBtnColor)
        Q_PROPERTY(QColor btnTextColor READ btnTextColor WRITE setBtnTextColor)

public:
        RoomInfoListItem(QString room_id, RoomInfo info, QWidget *parent = 0);

        void updateUnreadMessageCount(int count);
        void clearUnreadMessageCount() { updateUnreadMessageCount(0); };

        QString roomId() { return roomId_; }
        bool isPressed() const { return isPressed_; }
        int unreadMessageCount() const { return unreadMsgCount_; }

        void setAvatar(const QImage &avatar_image);
        void setDescriptionMessage(const DescInfo &info);
        DescInfo lastMessageInfo() const { return lastMsgInfo_; }

        QColor highlightedBackgroundColor() const { return highlightedBackgroundColor_; }
        QColor hoverBackgroundColor() const { return hoverBackgroundColor_; }
        QColor backgroundColor() const { return backgroundColor_; }
        QColor avatarBgColor() const { return avatarBgColor_; }
        QColor avatarFgColor() const { return avatarFgColor_; }

        QColor highlightedTitleColor() const { return highlightedTitleColor_; }
        QColor highlightedSubtitleColor() const { return highlightedSubtitleColor_; }
        QColor highlightedTimestampColor() const { return highlightedTimestampColor_; }

        QColor titleColor() const { return titleColor_; }
        QColor subtitleColor() const { return subtitleColor_; }
        QColor timestampColor() const { return timestampColor_; }
        QColor btnColor() const { return btnColor_; }
        QColor btnTextColor() const { return btnTextColor_; }

        QColor bubbleFgColor() const { return bubbleFgColor_; }
        QColor bubbleBgColor() const { return bubbleBgColor_; }

        void setHighlightedBackgroundColor(QColor &color) { highlightedBackgroundColor_ = color; }
        void setHoverBackgroundColor(QColor &color) { hoverBackgroundColor_ = color; }
        void setBackgroundColor(QColor &color) { backgroundColor_ = color; }
        void setTimestampColor(QColor &color) { timestampColor_ = color; }
        void setAvatarFgColor(QColor &color) { avatarFgColor_ = color; }
        void setAvatarBgColor(QColor &color) { avatarBgColor_ = color; }

        void setHighlightedTitleColor(QColor &color) { highlightedTitleColor_ = color; }
        void setHighlightedSubtitleColor(QColor &color) { highlightedSubtitleColor_ = color; }
        void setHighlightedTimestampColor(QColor &color) { highlightedTimestampColor_ = color; }

        void setTitleColor(QColor &color) { titleColor_ = color; }
        void setSubtitleColor(QColor &color) { subtitleColor_ = color; }

        void setBtnColor(QColor &color) { btnColor_ = color; }
        void setBtnTextColor(QColor &color) { btnTextColor_ = color; }

        void setBubbleFgColor(QColor &color) { bubbleFgColor_ = color; }
        void setBubbleBgColor(QColor &color) { bubbleBgColor_ = color; }

        void setRoomName(const QString &name) { roomName_ = name; }
        void setRoomType(bool isInvite)
        {
                if (isInvite)
                        roomType_ = RoomType::Invited;
                else
                        roomType_ = RoomType::Joined;
        }

        bool isInvite() { return roomType_ == RoomType::Invited; }

signals:
        void clicked(const QString &room_id);
        void leaveRoom(const QString &room_id);
        void acceptInvite(const QString &room_id);
        void declineInvite(const QString &room_id);

public slots:
        void setPressedState(bool state);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        void init(QWidget *parent);
        QString roomName() { return roomName_; }

        RippleOverlay *ripple_overlay_;

        enum class RoomType
        {
                Joined,
                Invited,
        };

        RoomType roomType_ = RoomType::Joined;

        // State information for the invited rooms.
        mtx::responses::InvitedRoom invitedRoom_;

        QString roomId_;
        QString roomName_;

        DescInfo lastMsgInfo_;

        QPixmap roomAvatar_;

        Menu *menu_;
        QAction *leaveRoom_;

        bool isPressed_ = false;

        int unreadMsgCount_ = 0;

        QColor highlightedBackgroundColor_;
        QColor hoverBackgroundColor_;
        QColor backgroundColor_;

        QColor highlightedTitleColor_;
        QColor highlightedSubtitleColor_;

        QColor titleColor_;
        QColor subtitleColor_;

        QColor btnColor_;
        QColor btnTextColor_;

        QRectF acceptBtnRegion_;
        QRectF declineBtnRegion_;

        // Fonts
        QFont bubbleFont_;
        QFont font_;
        QFont headingFont_;
        QFont timestampFont_;
        QFont usernameFont_;
        QFont unreadCountFont_;

        QColor timestampColor_;
        QColor highlightedTimestampColor_;

        QColor avatarBgColor_;
        QColor avatarFgColor_;

        QColor bubbleBgColor_;
        QColor bubbleFgColor_;
};
