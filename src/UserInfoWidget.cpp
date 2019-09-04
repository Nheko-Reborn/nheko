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

#include <iostream>

#include "Config.h"
#include "MainWindow.h"
#include "UserInfoWidget.h"
#include "Utils.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"
#include "ui/OverlayModal.h"

UserInfoWidget::UserInfoWidget(QWidget *parent)
  : QWidget(parent)
  , display_name_("User")
  , user_id_("@user:homeserver.org")
{
        QFont f;
        f.setPointSizeF(f.pointSizeF());

        const int fontHeight    = QFontMetrics(f).height();
        const int widgetMargin  = fontHeight / 3;
        const int contentHeight = fontHeight * 3;

        logoutButtonSize_ = std::min(fontHeight, 20);

        setFixedHeight(contentHeight + widgetMargin);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(widgetMargin);

        avatarLayout_ = new QHBoxLayout();
        textLayout_   = new QVBoxLayout();
        textLayout_->setSpacing(widgetMargin / 2);
        textLayout_->setContentsMargins(widgetMargin * 2, widgetMargin, widgetMargin, widgetMargin);

        userAvatar_ = new Avatar(this, fontHeight * 2.5);
        userAvatar_->setObjectName("userAvatar");
        userAvatar_->setLetter(QChar('?'));

        QFont nameFont;
        nameFont.setPointSizeF(nameFont.pointSizeF() * 1.1);
        nameFont.setWeight(QFont::Medium);

        displayNameLabel_ = new QLabel(this);
        displayNameLabel_->setFont(nameFont);
        displayNameLabel_->setObjectName("displayNameLabel");
        displayNameLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

        userIdLabel_ = new QLabel(this);
        userIdLabel_->setFont(f);
        userIdLabel_->setObjectName("userIdLabel");
        userIdLabel_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);

        avatarLayout_->addWidget(userAvatar_);
        textLayout_->addWidget(displayNameLabel_, 0, Qt::AlignBottom);
        textLayout_->addWidget(userIdLabel_, 0, Qt::AlignTop);

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

        const auto sz = utils::calculateSidebarSizes(QFont{});

        if (width() <= sz.small) {
                topLayout_->setContentsMargins(0, 0, logoutButtonSize_, 0);

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
        update();
}

void
UserInfoWidget::setAvatar(const QString &url)
{
        userAvatar_->setImage(url);
        update();
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
