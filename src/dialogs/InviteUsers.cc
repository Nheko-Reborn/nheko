#include <QDebug>
#include <QIcon>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>

#include "Config.h"
#include "FlatButton.h"
#include "TextField.h"

#include "InviteeItem.h"
#include "dialogs/InviteUsers.h"

#include "mtx.hpp"

using namespace dialogs;

InviteUsers::InviteUsers(QWidget *parent)
  : QFrame(parent)
{
        setMaximumSize(400, 350);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(30);
        layout->setMargin(20);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        confirmBtn_ = new FlatButton("INVITE", this);
        confirmBtn_->setFontSize(conf::btn::fontSize);

        cancelBtn_ = new FlatButton(tr("CANCEL"), this);
        cancelBtn_->setFontSize(conf::btn::fontSize);

        buttonLayout->addStretch(1);
        buttonLayout->addWidget(confirmBtn_);
        buttonLayout->addWidget(cancelBtn_);

        QFont font;
        font.setPixelSize(conf::headerFontSize);

        inviteeInput_ = new TextField(this);
        inviteeInput_->setLabel(tr("User ID to invite"));

        inviteeList_ = new QListWidget;
        inviteeList_->setFrameStyle(QFrame::NoFrame);
        inviteeList_->setSelectionMode(QAbstractItemView::NoSelection);
        inviteeList_->setAttribute(Qt::WA_MacShowFocusRect, 0);
        inviteeList_->setSpacing(5);

        errorLabel_ = new QLabel(this);
        errorLabel_->setAlignment(Qt::AlignCenter);
        font.setPixelSize(12);
        errorLabel_->setFont(font);

        layout->addWidget(inviteeInput_);
        layout->addWidget(errorLabel_);
        layout->addWidget(inviteeList_);
        layout->addLayout(buttonLayout);

        connect(inviteeInput_, &TextField::returnPressed, this, &InviteUsers::addUser);
        connect(confirmBtn_, &QPushButton::clicked, [=]() {
                emit closing(true, invitedUsers());

                inviteeInput_->clear();
                inviteeList_->clear();
                errorLabel_->hide();
        });

        connect(cancelBtn_, &QPushButton::clicked, [=]() {
                QStringList emptyList;
                emit closing(false, emptyList);

                inviteeInput_->clear();
                inviteeList_->clear();
                errorLabel_->hide();
        });
}

void
InviteUsers::addUser()
{
        auto user_id = inviteeInput_->text();

        try {
                namespace ids = mtx::identifiers;
                auto user     = ids::parse<ids::User>(user_id.toStdString());

                auto item    = new QListWidgetItem(inviteeList_);
                auto invitee = new InviteeItem(user, this);

                item->setSizeHint(invitee->minimumSizeHint());
                item->setFlags(Qt::NoItemFlags);
                item->setTextAlignment(Qt::AlignCenter);

                inviteeList_->setItemWidget(item, invitee);

                connect(invitee, &InviteeItem::removeItem, this, [this, item]() {
                        emit removeInvitee(item);
                });

                errorLabel_->hide();
                inviteeInput_->clear();
        } catch (std::exception &e) {
                errorLabel_->setText(e.what());
                errorLabel_->show();
        }
}

void
InviteUsers::removeInvitee(QListWidgetItem *item)
{
        int row     = inviteeList_->row(item);
        auto widget = inviteeList_->takeItem(row);

        inviteeList_->removeItemWidget(widget);
}

void
InviteUsers::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

QStringList
InviteUsers::invitedUsers() const
{
        QStringList users;

        for (int ii = 0; ii < inviteeList_->count(); ++ii) {
                auto item    = inviteeList_->item(ii);
                auto widget  = inviteeList_->itemWidget(item);
                auto invitee = qobject_cast<InviteeItem *>(widget);

                if (invitee)
                        users << invitee->userID();
                else
                        qDebug() << "Cast InviteeItem failed";
        }

        return users;
}
