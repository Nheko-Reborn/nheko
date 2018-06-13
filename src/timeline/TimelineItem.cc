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

#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QMenu>
#include <QTimer>

#include "Avatar.h"
#include "ChatPage.h"
#include "Config.h"
#include "Logging.hpp"

#include "timeline/TimelineItem.h"
#include "timeline/widgets/AudioItem.h"
#include "timeline/widgets/FileItem.h"
#include "timeline/widgets/ImageItem.h"
#include "timeline/widgets/VideoItem.h"

constexpr const static char *CHECKMARK = "✓";

constexpr int MSG_RIGHT_MARGIN = 7;
constexpr int MSG_PADDING      = 20;

void
TimelineItem::init()
{
        userAvatar_ = nullptr;
        timestamp_  = nullptr;
        userName_   = nullptr;
        body_       = nullptr;

        font_.setPixelSize(conf::fontSize);
        usernameFont_ = font_;
        usernameFont_.setWeight(60);

        QFontMetrics fm(font_);

        contextMenu_      = new QMenu(this);
        showReadReceipts_ = new QAction("Read receipts", this);
        markAsRead_       = new QAction("Mark as read", this);
        redactMsg_        = new QAction("Redact message", this);
        contextMenu_->addAction(showReadReceipts_);
        contextMenu_->addAction(markAsRead_);
        contextMenu_->addAction(redactMsg_);

        connect(showReadReceipts_, &QAction::triggered, this, [this]() {
                if (!event_id_.isEmpty())
                        ChatPage::instance()->showReadReceipts(event_id_);
        });

        connect(this, &TimelineItem::eventRedacted, this, [this](const QString &event_id) {
                emit ChatPage::instance()->removeTimelineEvent(room_id_, event_id);
        });
        connect(this, &TimelineItem::redactionFailed, this, [](const QString &msg) {
                emit ChatPage::instance()->showNotification(msg);
        });
        connect(redactMsg_, &QAction::triggered, this, [this]() {
                if (!event_id_.isEmpty())
                        http::v2::client()->redact_event(
                          room_id_.toStdString(),
                          event_id_.toStdString(),
                          [this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                                  if (err) {
                                          emit redactionFailed(tr("Message redaction failed: %1")
                                                                 .arg(QString::fromStdString(
                                                                   err->matrix_error.error)));
                                          return;
                                  }

                                  emit eventRedacted(event_id_);
                          });
        });

        connect(markAsRead_, &QAction::triggered, this, [this]() { sendReadReceipt(); });

        topLayout_     = new QHBoxLayout(this);
        mainLayout_    = new QVBoxLayout;
        messageLayout_ = new QHBoxLayout;
        messageLayout_->setContentsMargins(0, 0, MSG_RIGHT_MARGIN, 0);
        messageLayout_->setSpacing(MSG_PADDING);

        topLayout_->setContentsMargins(
          conf::timeline::msgLeftMargin, conf::timeline::msgTopMargin, 0, 0);
        topLayout_->setSpacing(0);
        topLayout_->addLayout(mainLayout_, 1);

        mainLayout_->setContentsMargins(conf::timeline::headerLeftMargin, 0, 0, 0);
        mainLayout_->setSpacing(0);

        QFont checkmarkFont;
        checkmarkFont.setPixelSize(conf::timeline::fonts::timestamp);

        // Setting fixed width for checkmark because systems may have a differing width for a
        // space and the Unicode checkmark.
        checkmark_ = new QLabel(this);
        checkmark_->setFont(checkmarkFont);
        checkmark_->setFixedWidth(QFontMetrics{checkmarkFont}.width(CHECKMARK));
}

/*
 * For messages created locally.
 */
