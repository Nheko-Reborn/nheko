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
#include <QPainter>
#include <QStyleOption>

#include "TextInputWidget.h"

TextInputWidget::TextInputWidget(QWidget *parent)
    : QWidget(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setCursor(Qt::ArrowCursor);
	setStyleSheet("background-color: #f8fbfe; height: 45px;");

	top_layout_ = new QHBoxLayout();
	top_layout_->setSpacing(6);
	top_layout_->setContentsMargins(6, 0, 0, 0);

	send_file_button_ = new FlatButton(this);
	send_file_button_->setCursor(Qt::PointingHandCursor);

	QIcon send_file_icon;
	send_file_icon.addFile(":/icons/icons/clip-dark.png", QSize(), QIcon::Normal, QIcon::Off);
	send_file_button_->setForegroundColor(QColor("#acc7dc"));
	send_file_button_->setIcon(send_file_icon);
	send_file_button_->setIconSize(QSize(24, 24));

	input_ = new QLineEdit(this);
	input_->setPlaceholderText("Write a message...");
	input_->setStyleSheet("color: black; font-size: 10pt; border-radius: 0; padding: 2px; margin-bottom: 4px;");

	send_message_button_ = new FlatButton(this);
	send_message_button_->setCursor(Qt::PointingHandCursor);
	send_message_button_->setForegroundColor(QColor("#acc7dc"));

	QIcon send_message_icon;
	send_message_icon.addFile(":/icons/icons/share-dark.png", QSize(), QIcon::Normal, QIcon::Off);
	send_message_button_->setIcon(send_message_icon);
	send_message_button_->setIconSize(QSize(24, 24));

	top_layout_->addWidget(send_file_button_);
	top_layout_->addWidget(input_);
	top_layout_->addWidget(send_message_button_);

	setLayout(top_layout_);

	connect(send_message_button_, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
	connect(input_, SIGNAL(returnPressed()), send_message_button_, SIGNAL(clicked()));
}

void TextInputWidget::onSendButtonClicked()
{
	auto msg_text = input_->text().trimmed();

	if (msg_text.isEmpty())
		return;

	emit sendTextMessage(msg_text);
	input_->clear();
}

void TextInputWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QStyleOption option;
	option.initFrom(this);

	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

TextInputWidget::~TextInputWidget()
{
}
