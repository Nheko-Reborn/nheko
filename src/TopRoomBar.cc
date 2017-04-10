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
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumSize(QSize(0, 70));
	setStyleSheet("background-color: #171919; color: #ebebeb;");

	top_layout_ = new QHBoxLayout();
	top_layout_->setSpacing(10);
	top_layout_->setContentsMargins(10, 10, 0, 10);

	avatar_ = new Avatar(this);
	avatar_->setLetter(QChar('?'));
	avatar_->setBackgroundColor(QColor("#ebebeb"));
	avatar_->setSize(45);

	text_layout_ = new QVBoxLayout();
	text_layout_->setSpacing(0);
	text_layout_->setContentsMargins(0, 0, 0, 0);

	name_label_ = new QLabel(this);
	name_label_->setStyleSheet("font-size: 11pt;");

	topic_label_ = new QLabel(this);
	topic_label_->setStyleSheet("font-size: 10pt; color: #6c7278;");

	text_layout_->addWidget(name_label_);
	text_layout_->addWidget(topic_label_);

	settings_button_ = new FlatButton(this);
	settings_button_->setForegroundColor(QColor("#ebebeb"));
	settings_button_->setCursor(QCursor(Qt::PointingHandCursor));
	settings_button_->setStyleSheet("width: 30px; height: 30px;");

	QIcon settings_icon;
	settings_icon.addFile(":/icons/icons/cog.png", QSize(), QIcon::Normal, QIcon::Off);
	settings_button_->setIcon(settings_icon);
	settings_button_->setIconSize(QSize(16, 16));

	search_button_ = new FlatButton(this);
	search_button_->setForegroundColor(QColor("#ebebeb"));
	search_button_->setCursor(QCursor(Qt::PointingHandCursor));
	search_button_->setStyleSheet("width: 30px; height: 30px;");

	QIcon search_icon;
	search_icon.addFile(":/icons/icons/search.png", QSize(), QIcon::Normal, QIcon::Off);
	search_button_->setIcon(search_icon);
	search_button_->setIconSize(QSize(16, 16));

	top_layout_->addWidget(avatar_);
	top_layout_->addLayout(text_layout_);
	top_layout_->addStretch(1);
	top_layout_->addWidget(search_button_);
	top_layout_->addWidget(settings_button_);

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

TopRoomBar::~TopRoomBar()
{
}
