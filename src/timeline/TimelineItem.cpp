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
#include <functional>

#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMenu>
#include <QTimer>

#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "MainWindow.h"
#include "Olm.h"
#include "ui/Avatar.h"
#include "ui/Painter.h"
#include "ui/TextLabel.h"

#include "timeline/TimelineItem.h"
#include "timeline/widgets/AudioItem.h"
#include "timeline/widgets/FileItem.h"
#include "timeline/widgets/ImageItem.h"
#include "timeline/widgets/VideoItem.h"

#include "dialogs/RawMessage.h"
#include "mtx/identifiers.hpp"

constexpr int MSG_RIGHT_MARGIN = 7;
constexpr int MSG_PADDING      = 20;

StatusIndicator::StatusIndicator(QWidget *parent)
  : QWidget(parent)
{
        lockIcon_.addFile(":/icons/icons/ui/lock.png");
        clockIcon_.addFile(":/icons/icons/ui/clock.png");
        checkmarkIcon_.addFile(":/icons/icons/ui/checkmark.png");
        doubleCheckmarkIcon_.addFile(":/icons/icons/ui/double-tick-indicator.png");
}

void
StatusIndicator::paintIcon(QPainter &p, QIcon &icon)
{
        auto pixmap = icon.pixmap(width());

        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), p.pen().color());

        QIcon(pixmap).paint(&p, rect(), Qt::AlignCenter, QIcon::Normal);
}

void
StatusIndicator::paintEvent(QPaintEvent *)
{
        if (state_ == StatusIndicatorState::Empty)
                return;

        Painter p(this);
        PainterHighQualityEnabler hq(p);

        p.setPen(iconColor_);

        switch (state_) {
        case StatusIndicatorState::Sent: {
                paintIcon(p, clockIcon_);
                break;
        }
        case StatusIndicatorState::Encrypted:
                paintIcon(p, lockIcon_);
                break;
        case StatusIndicatorState::Received: {
                paintIcon(p, checkmarkIcon_);
                break;
        }
        case StatusIndicatorState::Read: {
                paintIcon(p, doubleCheckmarkIcon_);
                break;
        }
        case StatusIndicatorState::Empty:
                break;
        }
}

void
StatusIndicator::setState(StatusIndicatorState state)
{
        state_ = state;

        switch (state) {
        case StatusIndicatorState::Encrypted:
                setToolTip(tr("Encrypted"));
                break;
        case StatusIndicatorState::Received:
                setToolTip(tr("Delivered"));
                break;
        case StatusIndicatorState::Read:
                setToolTip(tr("Seen"));
                break;
        case StatusIndicatorState::Sent:
                setToolTip(tr("Sent"));
                break;
        case StatusIndicatorState::Empty:
                setToolTip("");
                break;
        }

        update();
}

void
TimelineItem::adjustMessageLayoutForWidget()
{
        messageLayout_->addLayout(widgetLayout_, 1);
        actionLayout_->addWidget(replyBtn_);
        actionLayout_->addWidget(contextBtn_);
        messageLayout_->addLayout(actionLayout_);
        messageLayout_->addWidget(statusIndicator_);
        messageLayout_->addWidget(timestamp_);

        actionLayout_->setAlignment(replyBtn_, Qt::AlignTop | Qt::AlignRight);
        actionLayout_->setAlignment(contextBtn_, Qt::AlignTop | Qt::AlignRight);
        messageLayout_->setAlignment(statusIndicator_, Qt::AlignTop);
        messageLayout_->setAlignment(timestamp_, Qt::AlignTop);
        messageLayout_->setAlignment(actionLayout_, Qt::AlignTop);

        mainLayout_->addLayout(messageLayout_);
}

void
TimelineItem::adjustMessageLayout()
{
        messageLayout_->addWidget(body_, 1);
        actionLayout_->addWidget(replyBtn_);
        actionLayout_->addWidget(contextBtn_);
        messageLayout_->addLayout(actionLayout_);
        messageLayout_->addWidget(statusIndicator_);
        messageLayout_->addWidget(timestamp_);

        actionLayout_->setAlignment(replyBtn_, Qt::AlignTop | Qt::AlignRight);
        actionLayout_->setAlignment(contextBtn_, Qt::AlignTop | Qt::AlignRight);
        messageLayout_->setAlignment(statusIndicator_, Qt::AlignTop);
        messageLayout_->setAlignment(timestamp_, Qt::AlignTop);
        messageLayout_->setAlignment(actionLayout_, Qt::AlignTop);

        mainLayout_->addLayout(messageLayout_);
}

