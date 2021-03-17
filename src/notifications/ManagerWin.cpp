// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifications/Manager.h"
#include "wintoastlib.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocumentFragment>

#include "Cache.h"
#include "EventAccessors.h"
#include "Utils.h"

using namespace WinToastLib;

class CustomHandler : public IWinToastHandler
{
public:
        void toastActivated() const {}
        void toastActivated(int) const {}
        void toastFailed() const { std::wcout << L"Error showing current toast" << std::endl; }
        void toastDismissed(WinToastDismissalReason) const {}
};

namespace {
bool isInitialized = false;

void
init()
{
        isInitialized = true;

        WinToast::instance()->setAppName(L"Nheko");
        WinToast::instance()->setAppUserModelId(WinToast::configureAUMI(L"nheko", L"nheko"));
        if (!WinToast::instance()->initialize())
                std::wcout << "Your system is not compatible with toast notifications\n";
}
}

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
{}

void
NotificationsManager::postNotification(const mtx::responses::Notification &notification,
                                       const QImage &icon)
{
        const auto room_name =
          QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));
        const auto text = formatNotification(notification);

        systemPostNotification(room_name, sender, text, icon);
}

void
NotificationsManager::systemPostNotification(const QString &roomName,
                                             const QString &sender,
                                             const QString &text,
                                             const QImage &icon)
{
        if (!isInitialized)
                init();

        auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
        if (roomName != sender)
                templ.setTextField(QString("%1 - %2").arg(sender).arg(roomName).toStdWString(),
                                   WinToastTemplate::FirstLine);
        else
                templ.setTextField(sender.toStdWString(), WinToastTemplate::FirstLine);
        templ.setTextField(text.toStdWString(), WinToastTemplate::SecondLine);

        auto iconPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + roomName +
                        "-room-avatar.png";
        if (icon.save(iconPath))
                templ.setImagePath(iconPath.toStdWString());

        WinToast::instance()->showToast(templ, new CustomHandler());
}

void NotificationsManager::actionInvoked(uint, QString) {}
void NotificationsManager::notificationReplied(uint, QString) {}

void NotificationsManager::notificationClosed(uint, uint) {}

void
NotificationsManager::removeNotification(const QString &, const QString &)
{}

QString
NotificationsManager::formatNotification(const mtx::responses::Notification &notification)
{
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));

        return QTextDocumentFragment::fromHtml(
                 mtx::accessors::formattedBodyWithFallback(notification.event)
                   .replace(QRegularExpression("(<mx-reply>.+\\<\\/mx-reply\\>)"), ""))
          .toPlainText()
          .prepend((mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote)
                     ? "* " + sender + " "
                     : "");
}
