// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineViewManager.h"

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
#include "CombinedImagePackModel.h"
#include "CompletionProxyModel.h"
#include "DelegateChooser.h"
#include "EventAccessors.h"
#include "ImagePackListModel.h"
#include "InviteesModel.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "ReadReceiptsModel.h"
#include "RoomDirectoryModel.h"
#include "RoomsModel.h"
#include "SingleImagePackModel.h"
#include "UserSettingsPage.h"
#include "UsersModel.h"
#include "dialogs/ImageOverlay.h"
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"
#include "encryption/DeviceVerificationFlow.h"
#include "encryption/SelfVerificationStatus.h"
#include "ui/MxcAnimatedImage.h"
#include "ui/MxcMediaProxy.h"
#include "ui/NhekoCursorShape.h"
#include "ui/NhekoDropArea.h"
#include "ui/NhekoGlobalObject.h"
#include "ui/UIA.h"

Q_DECLARE_METATYPE(mtx::events::collections::TimelineEvents)
Q_DECLARE_METATYPE(std::vector<DeviceInfo>)
Q_DECLARE_METATYPE(std::vector<mtx::responses::PublicRoomsChunk>)

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
            e.content.formatted_body = utils::stripReplyFromFormattedBody(e.content.formatted_body);
        }
    }
}
}

void
TimelineViewManager::updateColorPalette()
{
    userColors.clear();

    if (ChatPage::instance()->userSettings()->theme() == QLatin1String("light")) {
        view->rootContext()->setContextProperty(QStringLiteral("currentActivePalette"), QPalette());
        view->rootContext()->setContextProperty(QStringLiteral("currentInactivePalette"),
                                                QPalette());
    } else if (ChatPage::instance()->userSettings()->theme() == QLatin1String("dark")) {
        view->rootContext()->setContextProperty(QStringLiteral("currentActivePalette"), QPalette());
        view->rootContext()->setContextProperty(QStringLiteral("currentInactivePalette"),
                                                QPalette());
    } else {
        view->rootContext()->setContextProperty(QStringLiteral("currentActivePalette"), QPalette());
        view->rootContext()->setContextProperty(QStringLiteral("currentInactivePalette"), nullptr);
    }
}

QColor
TimelineViewManager::userColor(QString id, QColor background)
{
    QPair<QString, quint64> idx{id, background.rgba64()};
    if (!userColors.contains(idx))
        userColors.insert(idx, QColor(utils::generateContrastingHexColor(id, background)));
    return userColors.value(idx);
}