void
TimelineItem::init()
{
        userAvatar_      = nullptr;
        timestamp_       = nullptr;
        userName_        = nullptr;
        body_            = nullptr;
        auto buttonSize_ = 32;

        contextMenu_      = new QMenu(this);
        showReadReceipts_ = new QAction("Read receipts", this);
        markAsRead_       = new QAction("Mark as read", this);
        viewRawMessage_   = new QAction("View raw message", this);
        redactMsg_        = new QAction("Redact message", this);
        contextMenu_->addAction(showReadReceipts_);
        contextMenu_->addAction(viewRawMessage_);
        contextMenu_->addAction(markAsRead_);
        contextMenu_->addAction(redactMsg_);

        connect(showReadReceipts_, &QAction::triggered, this, [this]() {
                if (!event_id_.isEmpty())
                        MainWindow::instance()->openReadReceiptsDialog(event_id_);
        });

        connect(this, &TimelineItem::eventRedacted, this, [this](const QString &event_id) {
                emit ChatPage::instance()->removeTimelineEvent(room_id_, event_id);
        });
        connect(this, &TimelineItem::redactionFailed, this, [](const QString &msg) {
                emit ChatPage::instance()->showNotification(msg);
        });
        connect(redactMsg_, &QAction::triggered, this, [this]() {
                if (!event_id_.isEmpty())
                        http::client()->redact_event(
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
        connect(
          ChatPage::instance(), &ChatPage::themeChanged, this, &TimelineItem::refreshAuthorColor);
        connect(markAsRead_, &QAction::triggered, this, &TimelineItem::sendReadReceipt);
        connect(viewRawMessage_, &QAction::triggered, this, &TimelineItem::openRawMessageViewer);

        colorGenerating_ = new QFutureWatcher<QString>(this);
        connect(colorGenerating_,
                &QFutureWatcher<QString>::finished,
                this,
                &TimelineItem::finishedGeneratingColor);

        topLayout_     = new QHBoxLayout(this);
        mainLayout_    = new QVBoxLayout;
        messageLayout_ = new QHBoxLayout;
        actionLayout_  = new QHBoxLayout;
        messageLayout_->setContentsMargins(0, 0, MSG_RIGHT_MARGIN, 0);
        messageLayout_->setSpacing(MSG_PADDING);

        actionLayout_->setContentsMargins(13, 1, 13, 0);
        actionLayout_->setSpacing(0);

        topLayout_->setContentsMargins(
          conf::timeline::msgLeftMargin, conf::timeline::msgTopMargin, 0, 0);
        topLayout_->setSpacing(0);
        topLayout_->addLayout(mainLayout_);

        mainLayout_->setContentsMargins(conf::timeline::headerLeftMargin, 0, 0, 0);
        mainLayout_->setSpacing(0);

        replyBtn_ = new FlatButton(this);
        replyBtn_->setToolTip(tr("Reply"));
        replyBtn_->setFixedSize(buttonSize_, buttonSize_);
        replyBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon reply_icon;
        reply_icon.addFile(":/icons/icons/ui/mail-reply.png");
        replyBtn_->setIcon(reply_icon);
        replyBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));
        connect(replyBtn_, &FlatButton::clicked, this, &TimelineItem::replyAction);

        contextBtn_ = new FlatButton(this);
        contextBtn_->setToolTip(tr("Options"));
        contextBtn_->setFixedSize(buttonSize_, buttonSize_);
        contextBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon context_icon;
        context_icon.addFile(":/icons/icons/ui/vertical-ellipsis.png");
        contextBtn_->setIcon(context_icon);
        contextBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));
        contextBtn_->setMenu(contextMenu_);

        timestampFont_.setPointSizeF(timestampFont_.pointSizeF() * 0.9);
        timestampFont_.setFamily("Monospace");
        timestampFont_.setStyleHint(QFont::Monospace);

        QFontMetrics tsFm(timestampFont_);

        statusIndicator_ = new StatusIndicator(this);
        statusIndicator_->setFixedWidth(tsFm.height() - tsFm.leading());
        statusIndicator_->setFixedHeight(tsFm.height() - tsFm.leading());

        parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
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
        addReplyAction();

        auto displayName = Cache::displayName(room_id_, userid);
        auto timestamp   = QDateTime::currentDateTime();

        // Generate the html body to be rendered.
        auto formatted_body = utils::markdownToHtml(body);

        // Escape html if the input is not formatted.
        if (formatted_body == body.trimmed().toHtmlEscaped())
                formatted_body = body.toHtmlEscaped();

        QString emptyEventId;

        if (ty == mtx::events::MessageType::Emote) {
                formatted_body  = QString("<em>%1</em>").arg(formatted_body);
                descriptionMsg_ = {emptyEventId,
                                   "",
                                   userid,
                                   QString("* %1 %2").arg(displayName).arg(body),
                                   utils::descriptiveTime(timestamp),
                                   timestamp};
        } else {
                descriptionMsg_ = {emptyEventId,
                                   "You: ",
                                   userid,
                                   body,
                                   utils::descriptiveTime(timestamp),
                                   timestamp};
        }

        formatted_body = utils::linkifyMessage(formatted_body);

        generateTimestamp(timestamp);

        if (withSender) {
                generateBody(userid, displayName, formatted_body);
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, userid, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(formatted_body);
                setupSimpleLayout();
        }

        adjustMessageLayout();
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

        markOwnMessagesAsReceived(event.sender);

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

        markOwnMessagesAsReceived(event.sender);

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

        markOwnMessagesAsReceived(event.sender);
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

        markOwnMessagesAsReceived(event.sender);
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

        markOwnMessagesAsReceived(event.sender);
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
        addReplyAction();

        markOwnMessagesAsReceived(event.sender);

        event_id_            = QString::fromStdString(event.event_id);
        const auto sender    = QString::fromStdString(event.sender);
        const auto timestamp = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);

        auto formatted_body = utils::linkifyMessage(utils::getMessageBody(event).trimmed());
        auto body           = QString::fromStdString(event.content.body).trimmed().toHtmlEscaped();

        descriptionMsg_ = {event_id_,
                           Cache::displayName(room_id_, sender),
                           sender,
                           " sent a notification",
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        if (with_sender) {
                auto displayName = Cache::displayName(room_id_, sender);

                generateBody(sender, displayName, formatted_body);
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(formatted_body);
                setupSimpleLayout();
        }

        adjustMessageLayout();
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
        addReplyAction();

        markOwnMessagesAsReceived(event.sender);

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto formatted_body = utils::linkifyMessage(utils::getMessageBody(event).trimmed());
        auto body           = QString::fromStdString(event.content.body).trimmed().toHtmlEscaped();

        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = Cache::displayName(room_id_, sender);
        formatted_body   = QString("<em>%1</em>").arg(formatted_body);

        descriptionMsg_ = {event_id_,
                           "",
                           sender,
                           QString("* %1 %2").arg(displayName).arg(body),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        if (with_sender) {
                generateBody(sender, displayName, formatted_body);
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(formatted_body);
                setupSimpleLayout();
        }

        adjustMessageLayout();
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
        addReplyAction();

        markOwnMessagesAsReceived(event.sender);

        event_id_         = QString::fromStdString(event.event_id);
        const auto sender = QString::fromStdString(event.sender);

        auto formatted_body = utils::linkifyMessage(utils::getMessageBody(event).trimmed());
        auto body           = QString::fromStdString(event.content.body).trimmed().toHtmlEscaped();

        auto timestamp   = QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
        auto displayName = Cache::displayName(room_id_, sender);

        QSettings settings;
        descriptionMsg_ = {event_id_,
                           sender == settings.value("auth/user_id") ? "You" : displayName,
                           sender,
                           QString(": %1").arg(body),
                           utils::descriptiveTime(timestamp),
                           timestamp};

        generateTimestamp(timestamp);

        if (with_sender) {
                generateBody(sender, displayName, formatted_body);
                setupAvatarLayout(displayName);

                AvatarProvider::resolve(
                  room_id_, sender, this, [this](const QImage &img) { setUserAvatar(img); });
        } else {
                generateBody(formatted_body);
                setupSimpleLayout();
        }

        adjustMessageLayout();
}

TimelineItem::~TimelineItem()
{
        colorGenerating_->cancel();
        colorGenerating_->waitForFinished();
}

void
TimelineItem::markSent()
{
        statusIndicator_->setState(StatusIndicatorState::Sent);
}

void
TimelineItem::markOwnMessagesAsReceived(const std::string &sender)
{
        QSettings settings;
        if (sender == settings.value("auth/user_id").toString().toStdString())
                statusIndicator_->setState(StatusIndicatorState::Received);
}

void
TimelineItem::markRead()
{
        if (statusIndicator_->state() != StatusIndicatorState::Encrypted)
                statusIndicator_->setState(StatusIndicatorState::Read);
}

void
TimelineItem::markReceived(bool isEncrypted)
{
        isReceived_ = true;

        if (isEncrypted)
                statusIndicator_->setState(StatusIndicatorState::Encrypted);
        else
                statusIndicator_->setState(StatusIndicatorState::Received);

        sendReadReceipt();
}

// Only the body is displayed.
void
TimelineItem::generateBody(const QString &body)
{
        body_ = new TextLabel(replaceEmoji(body), this);
        body_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);

        connect(body_, &TextLabel::userProfileTriggered, this, [](const QString &user_id) {
                MainWindow::instance()->openUserProfile(user_id,
                                                        ChatPage::instance()->currentRoom());
        });
}

