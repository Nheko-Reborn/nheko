// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineViewManager.h"

#include <QDesktopServices>
#include <QDropEvent>
#include <QMetaType>
#include <QPalette>
#include <QQmlContext>
#include <QQmlEngine>
#include <QString>

#include "BlurhashProvider.h"
#include "ChatPage.h"
#include "Clipboard.h"
#include "ColorImageProvider.h"
#include "CompletionProxyModel.h"
#include "DelegateChooser.h"
#include "DeviceVerificationFlow.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "RoomsModel.h"
#include "UserSettingsPage.h"
#include "UsersModel.h"
#include "dialogs/ImageOverlay.h"
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"
#include "RoomDirectoryModel.h"
#include "ui/NhekoCursorShape.h"
#include "ui/NhekoDropArea.h"

Q_DECLARE_METATYPE(mtx::events::collections::TimelineEvents)
Q_DECLARE_METATYPE(std::vector<DeviceInfo>)

namespace msgs = mtx::events::msg;

namespace {
template<template<class...> class Op, class... Args>
using is_detected = typename nheko::detail::detector<nheko::nonesuch, void, Op, Args...>::value_t;

template<class Content>
using file_t = decltype(Content::file);

template<class Content>
using url_t = decltype(Content::url);

template<class Content>
using body_t = decltype(Content::body);

template<class Content>
using formatted_body_t = decltype(Content::formatted_body);

template<typename T>
static constexpr bool
messageWithFileAndUrl(const mtx::events::Event<T> &)
{
        return is_detected<file_t, T>::value && is_detected<url_t, T>::value;
}

template<typename T>
static constexpr void
removeReplyFallback(mtx::events::Event<T> &e)
{
        if constexpr (is_detected<body_t, T>::value) {
                if constexpr (std::is_same_v<std::optional<std::string>,
                                             std::remove_cv_t<decltype(e.content.body)>>) {
                        if (e.content.body) {
                                e.content.body = utils::stripReplyFromBody(e.content.body);
                        }
                } else if constexpr (std::is_same_v<std::string,
                                                    std::remove_cv_t<decltype(e.content.body)>>) {
                        e.content.body = utils::stripReplyFromBody(e.content.body);
                }
        }

        if constexpr (is_detected<formatted_body_t, T>::value) {
                if (e.content.format == "org.matrix.custom.html") {
                        e.content.formatted_body =
                          utils::stripReplyFromFormattedBody(e.content.formatted_body);
                }
        }
}
}

