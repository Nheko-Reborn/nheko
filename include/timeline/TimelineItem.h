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

#include "Audio.h"
#include "Emote.h"
#include "File.h"
#include "Image.h"
#include "Notice.h"
#include "Text.h"
#include "Video.h"

#include "AvatarProvider.h"
#include "MessageEvent.h"
#include "RoomInfoListItem.h"
#include "TimelineViewManager.h"

class ImageItem;
class AudioItem;
class VideoItem;
class FileItem;
class Avatar;

namespace events = matrix::events;
namespace msgs   = matrix::events::messages;

class TimelineItem : public QWidget
{
        Q_OBJECT
public:
        TimelineItem(const events::MessageEvent<msgs::Notice> &e,
                     bool with_sender,
                     QWidget *parent = 0);
        TimelineItem(const events::MessageEvent<msgs::Text> &e,
                     bool with_sender,
                     QWidget *parent = 0);
        TimelineItem(const events::MessageEvent<msgs::Emote> &e,
                     bool with_sender,
                     QWidget *parent = 0);

        // For local messages.
        // m.text & m.emote
        TimelineItem(events::MessageEventType ty,
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
                     const events::MessageEvent<msgs::Image> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(FileItem *file,
                     const events::MessageEvent<msgs::File> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(AudioItem *audio,
                     const events::MessageEvent<msgs::Audio> &e,
                     bool with_sender,
                     QWidget *parent);
        TimelineItem(VideoItem *video,
                     const events::MessageEvent<msgs::Video> &e,
                     bool with_sender,
                     QWidget *parent);

        void setUserAvatar(const QImage &pixmap);
        DescInfo descriptionMessage() const { return descriptionMsg_; }
        QString eventId() const { return event_id_; }

        ~TimelineItem();

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        void init();

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
        QString descriptiveTime(const QDateTime &then);

        void setupAvatarLayout(const QString &userName);
        void setupSimpleLayout();

        QString replaceEmoji(const QString &body);
        QString event_id_;

        DescInfo descriptionMsg_;

        QHBoxLayout *topLayout_;
        QVBoxLayout *sideLayout_; // Avatar or Timestamp
        QVBoxLayout *mainLayout_; // Header & Message body

        QHBoxLayout *headerLayout_; // Username (&) Timestamp

        Avatar *userAvatar_;

        QFont font_;

        QLabel *timestamp_;
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

        descriptionMsg_ = {
          "You", userid, QString(" %1").arg(msgDescription), descriptiveTime(timestamp)};

        generateTimestamp(timestamp);

        auto widgetLayout = new QHBoxLayout();
        widgetLayout->setContentsMargins(0, 5, 0, 0);
        widgetLayout->addWidget(widget);
        widgetLayout->addStretch(1);

        if (withSender) {
                generateBody(displayName, "");
                setupAvatarLayout(displayName);
                mainLayout_->addLayout(headerLayout_);

                AvatarProvider::resolve(userid, this);
        } else {
                setupSimpleLayout();
        }

        mainLayout_->addLayout(widgetLayout);
}

template<class Event, class Widget>
void
TimelineItem::setupWidgetLayout(Widget *widget,
                                const Event &event,
                                const QString &msgDescription,
                                bool withSender)
{
        init();

        event_id_ = event.eventId();

        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.timestamp());
        auto displayName = TimelineViewManager::displayName(event.sender());

        QSettings settings;
        descriptionMsg_ = {event.sender() == settings.value("auth/user_id") ? "You" : displayName,
                           event.sender(),
                           msgDescription,
                           descriptiveTime(QDateTime::fromMSecsSinceEpoch(event.timestamp()))};

        generateTimestamp(timestamp);

        auto widgetLayout = new QHBoxLayout();
        widgetLayout->setContentsMargins(0, 5, 0, 0);
        widgetLayout->addWidget(widget);
        widgetLayout->addStretch(1);

        if (withSender) {
                generateBody(displayName, "");
                setupAvatarLayout(displayName);

                mainLayout_->addLayout(headerLayout_);

                AvatarProvider::resolve(event.sender(), this);
        } else {
                setupSimpleLayout();
        }

        mainLayout_->addLayout(widgetLayout);
}
