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

#include <QApplication>
#include <QLayout>
#include <QPluginLoader>
#include <QSettings>
#include <QShortcut>

#include <mtx/requests.hpp>

#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "LoginPage.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RegisterPage.h"
#include "Splitter.h"
#include "TrayIcon.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "WelcomePage.h"
#include "ui/LoadingIndicator.h"
#include "ui/OverlayModal.h"
#include "ui/SnackBar.h"

#include "dialogs/CreateRoom.h"
#include "dialogs/InviteUsers.h"
#include "dialogs/JoinRoom.h"
#include "dialogs/LeaveRoom.h"
#include "dialogs/Logout.h"
#include "dialogs/MemberList.h"
#include "dialogs/ReadReceipts.h"
#include "dialogs/RoomSettings.h"

MainWindow *MainWindow::instance_ = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
        setWindowTitle("nheko");
        setObjectName("MainWindow");

        modal_ = new OverlayModal(this);

        restoreWindowSize();

        QFont font;
        font.setStyleStrategy(QFont::PreferAntialias);
        setFont(font);

        userSettings_ = QSharedPointer<UserSettings>(new UserSettings);
        trayIcon_     = new TrayIcon(":/logos/nheko.svg", this);

        welcome_page_     = new WelcomePage(this);
        login_page_       = new LoginPage(this);
        register_page_    = new RegisterPage(this);
        chat_page_        = new ChatPage(userSettings_, this);
        userSettingsPage_ = new UserSettingsPage(userSettings_, this);

        // Initialize sliding widget manager.
        pageStack_ = new QStackedWidget(this);
        pageStack_->addWidget(welcome_page_);
        pageStack_->addWidget(login_page_);
        pageStack_->addWidget(register_page_);
        pageStack_->addWidget(chat_page_);
        pageStack_->addWidget(userSettingsPage_);

        setCentralWidget(pageStack_);

        connect(welcome_page_, SIGNAL(userLogin()), this, SLOT(showLoginPage()));
        connect(welcome_page_, SIGNAL(userRegister()), this, SLOT(showRegisterPage()));

        connect(login_page_, SIGNAL(backButtonClicked()), this, SLOT(showWelcomePage()));
        connect(login_page_, &LoginPage::loggingIn, this, &MainWindow::showOverlayProgressBar);
        connect(
          register_page_, &RegisterPage::registering, this, &MainWindow::showOverlayProgressBar);
        connect(
          login_page_, &LoginPage::errorOccurred, this, [this]() { removeOverlayProgressBar(); });
        connect(register_page_, &RegisterPage::errorOccurred, this, [this]() {
                removeOverlayProgressBar();
        });
        connect(register_page_, SIGNAL(backButtonClicked()), this, SLOT(showWelcomePage()));

        connect(chat_page_, &ChatPage::closing, this, &MainWindow::showWelcomePage);
        connect(
          chat_page_, &ChatPage::showOverlayProgressBar, this, &MainWindow::showOverlayProgressBar);
        connect(
          chat_page_, SIGNAL(changeWindowTitle(QString)), this, SLOT(setWindowTitle(QString)));
        connect(chat_page_, SIGNAL(unreadMessages(int)), trayIcon_, SLOT(setUnreadCount(int)));
        connect(chat_page_, &ChatPage::showLoginPage, this, [this](const QString &msg) {
                login_page_->loginError(msg);
                showLoginPage();
        });

        connect(userSettingsPage_, &UserSettingsPage::moveBack, this, [this]() {
                pageStack_->setCurrentWidget(chat_page_);
        });

        connect(
          userSettingsPage_, SIGNAL(trayOptionChanged(bool)), trayIcon_, SLOT(setVisible(bool)));
        connect(
          userSettingsPage_, &UserSettingsPage::themeChanged, chat_page_, &ChatPage::themeChanged);
        connect(userSettingsPage_,
                &UserSettingsPage::decryptSidebarChanged,
                chat_page_,
                &ChatPage::decryptSidebarChanged);
        connect(trayIcon_,
                SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this,
                SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

        connect(chat_page_, SIGNAL(contentLoaded()), this, SLOT(removeOverlayProgressBar()));
        connect(
          chat_page_, &ChatPage::showUserSettingsPage, this, &MainWindow::showUserSettingsPage);

        connect(login_page_, &LoginPage::loginOk, this, [this](const mtx::responses::Login &res) {
                http::client()->set_user(res.user_id);
                showChatPage();
        });

        connect(register_page_, &RegisterPage::registerOk, this, &MainWindow::showChatPage);

        QShortcut *quitShortcut = new QShortcut(QKeySequence::Quit, this);
        connect(quitShortcut, &QShortcut::activated, this, QApplication::quit);

        QShortcut *quickSwitchShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
        connect(quickSwitchShortcut, &QShortcut::activated, this, [this]() {
                if (chat_page_->isVisible() && !hasActiveDialogs())
                        chat_page_->showQuickSwitcher();
        });

        QSettings settings;

        trayIcon_->setVisible(userSettings_->tray());

        if (hasActiveUser()) {
                QString token       = settings.value("auth/access_token").toString();
                QString home_server = settings.value("auth/home_server").toString();
                QString user_id     = settings.value("auth/user_id").toString();
                QString device_id   = settings.value("auth/device_id").toString();

                http::client()->set_access_token(token.toStdString());
                http::client()->set_server(home_server.toStdString());
                http::client()->set_device_id(device_id.toStdString());

                try {
                        using namespace mtx::identifiers;
                        http::client()->set_user(parse<User>(user_id.toStdString()));
                } catch (const std::invalid_argument &e) {
                        nhlog::ui()->critical("bootstrapped with invalid user_id: {}",
                                              user_id.toStdString());
                }

                showChatPage();
        }

        if (loadJdenticonPlugin()) {
                nhlog::ui()->info("loaded jdenticon.");
        }
}

