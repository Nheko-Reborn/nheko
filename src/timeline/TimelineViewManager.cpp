#include "TimelineViewManager.h"

#include <QMetaType>
#include <QPalette>
#include <QQmlContext>
#include <QQmlEngine>

#include "BlurhashProvider.h"
#include "ChatPage.h"
#include "ColorImageProvider.h"
#include "DelegateChooser.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "UserSettingsPage.h"
#include "dialogs/ImageOverlay.h"
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"

Q_DECLARE_METATYPE(mtx::events::collections::TimelineEvents)
Q_DECLARE_METATYPE(std::vector<DeviceInfo>)

namespace msgs = mtx::events::msg;

void
DeviceVerificationList::add(QString tran_id)
{
        this->deviceVerificationList.push_back(tran_id);
}
void
DeviceVerificationList::remove(QString tran_id)
{
        this->deviceVerificationList.removeOne(tran_id);
}
bool
DeviceVerificationList::exist(QString tran_id)
{
        return this->deviceVerificationList.contains(tran_id);
}

void
TimelineViewManager::updateEncryptedDescriptions()
{
        auto decrypt = settings->decryptSidebar();
        QHash<QString, QSharedPointer<TimelineModel>>::iterator i;
        for (i = models.begin(); i != models.end(); ++i) {
                auto ptr = i.value();

                if (!ptr.isNull()) {
                        ptr->setDecryptDescription(decrypt);
                        ptr->updateLastMessage();
                }
        }
}

