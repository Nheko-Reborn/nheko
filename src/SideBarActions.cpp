#include <QDebug>
#include <QIcon>

#include <mtx/requests.hpp>

#include "Config.h"
#include "MainWindow.h"
#include "SideBarActions.h"
#include "ui/FlatButton.h"
#include "ui/Menu.h"
#include "ui/OverlayModal.h"
#include "ui/Theme.h"

SideBarActions::SideBarActions(QWidget *parent)
  : QWidget{parent}
{
        setFixedHeight(conf::sidebarActions::height);

        layout_ = new QHBoxLayout(this);
        layout_->setMargin(0);

        QIcon settingsIcon;
        settingsIcon.addFile(":/icons/icons/ui/settings.png");

        QIcon createRoomIcon;
        createRoomIcon.addFile(":/icons/icons/ui/add-square-button.png");

        QIcon joinRoomIcon;
        joinRoomIcon.addFile(":/icons/icons/ui/speech-bubbles-comment-option.png");

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setToolTip(tr("User settings"));
        settingsBtn_->setIcon(settingsIcon);
        settingsBtn_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        settingsBtn_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        addMenu_          = new Menu(this);
        createRoomAction_ = new QAction(tr("Create new room"), this);
        joinRoomAction_   = new QAction(tr("Join a room"), this);

        connect(joinRoomAction_, &QAction::triggered, this, [this]() {
                MainWindow::instance()->openJoinRoomDialog(
                  [this](const QString &room_id) { emit joinRoom(room_id); });
        });

        connect(createRoomAction_, &QAction::triggered, this, [this]() {
                MainWindow::instance()->openCreateRoomDialog(
                  [this](const mtx::requests::CreateRoom &req) { emit createRoom(req); });
        });

        addMenu_->addAction(createRoomAction_);
        addMenu_->addAction(joinRoomAction_);

        createRoomBtn_ = new FlatButton(this);
        createRoomBtn_->setToolTip(tr("Start a new chat"));
        createRoomBtn_->setIcon(createRoomIcon);
        createRoomBtn_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        createRoomBtn_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        connect(createRoomBtn_, &QPushButton::clicked, this, [this]() {
                auto pos     = mapToGlobal(createRoomBtn_->pos());
                auto padding = conf::sidebarActions::iconSize / 2;

                addMenu_->popup(
                  QPoint(pos.x() + padding, pos.y() - padding - addMenu_->sizeHint().height()));
        });

        roomDirectory_ = new FlatButton(this);
        roomDirectory_->setToolTip(tr("Room directory"));
        roomDirectory_->setEnabled(false);
        roomDirectory_->setIcon(joinRoomIcon);
        roomDirectory_->setCornerRadius(conf::sidebarActions::iconSize / 2);
        roomDirectory_->setIconSize(
          QSize(conf::sidebarActions::iconSize, conf::sidebarActions::iconSize));

        layout_->addWidget(createRoomBtn_);
        layout_->addWidget(roomDirectory_);
        layout_->addWidget(settingsBtn_);

        connect(settingsBtn_, &QPushButton::clicked, this, &SideBarActions::showSettings);
}

void
SideBarActions::resizeEvent(QResizeEvent *event)
{
        Q_UNUSED(event);

        if (width() <= ui::sidebar::SmallSize) {
                roomDirectory_->hide();
                createRoomBtn_->hide();
        } else {
                roomDirectory_->show();
                createRoomBtn_->show();
        }
}

void
SideBarActions::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