void
TimelineItem::refreshAuthorColor()
{
        // Cancel and wait if we are already generating the color.
        if (colorGenerating_->isRunning()) {
                colorGenerating_->cancel();
                colorGenerating_->waitForFinished();
        }
        if (userName_) {
                // generate user's unique color.
                std::function<QString()> generate = [this]() {
                        QString userColor = utils::generateContrastingHexColor(
                          userName_->toolTip(), backgroundColor().name());
                        return userColor;
                };

                QString userColor = Cache::userColor(userName_->toolTip());

                // If the color is empty, then generate it asynchronously
                if (userColor.isEmpty()) {
                        colorGenerating_->setFuture(QtConcurrent::run(generate));
                } else {
                        userName_->setStyleSheet("QLabel { color : " + userColor + "; }");
                }
        }
}

void
TimelineItem::finishedGeneratingColor()
{
        nhlog::ui()->debug("finishedGeneratingColor for: {}", userName_->toolTip().toStdString());
        QString userColor = colorGenerating_->result();

        if (!userColor.isEmpty()) {
                // another TimelineItem might have inserted in the meantime.
                if (Cache::userColor(userName_->toolTip()).isEmpty()) {
                        Cache::insertUserColor(userName_->toolTip(), userColor);
                }
                userName_->setStyleSheet("QLabel { color : " + userColor + "; }");
        }
}
// The username/timestamp is displayed along with the message body.
void
TimelineItem::generateBody(const QString &user_id, const QString &displayname, const QString &body)
{
        generateUserName(user_id, displayname);
        generateBody(body);
}

