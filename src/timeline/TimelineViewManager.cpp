// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineViewManager.h"

#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QString>

#include "ChatPage.h"
#include "CombinedImagePackModel.h"
#include "CompletionProxyModel.h"
#include "EventAccessors.h"
#include "ImagePackListModel.h"
#include "InviteesModel.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RoomsModel.h"
#include "UserSettingsPage.h"
#include "UsersModel.h"
#include "Utils.h"
#include "emoji/EmojiModel.h"
#include "encryption/VerificationManager.h"
#include "voip/CallManager.h"
#include "voip/WebRTCSession.h"

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
}

QColor
TimelineViewManager::userColor(QString id, QColor background)
{
    QPair<QString, quint64> idx{id, background.rgba64()};
    if (!userColors.contains(idx))
        userColors.insert(idx, QColor(utils::generateContrastingHexColor(id, background)));
    return userColors.value(idx);
}

TimelineViewManager::TimelineViewManager(CallManager *, ChatPage *parent)
  : QObject(parent)
  , rooms_(new RoomlistModel(this))
  , communities_(new CommunitiesModel(this))
  , verificationManager_(new VerificationManager(this))
  , presenceEmitter(new PresenceEmitter(this))
{
    static auto self = this;
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "TimelineManager", self);
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
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "VerificationManager", verificationManager_);
    qmlRegisterSingletonInstance("im.nheko", 1, 0, "Presence", presenceEmitter);

    updateColorPalette();

    connect(UserSettings::instance().get(),
            &UserSettings::themeChanged,
            this,
            &TimelineViewManager::updateColorPalette);
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
    connect(parent, &ChatPage::connectionLost, this, [this] {
        isConnected_ = false;
        emit isConnectedChanged(false);
    });
    connect(parent, &ChatPage::connectionRestored, this, [this] {
        isConnected_ = true;
        emit isConnectedChanged(true);
    });
}

void
TimelineViewManager::openRoomMembers(TimelineModel *room)
{
    if (!room)
        return;
    MemberList *memberList = new MemberList(room->roomId());
    QQmlEngine::setObjectOwnership(memberList, QQmlEngine::JavaScriptOwnership);
    emit openRoomMembersDialog(memberList, room);
}

void
TimelineViewManager::openRoomSettings(QString room_id)
{
    RoomSettings *settings = new RoomSettings(room_id);
    connect(rooms_->getRoomById(room_id).data(),
            &TimelineModel::roomAvatarUrlChanged,
            settings,
            &RoomSettings::avatarChanged);
    QQmlEngine::setObjectOwnership(settings, QQmlEngine::JavaScriptOwnership);
    emit openRoomSettingsDialog(settings);
}

void
TimelineViewManager::openInviteUsers(QString roomId)
{
    InviteesModel *model = new InviteesModel{};
    connect(model, &InviteesModel::accept, this, [this, model, roomId]() {
        emit inviteUsers(roomId, model->mxids());
    });
    QQmlEngine::setObjectOwnership(model, QQmlEngine::JavaScriptOwnership);
    emit openInviteUsersDialog(model);
}

void
TimelineViewManager::openGlobalUserProfile(QString userId)
{
    UserProfile *profile = new UserProfile{QString{}, userId, this};
    QQmlEngine::setObjectOwnership(profile, QQmlEngine::JavaScriptOwnership);
    emit openProfile(profile);
}

UserProfile *
TimelineViewManager::getGlobalUserProfile(QString userId)
{
    UserProfile *profile = new UserProfile{QString{}, userId, this};
    QQmlEngine::setObjectOwnership(profile, QQmlEngine::JavaScriptOwnership);
    return (profile);
}

void
TimelineViewManager::setVideoCallItem()
{
    WebRTCSession::instance().setVideoItem(
      MainWindow::instance()->rootObject()->findChild<QQuickItem *>(
        QStringLiteral("videoCallItem")));
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
        auto exWin = MainWindow::instance()->windowForRoom(room_id);
        if (exWin) {
            exWin->requestActivate();
        } else if (rooms_->currentRoom() != room) {
            rooms_->setCurrentRoom(room_id);
            MainWindow::instance()->requestActivate();
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
TimelineViewManager::openImageOverlay(TimelineModel *room,
                                      QString mxcUrl,
                                      QString eventId,
                                      double originalWidth,
                                      double proportionalHeight)
{
    if (mxcUrl.isEmpty()) {
        return;
    }

    emit showImageOverlay(room, eventId, mxcUrl, originalWidth, proportionalHeight);
}

void
TimelineViewManager::openImagePackSettings(QString roomid)
{
    auto room  = rooms_->getRoomById(roomid).get();
    auto model = new ImagePackListModel(roomid.toStdString());
    QQmlEngine::setObjectOwnership(model, QQmlEngine::JavaScriptOwnership);
    emit showImagePackSettings(room, model);
}

void
TimelineViewManager::saveMedia(QString mxcUrl)
{
    const QString downloadsFolder =
      QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString openLocation = downloadsFolder + "/" + mxcUrl.splitRef(u'/').constLast();

    const QString filename = QFileDialog::getSaveFileName(nullptr, {}, openLocation);

    if (filename.isEmpty())
        return;

    const auto url = mxcUrl.toStdString();

    http::client()->download(url,
                             [filename, url](const std::string &data,
                                             const std::string &,
                                             const std::string &,
                                             mtx::http::RequestErr err) {
                                 if (err) {
                                     nhlog::net()->warn("failed to retrieve image {}: {} {}",
                                                        url,
                                                        err->matrix_error.error,
                                                        static_cast<int>(err->status_code));
                                     return;
                                 }

                                 try {
                                     QFile file(filename);

                                     if (!file.open(QIODevice::WriteOnly))
                                         return;

                                     file.write(QByteArray(data.data(), (int)data.size()));
                                     file.close();

                                     return;
                                 } catch (const std::exception &e) {
                                     nhlog::ui()->warn("Error while saving file to: {}", e.what());
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
                                      const mtx::events::voip::CallInvite &callInvite)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callInvite, mtx::events::EventType::CallInvite);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallCandidates &callCandidates)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callCandidates, mtx::events::EventType::CallCandidates);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallAnswer &callAnswer)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callAnswer, mtx::events::EventType::CallAnswer);
}

void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallHangUp &callHangUp)
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
