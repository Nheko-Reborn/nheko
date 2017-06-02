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

#include "InputValidator.h"
#include "LoginPage.h"

LoginPage::LoginPage(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , settings_modal_{nullptr}
    , login_settings_{nullptr}
    , client_{client}
{
	setStyleSheet("background-color: #f9f9f9");

	top_layout_ = new QVBoxLayout();

	top_bar_layout_ = new QHBoxLayout();
	top_bar_layout_->setSpacing(0);
	top_bar_layout_->setMargin(0);

	back_button_ = new FlatButton(this);
	back_button_->setMinimumSize(QSize(30, 30));
	back_button_->setForegroundColor("#333333");

	advanced_settings_button_ = new FlatButton(this);
	advanced_settings_button_->setMinimumSize(QSize(30, 30));
	advanced_settings_button_->setForegroundColor("#333333");

	QIcon icon;
	icon.addFile(":/icons/icons/left-angle.png", QSize(), QIcon::Normal, QIcon::Off);

	back_button_->setIcon(icon);
	back_button_->setIconSize(QSize(24, 24));

	QIcon advanced_settings_icon;
	advanced_settings_icon.addFile(":/icons/icons/cog.png", QSize(), QIcon::Normal, QIcon::Off);

	advanced_settings_button_->setIcon(advanced_settings_icon);
	advanced_settings_button_->setIconSize(QSize(24, 24));

	top_bar_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
	top_bar_layout_->addStretch(1);
	top_bar_layout_->addWidget(advanced_settings_button_, 0, Qt::AlignRight | Qt::AlignVCenter);

	logo_ = new QLabel(this);
	logo_->setPixmap(QPixmap(":/logos/nheko-128.png"));

	logo_layout_ = new QHBoxLayout();
	logo_layout_->setContentsMargins(0, 0, 0, 20);
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

	matrixid_input_ = new TextField(this);
	matrixid_input_->setTextColor("#333333");
	matrixid_input_->setLabel(tr("Matrix ID"));
	matrixid_input_->setInkColor("#555459");
	matrixid_input_->setBackgroundColor("#f9f9f9");
	matrixid_input_->setPlaceholderText(tr("e.g @joe:matrix.org"));

	password_input_ = new TextField(this);
	password_input_->setTextColor("#333333");
	password_input_->setLabel(tr("Password"));
	password_input_->setInkColor("#555459");
	password_input_->setBackgroundColor("#f9f9f9");
	password_input_->setEchoMode(QLineEdit::Password);

	form_layout_->addWidget(matrixid_input_, Qt::AlignHCenter, 0);
	form_layout_->addWidget(password_input_, Qt::AlignHCenter, 0);

	button_layout_ = new QHBoxLayout();
	button_layout_->setSpacing(0);
	button_layout_->setContentsMargins(0, 0, 0, 30);

	login_button_ = new RaisedButton(tr("LOGIN"), this);
	login_button_->setBackgroundColor(QColor("#333333"));
	login_button_->setForegroundColor(QColor("white"));
	login_button_->setMinimumSize(350, 65);
	login_button_->setFontSize(17);
	login_button_->setCornerRadius(3);

	button_layout_->addStretch(1);
	button_layout_->addWidget(login_button_);
	button_layout_->addStretch(1);

	error_label_ = new QLabel(this);
	error_label_->setStyleSheet("color: #E22826; font-size: 11pt;");

	top_layout_->addLayout(top_bar_layout_);
	top_layout_->addStretch(1);
	top_layout_->addLayout(logo_layout_);
	top_layout_->addLayout(form_wrapper_);
	top_layout_->addStretch(1);
	top_layout_->addLayout(button_layout_);
	top_layout_->addWidget(error_label_, 0, Qt::AlignHCenter);
	top_layout_->addStretch(1);

	setLayout(top_layout_);

	connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
	connect(login_button_, SIGNAL(clicked()), this, SLOT(onLoginButtonClicked()));
	connect(matrixid_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
	connect(password_input_, SIGNAL(returnPressed()), login_button_, SLOT(click()));
	connect(client_.data(), SIGNAL(loginError(QString)), this, SLOT(loginError(QString)));
	connect(advanced_settings_button_, SIGNAL(clicked()), this, SLOT(showSettingsModal()));

	matrixid_input_->setValidator(&InputValidator::Id);
}

void LoginPage::loginError(QString error)
{
	error_label_->setText(error);
}

void LoginPage::onLoginButtonClicked()
{
	error_label_->setText("");

	if (!matrixid_input_->hasAcceptableInput()) {
		loginError(tr("Invalid Matrix ID"));
	} else if (password_input_->text().isEmpty()) {
		loginError(tr("Empty password"));
	} else {
		QString user = matrixid_input_->text().split(":").at(0).split("@").at(1);
		QString password = password_input_->text();

		QString home_server = custom_domain_.isEmpty()
					      ? matrixid_input_->text().split(":").at(1)
					      : custom_domain_;

		client_->setServer(home_server);
		client_->login(user, password);
	}
}

void LoginPage::showSettingsModal()
{
	if (login_settings_ == nullptr) {
		login_settings_ = new LoginSettings(this);
		connect(login_settings_, &LoginSettings::closing, this, &LoginPage::closeSettingsModal);
	}

	if (settings_modal_ == nullptr) {
		settings_modal_ = new OverlayModal(this, login_settings_);
		settings_modal_->setDuration(100);
		settings_modal_->setColor(QColor(55, 55, 55, 170));
	}

	settings_modal_->fadeIn();
}

void LoginPage::closeSettingsModal(const QString &server)
{
	custom_domain_ = server;
	settings_modal_->fadeOut();
}

void LoginPage::reset()
{
	matrixid_input_->clear();
	password_input_->clear();

	if (settings_modal_ != nullptr) {
		settings_modal_->deleteLater();
		settings_modal_ = nullptr;
	}

	if (login_settings_ != nullptr) {
		login_settings_->deleteLater();
		login_settings_ = nullptr;
	}
}

void LoginPage::onBackButtonClicked()
{
	emit backButtonClicked();
}

LoginPage::~LoginPage()
{
}
