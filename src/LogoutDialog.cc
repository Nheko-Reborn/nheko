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

#include "LogoutDialog.h"
#include "Theme.h"

LogoutDialog::LogoutDialog(QWidget *parent)
    : QFrame(parent)
{
	setMaximumSize(400, 400);
	setStyleSheet("background-color: #f9f9f9");

	auto layout = new QVBoxLayout(this);
	layout->setSpacing(30);
	layout->setMargin(20);

	auto buttonLayout = new QHBoxLayout();
	buttonLayout->setSpacing(0);
	buttonLayout->setMargin(0);

	confirmBtn_ = new FlatButton("OK", this);
	confirmBtn_->setFontSize(12);

	cancelBtn_ = new FlatButton(tr("CANCEL"), this);
	cancelBtn_->setFontSize(12);

	buttonLayout->addStretch(1);
	buttonLayout->addWidget(confirmBtn_);
	buttonLayout->addWidget(cancelBtn_);

	auto label = new QLabel(tr("Logout. Are you sure?"), this);
	label->setFont(QFont("Open Sans", 14));
	label->setStyleSheet("color: #333333");

	layout->addWidget(label);
	layout->addLayout(buttonLayout);

	connect(confirmBtn_, &QPushButton::clicked, [=]() { emit closing(true); });
	connect(cancelBtn_, &QPushButton::clicked, [=]() { emit closing(false); });
}
