// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QQuickView>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QSystemTrayIcon>

#include "UserSettingsPage.h"
#include "ui/OverlayModal.h"

#include "jdenticoninterface.h"

class ChatPage;
class RegisterPage;
class LoginPage;
class WelcomePage;

class LoadingIndicator;
class OverlayModal;
class SnackBar;
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

    void
    openCreateRoomDialog(std::function<void(const mtx::requests::CreateRoom &request)> callback);
    void openJoinRoomDialog(std::function<void(const QString &room_id)> callback);

    void hideOverlay();
    void showSolidOverlayModal(QWidget *content, QFlags<Qt::AlignmentFlag> flags = Qt::AlignCenter);
    void
    showTransparentOverlayModal(QWidget *content,
                                QFlags<Qt::AlignmentFlag> flags = Qt::AlignTop | Qt::AlignHCenter);

    MxcImageProvider *imageProvider() { return imgProvider; }

protected:
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event) override;

private slots:
    //! Handle interaction with the tray icon.
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    //! Show the welcome page in the main window.
    void showWelcomePage();

    //! Show the login page in the main window.
    void showLoginPage();

    //! Show the register page in the main window.
    void showRegisterPage();

    //! Show the chat page and start communicating with the given access token.
    void showChatPage();

    void showOverlayProgressBar();
    void removeOverlayProgressBar();

    virtual void setWindowTitle(int notificationCount);

signals:
    void reload();
    void secretsChanged();

    void switchToChatPage();
    void switchToWelcomePage();

private:
    void showDialog(QWidget *dialog);
    bool hasActiveUser();
    void restoreWindowSize();
    //! Check if there is an open dialog.
    bool hasActiveDialogs() const;
    //! Check if the current page supports the "minimize to tray" functionality.
    bool pageSupportsTray() const;

    void registerQmlTypes();

    static MainWindow *instance_;

    //! The initial welcome screen.
    WelcomePage *welcome_page_;
    //! The login screen.
    LoginPage *login_page_;
    //! The register page.
    RegisterPage *register_page_;
    //! The main chat area.
    ChatPage *chat_page_;
    QSharedPointer<UserSettings> userSettings_;
    //! Tray icon that shows the unread message count.
    TrayIcon *trayIcon_;

    MxcImageProvider *imgProvider = nullptr;
};
