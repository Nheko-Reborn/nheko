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

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

#include "Emote.h"
#include "Image.h"
#include "MessageEvent.h"
#include "Notice.h"
#include "RoomInfoListItem.h"
#include "Text.h"

class ImageItem;
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

        TimelineItem(ImageItem *img,
                     const events::MessageEvent<msgs::Image> &e,
                     bool with_sender,
                     QWidget *parent);

        void setUserAvatar(const QImage &pixmap);
        DescInfo descriptionMessage() const { return descriptionMsg_; };

        ~TimelineItem();

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        void init();

        void generateBody(const QString &body);
        void generateBody(const QString &userid, const QString &body);
        void generateTimestamp(const QDateTime &time);
        QString descriptiveTime(const QDateTime &then);

        void setupAvatarLayout(const QString &userName);
        void setupSimpleLayout();

        QString replaceEmoji(const QString &body);

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
