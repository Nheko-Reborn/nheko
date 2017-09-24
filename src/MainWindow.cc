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

#include "MainWindow.h"
#include "Config.h"

#include <QLayout>
#include <QNetworkReply>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QShortcut>
#include <QApplication>

MainWindow *MainWindow::instance_ = nullptr;

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , progress_modal_{ nullptr }
  , spinner_{ nullptr }
{
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setSizePolicy(sizePolicy);
        setWindowTitle("nheko");
        setObjectName("MainWindow");
        setStyleSheet("QWidget#MainWindow {background-color: #fff}");

        restoreWindowSize();
        setMinimumSize(QSize(conf::window::minWidth, conf::window::minHeight));

        QFont font("Open Sans");
        font.setPixelSize(conf::fontSize);
        font.setStyleStrategy(QFont::PreferAntialias);
        setFont(font);

        client_   = QSharedPointer<MatrixClient>(new MatrixClient("matrix.org"));
        trayIcon_ = new TrayIcon(":/logos/nheko-32.png", this);

        welcome_page_  = new WelcomePage(this);
        login_page_    = new LoginPage(client_, this);
        register_page_ = new RegisterPage(client_, this);
        chat_page_     = new ChatPage(client_, this);

        // Initialize sliding widget manager.
        sliding_stack_ = new SlidingStackWidget(this);
        sliding_stack_->addWidget(welcome_page_);
        sliding_stack_->addWidget(login_page_);
        sliding_stack_->addWidget(register_page_);
        sliding_stack_->addWidget(chat_page_);

        setCentralWidget(sliding_stack_);

        connect(welcome_page_, SIGNAL(userLogin()), this, SLOT(showLoginPage()));
        connect(welcome_page_, SIGNAL(userRegister()), this, SLOT(showRegisterPage()));

        connect(login_page_, SIGNAL(backButtonClicked()), this, SLOT(showWelcomePage()));
        connect(register_page_, SIGNAL(backButtonClicked()), this, SLOT(showWelcomePage()));

        connect(chat_page_, SIGNAL(close()), this, SLOT(showWelcomePage()));
        connect(
          chat_page_, SIGNAL(changeWindowTitle(QString)), this, SLOT(setWindowTitle(QString)));
        connect(chat_page_, SIGNAL(unreadMessages(int)), trayIcon_, SLOT(setUnreadCount(int)));

        connect(trayIcon_,
                SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this,
                SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

        connect(chat_page_, SIGNAL(contentLoaded()), this, SLOT(removeOverlayProgressBar()));

        connect(client_.data(),
                SIGNAL(loginSuccess(QString, QString, QString)),
                this,
                SLOT(showChatPage(QString, QString, QString)));

        QShortcut *quitShortcut = new QShortcut(QKeySequence::Quit, this);
        connect(quitShortcut, &QShortcut::activated, this, QApplication::quit);

        QSettings settings;

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

                if (progress_modal_ != nullptr) {
                        progress_modal_->deleteLater();
                        progress_modal_->fadeOut();
                }

                if (spinner_ != nullptr)
                        spinner_->deleteLater();

                spinner_->stop();

                progress_modal_ = nullptr;
                spinner_        = nullptr;
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

        int index                = sliding_stack_->getWidgetIndex(chat_page_);
        int modalOpacityDuration = 300;

        // If we go directly from the welcome page don't show an animation.
        if (sliding_stack_->currentIndex() == 0) {
                sliding_stack_->setCurrentIndex(index);
                modalOpacityDuration = 0;
        } else {
                sliding_stack_->slideInIndex(index,
                                             SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);
        }

        if (spinner_ == nullptr) {
                spinner_ = new LoadingIndicator(this);
                spinner_->setColor("#acc7dc");
                spinner_->setFixedHeight(120);
                spinner_->setFixedWidth(120);
                spinner_->start();
        }

        if (progress_modal_ == nullptr) {
                progress_modal_ = new OverlayModal(this, spinner_);
                progress_modal_->fadeIn();
                progress_modal_->setDuration(modalOpacityDuration);
        }

        login_page_->reset();
        chat_page_->bootstrap(userid, homeserver, token);

        instance_ = this;
}

void
MainWindow::showWelcomePage()
{
        int index = sliding_stack_->getWidgetIndex(welcome_page_);

        if (sliding_stack_->currentIndex() == sliding_stack_->getWidgetIndex(login_page_))
                sliding_stack_->slideInIndex(index,
                                             SlidingStackWidget::AnimationDirection::RIGHT_TO_LEFT);
        else
                sliding_stack_->slideInIndex(index,
                                             SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);
}

void
MainWindow::showLoginPage()
{
        int index = sliding_stack_->getWidgetIndex(login_page_);
        sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);
}

void
MainWindow::showRegisterPage()
{
        int index = sliding_stack_->getWidgetIndex(register_page_);
        sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::RIGHT_TO_LEFT);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
        if (isVisible()) {
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

MainWindow *
MainWindow::instance()
{
        return instance_;
}

MainWindow::~MainWindow()
{
}
