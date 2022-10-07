// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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
    const auto room_name = QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);

    auto formatNotification = [this, notification] {
        const auto template_ = getMessageTemplate(notification);
        if (std::holds_alternative<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
              notification.event)) {
            return template_;
        }

        return template_.arg(utils::stripReplyFallbacks(notification.event, {}, {}).quoted_body);
    };

    auto iconPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + room_name +
                    "-room-avatar.png";
    if (!icon.save(iconPath))
        iconPath.clear();

    systemPostNotification(room_name, formatNotification(), iconPath);
}

void
NotificationsManager::systemPostNotification(const QString &line1,
                                             const QString &line2,
                                             const QString &iconPath)
{
    if (!isInitialized)
        init();

    auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
    templ.setTextField(line1.toStdWString(), WinToastTemplate::FirstLine);
    templ.setTextField(line2.toStdWString(), WinToastTemplate::SecondLine);

    if (!iconPath.isNull())
        templ.setImagePath(iconPath.toStdWString());

    WinToast::instance()->showToast(templ, new CustomHandler());
}

// clang-format off
// clang-format < 12 is buggy on this
void
NotificationsManager::actionInvoked(uint, QString)
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