void
TimelineItem::generateUserName(const QString &user_id, const QString &displayname)
{
        auto sender = displayname;

        if (displayname.startsWith("@")) {
                // TODO: Fix this by using a UserId type.
                if (displayname.split(":")[0].split("@").size() > 1)
                        sender = displayname.split(":")[0].split("@")[1];
        }

        QFont usernameFont;
        usernameFont.setPointSizeF(usernameFont.pointSizeF() * 1.1);
        usernameFont.setWeight(QFont::Medium);

        QFontMetrics fm(usernameFont);

        userName_ = new QLabel(this);
        userName_->setFont(usernameFont);
        userName_->setText(fm.elidedText(sender, Qt::ElideRight, 500));
        userName_->setToolTip(user_id);
        userName_->setToolTipDuration(1500);
        userName_->setAttribute(Qt::WA_Hover);
        userName_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        userName_->setFixedWidth(QFontMetrics(userName_->font()).width(userName_->text()));

        // Set the user color asynchronously if it hasn't been generated yet,
        // otherwise this will just set it.
        refreshAuthorColor();

        auto filter = new UserProfileFilter(user_id, userName_);
        userName_->installEventFilter(filter);
        userName_->setCursor(Qt::PointingHandCursor);

        connect(filter, &UserProfileFilter::hoverOn, this, [this]() {
                QFont f = userName_->font();
                f.setUnderline(true);
                userName_->setFont(f);
        });

        connect(filter, &UserProfileFilter::hoverOff, this, [this]() {
                QFont f = userName_->font();
                f.setUnderline(false);
                userName_->setFont(f);
        });

        connect(filter, &UserProfileFilter::clicked, this, [this, user_id]() {
                MainWindow::instance()->openUserProfile(user_id, room_id_);
        });
}

