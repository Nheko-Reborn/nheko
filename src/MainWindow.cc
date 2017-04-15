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
#include "ui_MainWindow.h"

#include <QLayout>
#include <QNetworkReply>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
{
	ui_->setupUi(this);
	client_ = QSharedPointer<MatrixClient>(new MatrixClient("matrix.org"));

	welcome_page_ = new WelcomePage(this);

	login_page_ = new LoginPage(client_, this);
	register_page_ = new RegisterPage(client_, this);
	chat_page_ = new ChatPage(client_, this);

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
	connect(chat_page_, SIGNAL(changeWindowTitle(QString)), this, SLOT(setWindowTitle(QString)));

	connect(client_.data(),
		SIGNAL(loginSuccess(QString, QString, QString)),
		this,
		SLOT(showChatPage(QString, QString, QString)));
}

void MainWindow::showChatPage(QString userid, QString homeserver, QString token)
{
	QSettings settings;
	settings.setValue("auth/access_token", token);
	settings.setValue("auth/home_server", homeserver);
	settings.setValue("auth/user_id", userid);

	int index = sliding_stack_->getWidgetIndex(chat_page_);
	sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);

	login_page_->reset();
	chat_page_->bootstrap(userid, homeserver, token);
}

void MainWindow::showWelcomePage()
{
	int index = sliding_stack_->getWidgetIndex(welcome_page_);

	if (sliding_stack_->currentIndex() == sliding_stack_->getWidgetIndex(login_page_))
		sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::RIGHT_TO_LEFT);
	else
		sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);
}

void MainWindow::showLoginPage()
{
	QSettings settings;

	if (settings.contains("auth/access_token") &&
	    settings.contains("auth/home_server") &&
	    settings.contains("auth/user_id")) {
		QString token = settings.value("auth/access_token").toString();
		QString home_server = settings.value("auth/home_server").toString();
		QString user_id = settings.value("auth/user_id").toString();

		showChatPage(user_id, home_server, token);

		return;
	}

	int index = sliding_stack_->getWidgetIndex(login_page_);
	sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::LEFT_TO_RIGHT);
}

void MainWindow::showRegisterPage()
{
	int index = sliding_stack_->getWidgetIndex(register_page_);
	sliding_stack_->slideInIndex(index, SlidingStackWidget::AnimationDirection::RIGHT_TO_LEFT);
}

MainWindow::~MainWindow()
{
	delete ui_;
}
