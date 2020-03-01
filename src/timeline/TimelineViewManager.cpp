#include "TimelineViewManager.h"

#include <QMetaType>
#include <QPalette>
#include <QQmlContext>

#include "BlurhashProvider.h"
#include "ChatPage.h"
#include "ColorImageProvider.h"
#include "DelegateChooser.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "UserSettingsPage.h"
#include "dialogs/ImageOverlay.h"

Q_DECLARE_METATYPE(mtx::events::collections::TimelineEvents)

void
TimelineViewManager::updateColorPalette()
{
        userColors.clear();

        if (settings->theme() == "light") {
                QPalette lightActive(/*windowText*/ QColor("#333"),
                                     /*button*/ QColor("#333"),
                                     /*light*/ QColor(),
                                     /*dark*/ QColor(220, 220, 220),
                                     /*mid*/ QColor(),
                                     /*text*/ QColor("#333"),
                                     /*bright_text*/ QColor(),
                                     /*base*/ QColor(220, 220, 220),
                                     /*window*/ QColor("white"));
                lightActive.setColor(QPalette::ToolTipBase, lightActive.base().color());
                lightActive.setColor(QPalette::ToolTipText, lightActive.text().color());
                lightActive.setColor(QPalette::Link, QColor("#0077b5"));
                view->rootContext()->setContextProperty("currentActivePalette", lightActive);
                view->rootContext()->setContextProperty("currentInactivePalette", lightActive);
        } else if (settings->theme() == "dark") {
                QPalette darkActive(/*windowText*/ QColor("#caccd1"),
                                    /*button*/ QColor("#caccd1"),
                                    /*light*/ QColor(),
                                    /*dark*/ QColor("#2d3139"),
                                    /*mid*/ QColor(),
                                    /*text*/ QColor("#caccd1"),
                                    /*bright_text*/ QColor(),
                                    /*base*/ QColor("#2d3139"),
                                    /*window*/ QColor("#202228"));
                darkActive.setColor(QPalette::Highlight, QColor("#e7e7e9"));
                darkActive.setColor(QPalette::ToolTipBase, darkActive.base().color());
                darkActive.setColor(QPalette::ToolTipText, darkActive.text().color());
                darkActive.setColor(QPalette::Link, QColor("#38a3d8"));
                view->rootContext()->setContextProperty("currentActivePalette", darkActive);
                view->rootContext()->setContextProperty("currentInactivePalette", darkActive);
        } else {
                view->rootContext()->setContextProperty("currentActivePalette", QPalette());
                view->rootContext()->setContextProperty("currentInactivePalette", nullptr);
        }
}

QColor
TimelineViewManager::userColor(QString id, QColor background)
{
        if (!userColors.contains(id))
                userColors.insert(
                  id, QColor(utils::generateContrastingHexColor(id, background.name())));
        return userColors.value(id);
}

TimelineViewManager::TimelineViewManager(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : imgProvider(new MxcImageProvider())
  , colorImgProvider(new ColorImageProvider())
  , blurhashProvider(new BlurhashProvider())
  , settings(userSettings)
{
        qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                         "im.nheko",
                                         1,
                                         0,
                                         "MtxEvent",
                                         "Can't instantiate enum!");
        qmlRegisterType<DelegateChoice>("im.nheko", 1, 0, "DelegateChoice");
        qmlRegisterType<DelegateChooser>("im.nheko", 1, 0, "DelegateChooser");
        qRegisterMetaType<mtx::events::collections::TimelineEvents>();

#ifdef USE_QUICK_VIEW
        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
#else
        view      = new QQuickWidget(parent);
        container = view;
        view->setResizeMode(QQuickWidget::SizeRootObjectToView);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(view, &QQuickWidget::statusChanged, this, [](QQuickWidget::Status status) {
                nhlog::ui()->debug("Status changed to {}", status);
        });
#endif
        container->setMinimumSize(200, 200);
        view->rootContext()->setContextProperty("timelineManager", this);
        updateColorPalette();
        view->engine()->addImageProvider("MxcImage", imgProvider);
        view->engine()->addImageProvider("colorimage", colorImgProvider);
        view->engine()->addImageProvider("blurhash", blurhashProvider);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));

        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::themeChanged,
                this,
                &TimelineViewManager::updateColorPalette);
}

