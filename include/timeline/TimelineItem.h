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

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>

#include "AvatarProvider.h"
#include "ChatPage.h"
#include "RoomInfoListItem.h"
#include "TimelineViewManager.h"
#include "Utils.h"

class ImageItem;
class AudioItem;
class VideoItem;
class FileItem;
class Avatar;

class TimelineItem : public QWidget
{
        Q_OBJECT
public:
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Notice> &e,
                     bool with_sender,
                     QWidget *parent = 0);
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Text> &e,
                     bool with_sender,
                     QWidget *parent = 0);
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Emote> &e,
                     bool with_sender,
                     QWidget *parent = 0);

        // For local messages.
        // m.text & m.emote
        TimelineItem(mtx::events::MessageType ty,
                     const QString &userid,
                     QString body,
                     bool withSender,
                     QWidget *parent = 0);
        // m.image
        TimelineItem(ImageItem *item, const QString &userid, bool withSender, QWidget *parent = 0);
        TimelineItem(FileItem *item, const QString &userid, bool withSender, QWidget *parent = 0);
        TimelineItem(AudioItem *item, const QString &userid, bool withSender, QWidget *parent = 0);
        TimelineItem(VideoItem *item, const QString &userid, bool withSender, QWidget *parent = 0);

        TimelineItem(ImageItem *img,
                     const mtx::events::RoomEvent<mtx::events::msg::Image> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(FileItem *file,
                     const mtx::events::RoomEvent<mtx::events::msg::File> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(AudioItem *audio,
                     const mtx::events::RoomEvent<mtx::events::msg::Audio> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(VideoItem *video,
                     const mtx::events::RoomEvent<mtx::events::msg::Video> &e,
                     bool with_sender,
                     QWidget *parent);

        void setUserAvatar(const QImage &pixmap);
        DescInfo descriptionMessage() const { return descriptionMsg_; }
        QString eventId() const { return event_id_; }
        void setEventId(const QString &event_id) { event_id_ = event_id; }
        void markReceived();
        void setRoomId(const QString &room_id) { room_id_ = room_id; }
        void sendReadReceipt() const
        {
                if (!event_id_.isEmpty())
                        ChatPage::instance()->readEvent(room_id_, event_id_);
        }

        //! Add a user avatar for this event.
        void addAvatar();

protected:
        void paintEvent(QPaintEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        void init();
        //! Add a context menu option to save the image of the timeline item.
        void addSaveImageAction(ImageItem *image);

        template<class Widget>
        void setupLocalWidgetLayout(Widget *widget,
                                    const QString &userid,
                                    const QString &msgDescription,
                                    bool withSender);

        template<class Event, class Widget>
        void setupWidgetLayout(Widget *widget,
                               const Event &event,
                               const QString &msgDescription,
                               bool withSender);

        void generateBody(const QString &body);
        void generateBody(const QString &userid, const QString &body);
        void generateTimestamp(const QDateTime &time);

        void setupAvatarLayout(const QString &userName);
        void setupSimpleLayout();

        QString replaceEmoji(const QString &body);
        QString event_id_;
        QString room_id_;

        DescInfo descriptionMsg_;

        QMenu *contextMenu_;
        QAction *showReadReceipts_;
        QAction *markAsRead_;
        QAction *redactMsg_;

        QHBoxLayout *topLayout_     = nullptr;
        QHBoxLayout *messageLayout_ = nullptr;
        QVBoxLayout *mainLayout_    = nullptr;
        QVBoxLayout *headerLayout_  = nullptr;
        QHBoxLayout *widgetLayout_  = nullptr;

        Avatar *userAvatar_;

        QFont font_;
        QFont usernameFont_;

        QLabel *timestamp_;
        QLabel *checkmark_;
        QLabel *userName_;
        QLabel *body_;
};

template<class Widget>
void
TimelineItem::setupLocalWidgetLayout(Widget *widget,
                                     const QString &userid,
                                     const QString &msgDescription,
                                     bool withSender)
{
        auto displayName = TimelineViewManager::displayName(userid);
        auto timestamp   = QDateTime::currentDateTime();

        descriptionMsg_ = {"You",
                           userid,
                           QString(" %1").arg(msgDescription),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        widgetLayout_ = new QHBoxLayout;
        widgetLayout_->setContentsMargins(0, 5, 0, 0);
        widgetLayout_->addWidget(widget);
        widgetLayout_->addStretch(1);

        if (withSender) {
                generateBody(displayName, "");
                setupAvatarLayout(displayName);

                headerLayout_->addLayout(widgetLayout_);
                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  userid, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                setupSimpleLayout();

                messageLayout_->addLayout(widgetLayout_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}

template<class Event, class Widget>
void
TimelineItem::setupWidgetLayout(Widget *widget,
                                const Event &event,
                                const QString &msgDescription,
                                bool withSender)
{
        init();

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = TimelineViewManager::displayName(sender);

        QSettings settings;
        descriptionMsg_ = {sender == settings.value("auth/user_id") ? "You" : displayName,
                           sender,
                           msgDescription,
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        widgetLayout_ = new QHBoxLayout();
        widgetLayout_->setContentsMargins(0, 5, 0, 0);
        widgetLayout_->addWidget(widget);
        widgetLayout_->addStretch(1);

        if (withSender) {
                generateBody(displayName, "");
                setupAvatarLayout(displayName);

                headerLayout_->addLayout(widgetLayout_);
                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                setupSimpleLayout();

                messageLayout_->addLayout(widgetLayout_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}
