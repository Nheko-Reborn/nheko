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
#include <QLayout>
#include <QStyleOption>

#include "Config.h"
#include "RaisedButton.h"
#include "WelcomePage.h"

WelcomePage::WelcomePage(QWidget *parent)
  : QWidget(parent)
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(20);

        QFont headingFont("Open Sans", 20);
        QFont subTitleFont("Open Sans", 19);

        QIcon icon;
        icon.addFile(":/logos/splash.png");

        auto logo_ = new QLabel(this);
        logo_->setPixmap(icon.pixmap(256));
        logo_->setAlignment(Qt::AlignCenter);

        QString heading(tr("Welcome to nheko! The desktop client for the Matrix protocol."));
        QString main(tr("Enjoy your stay!"));

        auto intoTxt_ = new QLabel(heading, this);
        intoTxt_->setFont(headingFont);
        intoTxt_->setContentsMargins(0, 20, 0, 0);

        auto subTitle = new QLabel(main, this);
        subTitle->setFont(subTitleFont);
        subTitle->setContentsMargins(0, 0, 0, 0);

        topLayout_->addStretch(1);
        topLayout_->addWidget(logo_);
        topLayout_->addWidget(intoTxt_, 0, Qt::AlignCenter);
        topLayout_->addWidget(subTitle, 0, Qt::AlignCenter);

        auto btnLayout_ = new QHBoxLayout();
        btnLayout_->setSpacing(20);
        btnLayout_->setContentsMargins(0, 20, 0, 20);

        registerBtn_ = new RaisedButton(tr("REGISTER"), this);
        registerBtn_->setBackgroundColor(QColor("#333333"));
        registerBtn_->setMinimumSize(240, 65);
        registerBtn_->setFontSize(conf::btn::fontSize);
        registerBtn_->setCornerRadius(conf::btn::cornerRadius);

        loginBtn_ = new RaisedButton(tr("LOGIN"), this);
        loginBtn_->setBackgroundColor(QColor("#333333"));
        loginBtn_->setMinimumSize(240, 65);
        loginBtn_->setFontSize(conf::btn::fontSize);
        loginBtn_->setCornerRadius(conf::btn::cornerRadius);

        btnLayout_->addStretch(1);
        btnLayout_->addWidget(registerBtn_);
        btnLayout_->addWidget(loginBtn_);
        btnLayout_->addStretch(1);

        topLayout_->addLayout(btnLayout_);
        topLayout_->addStretch(1);

        connect(registerBtn_, &QPushButton::clicked, this, &WelcomePage::userRegister);
        connect(loginBtn_, &QPushButton::clicked, this, &WelcomePage::userLogin);
}

void
WelcomePage::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
