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

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>
#include <QTextBrowser>
#include <QTimer>

#include "AvatarProvider.h"
#include "RoomInfoListItem.h"
#include "Utils.h"

#include "Cache.h"
#include "MatrixClient.h"

class ImageItem;
class StickerItem;
class AudioItem;
class VideoItem;
class FileItem;
class Avatar;

enum class StatusIndicatorState
{
        //! The encrypted message was received by the server.
        Encrypted,
        //! The plaintext message was received by the server.
        Received,
        //! The client sent the message. Not yet received.
        Sent,
        //! When the message is loaded from cache or backfill.
        Empty,
};

//!
//! Used to notify the user about the status of a message.
//!
class StatusIndicator : public QWidget
{
        Q_OBJECT

public:
        explicit StatusIndicator(QWidget *parent);
        void setState(StatusIndicatorState state);

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        void paintIcon(QPainter &p, QIcon &icon);

        QIcon lockIcon_;
        QIcon clockIcon_;
        QIcon checkmarkIcon_;

        QColor iconColor_ = QColor("#999");

        StatusIndicatorState state_ = StatusIndicatorState::Empty;

        static constexpr int MaxWidth = 24;
};

class TextLabel : public QTextBrowser
{
        Q_OBJECT

public:
        TextLabel(const QString &text, QWidget *parent = 0)
          : QTextBrowser(parent)
        {
                setText(text);
                setOpenExternalLinks(true);

                // Make it look and feel like an ordinary label.
                setReadOnly(true);
                setFrameStyle(QFrame::NoFrame);
                QPalette pal = palette();
                pal.setColor(QPalette::Base, Qt::transparent);
                setPalette(pal);

                // Wrap anywhere but prefer words, adjust minimum height on the fly.
                setLineWrapMode(QTextEdit::WidgetWidth);
                setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
                connect(document()->documentLayout(),
                        &QAbstractTextDocumentLayout::documentSizeChanged,
                        this,
                        &TextLabel::adjustHeight);
                document()->setDocumentMargin(0);

                setFixedHeight(20);
        }

        void wheelEvent(QWheelEvent *event) override { event->ignore(); }

private slots:
        void adjustHeight(const QSizeF &size) { setFixedHeight(size.height()); }
};

class UserProfileFilter : public QObject
{
        Q_OBJECT

public:
        explicit UserProfileFilter(const QString &user_id, QLabel *parent)
          : QObject(parent)
          , user_id_{user_id}
        {}

signals:
        void hoverOff();
        void hoverOn();

protected:
        bool eventFilter(QObject *obj, QEvent *event)
        {
                if (event->type() == QEvent::MouseButtonRelease) {
                        // QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                        // TODO: Open user profile
                        return true;
                } else if (event->type() == QEvent::HoverLeave) {
                        emit hoverOff();
                        return true;
                } else if (event->type() == QEvent::HoverEnter) {
                        emit hoverOn();
                        return true;
                }

                return QObject::eventFilter(obj, event);
        }

private:
        QString user_id_;
};

