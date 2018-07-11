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

NotificationsManager::NotificationsManager(QObject *parent): QObject(parent)
{

}

void
NotificationsManager::postNotification(const QString &, //roomid
                                const QString &,        //eventid
                                const QString &roomname,
                                const QString &sender,
                                const QString &text,
                                const QImage &)         //icon
{
        if (!isInitialized)
                init();

        auto templ = WinToastTemplate(WinToastTemplate::ImageAndText02);
        if (roomname != sender)
                templ.setTextField(QString("%1 - %2").arg(sender).arg(roomname).toStdWString(),
                                   WinToastTemplate::FirstLine);
        else
                templ.setTextField(QString("%1").arg(user).toStdWString(),
                                   WinToastTemplate::FirstLine);
        templ.setTextField(QString("%1").arg(msg).toStdWString(), WinToastTemplate::SecondLine);
        // TODO: implement room or user avatar
        // templ.setImagePath(L"C:/example.png");

        WinToast::instance()->showToast(templ, new CustomHandler());
}

//unused
void
NotificationsManager::actionInvoked(uint, QString)
{
}

void
NotificationsManager::notificationClosed(uint, uint)
{
}
