// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QHash>
#include <QQuickView>
#include <QSharedPointer>
#include <QSystemTrayIcon>

#include "UserSettingsPage.h"
#include "dock/Dock.h"

#include "jdenticoninterface.h"

class ChatPage;
class RegisterPage;
class WelcomePage;

class TrayIcon;
class UserSettings;
class MxcImageProvider;

namespace mtx {
namespace requests {
struct CreateRoom;
}
}

namespace dialogs {
class CreateRoom;
class InviteUsers;
class MemberList;
class ReCaptcha;
}

class MainWindow : public QQuickView
{
    Q_OBJECT

public:
    explicit MainWindow(QWindow *parent = nullptr);

    static MainWindow *instance() { return instance_; }
    void saveCurrentWindowSize();

    void openJoinRoomDialog(std::function<void(const QString &room_id)> callback);

    MxcImageProvider *imageProvider() { return imgProvider; }

    //! Show the chat page and start communicating with the given access token.
    void showChatPage();

#ifdef NHEKO_DBUS_SYS
    bool dbusAvailable() const { return dbusAvailable_; }
#endif

    Q_INVOKABLE void addPerRoomWindow(const QString &room, QWindow *window);
    Q_INVOKABLE void removePerRoomWindow(const QString &room, QWindow *window);
    QWindow *windowForRoom(const QString &room);
    QString focusedRoom() const;

protected:
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event) override;

private slots:
    //! Handle interaction with the tray icon.
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    virtual void setWindowTitle(int notificationCount);

signals:
    void reload();
    void secretsChanged();

    void showNotification(QString msg);

    void switchToChatPage();
    void switchToWelcomePage();
    void switchToLoginPage(QString error);

private:
    void showDialog(QWidget *dialog);
    bool hasActiveUser();
    void restoreWindowSize();
    //! Check if the current page supports the "minimize to tray" functionality.
    bool pageSupportsTray() const;

    void registerQmlTypes();

    static MainWindow *instance_;

    //! The initial welcome screen.
    WelcomePage *welcome_page_;
    //! The register page.
    RegisterPage *register_page_;
    //! The main chat area.
    ChatPage *chat_page_;
    QSharedPointer<UserSettings> userSettings_;
    //! Tray icon that shows the unread message count.
    TrayIcon *trayIcon_;
    Dock *dock_;

    MxcImageProvider *imgProvider = nullptr;

    QMultiHash<QString, QWindow *> roomWindows_;

#ifdef NHEKO_DBUS_SYS
    bool dbusAvailable_{false};
#endif
};
