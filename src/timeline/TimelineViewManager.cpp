// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimelineViewManager.h"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QStandardPaths>
#include <QString>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "CombinedImagePackModel.h"
#include "CommandCompleter.h"
#include "CompletionProxyModel.h"
#include "EventAccessors.h"
#include "GridImagePackModel.h"
#include "ImagePackListModel.h"
#include "InviteesModel.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MemberList.h"
#include "RoomsModel.h"
#include "TimelineModel.h"
#include "UserSettingsPage.h"
#include "UsersModel.h"
#include "Utils.h"
#include "encryption/VerificationManager.h"
#include "timeline/CommunitiesModel.h"
#include "timeline/PresenceEmitter.h"
#include "timeline/RoomlistModel.h"
#include "ui/RoomSettings.h"
#include "ui/UserProfile.h"
#include "voip/CallManager.h"
#include "voip/WebRTCSession.h"

namespace {
struct nonesuch
{
    ~nonesuch()                      = delete;
    nonesuch(nonesuch const &)       = delete;
    void operator=(nonesuch const &) = delete;
};

namespace detail {
template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector
{
    using value_t = std::false_type;
    using type    = Default;
};

template<class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
    using value_t = std::true_type;
    using type    = Op<Args...>;
};

} // namespace detail

template<template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

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
    std::pair<QString, quint64> idx{id, background.rgba64()};
    if (!userColors.contains(idx))
        userColors.insert(idx, QColor(utils::generateContrastingHexColor(id, background)));
    return userColors.value(idx);
}

TimelineViewManager::TimelineViewManager(CallManager *, ChatPage *parent)
  : QObject(parent)
  , rooms_(new RoomlistModel(this))
  , frooms_(new FilteredRoomlistModel(this->rooms_))
  , communities_(new CommunitiesModel(this))
  , verificationManager_(new VerificationManager(this))
  , presenceEmitter(new PresenceEmitter(this))
{
    instance_ = this;

    connect(this->communities_,
            &CommunitiesModel::currentTagIdChanged,
            frooms_,
            &FilteredRoomlistModel::updateFilterTag);
    connect(this->communities_,
            &CommunitiesModel::hiddenTagsChanged,
            frooms_,
            &FilteredRoomlistModel::updateHiddenTagsAndSpaces);

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
    connect(rooms_, &RoomlistModel::spaceSelected, communities_, [this](QString roomId) {
        communities_->setCurrentTagId("space:" + roomId);
    });
}

TimelineViewManager *
TimelineViewManager::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    // The instance has to exist before it is used. We cannot replace it.
    Q_ASSERT(instance_);

    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(qmlEngine->thread() == instance_->thread());

    // There can only be one engine accessing the singleton.
    static QJSEngine *s_engine = nullptr;
    if (s_engine)
        Q_ASSERT(qmlEngine == s_engine);
    else
        s_engine = qmlEngine;

    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
    return instance_;
}

void
TimelineViewManager::clearAll()
{
    rooms_->clear();
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
    if (!roomId.startsWith('!'))
        return;

    InviteesModel *model = new InviteesModel{rooms_->getRoomById(roomId).data()};
    connect(model, &InviteesModel::accept, this, [this, model, roomId]() {
        emit inviteUsers(roomId, model->mxids());
    });
    QQmlEngine::setObjectOwnership(model, QQmlEngine::JavaScriptOwnership);
    emit openInviteUsersDialog(model);
}

void
TimelineViewManager::openGlobalUserProfile(QString userId)
{
    if (!userId.startsWith('@'))
        return;

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
    this->processIgnoredUsers(sync_.account_data);

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
            exWin->setVisible(true);
            exWin->raise();
            exWin->requestActivate();
        } else {
            rooms_->setCurrentRoom(room_id);
            MainWindow::instance()->setVisible(true);
            MainWindow::instance()->raise();
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
                                      const QString &mxcUrl,
                                      const QString &eventId,
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
    const QString openLocation = downloadsFolder + "/" + mxcUrl.split(u'/').constLast();

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
TimelineViewManager::copyImage(const QString &mxcUrl) const
{
    const auto url = mxcUrl.toStdString();
    QString mimeType;

    http::client()->download(
      url,
      [url, mimeType](const std::string &data,
                      const std::string &,
                      const std::string &,
                      mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to retrieve media {}: {} {}",
                                 url,
                                 err->matrix_error.error,
                                 static_cast<int>(err->status_code));
              return;
          }

          try {
              auto img = utils::readImage(QByteArray(data.data(), (qsizetype)data.size()));
              QGuiApplication::clipboard()->setImage(img);

              return;
          } catch (const std::exception &e) {
              nhlog::ui()->warn("Error while copying file to clipboard: {}", e.what());
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
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallSelectAnswer &callSelectAnswer)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callSelectAnswer, mtx::events::EventType::CallSelectAnswer);
}
void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallReject &callReject)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callReject, mtx::events::EventType::CallReject);
}
void
TimelineViewManager::queueCallMessage(const QString &roomid,
                                      const mtx::events::voip::CallNegotiate &callNegotiate)
{
    if (auto room = rooms_->getRoomById(roomid))
        room->sendMessageEvent(callNegotiate, mtx::events::EventType::CallNegotiate);
}