void
TimelineViewManager::updateEncryptedDescriptions()
{
        auto decrypt = ChatPage::instance()->userSettings()->decryptSidebar();
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

        if (ChatPage::instance()->userSettings()->theme() == "light") {
                view->rootContext()->setContextProperty("currentActivePalette", QPalette());
                view->rootContext()->setContextProperty("currentInactivePalette", QPalette());
        } else if (ChatPage::instance()->userSettings()->theme() == "dark") {
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

TimelineViewManager::TimelineViewManager(CallManager *callManager, ChatPage *parent)
  : imgProvider(new MxcImageProvider())
  , colorImgProvider(new ColorImageProvider())
  , blurhashProvider(new BlurhashProvider())
  , callManager_(callManager)
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
        qmlRegisterUncreatableMetaObject(
          crypto::staticMetaObject, "im.nheko", 1, 0, "Crypto", "Can't instantiate enum!");
        qmlRegisterUncreatableMetaObject(verification::staticMetaObject,
                                         "im.nheko",
                                         1,
                                         0,
                                         "VerificationStatus",
                                         "Can't instantiate enum!");

        qmlRegisterType<DelegateChoice>("im.nheko", 1, 0, "DelegateChoice");
        qmlRegisterType<DelegateChooser>("im.nheko", 1, 0, "DelegateChooser");
        qmlRegisterType<NhekoDropArea>("im.nheko", 1, 0, "NhekoDropArea");
        qmlRegisterType<NhekoCursorShape>("im.nheko", 1, 0, "CursorShape");
        qmlRegisterUncreatableType<DeviceVerificationFlow>(
          "im.nheko", 1, 0, "DeviceVerificationFlow", "Can't create verification flow from QML!");
        qmlRegisterUncreatableType<UserProfile>(
          "im.nheko",
          1,
          0,
          "UserProfileModel",
          "UserProfile needs to be instantiated on the C++ side");
        qmlRegisterUncreatableType<RoomSettings>(
          "im.nheko",
          1,
          0,
          "RoomSettingsModel",
          "Room Settings needs to be instantiated on the C++ side");

        static auto self = this;
        qmlRegisterSingletonType<MainWindow>(
          "im.nheko", 1, 0, "MainWindow", [](QQmlEngine *, QJSEngine *) -> QObject * {
                  auto ptr = MainWindow::instance();
                  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
                  return ptr;
          });
        qmlRegisterSingletonType<TimelineViewManager>(
          "im.nheko", 1, 0, "TimelineManager", [](QQmlEngine *, QJSEngine *) -> QObject * {
                  auto ptr = self;
                  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
                  return ptr;
          });
        qmlRegisterSingletonType<UserSettings>(
          "im.nheko", 1, 0, "Settings", [](QQmlEngine *, QJSEngine *) -> QObject * {
                  auto ptr = ChatPage::instance()->userSettings().data();
                  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
                  return ptr;
          });
        qmlRegisterSingletonType<CallManager>(
          "im.nheko", 1, 0, "CallManager", [](QQmlEngine *, QJSEngine *) -> QObject * {
                  auto ptr = ChatPage::instance()->callManager();
                  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
                  return ptr;
          });
        qmlRegisterSingletonType<Clipboard>(
          "im.nheko", 1, 0, "Clipboard", [](QQmlEngine *, QJSEngine *) -> QObject * {
                  return new Clipboard();
          });

        qRegisterMetaType<mtx::events::collections::TimelineEvents>();
        qRegisterMetaType<std::vector<DeviceInfo>>();

        qmlRegisterType<emoji::EmojiModel>("im.nheko.EmojiModel", 1, 0, "EmojiModel");
        qmlRegisterUncreatableType<emoji::Emoji>(
          "im.nheko.EmojiModel", 1, 0, "Emoji", "Used by emoji models");
        qmlRegisterUncreatableMetaObject(emoji::staticMetaObject,
                                         "im.nheko.EmojiModel",
                                         1,
                                         0,
                                         "EmojiCategory",
                                         "Error: Only enums");
        qmlRegisterType<RoomDirectoryModel>("im.nheko.RoomDirectoryModel", 1, 0, "RoomDirectoryModel");

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
        updateColorPalette();
        view->engine()->addImageProvider("MxcImage", imgProvider);
        view->engine()->addImageProvider("colorimage", colorImgProvider);
        view->engine()->addImageProvider("blurhash", blurhashProvider);
        view->setSource(QUrl("qrc:///qml/TimelineView.qml"));

        connect(parent, &ChatPage::themeChanged, this, &TimelineViewManager::updateColorPalette);
        connect(parent,
                &ChatPage::decryptSidebarChanged,
                this,
                &TimelineViewManager::updateEncryptedDescriptions);
        connect(
          dynamic_cast<ChatPage *>(parent),
          &ChatPage::receivedRoomDeviceVerificationRequest,
          this,
          [this](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationRequest> &message,
                 TimelineModel *model) {
                  auto event_id = QString::fromStdString(message.event_id);
                  if (!this->dvList.contains(event_id)) {
                          if (auto flow = DeviceVerificationFlow::NewInRoomVerification(
                                this,
                                model,
                                message.content,
                                QString::fromStdString(message.sender),
                                event_id)) {
                                  dvList[event_id] = flow;
                                  emit newDeviceVerificationRequest(flow.data());
                          }
                  }
          });
        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::receivedDeviceVerificationRequest,
                this,
                [this](const mtx::events::msg::KeyVerificationRequest &msg, std::string sender) {
                        if (!msg.transaction_id)
                                return;

                        auto txnid = QString::fromStdString(msg.transaction_id.value());
                        if (!this->dvList.contains(txnid)) {
                                if (auto flow = DeviceVerificationFlow::NewToDeviceVerification(
                                      this, msg, QString::fromStdString(sender), txnid)) {
                                        dvList[txnid] = flow;
                                        emit newDeviceVerificationRequest(flow.data());
                                }
                        }
                });
        connect(dynamic_cast<ChatPage *>(parent),
                &ChatPage::receivedDeviceVerificationStart,
                this,
                [this](const mtx::events::msg::KeyVerificationStart &msg, std::string sender) {
                        if (!msg.transaction_id)
                                return;

                        auto txnid = QString::fromStdString(msg.transaction_id.value());
                        if (!this->dvList.contains(txnid)) {
                                if (auto flow = DeviceVerificationFlow::NewToDeviceVerification(
                                      this, msg, QString::fromStdString(sender), txnid)) {
                                        dvList[txnid] = flow;
                                        emit newDeviceVerificationRequest(flow.data());
                                }
                        }
                });
        connect(parent, &ChatPage::loggedOut, this, [this]() {
                isInitialSync_ = true;
                emit initialSyncChanged(true);
        });

        connect(this,
                &TimelineViewManager::openImageOverlayInternalCb,
                this,
                &TimelineViewManager::openImageOverlayInternal);
}

