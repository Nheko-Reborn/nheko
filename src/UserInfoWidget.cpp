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

#include <QTimer>

#include "Config.h"
#include "MainWindow.h"
#include "UserInfoWidget.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"
#include "ui/OverlayModal.h"

UserInfoWidget::UserInfoWidget(QWidget *parent)
  : QWidget(parent)
  , display_name_("User")
  , user_id_("@user:homeserver.org")
  , logoutButtonSize_{20}
{
        setFixedHeight(60);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(5);

        avatarLayout_ = new QHBoxLayout();
        textLayout_   = new QVBoxLayout();

        userAvatar_ = new Avatar(this);
        userAvatar_->setObjectName("userAvatar");
        userAvatar_->setLetter(QChar('?'));
        userAvatar_->setSize(45);

        QFont nameFont("Open Sans SemiBold");
        nameFont.setPixelSize(conf::userInfoWidget::fonts::displayName);

        displayNameLabel_ = new QLabel(this);
        displayNameLabel_->setFont(nameFont);
        displayNameLabel_->setObjectName("displayNameLabel");
        displayNameLabel_->setStyleSheet("padding: 0 9px; margin-bottom: -10px;");
        displayNameLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

        QFont useridFont("Open Sans");
        useridFont.setPixelSize(conf::userInfoWidget::fonts::userid);

        userIdLabel_ = new QLabel(this);
        userIdLabel_->setFont(useridFont);
        userIdLabel_->setObjectName("userIdLabel");
        userIdLabel_->setStyleSheet("padding: 0 8px 8px 8px;");
        userIdLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);

        avatarLayout_->addWidget(userAvatar_);
        textLayout_->addWidget(displayNameLabel_);
        textLayout_->addWidget(userIdLabel_);

        topLayout_->addLayout(avatarLayout_);
        topLayout_->addLayout(textLayout_);
        topLayout_->addStretch(1);

        buttonLayout_ = new QHBoxLayout();
        buttonLayout_->setSpacing(0);
        buttonLayout_->setMargin(0);

        logoutButton_ = new FlatButton(this);
        logoutButton_->setToolTip(tr("Logout"));
        logoutButton_->setCornerRadius(logoutButtonSize_ / 2);

        QIcon icon;
        icon.addFile(":/icons/icons/ui/power-button-off.png");

        logoutButton_->setIcon(icon);
        logoutButton_->setIconSize(QSize(logoutButtonSize_, logoutButtonSize_));

        buttonLayout_->addWidget(logoutButton_);

        topLayout_->addLayout(buttonLayout_);

        // Show the confirmation dialog.
        connect(logoutButton_, &QPushButton::clicked, this, []() {
                MainWindow::instance()->openLogoutDialog();
        });
}

void
UserInfoWidget::resizeEvent(QResizeEvent *event)
{
        Q_UNUSED(event);

        if (width() <= ui::sidebar::SmallSize) {
                topLayout_->setContentsMargins(0, 0, logoutButtonSize_ / 2 - 5 / 2, 0);

                userAvatar_->hide();
                displayNameLabel_->hide();
                userIdLabel_->hide();
        } else {
                topLayout_->setMargin(5);
                userAvatar_->show();
                displayNameLabel_->show();
                userIdLabel_->show();
        }

        QWidget::resizeEvent(event);
}

void
UserInfoWidget::reset()
{
        displayNameLabel_->setText("");
        userIdLabel_->setText("");
        userAvatar_->setLetter(QChar('?'));
}

void
UserInfoWidget::setAvatar(const QImage &img)
{
        avatar_image_ = img;
        userAvatar_->setImage(img);
        update();
}

void
UserInfoWidget::setDisplayName(const QString &name)
{
        if (name.isEmpty())
                display_name_ = user_id_.split(':')[0].split('@')[1];
        else
                display_name_ = name;

        displayNameLabel_->setText(display_name_);
        userAvatar_->setLetter(QChar(display_name_[0]));
        update();
}

void
UserInfoWidget::setUserId(const QString &userid)
{
        user_id_ = userid;
        userIdLabel_->setText(userid);
}

void
UserInfoWidget::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