void
TimelineViewManager::updateColorPalette()
{
        userColors.clear();

        if (settings->theme() == "light") {
                view->rootContext()->setContextProperty("currentActivePalette", QPalette());
                view->rootContext()->setContextProperty("currentInactivePalette", QPalette());
        } else if (settings->theme() == "dark") {
                view->rootContext()->setContextProperty("currentActivePalette", QPalette());
                view->rootContext()->setContextProperty("currentInactivePalette", QPalette());
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

QString
TimelineViewManager::userPresence(QString id) const
{
        if (id.isEmpty())
                return "";
        else
                return QString::fromStdString(
                  mtx::presence::to_string(cache::presenceState(id.toStdString())));
}

QString
TimelineViewManager::userStatus(QString id) const
{
        return QString::fromStdString(cache::statusMessage(id.toStdString()));
}

TimelineViewManager::TimelineViewManager(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : imgProvider(new MxcImageProvider())
  , colorImgProvider(new ColorImageProvider())
  , blurhashProvider(new BlurhashProvider())
  , settings(userSettings)
{
        qRegisterMetaType<mtx::events::msg::KeyVerificationAccept>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationCancel>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationDone>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationKey>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationMac>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationReady>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationRequest>();
        qRegisterMetaType<mtx::events::msg::KeyVerificationStart>();

        qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                         "im.nheko",
                                         1,
                                         0,
                                         "MtxEvent",
                                         "Can't instantiate enum!");
        qmlRegisterUncreatableMetaObject(verification::staticMetaObject,
                                         "im.nheko",
                                         1,
                                         0,
                                         "VerificationStatus",
                                         "Can't instantiate enum!");

        qmlRegisterType<DelegateChoice>("im.nheko", 1, 0, "DelegateChoice");
        qmlRegisterType<DelegateChooser>("im.nheko", 1, 0, "DelegateChooser");
        qmlRegisterType<DeviceVerificationFlow>("im.nheko", 1, 0, "DeviceVerificationFlow");
        qmlRegisterUncreatableType<UserProfile>(
          "im.nheko",
          1,
          0,
          "UserProfileModel",
          "UserProfile needs to be instantiated on the C++ side");
        qmlRegisterSingletonInstance("im.nheko", 1, 0, "TimelineManager", this);
        qmlRegisterSingletonInstance("im.nheko", 1, 0, "Settings", settings.data());

        qRegisterMetaType<mtx::events::collections::TimelineEvents>();
        qRegisterMetaType<std::vector<DeviceInfo>>();

        qmlRegisterType<emoji::EmojiModel>("im.nheko.EmojiModel", 1, 0, "EmojiModel");
        qmlRegisterType<emoji::EmojiProxyModel>("im.nheko.EmojiModel", 1, 0, "EmojiProxyModel");
        qmlRegisterUncreatableType<QAbstractItemModel>(
          "im.nheko.EmojiModel", 1, 0, "QAbstractItemModel", "Used by proxy models");
        qmlRegisterUncreatableType<emoji::Emoji>(
          "im.nheko.EmojiModel", 1, 0, "Emoji", "Used by emoji models");
        qmlRegisterUncreatableMetaObject(emoji::staticMetaObject,
                                         "im.nheko.EmojiModel",
                                         1,
                                         0,
                                         "EmojiCategory",
                                         "Error: Only enums");
        this->dvList = new DeviceVerificationList;

#ifdef USE_QUICK_VIEW
        view      = new QQuickView();
        container = QWidget::createWindowContainer(view, parent);
#else
        view      = new QQuickWidget(parent);
        container = view;
        view->setResizeMode(QQuickWidget::SizeRootObjectToView);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        view->quickWindow()->setTextRenderType(QQuickWindow::NativeTextRendering);
#endif

        connect(view, &QQuickWidget::statusChanged, this, [](QQuickWidget::Status status) {
                nhlog::ui()->debug("Status changed to {}", status);
        });
#endif
        container->setMinimumSize(200, 200);
        view->rootContext()->setContextProperty("deviceVerificationList", this->dvList);
        updateColorPalette();
        view->engine()->addImageProvider("MxcImage", imgProvider);
        view->engine()->addImageProvider("colorimage", colorImgProvider);
        view->engine()->addImageProvider("blurhash", blurhashProvider);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));

        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::themeChanged,
                this,
                &TimelineViewManager::updateColorPalette);
        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::decryptSidebarChanged,
                this,
                &TimelineViewManager::updateEncryptedDescriptions);
        connect(
          dynamic_cast<ChatPage *>(parent),
          &ChatPage::recievedDeviceVerificationRequest,
          this,
          [this](const mtx::events::msg::KeyVerificationRequest &msg, std::string sender) {
                  auto flow = new DeviceVerificationFlow(this);
                  if (!(this->dvList->exist(QString::fromStdString(msg.transaction_id.value())))) {
                          if (std::find(msg.methods.begin(),
                                        msg.methods.end(),
                                        mtx::events::msg::VerificationMethods::SASv1) !=
                              msg.methods.end()) {
                                  //   flow->sendVerificationReady();
                                  emit newDeviceVerificationRequest(
                                    std::move(flow),
                                    QString::fromStdString(msg.transaction_id.value()),
                                    QString::fromStdString(sender),
                                    QString::fromStdString(msg.from_device));
                          } else {
                                  flow->cancelVerification(
                                    DeviceVerificationFlow::Error::UnknownMethod);
                          }
                  }
          });
        connect(
          dynamic_cast<ChatPage *>(parent),
          &ChatPage::recievedDeviceVerificationStart,
          this,
          [this](const mtx::events::msg::KeyVerificationStart &msg, std::string sender) {
                  auto flow            = new DeviceVerificationFlow(this);
                  flow->canonical_json = nlohmann::json(msg);
                  if (!(this->dvList->exist(QString::fromStdString(msg.transaction_id.value())))) {
                          if ((std::find(msg.key_agreement_protocols.begin(),
                                         msg.key_agreement_protocols.end(),
                                         "curve25519-hkdf-sha256") !=
                               msg.key_agreement_protocols.end()) &&
                              (std::find(msg.hashes.begin(), msg.hashes.end(), "sha256") !=
                               msg.hashes.end()) &&
                              (std::find(msg.message_authentication_codes.begin(),
                                         msg.message_authentication_codes.end(),
                                         "hmac-sha256") !=
                               msg.message_authentication_codes.end())) {
                                  if (std::find(msg.short_authentication_string.begin(),
                                                msg.short_authentication_string.end(),
                                                mtx::events::msg::SASMethods::Emoji) !=
                                      msg.short_authentication_string.end()) {
                                          flow->setMethod(DeviceVerificationFlow::Method::Emoji);
                                  } else if (std::find(msg.short_authentication_string.begin(),
                                                       msg.short_authentication_string.end(),
                                                       mtx::events::msg::SASMethods::Decimal) !=
                                             msg.short_authentication_string.end()) {
                                          flow->setMethod(DeviceVerificationFlow::Method::Decimal);
                                  } else {
                                          flow->cancelVerification(
                                            DeviceVerificationFlow::Error::UnknownMethod);
                                          return;
                                  }
                                  emit newDeviceVerificationRequest(
                                    std::move(flow),
                                    QString::fromStdString(msg.transaction_id.value()),
                                    QString::fromStdString(sender),
                                    QString::fromStdString(msg.from_device));
                                  flow->canonical_json = nlohmann::json(msg);
                          } else {
                                  flow->cancelVerification(
                                    DeviceVerificationFlow::Error::UnknownMethod);
                          }
                  }
          });
}

