#include <QDebug>
#include <QIcon>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>

#include "dialogs/InviteUsers.h"

#include "Config.h"
#include "InviteeItem.h"
#include "ui/TextField.h"

#include "mtx.hpp"

using namespace dialogs;

InviteUsers::InviteUsers(QWidget *parent)
  : QFrame(parent)
{
        setAutoFillBackground(true);
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_DeleteOnClose, true);

        setMinimumWidth(conf::modals::MIN_WIDGET_WIDTH);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(conf::modals::WIDGET_SPACING);
        layout->setMargin(conf::modals::WIDGET_MARGIN);

        auto buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setMargin(0);

        confirmBtn_ = new QPushButton("Invite", this);
        cancelBtn_  = new QPushButton(tr("Cancel"), this);
        cancelBtn_->setDefault(true);

        buttonLayout->addStretch(1);
        buttonLayout->setSpacing(15);
        buttonLayout->addWidget(cancelBtn_);
        buttonLayout->addWidget(confirmBtn_);

        inviteeInput_ = new TextField(this);
        inviteeInput_->setLabel(tr("User ID to invite"));

        inviteeList_ = new QListWidget;
        inviteeList_->setFrameStyle(QFrame::NoFrame);
        inviteeList_->setSelectionMode(QAbstractItemView::NoSelection);
        inviteeList_->setAttribute(Qt::WA_MacShowFocusRect, 0);
        inviteeList_->setSpacing(5);

        errorLabel_ = new QLabel(this);
        errorLabel_->setAlignment(Qt::AlignCenter);

        layout->addWidget(inviteeInput_);
        layout->addWidget(errorLabel_);
        layout->addWidget(inviteeList_);
        layout->addLayout(buttonLayout);

        connect(inviteeInput_, &TextField::returnPressed, this, &InviteUsers::addUser);
        connect(confirmBtn_, &QPushButton::clicked, [this]() {
                emit sendInvites(invitedUsers());

                inviteeInput_->clear();
                inviteeList_->clear();
                errorLabel_->hide();
        });

        connect(cancelBtn_, &QPushButton::clicked, [this]() {
                inviteeInput_->clear();
                inviteeList_->clear();
                errorLabel_->hide();

                emit close();
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

void
InviteUsers::showEvent(QShowEvent *event)
{
        inviteeInput_->setFocus();

        QFrame::showEvent(event);
}
