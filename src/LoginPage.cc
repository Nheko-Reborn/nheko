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

#include <QDebug>

#include "LoginPage.h"

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
    , matrix_id_validator_(new InputValidator(parent))
{
	top_layout_ = new QVBoxLayout();

	back_layout_ = new QHBoxLayout();
	back_layout_->setSpacing(0);
	back_layout_->setContentsMargins(5, 5, -1, -1);

	back_button_ = new FlatButton(this);
	back_button_->setMinimumSize(QSize(30, 30));
	back_button_->setCursor(QCursor(Qt::PointingHandCursor));

	QIcon icon;
	icon.addFile(":/icons/icons/left-angle.png", QSize(), QIcon::Normal, QIcon::Off);

	back_button_->setIcon(icon);
	back_button_->setIconSize(QSize(24, 24));

	back_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
	back_layout_->addStretch(1);

	logo_layout_ = new QHBoxLayout();
	logo_layout_->setContentsMargins(0, 20, 0, 20);
	logo_ = new QLabel(this);
	logo_->setText("nheko");
	logo_->setStyleSheet("font-size: 22pt; font-weight: 400;");
	logo_layout_->addWidget(logo_, 0, Qt::AlignHCenter);

	form_wrapper_ = new QHBoxLayout();
	form_widget_ = new QWidget();
	form_widget_->setMinimumSize(QSize(350, 200));

	form_layout_ = new QVBoxLayout();
	form_layout_->setSpacing(20);
	form_layout_->setContentsMargins(0, 0, 0, 30);
	form_widget_->setLayout(form_layout_);

	form_wrapper_->addStretch(1);
	form_wrapper_->addWidget(form_widget_);
	form_wrapper_->addStretch(1);

	username_input_ = new TextField();
	username_input_->setLabel("Username");
	username_input_->setInkColor("#577275");
	username_input_->setBackgroundColor("#f9f9f9");

	password_input_ = new TextField();
	password_input_->setLabel("Password");
	password_input_->setInkColor("#577275");
	password_input_->setBackgroundColor("#f9f9f9");
	password_input_->setEchoMode(QLineEdit::Password);

	form_layout_->addWidget(username_input_, Qt::AlignHCenter, 0);
	form_layout_->addWidget(password_input_, Qt::AlignHCenter, 0);

	button_layout_ = new QHBoxLayout();
	button_layout_->setSpacing(0);
	button_layout_->setContentsMargins(0, 0, 0, 50);

	login_button_ = new RaisedButton("LOGIN", this);
	login_button_->setBackgroundColor(QColor("#171919"));
	login_button_->setForegroundColor(QColor("#ebebeb"));
	login_button_->setMinimumSize(350, 65);
	login_button_->setCursor(QCursor(Qt::PointingHandCursor));
	login_button_->setFontSize(17);
	login_button_->setCornerRadius(3);

	button_layout_->addStretch(1);
	button_layout_->addWidget(login_button_);
	button_layout_->addStretch(1);

	error_label_ = new QLabel(this);
	error_label_->setStyleSheet("color: #E22826; font-size: 11pt;");

	top_layout_->addLayout(back_layout_);
	top_layout_->addStretch(1);
	top_layout_->addLayout(logo_layout_);
	top_layout_->addLayout(form_wrapper_);
	top_layout_->addStretch(1);
	top_layout_->addLayout(button_layout_);
	top_layout_->addWidget(error_label_, 0, Qt::AlignHCenter);
	top_layout_->addStretch(1);

	connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
	connect(login_button_, SIGNAL(clicked()), this, SLOT(onLoginButtonClicked()));
	connect(username_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
	connect(password_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));

	username_input_->setValidator(matrix_id_validator_->id_);

	setLayout(top_layout_);
}

void LoginPage::loginError(QString error)
{
	qWarning() << "Error Message: " << error;
	error_label_->setText(error);
}

void LoginPage::onLoginButtonClicked()
{
	error_label_->setText("");

	if (!username_input_->hasAcceptableInput()) {
		loginError("Invalid Matrix ID");
	} else if (password_input_->text().isEmpty()) {
		loginError("Empty password");
	} else {
		QString user = username_input_->text().split("@").at(0);
		QString home_server = username_input_->text().split("@").at(1);
		QString password = password_input_->text();

		emit userLogin(user, password, home_server);
	}
}

void LoginPage::reset()
{
	username_input_->clear();
	password_input_->clear();
}

void LoginPage::onBackButtonClicked()
{
	emit backButtonClicked();
}

LoginPage::~LoginPage()
{
}