void
TimelineViewManager::setVideoCallItem()
{
        WebRTCSession::instance().setVideoItem(
          view->rootObject()->findChild<QQuickItem *>("videoCallItem"));
}

void
TimelineViewManager::sync(const mtx::responses::Rooms &rooms)
{
        for (const auto &[room_id, room] : rooms.join) {
                // addRoom will only add the room, if it doesn't exist
                addRoom(QString::fromStdString(room_id));
                const auto &room_model = models.value(QString::fromStdString(room_id));
                if (!isInitialSync_)
                        connect(room_model.data(),
                                &TimelineModel::newCallEvent,
                                callManager_,
                                &CallManager::syncEvent);
                room_model->syncState(room.state);
                room_model->addEvents(room.timeline);
                if (!isInitialSync_)
                        disconnect(room_model.data(),
                                   &TimelineModel::newCallEvent,
                                   callManager_,
                                   &CallManager::syncEvent);

                if (ChatPage::instance()->userSettings()->typingNotifications()) {
                        for (const auto &ev : room.ephemeral.events) {
                                if (auto t = std::get_if<
                                      mtx::events::EphemeralEvent<mtx::events::ephemeral::Typing>>(
                                      &ev)) {
                                        std::vector<QString> typing;
                                        typing.reserve(t->content.user_ids.size());
                                        for (const auto &user : t->content.user_ids) {
                                                if (user != http::client()->user_id().to_string())
                                                        typing.push_back(
                                                          QString::fromStdString(user));
                                        }
                                        room_model->updateTypingUsers(typing);
                                }
                        }
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
                newRoom->setDecryptDescription(
                  ChatPage::instance()->userSettings()->decryptSidebar());

                connect(newRoom.data(),
                        &TimelineModel::newEncryptedImage,
                        imgProvider,
                        &MxcImageProvider::addEncryptionInfo);
                connect(newRoom.data(),
                        &TimelineModel::forwardToRoom,
                        this,
                        &TimelineViewManager::forwardMessageToRoom);
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
                container->setFocus();
                nhlog::ui()->info("Activated room {}", room_id.toStdString());
        }
}

void
TimelineViewManager::highlightRoom(const QString &room_id)
{
        ChatPage::instance()->highlightRoom(room_id);
}

void
TimelineViewManager::showEvent(const QString &room_id, const QString &event_id)
{
        auto room = models.find(room_id);
        if (room != models.end()) {
                if (timeline_ != room.value().data()) {
                        timeline_ = room.value().data();
                        emit activeTimelineChanged(timeline_);
                        container->setFocus();
                        nhlog::ui()->info("Activated room {}", room_id.toStdString());
                }

                timeline_->showEvent(event_id);
        }
}

QString
TimelineViewManager::escapeEmoji(QString str) const
{
        return utils::replaceEmoji(str);
}

void
TimelineViewManager::openImageOverlay(QString mxcUrl, QString eventId)
{
        if (mxcUrl.isEmpty()) {
                return;
        }

        MxcImageProvider::download(
          mxcUrl.remove("mxc://"), QSize(), [this, eventId](QString, QSize, QImage img, QString) {
                  if (img.isNull()) {
                          nhlog::ui()->error("Error when retrieving image for overlay.");
                          return;
                  }

                  emit openImageOverlayInternalCb(eventId, std::move(img));
          });
}

void
TimelineViewManager::openImageOverlayInternal(QString eventId, QImage img)
{
        auto pixmap = QPixmap::fromImage(img);

        auto imgDialog = new dialogs::ImageOverlay(pixmap);
        imgDialog->showFullScreen();
        connect(imgDialog, &dialogs::ImageOverlay::saving, timeline_, [this, eventId, imgDialog]() {
                // hide the overlay while presenting the save dialog for better
                // cross platform support.
                imgDialog->hide();

                if (!timeline_->saveMedia(eventId)) {
                        imgDialog->show();
                } else {
                        imgDialog->close();
                }
        });
}

void
TimelineViewManager::openLink(QString link) const
{
        QUrl url(link);
        if (url.scheme() == "https" && url.host() == "matrix.to") {
                // handle matrix.to links internally
                QString p = url.fragment(QUrl::FullyEncoded);
                if (p.startsWith("/"))
                        p.remove(0, 1);

                auto temp = p.split("?");
                QString query;
                if (temp.size() >= 2)
                        query = QUrl::fromPercentEncoding(temp.takeAt(1).toUtf8());

                temp            = temp.first().split("/");
                auto identifier = QUrl::fromPercentEncoding(temp.takeFirst().toUtf8());
                QString eventId = QUrl::fromPercentEncoding(temp.join('/').toUtf8());
                if (!identifier.isEmpty()) {
                        if (identifier.startsWith("@")) {
                                QByteArray uri =
                                  "matrix:u/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        } else if (identifier.startsWith("#")) {
                                QByteArray uri =
                                  "matrix:r/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!eventId.isEmpty())
                                        uri.append("/e/" +
                                                   QUrl::toPercentEncoding(eventId.remove(0, 1)));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        } else if (identifier.startsWith("!")) {
                                QByteArray uri = "matrix:roomid/" +
                                                 QUrl::toPercentEncoding(identifier.remove(0, 1));
                                if (!eventId.isEmpty())
                                        uri.append("/e/" +
                                                   QUrl::toPercentEncoding(eventId.remove(0, 1)));
                                if (!query.isEmpty())
                                        uri.append("?" + query.toUtf8());
                                ChatPage::instance()->handleMatrixUri(QUrl::fromEncoded(uri));
                        }
                }
        } else {
                QDesktopServices::openUrl(url);
        }
}

void
TimelineViewManager::openInviteUsersDialog()
{
        MainWindow::instance()->openInviteUsersDialog(
          [this](const QStringList &invitees) { emit inviteUsers(invitees); });
}
void
TimelineViewManager::openMemberListDialog() const
{
        MainWindow::instance()->openMemberListDialog(timeline_->roomId());
}
void
TimelineViewManager::openLeaveRoomDialog() const
{
        MainWindow::instance()->openLeaveRoomDialog(timeline_->roomId());
}

void
TimelineViewManager::verifyUser(QString userid)
{
        auto joined_rooms = cache::joinedRooms();
        auto room_infos   = cache::getRoomInfo(joined_rooms);

        for (std::string room_id : joined_rooms) {
                if ((room_infos[QString::fromStdString(room_id)].member_count == 2) &&
                    cache::isRoomEncrypted(room_id)) {
                        auto room_members = cache::roomMembers(room_id);
                        if (std::find(room_members.begin(),
                                      room_members.end(),
                                      (userid).toStdString()) != room_members.end()) {
                                auto model = models.value(QString::fromStdString(room_id));
                                auto flow  = DeviceVerificationFlow::InitiateUserVerification(
                                  this, model.data(), userid);
                                connect(model.data(),
                                        &TimelineModel::updateFlowEventId,
                                        this,
                                        [this, flow](std::string eventId) {
                                                dvList[QString::fromStdString(eventId)] = flow;
                                        });
                                emit newDeviceVerificationRequest(flow.data());
                                return;
                        }
                }
        }

        emit ChatPage::instance()->showNotification(
          tr("No encrypted private chat found with this user. Create an "
             "encrypted private chat with this user and try again."));
}

void
TimelineViewManager::removeVerificationFlow(DeviceVerificationFlow *flow)
{
        for (auto it = dvList.keyValueBegin(); it != dvList.keyValueEnd(); ++it) {
                if ((*it).second == flow) {
                        dvList.remove((*it).first);
                        return;
                }
        }
}

void
TimelineViewManager::verifyDevice(QString userid, QString deviceid)
{
        auto flow = DeviceVerificationFlow::InitiateDeviceVerification(this, userid, deviceid);
        this->dvList[flow->transactionId()] = flow;
        emit newDeviceVerificationRequest(flow.data());
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
TimelineViewManager::receivedSessionKey(const std::string &room_id, const std::string &session_id)
{
        auto room = models.find(QString::fromStdString(room_id));
        if (room != models.end()) {
                room.value()->receivedSessionKey(session_id);
        }
}

void
TimelineViewManager::initWithMessages(const std::vector<QString> &roomIds)
{
        for (const auto &roomId : roomIds)
                addRoom(roomId);
}

void
TimelineViewManager::queueReply(const QString &roomid,
                                const QString &repliedToEvent,
                                const QString &replyBody)
{
        auto room = models.find(roomid);
        if (room != models.end()) {
                room.value()->setReply(repliedToEvent);
                room.value()->input()->message(replyBody);
        }
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
                mtx::common::Relation rel;
                rel.rel_type = mtx::common::RelationType::Annotation;
                rel.event_id = reactedEvent.toStdString();
                rel.key      = reactionKey.toStdString();
                reaction.relations.relations.push_back(rel);

                timeline_->sendMessageEvent(reaction, mtx::events::EventType::Reaction);
                // Otherwise, we have previously reacted and the reaction should be redacted
        } else {
                timeline_->redactEvent(selfReactedEvent);
        }
}
void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallInvite &callInvite)
{
        models.value(roomid)->sendMessageEvent(callInvite, mtx::events::EventType::CallInvite);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallCandidates &callCandidates)
{
        models.value(roomid)->sendMessageEvent(callCandidates,
                                               mtx::events::EventType::CallCandidates);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallAnswer &callAnswer)
{
        models.value(roomid)->sendMessageEvent(callAnswer, mtx::events::EventType::CallAnswer);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallHangUp &callHangUp)
{
        models.value(roomid)->sendMessageEvent(callHangUp, mtx::events::EventType::CallHangUp);
}

