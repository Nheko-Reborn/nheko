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

#pragma once

#include <QHBoxLayout>
#include <QPaintEvent>
#include <QTextEdit>
#include <QWidget>

#include "EmojiPickButton.h"
#include "FlatButton.h"

class FilteredTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	explicit FilteredTextEdit(QWidget *parent = nullptr);
	void keyPressEvent(QKeyEvent *event);

signals:
	void enterPressed();
};

class TextInputWidget : public QWidget
{
	Q_OBJECT

public:
	TextInputWidget(QWidget *parent = 0);
	~TextInputWidget();

public slots:
	void onSendButtonClicked();

private slots:
	void addSelectedEmoji(const QString &emoji);

signals:
	void sendTextMessage(QString msg);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QHBoxLayout *top_layout_;
	FilteredTextEdit *input_;

	FlatButton *send_file_button_;
	FlatButton *send_message_button_;
	EmojiPickButton *emoji_button_;

	const float TextFontRatio = 1.1;
	const float EmojiFontRatio = 1.3;
};