void
MainWindow::showEvent(QShowEvent *event)
{
        adjustSideBars();
        QMainWindow::showEvent(event);
}

void
MainWindow::resizeEvent(QResizeEvent *event)
{
        adjustSideBars();
        QMainWindow::resizeEvent(event);
}

void
MainWindow::adjustSideBars()
{
        const auto sz = splitter::calculateSidebarSizes(QFont{});

        const uint64_t timelineWidth     = chat_page_->timelineWidth();
        const uint64_t minAvailableWidth = sz.collapsePoint + sz.groups;

        if (timelineWidth < minAvailableWidth && !chat_page_->isSideBarExpanded()) {
                chat_page_->hideSideBars();
        } else {
                chat_page_->showSideBars();
        }
}

void
MainWindow::restoreWindowSize()
{
        QSettings settings;
        int savedWidth  = settings.value("window/width").toInt();
        int savedheight = settings.value("window/height").toInt();

        if (savedWidth == 0 || savedheight == 0)
                resize(conf::window::width, conf::window::height);
        else
                resize(savedWidth, savedheight);
}

void
MainWindow::saveCurrentWindowSize()
{
        QSettings settings;
        QSize current = size();

        settings.setValue("window/width", current.width());
        settings.setValue("window/height", current.height());
}

void
MainWindow::removeOverlayProgressBar()
{
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, [this, timer]() {
                timer->deleteLater();

                if (modal_)
                        modal_->hide();

                if (spinner_)
                        spinner_->stop();
        });

        // FIXME:  Snackbar doesn't work if it's initialized in the constructor.
        QTimer::singleShot(0, this, [this]() {
                snackBar_ = new SnackBar(this);
                connect(chat_page_, &ChatPage::showNotification, snackBar_, &SnackBar::showMessage);
        });

        timer->start(50);
}

void
MainWindow::showChatPage()
{
        auto userid     = QString::fromStdString(http::client()->user_id().to_string());
        auto device_id  = QString::fromStdString(http::client()->device_id());
        auto homeserver = QString::fromStdString(http::client()->server() + ":" +
                                                 std::to_string(http::client()->port()));
        auto token      = QString::fromStdString(http::client()->access_token());

        QSettings settings;
        settings.setValue("auth/access_token", token);
        settings.setValue("auth/home_server", homeserver);
        settings.setValue("auth/user_id", userid);
        settings.setValue("auth/device_id", device_id);

        showOverlayProgressBar();

        pageStack_->setCurrentWidget(chat_page_);

        pageStack_->removeWidget(welcome_page_);
        pageStack_->removeWidget(login_page_);
        pageStack_->removeWidget(register_page_);

        login_page_->reset();
        chat_page_->bootstrap(userid, homeserver, token);

        instance_ = this;
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
        if (!qApp->isSavingSession() && isVisible() && pageSupportsTray() &&
            userSettings_->tray()) {
                event->ignore();
                hide();
        }
}

void
MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
        switch (reason) {
        case QSystemTrayIcon::Trigger:
                if (!isVisible()) {
                        show();
                } else {
                        hide();
                }
                break;
        default:
                break;
        }
}

bool
MainWindow::hasActiveUser()
{
        QSettings settings;

        return settings.contains("auth/access_token") && settings.contains("auth/home_server") &&
               settings.contains("auth/user_id");
}

void
MainWindow::openRoomSettings(const QString &room_id)
{
        const auto roomToSearch = room_id.isEmpty() ? chat_page_->currentRoom() : "";

        auto dialog = new dialogs::RoomSettings(roomToSearch, this);

        showDialog(dialog);
}

void
MainWindow::openMemberListDialog(const QString &room_id)
{
        const auto roomToSearch = room_id.isEmpty() ? chat_page_->currentRoom() : "";
        auto dialog             = new dialogs::MemberList(roomToSearch, this);

        showDialog(dialog);
}

