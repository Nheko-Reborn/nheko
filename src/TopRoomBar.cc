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

#include <QStyleOption>

#include "TopRoomBar.h"

TopRoomBar::TopRoomBar(QWidget *parent)
    : QWidget(parent)
    , buttonSize_{32}
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumSize(QSize(0, 65));
	setStyleSheet("background-color: #f8fbfe; color: #171919;");

	top_layout_ = new QHBoxLayout();
	top_layout_->setSpacing(10);
	top_layout_->setMargin(10);

	avatar_ = new Avatar(this);
	avatar_->setLetter(QChar('?'));
	avatar_->setBackgroundColor(QColor("#d6dde3"));
	avatar_->setTextColor(QColor("#555459"));
	avatar_->setSize(35);

	text_layout_ = new QVBoxLayout();
	text_layout_->setSpacing(0);
	text_layout_->setContentsMargins(0, 0, 0, 0);

	QFont font;
	font.setPointSize(this->font().pointSize() * RoomNameFontRatio);
	font.setBold(true);

	name_label_ = new QLabel(this);
	name_label_->setFont(font);

	font.setBold(false);
	font.setPointSize(this->font().pointSize() * RoomDescriptionFontRatio);

	topic_label_ = new QLabel(this);
	topic_label_->setFont(font);

	text_layout_->addWidget(name_label_);
	text_layout_->addWidget(topic_label_);

	settingsBtn_ = new FlatButton(this);
	settingsBtn_->setForegroundColor(QColor("#acc7dc"));
	settingsBtn_->setFixedSize(buttonSize_, buttonSize_);
	settingsBtn_->setCornerRadius(buttonSize_ / 2);

	QIcon settings_icon;
	settings_icon.addFile(":/icons/icons/vertical-ellipsis.png", QSize(), QIcon::Normal, QIcon::Off);
	settingsBtn_->setIcon(settings_icon);
	settingsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

	top_layout_->addWidget(avatar_);
	top_layout_->addLayout(text_layout_);
	top_layout_->addStretch(1);
	top_layout_->addWidget(settingsBtn_);

	menu_ = new Menu(this);

	toggleNotifications_ = new QAction(tr("Disable notifications"), this);
	connect(toggleNotifications_, &QAction::triggered, this, [=]() {
		roomSettings_->toggleNotifications();
	});

	menu_->addAction(toggleNotifications_);

	connect(settingsBtn_, &QPushButton::clicked, this, [=]() {
		if (roomSettings_->isNotificationsEnabled())
			toggleNotifications_->setText(tr("Disable notifications"));
		else
			toggleNotifications_->setText(tr("Enable notifications"));

		auto pos = mapToGlobal(settingsBtn_->pos());
		menu_->popup(QPoint(pos.x() + buttonSize_ - menu_->sizeHint().width(),
				    pos.y() + buttonSize_));
	});

	setLayout(top_layout_);
}

void TopRoomBar::updateRoomAvatarFromName(const QString &name)
{
	QChar letter = '?';

	if (name.size() > 0)
		letter = name[0];

	avatar_->setLetter(letter);
}

void TopRoomBar::reset()
{
	name_label_->setText("");
	topic_label_->setText("");
	avatar_->setLetter(QChar('?'));
}

void TopRoomBar::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QStyleOption option;
	option.initFrom(this);

	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

void TopRoomBar::setRoomSettings(QSharedPointer<RoomSettings> settings)
{
	roomSettings_ = settings;
}

TopRoomBar::~TopRoomBar()
{
}