TimelineItem::TimelineItem(mtx::events::MessageType ty,
                           const QString &userid,
                           QString body,
                           bool withSender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        init();

        auto displayName = Cache::displayName(room_id_, userid);
        auto timestamp   = QDateTime::currentDateTime();

        if (ty == mtx::events::MessageType::Emote) {
                body            = QString("* %1 %2").arg(displayName).arg(body);
                descriptionMsg_ = {"", userid, body, utils::descriptiveTime(timestamp), timestamp};
        } else {
                descriptionMsg_ = {
                  "You: ", userid, body, utils::descriptiveTime(timestamp), timestamp};
        }

        body = body.toHtmlEscaped();
        body.replace(conf::strings::url_regex, conf::strings::url_html);
        body.replace("\n", "<br/>");
        generateTimestamp(timestamp);

        if (withSender) {
                generateBody(userid, displayName, body);
                setupAvatarLayout(displayName);

                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  room_id_, userid, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(body);
                setupSimpleLayout();

                messageLayout_->addWidget(body_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}

TimelineItem::TimelineItem(ImageItem *image,
                           const QString &userid,
                           bool withSender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget{parent}
  , room_id_{room_id}
{
        init();

        setupLocalWidgetLayout<ImageItem>(image, userid, withSender);

        addSaveImageAction(image);
}

TimelineItem::TimelineItem(FileItem *file,
                           const QString &userid,
                           bool withSender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget{parent}
  , room_id_{room_id}
{
        init();

        setupLocalWidgetLayout<FileItem>(file, userid, withSender);
}

TimelineItem::TimelineItem(AudioItem *audio,
                           const QString &userid,
                           bool withSender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget{parent}
  , room_id_{room_id}
{
        init();

        setupLocalWidgetLayout<AudioItem>(audio, userid, withSender);
}

TimelineItem::TimelineItem(VideoItem *video,
                           const QString &userid,
                           bool withSender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget{parent}
  , room_id_{room_id}
{
        init();

        setupLocalWidgetLayout<VideoItem>(video, userid, withSender);
}

TimelineItem::TimelineItem(ImageItem *image,
                           const mtx::events::RoomEvent<mtx::events::msg::Image> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        setupWidgetLayout<mtx::events::RoomEvent<mtx::events::msg::Image>, ImageItem>(
          image, event, with_sender);

        addSaveImageAction(image);
}

TimelineItem::TimelineItem(StickerItem *image,
                           const mtx::events::Sticker &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        setupWidgetLayout<mtx::events::Sticker, StickerItem>(image, event, with_sender);

        addSaveImageAction(image);
}

TimelineItem::TimelineItem(FileItem *file,
                           const mtx::events::RoomEvent<mtx::events::msg::File> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        setupWidgetLayout<mtx::events::RoomEvent<mtx::events::msg::File>, FileItem>(
          file, event, with_sender);
}

TimelineItem::TimelineItem(AudioItem *audio,
                           const mtx::events::RoomEvent<mtx::events::msg::Audio> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        setupWidgetLayout<mtx::events::RoomEvent<mtx::events::msg::Audio>, AudioItem>(
          audio, event, with_sender);
}

TimelineItem::TimelineItem(VideoItem *video,
                           const mtx::events::RoomEvent<mtx::events::msg::Video> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        setupWidgetLayout<mtx::events::RoomEvent<mtx::events::msg::Video>, VideoItem>(
          video, event, with_sender);
}

/*
 * Used to display remote notice messages.
 */
TimelineItem::TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Notice> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        init();

        event_id_            = QString::fromStdString(event.event_id);
        const auto sender    = QString::fromStdString(event.sender);
        const auto timestamp = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto body            = QString::fromStdString(event.content.body).trimmed().toHtmlEscaped();

        descriptionMsg_ = {Cache::displayName(room_id_, sender),
                           sender,
                           " sent a notification",
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        body.replace(conf::strings::url_regex, conf::strings::url_html);
        body.replace("\n", "<br/>");
        body = "<i>" + body + "</i>";

        if (with_sender) {
                auto displayName = Cache::displayName(room_id_, sender);

                generateBody(sender, displayName, body);
                setupAvatarLayout(displayName);

                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(body);
                setupSimpleLayout();

                messageLayout_->addWidget(body_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}

/*
 * Used to display remote emote messages.
 */
TimelineItem::TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Emote> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        init();

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto body        = QString::fromStdString(event.content.body).trimmed();
        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = Cache::displayName(room_id_, sender);
        auto emoteMsg    = QString("* %1 %2").arg(displayName).arg(body);

        descriptionMsg_ = {"", sender, emoteMsg, utils::descriptiveTime(timestamp), timestamp};

        generateTimestamp(timestamp);
        emoteMsg = emoteMsg.toHtmlEscaped();
        emoteMsg.replace(conf::strings::url_regex, conf::strings::url_html);
        emoteMsg.replace("\n", "<br/>");

        if (with_sender) {
                generateBody(sender, displayName, emoteMsg);
                setupAvatarLayout(displayName);

                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(emoteMsg);
                setupSimpleLayout();

                messageLayout_->addWidget(body_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}

/*
 * Used to display remote text messages.
 */
TimelineItem::TimelineItem(const mtx::events::RoomEvent<mtx::events::msg::Text> &event,
                           bool with_sender,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
{
        init();

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto body        = QString::fromStdString(event.content.body).trimmed();
        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = Cache::displayName(room_id_, sender);

        QSettings settings;
        descriptionMsg_ = {sender == settings.value("auth/user_id") ? "You" : displayName,
                           sender,
                           QString(": %1").arg(body),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        body = body.toHtmlEscaped();
        body.replace(conf::strings::url_regex, conf::strings::url_html);
        body.replace("\n", "<br/>");

        if (with_sender) {
                generateBody(sender, displayName, body);
                setupAvatarLayout(displayName);

                messageLayout_->addLayout(headerLayout_, 1);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(body);
                setupSimpleLayout();

                messageLayout_->addWidget(body_, 1);
        }

        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);
        mainLayout_->addLayout(messageLayout_);
}

void
TimelineItem::markReceived()
{
        checkmark_->setText(CHECKMARK);
        checkmark_->setAlignment(Qt::AlignTop);

        sendReadReceipt();
}

// Only the body is displayed.
void
TimelineItem::generateBody(const QString &body)
{
        if (body.isEmpty())
                return;

        QString content("<span>%1</span>");

        body_ = new TextLabel(content.arg(replaceEmoji(body)), this);
        body_->setFont(font_);
        body_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
}

// The username/timestamp is displayed along with the message body.
void
TimelineItem::generateBody(const QString &user_id, const QString &displayname, const QString &body)
{
        auto sender = displayname;

        if (displayname.startsWith("@")) {
                // TODO: Fix this by using a UserId type.
                if (displayname.split(":")[0].split("@").size() > 1)
                        sender = displayname.split(":")[0].split("@")[1];
        }

        QFontMetrics fm(usernameFont_);

        userName_ = new QLabel(this);
        userName_->setFont(usernameFont_);
        userName_->setText(fm.elidedText(sender, Qt::ElideRight, 500));
        userName_->setToolTip(user_id);
        userName_->setToolTipDuration(1500);
        userName_->setAttribute(Qt::WA_Hover);
        userName_->setAlignment(Qt::AlignLeft);
        userName_->setFixedWidth(QFontMetrics(userName_->font()).width(userName_->text()));

        auto filter = new UserProfileFilter(user_id, userName_);
        userName_->installEventFilter(filter);

        connect(filter, &UserProfileFilter::hoverOn, this, [this]() {
                QFont f = userName_->font();
                f.setUnderline(true);
                userName_->setCursor(Qt::PointingHandCursor);
                userName_->setFont(f);
        });

        connect(filter, &UserProfileFilter::hoverOff, this, [this]() {
                QFont f = userName_->font();
                f.setUnderline(false);
                userName_->setCursor(Qt::ArrowCursor);
                userName_->setFont(f);
        });

        generateBody(body);
}

void
TimelineItem::generateTimestamp(const QDateTime &time)
{
        QFont timestampFont;
        timestampFont.setPixelSize(conf::timeline::fonts::timestamp);

        QFontMetrics fm(timestampFont);
        int topMargin = QFontMetrics(font_).ascent() - fm.ascent();

        timestamp_ = new QLabel(this);
        timestamp_->setAlignment(Qt::AlignTop);
        timestamp_->setFont(timestampFont);
        timestamp_->setText(
          QString("<span style=\"color: #999\"> %1 </span>").arg(time.toString("HH:mm")));
        timestamp_->setContentsMargins(0, topMargin, 0, 0);
        timestamp_->setStyleSheet(
          QString("font-size: %1px;").arg(conf::timeline::fonts::timestamp));
}

QString
TimelineItem::replaceEmoji(const QString &body)
{
        QString fmtBody = "";

        QVector<uint> utf32_string = body.toUcs4();

        for (auto &code : utf32_string) {
                // TODO: Be more precise here.
                if (code > 9000)
                        fmtBody += QString("<span style=\"font-family: Emoji "
                                           "One; font-size: %1px\">")
                                     .arg(conf::emojiSize) +
                                   QString::fromUcs4(&code, 1) + "</span>";
                else
                        fmtBody += QString::fromUcs4(&code, 1);
        }

        return fmtBody;
}

void
TimelineItem::setupAvatarLayout(const QString &userName)
{
        topLayout_->setContentsMargins(
          conf::timeline::msgLeftMargin, conf::timeline::msgAvatarTopMargin, 0, 0);

        userAvatar_ = new Avatar(this);
        userAvatar_->setLetter(QChar(userName[0]).toUpper());
        userAvatar_->setSize(conf::timeline::avatarSize);

        // TODO: The provided user name should be a UserId class
        if (userName[0] == '@' && userName.size() > 1)
                userAvatar_->setLetter(QChar(userName[1]).toUpper());

        topLayout_->insertWidget(0, userAvatar_);
        topLayout_->setAlignment(userAvatar_, Qt::AlignTop);

        headerLayout_ = new QVBoxLayout;
        headerLayout_->setMargin(0);
        headerLayout_->setSpacing(conf::timeline::headerSpacing);

        if (userName_)
                headerLayout_->addWidget(userName_);

        if (body_)
                headerLayout_->addWidget(body_);
}

void
TimelineItem::setupSimpleLayout()
{
        topLayout_->setContentsMargins(conf::timeline::msgLeftMargin + conf::timeline::avatarSize +
                                         2,
                                       conf::timeline::msgTopMargin,
                                       0,
                                       0);
}

void
TimelineItem::setUserAvatar(const QImage &avatar)
{
        if (userAvatar_ == nullptr)
                return;

        userAvatar_->setImage(avatar);
}

void
TimelineItem::contextMenuEvent(QContextMenuEvent *event)
{
        if (contextMenu_)
                contextMenu_->exec(event->globalPos());
}

void
TimelineItem::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
TimelineItem::addSaveImageAction(ImageItem *image)
{
        if (contextMenu_) {
                auto saveImage = new QAction("Save image", this);
                contextMenu_->addAction(saveImage);

                connect(saveImage, &QAction::triggered, image, &ImageItem::saveAs);
        }
}

void
TimelineItem::addAvatar()
{
        if (userAvatar_)
                return;

        // TODO: should be replaced with the proper event struct.
        auto userid      = descriptionMsg_.userid;
        auto displayName = Cache::displayName(room_id_, userid);

        QFontMetrics fm(usernameFont_);
        userName_ = new QLabel(this);
        userName_->setFont(usernameFont_);
        userName_->setText(fm.elidedText(displayName, Qt::ElideRight, 500));

        QWidget *widget = nullptr;

        // Extract the widget before we delete its layout.
        if (widgetLayout_)
                widget = widgetLayout_->itemAt(0)->widget();

        // Remove all items from the layout.
        QLayoutItem *item;
        while ((item = messageLayout_->takeAt(0)) != 0)
                delete item;

        setupAvatarLayout(displayName);

        // Restore widget's layout.
        if (widget) {
                widgetLayout_ = new QHBoxLayout();
                widgetLayout_->setContentsMargins(0, 2, 0, 2);
                widgetLayout_->addWidget(widget);
                widgetLayout_->addStretch(1);

                headerLayout_->addLayout(widgetLayout_);
        }

        messageLayout_->addLayout(headerLayout_, 1);
        messageLayout_->addWidget(checkmark_);
        messageLayout_->addWidget(timestamp_);

        AvatarProvider::resolve(
          room_id_, userid, this, [this](const QImage &img) { setUserAvatar(img); });
}

void
TimelineItem::sendReadReceipt() const
{
        if (!event_id_.isEmpty())
                http::v2::client()->read_event(room_id_.toStdString(),
                                               event_id_.toStdString(),
                                               [this](mtx::http::RequestErr err) {
                                                       if (err) {
                                                               nhlog::net()->warn(
                                                                 "failed to read_event ({}, {})",
                                                                 room_id_.toStdString(),
                                                                 event_id_.toStdString());
                                                       }
                                               });
}