void
TimelineViewManager::sync(const mtx::responses::Rooms &rooms)
{
        for (const auto &[room_id, room] : rooms.join) {
                // addRoom will only add the room, if it doesn't exist
                addRoom(QString::fromStdString(room_id));
                const auto &room_model = models.value(QString::fromStdString(room_id));
                room_model->addEvents(room.timeline);

                if (ChatPage::instance()->userSettings()->isTypingNotificationsEnabled()) {
                        std::vector<QString> typing;
                        typing.reserve(room.ephemeral.typing.size());
                        for (const auto &user : room.ephemeral.typing) {
                                if (user != http::client()->user_id().to_string())
                                        typing.push_back(QString::fromStdString(user));
                        }
                        room_model->updateTypingUsers(typing);
                }
        }

        this->isInitialSync_ = false;
        emit initialSyncChanged(false);
}

void
TimelineViewManager::addRoom(const QString &room_id)
{
        if (!models.contains(room_id)) {
                QSharedPointer<TimelineModel> newRoom(new TimelineModel(this, room_id));
                connect(newRoom.data(),
                        &TimelineModel::newEncryptedImage,
                        imgProvider,
                        &MxcImageProvider::addEncryptionInfo);
                models.insert(room_id, std::move(newRoom));
        }
}

void
TimelineViewManager::setHistoryView(const QString &room_id)
{
        nhlog::ui()->info("Trying to activate room {}", room_id.toStdString());

        auto room = models.find(room_id);
        if (room != models.end()) {
                timeline_ = room.value().data();
                emit activeTimelineChanged(timeline_);
                nhlog::ui()->info("Activated room {}", room_id.toStdString());
        }
}

void
TimelineViewManager::openImageOverlay(QString mxcUrl, QString eventId) const
{
        QQuickImageResponse *imgResponse =
          imgProvider->requestImageResponse(mxcUrl.remove("mxc://"), QSize());
        connect(imgResponse, &QQuickImageResponse::finished, this, [this, eventId, imgResponse]() {
                if (!imgResponse->errorString().isEmpty()) {
                        nhlog::ui()->error("Error when retrieving image for overlay: {}",
                                           imgResponse->errorString().toStdString());
                        return;
                }
                auto pixmap = QPixmap::fromImage(imgResponse->textureFactory()->image());

                auto imgDialog = new dialogs::ImageOverlay(pixmap);
                imgDialog->showFullScreen();
                connect(imgDialog, &dialogs::ImageOverlay::saving, timeline_, [this, eventId]() {
                        timeline_->saveMedia(eventId);
                });
        });
}

void
TimelineViewManager::updateReadReceipts(const QString &room_id,
                                        const std::vector<QString> &event_ids)
{
        auto room = models.find(room_id);
        if (room != models.end()) {
                room.value()->markEventsAsRead(event_ids);
        }
}

void
TimelineViewManager::initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs)
{
        for (const auto &e : msgs) {
                addRoom(e.first);

                models.value(e.first)->addEvents(e.second);
        }
}

