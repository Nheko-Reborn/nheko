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

#include <QLabel>
#include <QVBoxLayout>

#include "LoginSettings.h"

LoginSettings::LoginSettings(QWidget *parent)
    : QFrame(parent)
{
	setMaximumSize(400, 400);
	setStyleSheet("background-color: #f9f9f9");

	auto layout = new QVBoxLayout(this);
	layout->setSpacing(30);
	layout->setContentsMargins(20, 20, 20, 10);

	input_ = new TextField(this);
	input_->setTextColor("#555459");
	input_->setLabel("Homeserver's domain");
	input_->setInkColor("#333333");
	input_->setBackgroundColor("#f9f9f9");
	input_->setPlaceholderText("e.g matrix.domain.org:3434");

	submit_button_ = new FlatButton("OK", this);
	submit_button_->setBackgroundColor("black");
	submit_button_->setForegroundColor("black");
	submit_button_->setCursor(QCursor(Qt::PointingHandCursor));
	submit_button_->setFontSize(15);
	submit_button_->setFixedHeight(50);
	submit_button_->setCornerRadius(3);

	auto label = new QLabel("Advanced Settings", this);
	label->setStyleSheet("color: #333333");

	layout->addWidget(label);
	layout->addWidget(input_);
	layout->addWidget(submit_button_);

	setLayout(layout);

	connect(input_, SIGNAL(returnPressed()), submit_button_, SIGNAL(clicked()));
	connect(submit_button_, &QPushButton::clicked, [=]() {
		emit closing(input_->text());
	});
}