TimelineViewManager::TimelineViewManager(CallManager *callManager, ChatPage *parent)
  : QObject(parent)
  , imgProvider(new MxcImageProvider())
  , colorImgProvider(new ColorImageProvider())
  , blurhashProvider(new BlurhashProvider())
  , jdenticonProvider(new JdenticonProvider())
  , rooms_(new RoomlistModel(this))
  , communities_(new CommunitiesModel(this))
  , callManager_(callManager)
  , verificationManager_(new VerificationManager(this))
  , presenceEmitter(new PresenceEmitter(this))
{
    qRegisterMetaType<mtx::events::msg::KeyVerificationAccept>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationCancel>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationDone>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationKey>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationMac>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationReady>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationRequest>();
    qRegisterMetaType<mtx::events::msg::KeyVerificationStart>();
    qRegisterMetaType<CombinedImagePackModel *>();

    qRegisterMetaType<std::vector<mtx::responses::PublicRoomsChunk>>();

    qmlRegisterUncreatableMetaObject(qml_mtx_events::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "MtxEvent",
                                     QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(
      olm::staticMetaObject, "im.nheko", 1, 0, "Olm", QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(crypto::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "Crypto",
                                     QStringLiteral("Can't instantiate enum!"));
    qmlRegisterUncreatableMetaObject(verification::staticMetaObject,
                                     "im.nheko",
                                     1,
                                     0,
                                     "VerificationStatus",
                                     QStringLiteral("Can't instantiate enum!"));

    qmlRegisterType<DelegateChoice>("im.nheko", 1, 0, "DelegateChoice");
    qmlRegisterType<DelegateChooser>("im.nheko", 1, 0, "DelegateChooser");
    qmlRegisterType<NhekoDropArea>("im.nheko", 1, 0, "NhekoDropArea");
    qmlRegisterType<NhekoCursorShape>("im.nheko", 1, 0, "CursorShape");
    qmlRegisterType<MxcAnimatedImage>("im.nheko", 1, 0, "MxcAnimatedImage");
    qmlRegisterType<MxcMediaProxy>("im.nheko", 1, 0, "MxcMedia");
    qmlRegisterUncreatableType<DeviceVerificationFlow>(
      "im.nheko",
      1,
      0,
      "DeviceVerificationFlow",
      QStringLiteral("Can't create verification flow from QML!"));
    qmlRegisterUncreatableType<UserProfile>(
      "im.nheko",
      1,
      0,
      "UserProfileModel",
      QStringLiteral("UserProfile needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<MemberList>(
      "im.nheko",
      1,
      0,
      "MemberList",
      QStringLiteral("MemberList needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<RoomSettings>(
      "im.nheko",
      1,
      0,
      "RoomSettingsModel",
      QStringLiteral("Room Settings needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<TimelineModel>(
      "im.nheko", 1, 0, "Room", QStringLiteral("Room needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<ImagePackListModel>(
      "im.nheko",
      1,
      0,
      "ImagePackListModel",
      QStringLiteral("ImagePackListModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<SingleImagePackModel>(
      "im.nheko",
      1,
      0,
      "SingleImagePackModel",
      QStringLiteral("SingleImagePackModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<InviteesModel>(
      "im.nheko",
      1,
      0,
      "InviteesModel",
      QStringLiteral("InviteesModel needs to be instantiated on the C++ side"));
    qmlRegisterUncreatableType<ReadReceiptsProxy>(
      "im.nheko",
      1,
      0,
      "ReadReceiptsProxy",
      QStringLiteral("ReadReceiptsProxy needs to be instantiated on the C++ side"));

    static auto self = this;
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "MainWindow", MainWindow::instance());
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "TimelineManager", self);
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "UIA", UIA::instance());
    qmlRegisterSingletonType<RoomlistModel>(
      "im.nheko", 1, 0, "Rooms", [](QQmlEngine *, QJSEngine *) -> QObject * {
          auto ptr = new FilteredRoomlistModel(self->rooms_);

          connect(self->communities_,
                  &CommunitiesModel::currentTagIdChanged,
                  ptr,
                  &FilteredRoomlistModel::updateFilterTag);
          connect(self->communities_,
                  &CommunitiesModel::hiddenTagsChanged,
                  ptr,
                  &FilteredRoomlistModel::updateHiddenTagsAndSpaces);
          return ptr;
      });
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "Communities", self->communities_);
    qmlRegisterSingletonInstance(
      "im.nheko", 1, 0, "Settings", ChatPage::instance()->userSettings().data());
    qmlRegisterSingletonInstance(
      "im.nheko", 1, 0, "CallManager", ChatPage::instance()->callManager());
    qmlRegisterSingletonType<Clipboard>(
      "im.nheko", 1, 0, "Clipboard", [](QQmlEngine *, QJSEngine *) -> QObject * {
          return new Clipboard();
      });
    qmlRegisterSingletonType<Nheko>(
      "im.nheko", 1, 0, "Nheko", [](QQmlEngine *, QJSEngine *) -> QObject * {
          return new Nheko();
      });
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "VerificationManager", verificationManager_);
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "Presence", presenceEmitter);
    qmlRegisterSingletonType<SelfVerificationStatus>(
      "im.nheko", 1, 0, "SelfVerificationStatus", [](QQmlEngine *, QJSEngine *) -> QObject * {
          auto ptr = new SelfVerificationStatus();
          QObject::connect(ChatPage::instance(),
                           &ChatPage::initializeEmptyViews,
                           ptr,
                           &SelfVerificationStatus::invalidate);
          return ptr;
      });

    qRegisterMetaType<mtx::events::collections::TimelineEvents>();
    qRegisterMetaType<std::vector<DeviceInfo>>();

    qmlRegisterUncreatableType<FilteredCommunitiesModel>(
      "im.nheko",
      1,
      0,
      "FilteredCommunitiesModel",
      QStringLiteral("Use Communities.filtered() to create a FilteredCommunitiesModel"));

    qmlRegisterType<emoji::EmojiModel>("im.nheko.EmojiModel", 1, 0, "EmojiModel");
    qmlRegisterUncreatableType<emoji::Emoji>(
      "im.nheko.EmojiModel", 1, 0, "Emoji", QStringLiteral("Used by emoji models"));
    qmlRegisterUncreatableMetaObject(emoji::staticMetaObject,
                                     "im.nheko.EmojiModel",
                                     1,
                                     0,
                                     "EmojiCategory",
                                     QStringLiteral("Error: Only enums"));

    qmlRegisterType<RoomDirectoryModel>("im.nheko", 1, 0, "RoomDirectoryModel");

#ifdef USE_QUICK_VIEW
    view      = new QQuickView(parent);
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
    view->engine()->addImageProvider(QStringLiteral("MxcImage"), imgProvider);
    view->engine()->addImageProvider(QStringLiteral("colorimage"), colorImgProvider);
    view->engine()->addImageProvider(QStringLiteral("blurhash"), blurhashProvider);
    if (JdenticonProvider::isAvailable())
        view->engine()->addImageProvider(QStringLiteral("jdenticon"), jdenticonProvider);
    view->setSource(QUrl(QStringLiteral("qrc:///qml/Root.qml")));

    connect(parent, &ChatPage::themeChanged, this, &TimelineViewManager::updateColorPalette);
    connect(parent,
            &ChatPage::receivedRoomDeviceVerificationRequest,
            verificationManager_,
            &VerificationManager::receivedRoomDeviceVerificationRequest);
    connect(parent,
            &ChatPage::receivedDeviceVerificationRequest,
            verificationManager_,
            &VerificationManager::receivedDeviceVerificationRequest);
    connect(parent,
            &ChatPage::receivedDeviceVerificationStart,
            verificationManager_,
            &VerificationManager::receivedDeviceVerificationStart);
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
TimelineViewManager::openRoomMembers(TimelineModel *room)
{
    if (!room)
        return;
    MemberList *memberList = new MemberList(room->roomId(), this);
    emit openRoomMembersDialog(memberList, room);
}

void
TimelineViewManager::openRoomSettings(QString room_id)
{
    RoomSettings *settings = new RoomSettings(room_id, this);
    connect(rooms_->getRoomById(room_id).data(),
            &TimelineModel::roomAvatarUrlChanged,
            settings,
            &RoomSettings::avatarChanged);
    emit openRoomSettingsDialog(settings);
}

void
TimelineViewManager::openInviteUsers(QString roomId)
{
    InviteesModel *model = new InviteesModel{this};
    connect(model, &InviteesModel::accept, this, [this, model, roomId]() {
        emit inviteUsers(roomId, model->mxids());
    });
    emit openInviteUsersDialog(model);
}

void
TimelineViewManager::openGlobalUserProfile(QString userId)
{
    UserProfile *profile = new UserProfile{QString{}, userId, this};
    emit openProfile(profile);
}

void
TimelineViewManager::setVideoCallItem()
{
    WebRTCSession::instance().setVideoItem(
      view->rootObject()->findChild<QQuickItem *>(QStringLiteral("videoCallItem")));
}

void
TimelineViewManager::sync(const mtx::responses::Sync &sync_)
{
    this->rooms_->sync(sync_);
    this->communities_->sync(sync_);
    this->presenceEmitter->sync(sync_.presence);

    if (isInitialSync_) {
        this->isInitialSync_ = false;
        emit initialSyncChanged(false);
    }
}

void
TimelineViewManager::showEvent(const QString &room_id, const QString &event_id)
{
    if (auto room = rooms_->getRoomById(room_id)) {
        if (rooms_->currentRoom() != room) {
            rooms_->setCurrentRoom(room_id);
            container->setFocus();
            nhlog::ui()->info("Activated room {}", room_id.toStdString());
        }

        room->showEvent(event_id);
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

    MxcImageProvider::download(mxcUrl.remove(QStringLiteral("mxc://")),
                               QSize(),
                               [this, eventId](QString, QSize, QImage img, QString) {
                                   if (img.isNull()) {
                                       nhlog::ui()->error(
                                         "Error when retrieving image for overlay.");
                                       return;
                                   }

                                   emit openImageOverlayInternalCb(eventId, std::move(img));
                               });
}

void
TimelineViewManager::openImagePackSettings(QString roomid)
{
    auto room = rooms_->getRoomById(roomid).get();
    emit showImagePackSettings(room, new ImagePackListModel(roomid.toStdString(), this));
}

void
TimelineViewManager::openImageOverlayInternal(QString eventId, QImage img)
{
    auto pixmap = QPixmap::fromImage(img);

    auto imgDialog = new dialogs::ImageOverlay(pixmap);
    imgDialog->showFullScreen();

    auto room = rooms_->currentRoom();
    connect(imgDialog, &dialogs::ImageOverlay::saving, room, [eventId, imgDialog, room]() {
        // hide the overlay while presenting the save dialog for better
        // cross platform support.
        imgDialog->hide();

        if (!room->saveMedia(eventId)) {
            imgDialog->show();
        } else {
            imgDialog->close();
        }
    });
}

void
TimelineViewManager::updateReadReceipts(const QString &room_id,
                                        const std::vector<QString> &event_ids)
{
    if (auto room = rooms_->getRoomById(room_id)) {
        room->markEventsAsRead(event_ids);
    }
}

void
TimelineViewManager::receivedSessionKey(const std::string &room_id, const std::string &session_id)
{
    if (auto room = rooms_->getRoomById(QString::fromStdString(room_id))) {
        room->receivedSessionKey(session_id);
    }
}

void
TimelineViewManager::initializeRoomlist()
{
    rooms_->initializeRooms();
    communities_->initializeSidebar();
}

void
TimelineViewManager::queueReply(const QString &roomid,
                                const QString &repliedToEvent,
                                const QString &replyBody)
{
    if (auto room = rooms_->getRoomById(roomid)) {
        room->setReply(repliedToEvent);
        room->input()->message(replyBody);
    }
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallInvite &callInvite)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callInvite, mtx::events::EventType::CallInvite);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallCandidates &callCandidates)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callCandidates, mtx::events::EventType::CallCandidates);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallAnswer &callAnswer)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callAnswer, mtx::events::EventType::CallAnswer);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::msg::CallHangUp &callHangUp)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callHangUp, mtx::events::EventType::CallHangUp);
}