void
MainWindow::openLeaveRoomDialog(const QString &room_id)
{
        auto roomToLeave = room_id.isEmpty() ? chat_page_->currentRoom() : room_id;

        auto dialog = new dialogs::LeaveRoom(this);
        connect(dialog, &dialogs::LeaveRoom::leaving, this, [this, roomToLeave]() {
                chat_page_->leaveRoom(roomToLeave);
        });

        showDialog(dialog);
}

void
MainWindow::showOverlayProgressBar()
{
        spinner_ = new LoadingIndicator(this);
        spinner_->setFixedHeight(100);
        spinner_->setFixedWidth(100);
        spinner_->setObjectName("ChatPageLoadSpinner");
        spinner_->start();

        showSolidOverlayModal(spinner_);
}

void
MainWindow::openInviteUsersDialog(std::function<void(const QStringList &invitees)> callback)
{
        auto dialog = new dialogs::InviteUsers(this);
        connect(dialog, &dialogs::InviteUsers::sendInvites, this, [callback](QStringList invitees) {
                if (!invitees.isEmpty())
                        callback(invitees);
        });

        showDialog(dialog);
}

void
MainWindow::openJoinRoomDialog(std::function<void(const QString &room_id)> callback)
{
        auto dialog = new dialogs::JoinRoom(this);
        connect(dialog, &dialogs::JoinRoom::joinRoom, this, [callback](const QString &room) {
                if (!room.isEmpty())
                        callback(room);
        });

        showDialog(dialog);
}

void
MainWindow::openCreateRoomDialog(
  std::function<void(const mtx::requests::CreateRoom &request)> callback)
{
        auto dialog = new dialogs::CreateRoom(this);
        connect(dialog,
                &dialogs::CreateRoom::createRoom,
                this,
                [callback](const mtx::requests::CreateRoom &request) { callback(request); });

        showDialog(dialog);
}

void
MainWindow::showTransparentOverlayModal(QWidget *content, QFlags<Qt::AlignmentFlag> flags)
{
        modal_->setWidget(content);
        modal_->setColor(QColor(30, 30, 30, 150));
        modal_->setDismissible(true);
        modal_->setContentAlignment(flags);
        modal_->raise();
        modal_->show();
}

void
MainWindow::showSolidOverlayModal(QWidget *content, QFlags<Qt::AlignmentFlag> flags)
{
        modal_->setWidget(content);
        modal_->setColor(QColor(30, 30, 30));
        modal_->setDismissible(false);
        modal_->setContentAlignment(flags);
        modal_->raise();
        modal_->show();
}

void
MainWindow::openLogoutDialog()
{
        auto dialog = new dialogs::Logout(this);
        connect(
          dialog, &dialogs::Logout::loggingOut, this, [this]() { chat_page_->initiateLogout(); });

        showDialog(dialog);
}

void
MainWindow::openReadReceiptsDialog(const QString &event_id)
{
        auto dialog = new dialogs::ReadReceipts(this);

        const auto room_id = chat_page_->currentRoom();

        try {
                dialog->addUsers(cache::readReceipts(event_id, room_id));
        } catch (const lmdb::error &e) {
                nhlog::db()->warn("failed to retrieve read receipts for {} {}",
                                  event_id.toStdString(),
                                  chat_page_->currentRoom().toStdString());
                dialog->deleteLater();

                return;
        }

        showDialog(dialog);
}

bool
MainWindow::hasActiveDialogs() const
{
        return !modal_ && modal_->isVisible();
}

bool
MainWindow::pageSupportsTray() const
{
        return !welcome_page_->isVisible() && !login_page_->isVisible() &&
               !register_page_->isVisible();
}

void
MainWindow::hideOverlay()
{
        if (modal_)
                modal_->hide();
}

inline void
MainWindow::showDialog(QWidget *dialog)
{
        utils::centerWidget(dialog, this);
        dialog->raise();
        dialog->show();
}

bool
MainWindow::loadJdenticonPlugin()
{
        QDir pluginsDir(qApp->applicationDirPath());

        bool plugins = pluginsDir.cd("plugins");
        if (plugins) {
                foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
                        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
                        QObject *plugin = pluginLoader.instance();
                        if (plugin) {
                                jdenticonInteface_ = qobject_cast<JdenticonInterface *>(plugin);
                                if (jdenticonInteface_) {
                                        nhlog::ui()->info("Found jdenticon plugin.");
                                        return true;
                                }
                        }
                }
        }

        nhlog::ui()->info("jdenticon plugin not found.");
        return false;
}
void
MainWindow::showWelcomePage()
{
        removeOverlayProgressBar();
        pageStack_->addWidget(welcome_page_);
        pageStack_->setCurrentWidget(welcome_page_);
}

void
MainWindow::showLoginPage()
{
        if (modal_)
                modal_->hide();

        pageStack_->addWidget(login_page_);
        pageStack_->setCurrentWidget(login_page_);
}

void
MainWindow::showRegisterPage()
{
        pageStack_->addWidget(register_page_);
        pageStack_->setCurrentWidget(register_page_);
}

void
MainWindow::showUserSettingsPage()
{
        pageStack_->setCurrentWidget(userSettingsPage_);
}
