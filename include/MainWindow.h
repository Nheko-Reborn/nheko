/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>

#include <QMainWindow>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QSystemTrayIcon>

#include "LoginPage.h"
#include "RegisterPage.h"
#include "UserSettingsPage.h"
#include "WelcomePage.h"

class ChatPage;
class LoadingIndicator;
class MatrixClient;
class OverlayModal;
class SnackBar;
class TrayIcon;
class UserSettings;

namespace mtx {
namespace requests {
struct CreateRoom;
}
}

namespace dialogs {
class CreateRoom;
class InviteUsers;
class JoinRoom;
class LeaveRoom;
class Logout;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

public:
        explicit MainWindow(QWidget *parent = 0);

        static MainWindow *instance() { return instance_; };
        void saveCurrentWindowSize();

        void openLeaveRoomDialog(const QString &room_id = "");
        void openInviteUsersDialog(std::function<void(const QStringList &invitees)> callback);
        void openCreateRoomDialog(
          std::function<void(const mtx::requests::CreateRoom &request)> callback);
        void openJoinRoomDialog(std::function<void(const QString &room_id)> callback);
        void openLogoutDialog(std::function<void()> callback);

protected:
        void closeEvent(QCloseEvent *event);

private slots:
        //! Handle interaction with the tray icon.
        void iconActivated(QSystemTrayIcon::ActivationReason reason);

        //! Show the welcome page in the main window.
        void showWelcomePage()
        {
                removeOverlayProgressBar();
                pageStack_->setCurrentWidget(welcome_page_);
        }

        //! Show the login page in the main window.
        void showLoginPage() { pageStack_->setCurrentWidget(login_page_); }

        //! Show the register page in the main window.
        void showRegisterPage() { pageStack_->setCurrentWidget(register_page_); }

        //! Show user settings page.
        void showUserSettingsPage() { pageStack_->setCurrentWidget(userSettingsPage_); }

        //! Show the chat page and start communicating with the given access token.
        void showChatPage(QString user_id, QString home_server, QString token);

        void showOverlayProgressBar();
        void removeOverlayProgressBar();

private:
        bool hasActiveUser();
        void restoreWindowSize();
        //! Check if there is an open dialog.
        bool hasActiveDialogs() const;

        static MainWindow *instance_;

        //! The initial welcome screen.
        WelcomePage *welcome_page_;
        //! The login screen.
        LoginPage *login_page_;
        //! The register page.
        RegisterPage *register_page_;
        //! A stacked widget that handles the transitions between widgets.
        QStackedWidget *pageStack_;
        //! The main chat area.
        ChatPage *chat_page_;
        UserSettingsPage *userSettingsPage_;
        QSharedPointer<UserSettings> userSettings_;
        //! Used to hide undefined states between page transitions.
        QSharedPointer<OverlayModal> progressModal_;
        QSharedPointer<LoadingIndicator> spinner_;
        //! Matrix Client API provider.
        QSharedPointer<MatrixClient> client_;
        //! Tray icon that shows the unread message count.
        TrayIcon *trayIcon_;
        //! Notifications display.
        QSharedPointer<SnackBar> snackBar_;
        //! Leave room modal.
        QSharedPointer<OverlayModal> leaveRoomModal_;
        //! Leave room dialog.
        QSharedPointer<dialogs::LeaveRoom> leaveRoomDialog_;
        //! Invite users modal.
        QSharedPointer<OverlayModal> inviteUsersModal_;
        //! Invite users dialog.
        QSharedPointer<dialogs::InviteUsers> inviteUsersDialog_;
        //! Join room modal.
        QSharedPointer<OverlayModal> joinRoomModal_;
        //! Join room dialog.
        QSharedPointer<dialogs::JoinRoom> joinRoomDialog_;
        //! Create room modal.
        QSharedPointer<OverlayModal> createRoomModal_;
        //! Create room dialog.
        QSharedPointer<dialogs::CreateRoom> createRoomDialog_;
        //! Logout modal.
        QSharedPointer<OverlayModal> logoutModal_;
        //! Logout dialog.
        QSharedPointer<dialogs::Logout> logoutDialog_;
};
