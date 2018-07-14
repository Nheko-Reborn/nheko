#include "notifications/Manager.h"
#include "wintoastlib.h"

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
                std::wcout << "Your system in not compatible with toast notifications\n";
}
}

NotificationsManager::NotificationsManager(QObject *parent)
  : QObject(parent)
{}

void
NotificationsManager::postNotification(const QString &room_id,
                                       const QString &event_id,
                                       const QString &room_name,
                                       const QString &sender,
                                       const QString &text,
                                       const QImage &icon)
{
        Q_UNUSED(room_id)
        Q_UNUSED(event_id)
        Q_UNUSED(icon)

        if (!isInitialized)
                init();

        auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
        if (room_name != sender)
                templ.setTextField(QString("%1 - %2").arg(sender).arg(room_name).toStdWString(),
                                   WinToastTemplate::FirstLine);
        else
                templ.setTextField(QString("%1").arg(sender).toStdWString(),
                                   WinToastTemplate::FirstLine);
        templ.setTextField(QString("%1").arg(text).toStdWString(), WinToastTemplate::SecondLine);
        // TODO: implement room or user avatar
        // templ.setImagePath(L"C:/example.png");

        WinToast::instance()->showToast(templ, new CustomHandler());
}

void NotificationsManager::actionInvoked(uint, QString) {}

void NotificationsManager::notificationClosed(uint, uint) {}