void
TimelineViewManager::focusMessageInput()
{
    emit focusInput();
}

QAbstractItemModel *
TimelineViewManager::completerFor(const QString &completerName, const QString &roomId)
{
    if (completerName == QLatin1String("user")) {
        auto userModel = new UsersModel(roomId.toStdString());
        auto proxy     = new CompletionProxyModel(userModel);
        userModel->setParent(proxy);
        return proxy;
    } else if (completerName == QLatin1String("emoji")) {
        auto emojiModel = new CombinedImagePackModel(roomId.toStdString());
        auto proxy      = new CompletionProxyModel(emojiModel);
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
    } else if (completerName == QLatin1String("emojigrid")) {
        auto stickerModel = new GridImagePackModel(roomId.toStdString(), false);
        return stickerModel;
    } else if (completerName == QLatin1String("stickergrid")) {
        auto stickerModel = new GridImagePackModel(roomId.toStdString(), true);
        return stickerModel;
    } else if (completerName == QLatin1String("command")) {
        auto commandCompleter = new CommandCompleter();
        auto proxy            = new CompletionProxyModel(commandCompleter);
        commandCompleter->setParent(proxy);
        return proxy;
    }
    return nullptr;
}

void
TimelineViewManager::forwardMessageToRoom(mtx::events::collections::TimelineEvents const *e,
                                          QString roomId)
{
    auto room                                                = rooms_->getRoomById(roomId);
    auto content                                             = mtx::accessors::url(*e);
    std::optional<mtx::crypto::EncryptedFile> encryptionInfo = mtx::accessors::file(*e);

    if (encryptionInfo && !cache::isRoomEncrypted(roomId.toStdString())) {
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
          constexpr auto type = mtx::events::message_content_to_type<decltype(e.content)>;
          if constexpr (type == mtx::events::EventType::RoomMessage ||
                        type == mtx::events::EventType::Sticker) {
              e.content.relations.relations.clear();
              removeReplyFallback(e);
              room->sendMessageEvent(e.content, type);
          }
      },
      *e);
}

//! WORKAROUND(Nico): for https://bugreports.qt.io/browse/QTBUG-93281
// QTBUG-93281 Fixed in 6.7.0
// https://github.com/qt/qtdeclarative/commit/7fb39a7accba014063e32ac41a58b77905bbd95b
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
void
TimelineViewManager::fixImageRendering([[maybe_unused]] QQuickTextDocument *t,
                                       [[maybe_unused]] QQuickItem *i)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    if (t) {
        QObject::connect(t->textDocument(), SIGNAL(imagesLoaded()), i, SLOT(updateWholeDocument()));
    }
#endif
}
#else
void
TimelineViewManager::fixImageRendering(QQuickTextDocument *, QQuickItem *)
{}
#endif

using IgnoredUsers = mtx::events::EphemeralEvent<mtx::events::account_data::IgnoredUsers>;

static QVector<QString>
convertIgnoredToQt(const IgnoredUsers &ev)
{
    QVector<QString> users;
    for (const mtx::events::account_data::IgnoredUser &user : ev.content.users) {
        users.push_back(QString::fromStdString(user.id));
    }

    return users;
}

QVector<QString>
TimelineViewManager::getIgnoredUsers()
{
    const auto cache = cache::client()->getAccountData(mtx::events::EventType::IgnoredUsers);
    if (!cache) {
        return {};
    }

    return convertIgnoredToQt(std::get<IgnoredUsers>(*cache));
}

void
TimelineViewManager::processIgnoredUsers(const mtx::responses::AccountData &data)
{
    for (const mtx::events::collections::RoomAccountDataEvents::variant &ev : data.events) {
        if (!std::holds_alternative<IgnoredUsers>(ev)) {
            continue;
        }
        const auto &ignoredEv = std::get<IgnoredUsers>(ev);

        emit this->ignoredUsersChanged(convertIgnoredToQt(ignoredEv));
        break;
    }
}
#include "moc_TimelineViewManager.cpp"
