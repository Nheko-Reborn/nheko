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
#include <QFile>
#include <QPainter>
#include <QStyleOption>

#include "TextInputWidget.h"

FilteredTextEdit::FilteredTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

void FilteredTextEdit::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		emit enterPressed();
	else
		QTextEdit::keyPressEvent(event);
}

TextInputWidget::TextInputWidget(QWidget *parent)
    : QWidget(parent)
{
	setFont(QFont("Emoji One"));

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setCursor(Qt::ArrowCursor);
	setStyleSheet("background-color: #f8fbfe; height: 45px;");

	top_layout_ = new QHBoxLayout();
	top_layout_->setSpacing(0);
	top_layout_->setMargin(0);

	send_file_button_ = new FlatButton(this);
	send_file_button_->setCursor(Qt::PointingHandCursor);

	QIcon send_file_icon;
	send_file_icon.addFile(":/icons/icons/clip-dark.png", QSize(), QIcon::Normal, QIcon::Off);
	send_file_button_->setForegroundColor(QColor("#acc7dc"));
	send_file_button_->setIcon(send_file_icon);
	send_file_button_->setIconSize(QSize(24, 24));

	input_ = new FilteredTextEdit(this);
	input_->setFixedHeight(45);
	input_->setPlaceholderText("Write a message...");
	input_->setStyleSheet("color: #333333; font-size: 13px; border-radius: 0; padding-top: 10px;");

	send_message_button_ = new FlatButton(this);
	send_message_button_->setCursor(Qt::PointingHandCursor);
	send_message_button_->setForegroundColor(QColor("#acc7dc"));

	QIcon send_message_icon;
	send_message_icon.addFile(":/icons/icons/share-dark.png", QSize(), QIcon::Normal, QIcon::Off);
	send_message_button_->setIcon(send_message_icon);
	send_message_button_->setIconSize(QSize(24, 24));

	emoji_button_ = new EmojiPickButton(this);
	emoji_button_->setCursor(Qt::PointingHandCursor);
	emoji_button_->setForegroundColor(QColor("#acc7dc"));

	QIcon emoji_icon;
	emoji_icon.addFile(":/icons/icons/smile.png", QSize(), QIcon::Normal, QIcon::Off);
	emoji_button_->setIcon(emoji_icon);
	emoji_button_->setIconSize(QSize(24, 24));

	top_layout_->addWidget(send_file_button_);
	top_layout_->addWidget(input_);
	top_layout_->addWidget(emoji_button_);
	top_layout_->addWidget(send_message_button_);

	setLayout(top_layout_);

	connect(send_message_button_, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
	connect(input_, SIGNAL(enterPressed()), send_message_button_, SIGNAL(clicked()));
	connect(emoji_button_, SIGNAL(emojiSelected(const QString &)), this, SLOT(addSelectedEmoji(const QString &)));
}

void TextInputWidget::addSelectedEmoji(const QString &emoji)
{
	QTextCursor cursor = input_->textCursor();

	QFont emoji_font("Emoji One");
	emoji_font.setPixelSize(18);

	QFont text_font("Open Sans");
	text_font.setPixelSize(13);

	QTextCharFormat charfmt;
	charfmt.setFont(emoji_font);
	input_->setCurrentCharFormat(charfmt);

	input_->insertPlainText(emoji);
	cursor.movePosition(QTextCursor::End);

	charfmt.setFont(text_font);
	input_->setCurrentCharFormat(charfmt);

	input_->show();
}

void TextInputWidget::onSendButtonClicked()
{
	auto msg_text = input_->document()->toPlainText().trimmed();

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
