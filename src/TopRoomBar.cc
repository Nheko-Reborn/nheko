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
#include <QStyleOption>

#include "Avatar.h"
#include "Config.h"
#include "FlatButton.h"
#include "MainWindow.h"
#include "Menu.h"
#include "OverlayModal.h"
#include "TopRoomBar.h"
#include "Utils.h"

TopRoomBar::TopRoomBar(QWidget *parent)
  : QWidget(parent)
  , buttonSize_{32}
{
        setFixedHeight(60);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(8);
        topLayout_->setMargin(8);

        avatar_ = new Avatar(this);
        avatar_->setLetter("");
        avatar_->setSize(35);

        textLayout_ = new QVBoxLayout();
        textLayout_->setSpacing(0);
        textLayout_->setContentsMargins(0, 0, 0, 0);

        QFont roomFont("Open Sans SemiBold");
        roomFont.setPixelSize(conf::topRoomBar::fonts::roomName);

        nameLabel_ = new QLabel(this);
        nameLabel_->setFont(roomFont);
        nameLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        QFont descriptionFont("Open Sans");
        descriptionFont.setPixelSize(conf::topRoomBar::fonts::roomDescription);

        topicLabel_ = new QLabel(this);
        topicLabel_->setFont(descriptionFont);
        topicLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        topicLabel_->setOpenExternalLinks(true);
        topicLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        textLayout_->addWidget(nameLabel_);
        textLayout_->addWidget(topicLabel_);

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setFixedSize(buttonSize_, buttonSize_);
        settingsBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon settings_icon;
        settings_icon.addFile(":/icons/icons/ui/vertical-ellipsis.png");
        settingsBtn_->setIcon(settings_icon);
        settingsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_, 1);
        topLayout_->addWidget(settingsBtn_, 0, Qt::AlignRight);

        menu_ = new Menu(this);

        inviteUsers_ = new QAction(tr("Invite users"), this);
        connect(inviteUsers_, &QAction::triggered, this, [this]() {
                MainWindow::instance()->openInviteUsersDialog(
                  [this](const QStringList &invitees) { emit inviteUsers(invitees); });
        });

        roomMembers_ = new QAction(tr("Members"), this);
        connect(roomMembers_, &QAction::triggered, this, []() {
                MainWindow::instance()->openMemberListDialog();
        });

        leaveRoom_ = new QAction(tr("Leave room"), this);
        connect(leaveRoom_, &QAction::triggered, this, []() {
                MainWindow::instance()->openLeaveRoomDialog();
        });

        roomSettings_ = new QAction(tr("Settings"), this);
        connect(roomSettings_, &QAction::triggered, this, []() {
                MainWindow::instance()->openRoomSettings();
        });

        menu_->addAction(inviteUsers_);
        menu_->addAction(roomMembers_);
        menu_->addAction(leaveRoom_);
        menu_->addAction(roomSettings_);

        connect(settingsBtn_, &QPushButton::clicked, this, [this]() {
                auto pos = mapToGlobal(settingsBtn_->pos());
                menu_->popup(
                  QPoint(pos.x() + buttonSize_ - menu_->sizeHint().width(), pos.y() + buttonSize_));
        });
}

void
TopRoomBar::updateRoomAvatarFromName(const QString &name)
{
        avatar_->setLetter(utils::firstChar(name));
        update();
}

void
TopRoomBar::reset()
{
        nameLabel_->setText("");
        topicLabel_->setText("");
        avatar_->setLetter("");
}

void
TopRoomBar::updateRoomAvatar(const QImage &avatar_image)
{
        avatar_->setImage(avatar_image);
        update();
}

void
TopRoomBar::updateRoomAvatar(const QIcon &icon)
{
        avatar_->setIcon(icon);
        update();
}

void
TopRoomBar::updateRoomName(const QString &name)
{
        nameLabel_->setText(name);
        update();
}

void
TopRoomBar::updateRoomTopic(QString topic)
{
        topic.replace(conf::strings::url_regex, conf::strings::url_html);
        topicLabel_->setText(topic);
        update();
}