class TimelineItem : public QWidget
{
        Q_OBJECT
public:
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Notice> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Text> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Emote> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent = 0);

        // For local messages.
        // m.text & m.emote
        TimelineItem(mtx::events::MessageType ty,
                     const QString &userid,
                     QString body,
                     bool withSender,
                     const QString &room_id,
                     QWidget *parent = 0);
        // m.image
        TimelineItem(ImageItem *item,
                     const QString &userid,
                     bool withSender,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineItem(FileItem *item,
                     const QString &userid,
                     bool withSender,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineItem(AudioItem *item,
                     const QString &userid,
                     bool withSender,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineItem(VideoItem *item,
                     const QString &userid,
                     bool withSender,
                     const QString &room_id,
                     QWidget *parent = 0);

        TimelineItem(ImageItem *img,
                     const mtx::events::RoomEvent<mtx::events::msg::Image> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent);
        TimelineItem(StickerItem *img,
                     const mtx::events::Sticker &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent);
        TimelineItem(FileItem *file,
                     const mtx::events::RoomEvent<mtx::events::msg::File> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent);
        TimelineItem(AudioItem *audio,
                     const mtx::events::RoomEvent<mtx::events::msg::Audio> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent);
        TimelineItem(VideoItem *video,
                     const mtx::events::RoomEvent<mtx::events::msg::Video> &e,
                     bool with_sender,
                     const QString &room_id,
                     QWidget *parent);

        void setUserAvatar(const QImage &pixmap);
        DescInfo descriptionMessage() const { return descriptionMsg_; }
        QString eventId() const { return event_id_; }
        void setEventId(const QString &event_id) { event_id_ = event_id; }
        void markReceived(bool isEncrypted);
        void markSent();
        bool isReceived() { return isReceived_; };
        void setRoomId(QString room_id) { room_id_ = room_id; }
        void sendReadReceipt() const;

        //! Add a user avatar for this event.
        void addAvatar();
        void addKeyRequestAction();

signals:
        void eventRedacted(const QString &event_id);
        void redactionFailed(const QString &msg);

protected:
        void paintEvent(QPaintEvent *event) override;
        void contextMenuEvent(QContextMenuEvent *event) override;

private:
        void init();
        //! Add a context menu option to save the image of the timeline item.
        void addSaveImageAction(ImageItem *image);
        //! Add the reply action in the context menu for widgets that support it.
        void addReplyAction();

        template<class Widget>
        void setupLocalWidgetLayout(Widget *widget, const QString &userid, bool withSender);

        template<class Event, class Widget>
        void setupWidgetLayout(Widget *widget, const Event &event, bool withSender);

        void generateBody(const QString &body);
        void generateBody(const QString &user_id, const QString &displayname, const QString &body);
        void generateTimestamp(const QDateTime &time);

        void setupAvatarLayout(const QString &userName);
        void setupSimpleLayout();

        void adjustMessageLayout();
        void adjustMessageLayoutForWidget();

        //! Whether or not the event associated with the widget
        //! has been acknowledged by the server.
        bool isReceived_ = false;

        QString replaceEmoji(const QString &body);
        QString event_id_;
        QString room_id_;

        DescInfo descriptionMsg_;

        QMenu *contextMenu_;
        QAction *showReadReceipts_;
        QAction *markAsRead_;
        QAction *redactMsg_;
        QAction *replyMsg_;

        QHBoxLayout *topLayout_     = nullptr;
        QHBoxLayout *messageLayout_ = nullptr;
        QVBoxLayout *mainLayout_    = nullptr;
        QHBoxLayout *widgetLayout_  = nullptr;

        Avatar *userAvatar_;

        QFont font_;
        QFont usernameFont_;

        StatusIndicator *statusIndicator_;

        QLabel *timestamp_;
        QLabel *userName_;
        TextLabel *body_;
};

template<class Widget>
void
TimelineItem::setupLocalWidgetLayout(Widget *widget, const QString &userid, bool withSender)
{
        auto displayName = Cache::displayName(room_id_, userid);
        auto timestamp   = QDateTime::currentDateTime();

        descriptionMsg_ = {"You",
                           userid,
                           QString(" %1").arg(utils::messageDescription<Widget>()),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        widgetLayout_ = new QHBoxLayout;
        widgetLayout_->setContentsMargins(0, 2, 0, 2);
        widgetLayout_->addWidget(widget);
        widgetLayout_->addStretch(1);

        if (withSender) {
                generateBody(userid, displayName, "");
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, userid, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                setupSimpleLayout();
        }

        adjustMessageLayoutForWidget();
}

template<class Event, class Widget>
void
TimelineItem::setupWidgetLayout(Widget *widget, const Event &event, bool withSender)
{
        init();

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = Cache::displayName(room_id_, sender);

        QSettings settings;
        descriptionMsg_ = {sender == settings.value("auth/user_id") ? "You" : displayName,
                           sender,
                           QString(" %1").arg(utils::messageDescription<Widget>()),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        widgetLayout_ = new QHBoxLayout();
        widgetLayout_->setContentsMargins(0, 2, 0, 2);
        widgetLayout_->addWidget(widget);
        widgetLayout_->addStretch(1);

        if (withSender) {
                generateBody(sender, displayName, "");
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                setupSimpleLayout();
        }

        adjustMessageLayoutForWidget();
}
