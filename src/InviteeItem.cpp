#include <QHBoxLayout>
#include <QPushButton>

#include "InviteeItem.h"

constexpr int SidePadding = 10;

InviteeItem::InviteeItem(mtx::identifiers::User user, QWidget *parent)
  : QWidget{parent}
  , user_{QString::fromStdString(user.to_string())}
{
        auto topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(SidePadding, 0, 3 * SidePadding, 0);

        name_          = new QLabel(user_, this);
        removeUserBtn_ = new QPushButton(tr("Remove"), this);

        topLayout_->addWidget(name_);
        topLayout_->addWidget(removeUserBtn_, 0, Qt::AlignRight);

        connect(removeUserBtn_, &QPushButton::clicked, this, &InviteeItem::removeItem);
}
