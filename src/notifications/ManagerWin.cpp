// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"
#include "wintoastlib.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocumentFragment>

#include <variant>

#include "Cache.h"
#include "EventAccessors.h"
#include "MxcImageProvider.h"
#include "Utils.h"

using namespace WinToastLib;

class CustomHandler : public IWinToastHandler
{
public:
    CustomHandler(NotificationsManager *manager_, const QString &roomid_, const QString &eventid_)
      : manager(manager_)
      , roomid(roomid_)
      , eventid(eventid_)
    {
    }

    void toastActivated() const { manager->notificationClicked(roomid, eventid); }
    void toastActivated(int) const {}
    void toastFailed() const { std::wcout << L"Error showing current toast" << std::endl; }
    void toastDismissed(WinToastDismissalReason) const {}

    NotificationsManager *manager;
    QString roomid;
    QString eventid;
};

namespace {
bool isInitialized = false;

void
init()
{
    isInitialized = true;

    WinToast::instance()->setAppUserModelId(
      WinToast::configureAUMI(L"NhekoReborn", L"in.nheko.Nheko"));
    WinToast::instance()->setAppName(L"Nheko");
    if (!WinToast::instance()->initialize())
        std::wcout << "Your system is not compatible with toast notifications\n";
}
}

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
{
}

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
    const auto room_name = QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);
    auto roomid          = QString::fromStdString(notification.room_id);
    auto eventid         = QString::fromStdString(mtx::accessors::event_id(notification.event));

    auto formatNotification = [this, notification] {
        const auto template_ = getMessageTemplate(notification);
        if (std::holds_alternative<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
              notification.event)) {
            return template_;
        }

        return template_.arg(utils::stripReplyFallbacks(notification.event, {}, {}).quoted_body);
    }();

    auto iconPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + room_name +
                    "-room-avatar.png";
    if (!icon.save(iconPath))
        iconPath.clear();

    if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Image ||
        mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Image) {
        MxcImageProvider::download(
          QString::fromStdString(mtx::accessors::url(notification.event))
            .remove(QStringLiteral("mxc://")),
          QSize(200, 80),
          [this, roomid, eventid, room_name, formatNotification, iconPath](
            QString, QSize, QImage, QString imgPath) {
              if (imgPath.isEmpty())
                  systemPostNotification(
                    roomid, eventid, room_name, formatNotification, iconPath, "");
              else
                  systemPostNotification(
                    roomid, eventid, room_name, formatNotification, iconPath, imgPath);
          });
    } else {
        systemPostNotification(roomid, eventid, room_name, formatNotification, iconPath, "");
    }
}

void
NotificationsManager::systemPostNotification(const QString &roomid,
                                             const QString &eventid,
                                             const QString &line1,
                                             const QString &line2,
                                             const QString &iconPath,
                                             const QString &bodyImagePath)
{
    if (!isInitialized)
        init();

    auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
    templ.setTextField(line1.toStdWString(), WinToastTemplate::FirstLine);
    templ.setTextField(line2.toStdWString(), WinToastTemplate::SecondLine);

    if (!iconPath.isNull())
        templ.setImagePath(iconPath.toStdWString());
    if (!bodyImagePath.isNull())
        templ.setHeroImagePath(bodyImagePath.toStdWString(), true);

    templ.setAudioPath(WinToastTemplate::IM);

    WinToast::instance()->showToast(templ, new CustomHandler(this, roomid, eventid));
}

// clang-format off
// clang-format < 12 is buggy on this
void
NotificationsManager::actionInvoked(uint, QString)
{}

void
NotificationsManager::activationToken(uint, QString)
{}

void
NotificationsManager::notificationReplied(uint, QString)
{}

void
NotificationsManager::notificationClosed(uint, uint)
{}

void
NotificationsManager::removeNotification(const QString &, const QString &)
{}
