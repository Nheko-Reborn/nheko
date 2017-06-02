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

#include "WelcomePage.h"

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	top_layout_ = new QVBoxLayout(this);
	top_layout_->setSpacing(0);
	top_layout_->setMargin(0);

	intro_banner_ = new QLabel(this);
	intro_banner_->setPixmap(QPixmap(":/logos/nheko-256.png"));
	intro_banner_->setAlignment(Qt::AlignCenter);

	intro_text_ = new QLabel(this);

	QString heading(tr("Welcome to nheko! The desktop client for the Matrix protocol."));
	QString main(tr("Enjoy your stay!"));

	intro_text_->setText(QString("<p align=\"center\" style=\"margin: 0; line-height: 2pt\">"
				     "  <span style=\" font-size:18px; color:#515151;\"> %1 </span>"
				     "</p>"
				     "<p align=\"center\" style=\"margin: 1pt; line-height: 2pt;\">"
				     "  <span style=\" font-size:18px; color:#515151;\"> %2 </span>"
				     "</p>")
				     .arg(heading)
				     .arg(main));

	top_layout_->addStretch(1);
	top_layout_->addWidget(intro_banner_);
	top_layout_->addStretch(1);
	top_layout_->addWidget(intro_text_, 0, Qt::AlignCenter);
	top_layout_->addStretch(1);

	button_layout_ = new QHBoxLayout();
	button_layout_->setSpacing(0);
	button_layout_->setContentsMargins(0, 20, 0, 80);

	register_button_ = new RaisedButton(tr("REGISTER"), this);
	register_button_->setBackgroundColor(QColor("#333333"));
	register_button_->setForegroundColor(QColor("white"));
	register_button_->setMinimumSize(240, 60);
	register_button_->setFontSize(14);
	register_button_->setCornerRadius(3);

	login_button_ = new RaisedButton(tr("LOGIN"), this);
	login_button_->setBackgroundColor(QColor("#333333"));
	login_button_->setForegroundColor(QColor("white"));
	login_button_->setMinimumSize(240, 60);
	login_button_->setFontSize(14);
	login_button_->setCornerRadius(3);

	button_spacer_ = new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

	button_layout_->addStretch(1);
	button_layout_->addWidget(register_button_);
	button_layout_->addItem(button_spacer_);
	button_layout_->addWidget(login_button_);
	button_layout_->addStretch(1);

	top_layout_->addLayout(button_layout_);

	connect(register_button_, SIGNAL(clicked()), this, SLOT(onRegisterButtonClicked()));
	connect(login_button_, SIGNAL(clicked()), this, SLOT(onLoginButtonClicked()));
}

void WelcomePage::onLoginButtonClicked()
{
	emit userLogin();
}

void WelcomePage::onRegisterButtonClicked()
{
	emit userRegister();
}

WelcomePage::~WelcomePage()
{
}
