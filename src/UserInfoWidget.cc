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
    , userid_("@user:homeserver.org")
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
	userAvatar_->setSize(50);
	userAvatar_->setMaximumSize(QSize(55, 55));

	displayNameLabel_ = new QLabel(this);
	displayNameLabel_->setStyleSheet(
		"padding: 0 9px;"
		"color: #ebebeb;"
		"font-size: 11pt;"
		"margin-bottom: -10px;");
	displayNameLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

	userIdLabel_ = new QLabel(this);
	userIdLabel_->setStyleSheet(
		"padding: 0 8px 8px 8px;"
		"color: #5D6565;"
		"font-size: 10pt;");
	userIdLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);

	avatarLayout_->addWidget(userAvatar_);
	textLayout_->addWidget(displayNameLabel_);
	textLayout_->addWidget(userIdLabel_);

	topLayout_->addLayout(avatarLayout_);
	topLayout_->addLayout(textLayout_);
	topLayout_->addStretch(1);

	buttonLayout_ = new QHBoxLayout();

	settingsButton_ = new FlatButton(this);
	settingsButton_->setForegroundColor(QColor("#ebebeb"));
	settingsButton_->setCursor(QCursor(Qt::PointingHandCursor));
	settingsButton_->setStyleSheet("width: 30px; height: 30px;");

	QIcon icon;
	icon.addFile(":/icons/icons/user-shape.png", QSize(), QIcon::Normal, QIcon::Off);

	settingsButton_->setIcon(icon);
	settingsButton_->setIconSize(QSize(16, 16));

	buttonLayout_->addWidget(settingsButton_);

	topLayout_->addLayout(buttonLayout_);
}

UserInfoWidget::~UserInfoWidget()
{
}

void UserInfoWidget::setAvatar(const QImage &img)
{
	avatar_image_ = img;
	userAvatar_->setImage(img);
}

void UserInfoWidget::setDisplayName(const QString &name)
{
	display_name_ = name;
	displayNameLabel_->setText(name);
}

void UserInfoWidget::setUserId(const QString &userid)
{
	userid_ = userid;
	userIdLabel_->setText(userid);
}
