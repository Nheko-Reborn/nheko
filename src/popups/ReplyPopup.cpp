#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "../Config.h"
#include "../Utils.h"
#include "../ui/Avatar.h"
#include "../ui/DropShadow.h"
#include "ReplyPopup.h"

ReplyPopup::ReplyPopup(QWidget *parent)
  : QWidget(parent)
{
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint);

        layout_ = new QVBoxLayout(this);
        layout_->setMargin(0);
        layout_->setSpacing(0);
}

void
ReplyPopup::setReplyContent(const QString &user, const QString &msg, const QString &srcEvent)
{
        QLayoutItem *child;
        while ((child = layout_->takeAt(0)) != 0) {
                delete child->widget();
                delete child;
        }
        // Create a new widget if there isn't already one in that
        // layout position.
        // if (!item) {
        auto userItem = new UserItem(this, user);
        auto *text    = new QLabel(this);
        text->setText(msg);
        auto *event = new QLabel(this);
        event->setText(srcEvent);
        connect(userItem, &UserItem::clicked, this, &ReplyPopup::userSelected);
        layout_->addWidget(userItem);
        layout_->addWidget(text);
        layout_->addWidget(event);
        // } else {
        // Update the current widget with the new data.
        // auto userWidget = qobject_cast<UserItem *>(item->widget());
        // if (userWidget)
        // userWidget->updateItem(users.at(i).user_id);
        // }

        adjustSize();
}

void
ReplyPopup::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