void
TimelineItem::generateTimestamp(const QDateTime &time)
{
        timestamp_ = new QLabel(this);
        timestamp_->setFont(timestampFont_);
        timestamp_->setText(
          QString("<span style=\"color: #999\"> %1 </span>").arg(time.toString("HH:mm")));
}

QString
TimelineItem::replaceEmoji(const QString &body)
{
        QString fmtBody = "";

        QVector<uint> utf32_string = body.toUcs4();

        for (auto &code : utf32_string) {
                // TODO: Be more precise here.
                if (code > 9000)
                        fmtBody += QString("<span style=\"font-family:  emoji;\">") +
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

        QFont f;
        f.setPointSizeF(f.pointSizeF());

        userAvatar_ = new Avatar(this);
        userAvatar_->setLetter(QChar(userName[0]).toUpper());
        userAvatar_->setSize(QFontMetrics(f).height() * 2);

        // TODO: The provided user name should be a UserId class
        if (userName[0] == '@' && userName.size() > 1)
                userAvatar_->setLetter(QChar(userName[1]).toUpper());

        topLayout_->insertWidget(0, userAvatar_);
        topLayout_->setAlignment(userAvatar_, Qt::AlignTop | Qt::AlignLeft);

        if (userName_)
                mainLayout_->insertWidget(0, userName_, Qt::AlignTop | Qt::AlignLeft);
}

void
TimelineItem::setupSimpleLayout()
{
        QFont f;
        f.setPointSizeF(f.pointSizeF());

        topLayout_->setContentsMargins(conf::timeline::msgLeftMargin +
                                         QFontMetrics(f).height() * 2 + 2,
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
TimelineItem::addReplyAction()
{
        if (contextMenu_) {
                auto replyAction = new QAction("Reply", this);
                contextMenu_->addAction(replyAction);

                connect(replyAction, &QAction::triggered, this, &TimelineItem::replyAction);
        }
}

void
TimelineItem::replyAction()
{
        if (!body_)
                return;

        emit ChatPage::instance()->messageReply(
          Cache::displayName(room_id_, descriptionMsg_.userid), body_->toPlainText());
}

void
TimelineItem::addKeyRequestAction()
{
        if (contextMenu_) {
                auto requestKeys = new QAction("Request encryption keys", this);
                contextMenu_->addAction(requestKeys);

                connect(requestKeys, &QAction::triggered, this, [this]() {
                        olm::request_keys(room_id_.toStdString(), event_id_.toStdString());
                });
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

        generateUserName(userid, displayName);

        setupAvatarLayout(displayName);

        AvatarProvider::resolve(
          room_id_, userid, this, [this](const QImage &img) { setUserAvatar(img); });
}

void
TimelineItem::sendReadReceipt() const
{
        if (!event_id_.isEmpty())
                http::client()->read_event(room_id_.toStdString(),
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

void
TimelineItem::openRawMessageViewer() const
{
        const auto event_id = event_id_.toStdString();
        const auto room_id  = room_id_.toStdString();

        auto proxy = std::make_shared<EventProxy>();
        connect(proxy.get(), &EventProxy::eventRetrieved, this, [](const nlohmann::json &obj) {
                auto dialog = new dialogs::RawMessage{QString::fromStdString(obj.dump(4))};
                Q_UNUSED(dialog);
        });

        http::client()->get_event(
          room_id,
          event_id,
          [event_id, room_id, proxy = std::move(proxy)](
            const mtx::events::collections::TimelineEvents &res, mtx::http::RequestErr err) {
                  using namespace mtx::events;

                  if (err) {
                          nhlog::net()->warn(
                            "failed to retrieve event {} from {}", event_id, room_id);
                          return;
                  }

                  try {
                          emit proxy->eventRetrieved(utils::serialize_event(res));
                  } catch (const nlohmann::json::exception &e) {
                          nhlog::net()->warn(
                            "failed to serialize event ({}, {})", room_id, event_id);
                  }
          });
}