void
TimelineViewManager::sync(const mtx::responses::Rooms &rooms)
{
        for (const auto &[room_id, room] : rooms.join) {
                // addRoom will only add the room, if it doesn't exist
                addRoom(QString::fromStdString(room_id));
                const auto &room_model = models.value(QString::fromStdString(room_id));
                room_model->addEvents(room.timeline);

                if (ChatPage::instance()->userSettings()->typingNotifications()) {
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
                newRoom->setDecryptDescription(settings->decryptSidebar());

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
                connect(imgDialog,
                        &dialogs::ImageOverlay::saving,
                        timeline_,
                        [this, eventId, imgDialog]() {
                                // hide the overlay while presenting the save dialog for better
                                // cross platform support.
                                imgDialog->hide();

                                if (!timeline_->saveMedia(eventId)) {
                                        imgDialog->show();
                                } else {
                                        imgDialog->close();
                                }
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
TimelineViewManager::queueTextMessage(const QString &msg)
{
        if (!timeline_)
                return;

        mtx::events::msg::Text text = {};
        text.body                   = msg.trimmed().toStdString();

        if (settings->markdown()) {
                text.formatted_body = utils::markdownToHtml(msg).toStdString();

                // Don't send formatted_body, when we don't need to
                if (text.formatted_body.find("<") == std::string::npos)
                        text.formatted_body = "";
                else
                        text.format = "org.matrix.custom.html";
        }

        if (!timeline_->reply().isEmpty()) {
                auto related = timeline_->relatedInfo(timeline_->reply());

                QString body;
                bool firstLine = true;
                for (const auto &line : related.quoted_body.split("\n")) {
                        if (firstLine) {
                                firstLine = false;
                                body = QString("> <%1> %2\n").arg(related.quoted_user).arg(line);
                        } else {
                                body = QString("%1\n> %2\n").arg(body).arg(line);
                        }
                }

                text.body = QString("%1\n%2").arg(body).arg(msg).toStdString();

                // NOTE(Nico): rich replies always need a formatted_body!
                text.format = "org.matrix.custom.html";
                if (settings->markdown())
                        text.formatted_body =
                          utils::getFormattedQuoteBody(related, utils::markdownToHtml(msg))
                            .toStdString();
                else
                        text.formatted_body =
                          utils::getFormattedQuoteBody(related, msg.toHtmlEscaped()).toStdString();

                text.relates_to.in_reply_to.event_id = related.related_event;
                timeline_->resetReply();
        }

        timeline_->sendMessage(text);
}

void
TimelineViewManager::queueEmoteMessage(const QString &msg)
{
        auto html = utils::markdownToHtml(msg);

        mtx::events::msg::Emote emote;
        emote.body = msg.trimmed().toStdString();

        if (html != msg.trimmed().toHtmlEscaped() && settings->markdown()) {
                emote.formatted_body = html.toStdString();
                emote.format         = "org.matrix.custom.html";
        }

        if (!timeline_->reply().isEmpty()) {
                emote.relates_to.in_reply_to.event_id = timeline_->reply().toStdString();
                timeline_->resetReply();
        }

        if (timeline_)
                timeline_->sendMessage(emote);
}

void
TimelineViewManager::queueReactionMessage(const QString &reactedEvent, const QString &reactionKey)
{
        if (!timeline_)
                return;

        auto reactions = timeline_->reactions(reactedEvent.toStdString());

        QString selfReactedEvent;
        for (const auto &reaction : reactions) {
                if (reactionKey == reaction.key_) {
                        selfReactedEvent = reaction.selfReactedEvent_;
                        break;
                }
        }

        if (selfReactedEvent.startsWith("m"))
                return;

        // If selfReactedEvent is empty, that means we haven't previously reacted
        if (selfReactedEvent.isEmpty()) {
                mtx::events::msg::Reaction reaction;
                reaction.relates_to.rel_type = mtx::common::RelationType::Annotation;
                reaction.relates_to.event_id = reactedEvent.toStdString();
                reaction.relates_to.key      = reactionKey.toStdString();

                timeline_->sendMessage(reaction);
                // Otherwise, we have previously reacted and the reaction should be redacted
        } else {
                timeline_->redactEvent(selfReactedEvent);
        }
}

void
TimelineViewManager::queueImageMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize,
                                       const QSize &dimensions,
                                       const QString &blurhash)
{
        mtx::events::msg::Image image;
        image.info.mimetype = mime.toStdString();
        image.info.size     = dsize;
        image.info.blurhash = blurhash.toStdString();
        image.body          = filename.toStdString();
        image.info.h        = dimensions.height();
        image.info.w        = dimensions.width();

        if (file)
                image.file = file;
        else
                image.url = url.toStdString();

        auto model = models.value(roomid);
        if (!model->reply().isEmpty()) {
                image.relates_to.in_reply_to.event_id = model->reply().toStdString();
                model->resetReply();
        }

        model->sendMessage(image);
}

void
TimelineViewManager::queueFileMessage(
  const QString &roomid,
  const QString &filename,
  const std::optional<mtx::crypto::EncryptedFile> &encryptedFile,
  const QString &url,
  const QString &mime,
  uint64_t dsize)
{
        mtx::events::msg::File file;
        file.info.mimetype = mime.toStdString();
        file.info.size     = dsize;
        file.body          = filename.toStdString();

        if (encryptedFile)
                file.file = encryptedFile;
        else
                file.url = url.toStdString();

        auto model = models.value(roomid);
        if (!model->reply().isEmpty()) {
                file.relates_to.in_reply_to.event_id = model->reply().toStdString();
                model->resetReply();
        }

        model->sendMessage(file);
}

void
TimelineViewManager::queueAudioMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize)
{
        mtx::events::msg::Audio audio;
        audio.info.mimetype = mime.toStdString();
        audio.info.size     = dsize;
        audio.body          = filename.toStdString();
        audio.url           = url.toStdString();

        if (file)
                audio.file = file;
        else
                audio.url = url.toStdString();

        auto model = models.value(roomid);
        if (!model->reply().isEmpty()) {
                audio.relates_to.in_reply_to.event_id = model->reply().toStdString();
                model->resetReply();
        }

        model->sendMessage(audio);
}

void
TimelineViewManager::queueVideoMessage(const QString &roomid,
                                       const QString &filename,
                                       const std::optional<mtx::crypto::EncryptedFile> &file,
                                       const QString &url,
                                       const QString &mime,
                                       uint64_t dsize)
{
        mtx::events::msg::Video video;
        video.info.mimetype = mime.toStdString();
        video.info.size     = dsize;
        video.body          = filename.toStdString();

        if (file)
                video.file = file;
        else
                video.url = url.toStdString();

        auto model = models.value(roomid);
        if (!model->reply().isEmpty()) {
                video.relates_to.in_reply_to.event_id = model->reply().toStdString();
                model->resetReply();
        }

        model->sendMessage(video);
}