void
TimelineViewManager::focusMessageInput()
{
    emit focusInput();
}

QObject *
TimelineViewManager::completerFor(QString completerName, QString roomId)
{
    if (completerName == QLatin1String("user")) {
        auto userModel = new UsersModel(roomId.toStdString());
        auto proxy     = new CompletionProxyModel(userModel);
        userModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("emoji")) {
        auto emojiModel = new emoji::EmojiModel();
        auto proxy      = new CompletionProxyModel(emojiModel);
        emojiModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("allemoji")) {
        auto emojiModel = new emoji::EmojiModel();
        auto proxy      = new CompletionProxyModel(emojiModel, 1, static_cast<size_t>(-1) / 4);
        emojiModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("room")) {
        auto roomModel = new RoomsModel(false);
        auto proxy     = new CompletionProxyModel(roomModel, 4);
        roomModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("roomAliases")) {
        auto roomModel = new RoomsModel(true);
        auto proxy     = new CompletionProxyModel(roomModel);
        roomModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("stickers")) {
        auto stickerModel = new CombinedImagePackModel(roomId.toStdString(), true);
        auto proxy        = new CompletionProxyModel(stickerModel, 1, static_cast<size_t>(-1) / 4);
        stickerModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("customEmoji")) {
        auto stickerModel = new CombinedImagePackModel(roomId.toStdString(), false);
        auto proxy        = new CompletionProxyModel(stickerModel);
        stickerModel->setParent(proxy);
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
TimelineViewManager::forwardMessageToRoom(mtx::events::collections::TimelineEvents *e,
                                          QString roomId)
{
    auto room                                                = rooms_->getRoomById(roomId);
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

              auto data =
                mtx::crypto::to_string(mtx::crypto::decrypt_file(res, encryptionInfo.value()));

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
                          using namespace mtx::events;
                          if constexpr (EventType::RoomMessage ==
                                          message_content_to_type<decltype(ev.content)> ||
                                        EventType::Sticker ==
                                          message_content_to_type<decltype(ev.content)>) {
                              if constexpr (messageWithFileAndUrl(ev)) {
                                  ev.content.relations.relations.clear();
                                  ev.content.file.reset();
                                  ev.content.url = url;
                              }

                              if (auto room = rooms_->getRoomById(roomId)) {
                                  removeReplyFallback(ev);
                                  ev.content.relations.relations.clear();
                                  room->sendMessageEvent(ev.content,
                                                         mtx::events::EventType::RoomMessage);
                              }
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
              room->sendMessageEvent(e.content, mtx::events::EventType::RoomMessage);
          }
      },
      *e);
}

//! WORKAROUND(Nico): for https://bugreports.qt.io/browse/QTBUG-93281
void
TimelineViewManager::fixImageRendering(QQuickTextDocument *t, QQuickItem *i)
{
    if (t) {
        QObject::connect(t->textDocument(), SIGNAL(imagesLoaded()), i, SLOT(updateWholeDocument()));
    }
}