void
TimelineViewManager::focusMessageInput()
{
        emit focusInput();
}

QObject *
TimelineViewManager::completerFor(QString completerName, QString roomId)
{
        if (completerName == "user") {
                auto userModel = new UsersModel(roomId.toStdString());
                auto proxy     = new CompletionProxyModel(userModel);
                userModel->setParent(proxy);
                return proxy;
        } else if (completerName == "emoji") {
                auto emojiModel = new emoji::EmojiModel();
                auto proxy      = new CompletionProxyModel(emojiModel);
                emojiModel->setParent(proxy);
                return proxy;
        } else if (completerName == "allemoji") {
                auto emojiModel = new emoji::EmojiModel();
                auto proxy = new CompletionProxyModel(emojiModel, 1, static_cast<size_t>(-1) / 4);
                emojiModel->setParent(proxy);
                return proxy;
        } else if (completerName == "room") {
                auto roomModel = new RoomsModel(false);
                auto proxy     = new CompletionProxyModel(roomModel, 4);
                roomModel->setParent(proxy);
                return proxy;
        } else if (completerName == "roomAliases") {
                auto roomModel = new RoomsModel(true);
                auto proxy     = new CompletionProxyModel(roomModel);
                roomModel->setParent(proxy);
                return proxy;
        }
        return nullptr;
}

