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

#include "Config.h"
#include "MainWindow.h"
#include "TopRoomBar.h"
#include "Utils.h"
#include "ui/Avatar.h"
#include "ui/FlatButton.h"
#include "ui/Menu.h"
#include "ui/OverlayModal.h"
#include "ui/TextLabel.h"

TopRoomBar::TopRoomBar(QWidget *parent)
  : QWidget(parent)
  , buttonSize_{32}
{
        QFont f;
        f.setPointSizeF(f.pointSizeF());

        const int fontHeight    = QFontMetrics(f).height();
        const int widgetMargin  = fontHeight / 3;
        const int contentHeight = fontHeight * 3;

        setFixedHeight(contentHeight + widgetMargin);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(widgetMargin);
        topLayout_->setContentsMargins(
          2 * widgetMargin, widgetMargin, 2 * widgetMargin, widgetMargin);

        avatar_ = new Avatar(this, fontHeight * 2);
        avatar_->setLetter("");

        textLayout_ = new QVBoxLayout();
        textLayout_->setSpacing(0);
        textLayout_->setMargin(0);

        QFont roomFont;
        roomFont.setPointSizeF(roomFont.pointSizeF() * 1.1);
        roomFont.setWeight(QFont::Medium);

        nameLabel_ = new QLabel(this);
        nameLabel_->setFont(roomFont);
        nameLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        QFont descriptionFont;

        topicLabel_ = new TextLabel(this);
        topicLabel_->setLineWrapMode(QTextEdit::NoWrap);
        topicLabel_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        topicLabel_->setFont(descriptionFont);
        topicLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        topicLabel_->setOpenExternalLinks(true);
        topicLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        textLayout_->addWidget(nameLabel_);
        textLayout_->addWidget(topicLabel_);

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setToolTip(tr("Room options"));
        settingsBtn_->setFixedSize(buttonSize_, buttonSize_);
        settingsBtn_->setCornerRadius(buttonSize_ / 2);

        mentionsBtn_ = new FlatButton(this);
        mentionsBtn_->setToolTip(tr("Mentions"));
        mentionsBtn_->setFixedSize(buttonSize_, buttonSize_);
        mentionsBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon settings_icon;
        settings_icon.addFile(":/icons/icons/ui/vertical-ellipsis.png");
        settingsBtn_->setIcon(settings_icon);
        settingsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

        QIcon mentions_icon;
        mentions_icon.addFile(":/icons/icons/ui/at-solid.svg");
        mentionsBtn_->setIcon(mentions_icon);
        mentionsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

        backBtn_ = new FlatButton(this);
        backBtn_->setFixedSize(buttonSize_, buttonSize_);
        backBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon backIcon;
        backIcon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");
        backBtn_->setIcon(backIcon);
        backBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));
        backBtn_->hide();

        connect(backBtn_, &QPushButton::clicked, this, &TopRoomBar::showRoomList);

        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(backBtn_);
        topLayout_->addLayout(textLayout_, 1);
        topLayout_->addWidget(mentionsBtn_, 0, Qt::AlignRight);
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

        connect(mentionsBtn_, &QPushButton::clicked, this, [this]() {
                auto pos = mapToGlobal(mentionsBtn_->pos());
                emit mentionsClicked(pos);
        });
}

void
TopRoomBar::enableBackButton()
{
        avatar_->hide();
        backBtn_->show();
}

void
TopRoomBar::disableBackButton()
{
        avatar_->show();
        backBtn_->hide();
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
TopRoomBar::updateRoomAvatar(const QString &avatar_image)
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
        topicLabel_->clearLinks();
        topicLabel_->setHtml(topic);
        update();
}
