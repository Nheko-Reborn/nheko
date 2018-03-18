#include <QHBoxLayout>

#include "FlatButton.h"
#include "InviteeItem.h"
#include "Theme.h"

constexpr int SidePadding = 10;
constexpr int IconSize    = 13;

InviteeItem::InviteeItem(mtx::identifiers::User user, QWidget *parent)
  : QWidget{parent}
  , user_{QString::fromStdString(user.to_string())}
{
        auto topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(SidePadding, 0, 3 * SidePadding, 0);

        QFont font;
        font.setPixelSize(15);

        name_ = new QLabel(user_, this);
        name_->setFont(font);

        QIcon removeUserIcon;
        removeUserIcon.addFile(":/icons/icons/ui/remove-symbol.png");

        removeUserBtn_ = new FlatButton(this);
        removeUserBtn_->setIcon(removeUserIcon);
        removeUserBtn_->setIconSize(QSize(IconSize, IconSize));
        removeUserBtn_->setFixedSize(QSize(IconSize, IconSize));
        removeUserBtn_->setRippleStyle(ui::RippleStyle::NoRipple);

        topLayout_->addWidget(name_);
        topLayout_->addWidget(removeUserBtn_);

        connect(removeUserBtn_, &FlatButton::clicked, this, &InviteeItem::removeItem);
}
