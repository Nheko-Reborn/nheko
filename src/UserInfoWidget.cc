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

#include "UserInfoWidget.h"
#include "FlatButton.h"

UserInfoWidget::UserInfoWidget(QWidget *parent)
    : QWidget(parent)
    , display_name_("User")
    , user_id_("@user:homeserver.org")
{
	QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	setSizePolicy(sizePolicy);
	setMinimumSize(QSize(0, 65));

	topLayout_ = new QHBoxLayout(this);
	topLayout_->setSpacing(0);
	topLayout_->setContentsMargins(5, 5, 5, 5);

	avatarLayout_ = new QHBoxLayout();
	textLayout_ = new QVBoxLayout();

	userAvatar_ = new Avatar(this);
	userAvatar_->setLetter(QChar('?'));
	userAvatar_->setSize(55);
	userAvatar_->setBackgroundColor("#f9f9f9");
	userAvatar_->setTextColor("#333333");

	displayNameLabel_ = new QLabel(this);
	displayNameLabel_->setStyleSheet(
		"padding: 0 9px;"
		"color: #171919;"
		"font-size: 14px;"
		"font-weight: 500;"
		"margin-bottom: -10px;");
	displayNameLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

	userIdLabel_ = new QLabel(this);
	userIdLabel_->setStyleSheet(
		"padding: 0 8px 8px 8px;"
		"color: #555459;"
		"font-size: 13px");
	userIdLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);

	avatarLayout_->addWidget(userAvatar_);
	textLayout_->addWidget(displayNameLabel_);
	textLayout_->addWidget(userIdLabel_);

	topLayout_->addLayout(avatarLayout_);
	topLayout_->addLayout(textLayout_);
	topLayout_->addStretch(1);

	buttonLayout_ = new QHBoxLayout();

	logoutButton_ = new FlatButton(this);
	logoutButton_->setForegroundColor(QColor("#555459"));
	logoutButton_->setCursor(QCursor(Qt::PointingHandCursor));
	logoutButton_->setStyleSheet("width: 30px; height: 30px;");

	QIcon icon;
	icon.addFile(":/icons/icons/power-button-off.png", QSize(), QIcon::Normal, QIcon::Off);

	logoutButton_->setIcon(icon);
	logoutButton_->setIconSize(QSize(16, 16));

	buttonLayout_->addWidget(logoutButton_);

	topLayout_->addLayout(buttonLayout_);

	connect(logoutButton_, SIGNAL(clicked()), this, SIGNAL(logout()));
}

UserInfoWidget::~UserInfoWidget()
{
}

void UserInfoWidget::reset()
{
	displayNameLabel_->setText("");
	userIdLabel_->setText("");
	userAvatar_->setLetter(QChar('?'));
}

void UserInfoWidget::setAvatar(const QImage &img)
{
	avatar_image_ = img;
	userAvatar_->setImage(img);
}

void UserInfoWidget::setDisplayName(const QString &name)
{
	if (name.isEmpty())
		display_name_ = user_id_.split(':')[0].split('@')[1];
	else
		display_name_ = name;

	displayNameLabel_->setText(display_name_);
	userAvatar_->setLetter(QChar(display_name_[0]));
}

void UserInfoWidget::setUserId(const QString &userid)
{
	user_id_ = userid;
	userIdLabel_->setText(userid);
}
