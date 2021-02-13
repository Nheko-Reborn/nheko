#include "notifications/Manager.h"
#include "wintoastlib.h"

#include "Cache.h"
#include "EventAccessors.h"
#include "MatrixClient.h"
#include "Utils.h"
#include <mtx/responses/notifications.hpp>

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
        Q_UNUSED(icon)

        const auto room_name =
          QString::fromStdString(cache::singleRoomInfo(notification.room_id).name);
        const auto sender =
          cache::displayName(QString::fromStdString(notification.room_id),
                             QString::fromStdString(mtx::accessors::sender(notification.event)));
        const auto text = utils::event_body(notification.event);

        if (!isInitialized)
                init();

        auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
        if (room_name != sender)
                templ.setTextField(QString("%1 - %2").arg(sender).arg(room_name).toStdWString(),
                                   WinToastTemplate::FirstLine);
        else
                templ.setTextField(QString("%1").arg(sender).toStdWString(),
                                   WinToastTemplate::FirstLine);
        if (mtx::accessors::msg_type(notification.event) == mtx::events::MessageType::Emote)
                templ.setTextField(
                  QString("* ").append(sender).append(" ").append(text).toStdWString(),
                  WinToastTemplate::SecondLine);
        else
                templ.setTextField(QString("%1").arg(text).toStdWString(),
                                   WinToastTemplate::SecondLine);
        // TODO: implement room or user avatar
        // templ.setImagePath(L"C:/example.png");

        WinToast::instance()->showToast(templ, new CustomHandler());
}

void NotificationsManager::actionInvoked(uint, QString) {}
void NotificationsManager::notificationReplied(uint, QString) {}

void NotificationsManager::notificationClosed(uint, uint) {}

void
NotificationsManager::removeNotification(const QString &, const QString &)
{}