void
TimelineViewManager::focusTimeline()
{
        getWidget()->setFocus();
}

void
TimelineViewManager::showRoomDirectory()
{
        nhlog::ui()->debug("Plumbed");
        emit showPublicRooms();
}

void
TimelineViewManager::forwardMessageToRoom(mtx::events::collections::TimelineEvents *e,
                                          QString roomId)
{
        auto room                                                = models.find(roomId);
        auto content                                             = mtx::accessors::url(*e);
        std::optional<mtx::crypto::EncryptedFile> encryptionInfo = mtx::accessors::file(*e);

        if (encryptionInfo) {
                http::client()->download(
                  content,
                  [this, roomId, e, encryptionInfo](const std::string &res,
                                                    const std::string &content_type,
                                                    const std::string &originalFilename,
                                                    mtx::http::RequestErr err) {
                          if (err)
                                  return;

                          auto data = mtx::crypto::to_string(
                            mtx::crypto::decrypt_file(res, encryptionInfo.value()));

                          http::client()->upload(
                            data,
                            content_type,
                            originalFilename,
                            [this, roomId, e](const mtx::responses::ContentURI &res,
                                              mtx::http::RequestErr err) mutable {
                                    if (err) {
                                            nhlog::net()->warn("failed to upload media: {} {} ({})",
                                                               err->matrix_error.error,
                                                               to_string(err->matrix_error.errcode),
                                                               static_cast<int>(err->status_code));
                                            return;
                                    }

                                    std::visit(
                                      [this, roomId, url = res.content_uri](auto ev) {
                                              if constexpr (mtx::events::message_content_to_type<
                                                              decltype(ev.content)> ==
                                                            mtx::events::EventType::RoomMessage) {
                                                      if constexpr (messageWithFileAndUrl(ev)) {
                                                              ev.content.relations.relations
                                                                .clear();
                                                              ev.content.file.reset();
                                                              ev.content.url = url;
                                                      }

                                                      auto room = models.find(roomId);
                                                      removeReplyFallback(ev);
                                                      ev.content.relations.relations.clear();
                                                      room.value()->sendMessageEvent(
                                                        ev.content,
                                                        mtx::events::EventType::RoomMessage);
                                              }
                                      },
                                      *e);
                            });

                          return;
                  });

                return;
        }

        std::visit(
          [room](auto e) {
                  if constexpr (mtx::events::message_content_to_type<decltype(e.content)> ==
                                mtx::events::EventType::RoomMessage) {
                          e.content.relations.relations.clear();
                          removeReplyFallback(e);
                          room.value()->sendMessageEvent(e.content,
                                                         mtx::events::EventType::RoomMessage);
                  }
          },
          *e);
}
