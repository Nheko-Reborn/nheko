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

#include <QStyleOption>

#include "Config.h"
#include "MainWindow.h"
#include "TopRoomBar.h"

TopRoomBar::TopRoomBar(QWidget *parent)
  : QWidget(parent)
  , buttonSize_{ 32 }
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumSize(QSize(0, 65));
        setStyleSheet("background-color: #fff; color: #171919;");

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(10);
        topLayout_->setMargin(10);

        avatar_ = new Avatar(this);
        avatar_->setLetter(QChar('?'));
        avatar_->setBackgroundColor(QColor("#d6dde3"));
        avatar_->setTextColor(QColor("#555459"));
        avatar_->setSize(35);

        textLayout_ = new QVBoxLayout();
        textLayout_->setSpacing(0);
        textLayout_->setContentsMargins(0, 0, 0, 0);

        QFont roomFont("Open Sans SemiBold");
        roomFont.setPixelSize(conf::topRoomBar::fonts::roomName);

        nameLabel_ = new QLabel(this);
        nameLabel_->setFont(roomFont);

        QFont descriptionFont("Open Sans");
        descriptionFont.setPixelSize(conf::topRoomBar::fonts::roomDescription);

        topicLabel_ = new QLabel(this);
        topicLabel_->setFont(descriptionFont);
        topicLabel_->setTextFormat(Qt::RichText);
        topicLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        topicLabel_->setOpenExternalLinks(true);

        textLayout_->addWidget(nameLabel_);
        textLayout_->addWidget(topicLabel_);

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setForegroundColor(QColor("#acc7dc"));
        settingsBtn_->setFixedSize(buttonSize_, buttonSize_);
        settingsBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon settings_icon;
        settings_icon.addFile(
          ":/icons/icons/vertical-ellipsis.png", QSize(), QIcon::Normal, QIcon::Off);
        settingsBtn_->setIcon(settings_icon);
        settingsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_);
        topLayout_->addStretch(1);
        topLayout_->addWidget(settingsBtn_);

        menu_ = new Menu(this);

        toggleNotifications_ = new QAction(tr("Disable notifications"), this);
        connect(toggleNotifications_, &QAction::triggered, this, [=]() {
                roomSettings_->toggleNotifications();
        });

        leaveRoom_ = new QAction(tr("Leave room"), this);
        connect(leaveRoom_, &QAction::triggered, this, [=]() {
                leaveRoomDialog_ = new LeaveRoomDialog(this);
                connect(
                  leaveRoomDialog_, SIGNAL(closing(bool)), this, SLOT(closeLeaveRoomDialog(bool)));

                leaveRoomModal = new OverlayModal(MainWindow::instance(), leaveRoomDialog_);
                leaveRoomModal->setDuration(100);
                leaveRoomModal->setColor(QColor(55, 55, 55, 170));

                leaveRoomModal->fadeIn();
        });

        menu_->addAction(toggleNotifications_);
        menu_->addAction(leaveRoom_);

        connect(settingsBtn_, &QPushButton::clicked, this, [=]() {
                if (roomSettings_->isNotificationsEnabled())
                        toggleNotifications_->setText(tr("Disable notifications"));
                else
                        toggleNotifications_->setText(tr("Enable notifications"));

                auto pos = mapToGlobal(settingsBtn_->pos());
                menu_->popup(
                  QPoint(pos.x() + buttonSize_ - menu_->sizeHint().width(), pos.y() + buttonSize_));
        });

        setLayout(topLayout_);
}

void
TopRoomBar::closeLeaveRoomDialog(bool leaving)
{
        leaveRoomModal->fadeOut();

        if (leaving) {
                emit leaveRoom();
        }
}

void
TopRoomBar::updateRoomAvatarFromName(const QString &name)
{
        QChar letter = '?';

        if (name.size() > 0)
                letter = name[0];

        avatar_->setLetter(letter);
        update();
}

void
TopRoomBar::reset()
{
        nameLabel_->setText("");
        topicLabel_->setText("");
        avatar_->setLetter(QChar('?'));
}

void
TopRoomBar::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QStyleOption option;
        option.initFrom(this);

        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

void
TopRoomBar::setRoomSettings(QSharedPointer<RoomSettings> settings)
{
        roomSettings_ = settings;
}

TopRoomBar::~TopRoomBar() {}