void
TimelineViewManager::queueTextMessage(const QString &msg, const std::optional<RelatedInfo> &related)
{
        mtx::events::msg::Text text = {};
        text.body                   = msg.trimmed().toStdString();

        if (settings->isMarkdownEnabled()) {
                text.formatted_body = utils::markdownToHtml(msg).toStdString();

                // Don't send formatted_body, when we don't need to
                if (text.formatted_body.find("<") == std::string::npos)
                        text.formatted_body = "";
                else
                        text.format = "org.matrix.custom.html";
        }

        if (related) {
                QString body;
                bool firstLine = true;
                for (const auto &line : related->quoted_body.split("\n")) {
                        if (firstLine) {
                                firstLine = false;
                                body = QString("> <%1> %2\n").arg(related->quoted_user).arg(line);
                        } else {
                                body = QString("%1\n> %2\n").arg(body).arg(line);
                        }
                }

                text.body = QString("%1\n%2").arg(body).arg(msg).toStdString();

                // NOTE(Nico): rich replies always need a formatted_body!
                text.format = "org.matrix.custom.html";
                if (settings->isMarkdownEnabled())
                        text.formatted_body =
                          utils::getFormattedQuoteBody(*related, utils::markdownToHtml(msg))
                            .toStdString();
                else
                        text.formatted_body =
                          utils::getFormattedQuoteBody(*related, msg.toHtmlEscaped()).toStdString();

                text.relates_to.in_reply_to.event_id = related->related_event;
        }

        if (timeline_)
                timeline_->sendMessage(text);
}

void
TimelineViewManager::queueEmoteMessage(const QString &msg)
{
        auto html = utils::markdownToHtml(msg);

        mtx::events::msg::Emote emote;
        emote.body = msg.trimmed().toStdString();

        if (html != msg.trimmed().toHtmlEscaped() && settings->isMarkdownEnabled()) {
                emote.formatted_body = html.toStdString();
                emote.format         = "org.matrix.custom.html";
        }

        if (timeline_)
                timeline_->sendMessage(emote);
}

void
TimelineViewManager::queueImageMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize,
                                       const QSize &dimensions,
                                       const QString &blurhash,
                                       const std::optional<RelatedInfo> &related)
{
        mtx::events::msg::Image image;
        image.info.mimetype = mime.toStdString();
        image.info.size     = dsize;
        image.info.blurhash = blurhash.toStdString();
        image.body          = filename.toStdString();
        image.url           = url.toStdString();
        image.info.h        = dimensions.height();
        image.info.w        = dimensions.width();
        image.file          = file;

        if (related)
                image.relates_to.in_reply_to.event_id = related->related_event;

        models.value(roomid)->sendMessage(image);
}

void
TimelineViewManager::queueFileMessage(
  const QString &roomid,
  const QString &filename,
  const std::optional<mtx::crypto::EncryptedFile> &encryptedFile,
  const QString &url,
  const QString &mime,
  uint64_t dsize,
  const std::optional<RelatedInfo> &related)
{
        mtx::events::msg::File file;
        file.info.mimetype = mime.toStdString();
        file.info.size     = dsize;
        file.body          = filename.toStdString();
        file.url           = url.toStdString();
        file.file          = encryptedFile;

        if (related)
                file.relates_to.in_reply_to.event_id = related->related_event;

        models.value(roomid)->sendMessage(file);
}

void
TimelineViewManager::queueAudioMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize,
                                       const std::optional<RelatedInfo> &related)
{
        mtx::events::msg::Audio audio;
        audio.info.mimetype = mime.toStdString();
        audio.info.size     = dsize;
        audio.body          = filename.toStdString();
        audio.url           = url.toStdString();
        audio.file          = file;

        if (related)
                audio.relates_to.in_reply_to.event_id = related->related_event;

        models.value(roomid)->sendMessage(audio);
}

void
TimelineViewManager::queueVideoMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize,
                                       const std::optional<RelatedInfo> &related)
{
        mtx::events::msg::Video video;
        video.info.mimetype = mime.toStdString();
        video.info.size     = dsize;
        video.body          = filename.toStdString();
        video.url           = url.toStdString();
        video.file          = file;

        if (related)
                video.relates_to.in_reply_to.event_id = related->related_event;

        models.value(roomid)->sendMessage(video);
}
