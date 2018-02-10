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
#include <QNetworkReply>
#include <QSettings>
#include <QShortcut>

#include "ChatPage.h"
#include "Config.h"
#include "LoadingIndicator.h"
#include "LoginPage.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "RegisterPage.h"
#include "SnackBar.h"
#include "TrayIcon.h"
#include "UserSettingsPage.h"
#include "WelcomePage.h"
#include "dialogs/LeaveRoom.h"

MainWindow *MainWindow::instance_ = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , progressModal_{nullptr}
  , spinner_{nullptr}
{
        setWindowTitle("nheko");
        setObjectName("MainWindow");

        restoreWindowSize();

        QFont font("Open Sans");
        font.setPixelSize(conf::fontSize);
        font.setStyleStrategy(QFont::PreferAntialias);
        setFont(font);

        client_       = QSharedPointer<MatrixClient>(new MatrixClient("matrix.org"));
        userSettings_ = QSharedPointer<UserSettings>(new UserSettings);
        trayIcon_     = new TrayIcon(":/logos/nheko-32.png", this);

        welcome_page_     = new WelcomePage(this);
        login_page_       = new LoginPage(client_, this);
        register_page_    = new RegisterPage(client_, this);
        chat_page_        = new ChatPage(client_, userSettings_, this);
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
        connect(register_page_, SIGNAL(backButtonClicked()), this, SLOT(showWelcomePage()));

        connect(chat_page_, SIGNAL(close()), this, SLOT(showWelcomePage()));
        connect(
          chat_page_, SIGNAL(changeWindowTitle(QString)), this, SLOT(setWindowTitle(QString)));
        connect(chat_page_, SIGNAL(unreadMessages(int)), trayIcon_, SLOT(setUnreadCount(int)));
        connect(chat_page_, &ChatPage::showLoginPage, this, [=](const QString &msg) {
                login_page_->loginError(msg);
                showLoginPage();
        });

        connect(userSettingsPage_, &UserSettingsPage::moveBack, this, [=]() {
                pageStack_->setCurrentWidget(chat_page_);
        });

        connect(
          userSettingsPage_, SIGNAL(trayOptionChanged(bool)), trayIcon_, SLOT(setVisible(bool)));

        connect(trayIcon_,
                SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this,
                SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

        connect(chat_page_, SIGNAL(contentLoaded()), this, SLOT(removeOverlayProgressBar()));
        connect(
          chat_page_, &ChatPage::showUserSettingsPage, this, &MainWindow::showUserSettingsPage);

        connect(client_.data(),
                SIGNAL(loginSuccess(QString, QString, QString)),
                this,
                SLOT(showChatPage(QString, QString, QString)));

        QShortcut *quitShortcut = new QShortcut(QKeySequence::Quit, this);
        connect(quitShortcut, &QShortcut::activated, this, QApplication::quit);

        QShortcut *quickSwitchShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
        connect(quickSwitchShortcut, &QShortcut::activated, this, [=]() {
                chat_page_->showQuickSwitcher();
        });

        QSettings settings;

        trayIcon_->setVisible(userSettings_->isTrayEnabled());

        if (hasActiveUser()) {
                QString token       = settings.value("auth/access_token").toString();
                QString home_server = settings.value("auth/home_server").toString();
                QString user_id     = settings.value("auth/user_id").toString();

                showChatPage(user_id, home_server, token);
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

        connect(timer, &QTimer::timeout, [=]() {
                timer->deleteLater();

                if (!progressModal_.isNull())
                        progressModal_->fadeOut();

                if (!spinner_.isNull())
                        spinner_->stop();

                progressModal_.reset();
                spinner_.reset();
        });

        // FIXME:  Snackbar doesn't work if it's initialized in the constructor.
        QTimer::singleShot(100, this, [=]() {
                snackBar_ = QSharedPointer<SnackBar>(new SnackBar(this));
                connect(chat_page_,
                        &ChatPage::showNotification,
                        snackBar_.data(),
                        &SnackBar::showMessage);
        });

        timer->start(500);
}

void
MainWindow::showChatPage(QString userid, QString homeserver, QString token)
{
        QSettings settings;
        settings.setValue("auth/access_token", token);
        settings.setValue("auth/home_server", homeserver);
        settings.setValue("auth/user_id", userid);

        int modalOpacityDuration = 300;

        // If we go directly from the welcome page don't show an animation.
        if (pageStack_->currentIndex() == 0)
                modalOpacityDuration = 0;

        QTimer::singleShot(
          modalOpacityDuration + 100, this, [=]() { pageStack_->setCurrentWidget(chat_page_); });

        if (spinner_.isNull()) {
                spinner_ = QSharedPointer<LoadingIndicator>(
                  new LoadingIndicator(this),
                  [=](LoadingIndicator *indicator) { indicator->deleteLater(); });
                spinner_->setFixedHeight(100);
                spinner_->setFixedWidth(100);
                spinner_->setObjectName("ChatPageLoadSpinner");
                spinner_->start();
        }

        if (progressModal_.isNull()) {
                progressModal_ =
                  QSharedPointer<OverlayModal>(new OverlayModal(this, spinner_.data()),
                                               [=](OverlayModal *modal) { modal->deleteLater(); });
                progressModal_->setDismissible(false);
                progressModal_->fadeIn();
                progressModal_->setDuration(modalOpacityDuration);
        }

        login_page_->reset();
        chat_page_->bootstrap(userid, homeserver, token);

        instance_ = this;
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
        // Decide whether or not we should enable tray for the current page.
        bool pageSupportsTray =
          !welcome_page_->isVisible() && !login_page_->isVisible() && !register_page_->isVisible();

        if (isVisible() && pageSupportsTray && userSettings_->isTrayEnabled()) {
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
MainWindow::openLeaveRoomDialog(const QString &room_id)
{
        auto roomToLeave = room_id.isEmpty() ? chat_page_->currentRoom() : room_id;

        leaveRoomDialog_ = QSharedPointer<dialogs::LeaveRoom>(new dialogs::LeaveRoom(this));

        connect(leaveRoomDialog_.data(), &dialogs::LeaveRoom::closing, this, [=](bool leaving) {
                leaveRoomModal_->fadeOut();

                if (leaving)
                        client_->leaveRoom(roomToLeave);
        });

        leaveRoomModal_ =
          QSharedPointer<OverlayModal>(new OverlayModal(this, leaveRoomDialog_.data()));
        leaveRoomModal_->setDuration(0);
        leaveRoomModal_->setColor(QColor(30, 30, 30, 170));

        leaveRoomModal_->fadeIn();
}
