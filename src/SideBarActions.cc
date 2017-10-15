#include <QDebug>
#include <QIcon>

#include "Config.h"
#include "Theme.h"
#include <SideBarActions.h>

SideBarActions::SideBarActions(QWidget *parent)
  : QWidget{ parent }
{
        setFixedHeight(conf::sidebarActions::height);

        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setSizePolicy(sizePolicy);

        layout_ = new QHBoxLayout(this);
        layout_->setMargin(0);

        QIcon settingsIcon;
        settingsIcon.addFile(":/icons/icons/ui/settings.png");

        QIcon createRoomIcon;
        createRoomIcon.addFile(":/icons/icons/ui/add-square-button.png");

        QIcon joinRoomIcon;
        joinRoomIcon.addFile(":/icons/icons/ui/speech-bubbles-comment-option.png");

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setIcon(settingsIcon);
        settingsBtn_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        settingsBtn_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        createRoomBtn_ = new FlatButton(this);
        createRoomBtn_->setIcon(createRoomIcon);
        createRoomBtn_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        createRoomBtn_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        joinRoomBtn_ = new FlatButton(this);
        joinRoomBtn_->setIcon(joinRoomIcon);
        joinRoomBtn_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        joinRoomBtn_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        layout_->addWidget(createRoomBtn_);
        layout_->addWidget(joinRoomBtn_);
        layout_->addWidget(settingsBtn_);
}

SideBarActions::~SideBarActions() {}

void
SideBarActions::resizeEvent(QResizeEvent *event)
{
        Q_UNUSED(event);

        if (width() <= ui::sidebar::SmallSize) {
                joinRoomBtn_->hide();
                createRoomBtn_->hide();
        } else {
                joinRoomBtn_->show();
                createRoomBtn_->show();
        }
}
