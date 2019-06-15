#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

#include "../Config.h"
#include "../Utils.h"
#include "../ui/Avatar.h"
#include "../ui/DropShadow.h"
#include "../ui/TextLabel.h"
#include "ReplyPopup.h"

ReplyPopup::ReplyPopup(QWidget *parent)
  : QWidget(parent)
  , userItem_{0}
  , msgLabel_{0}
  , eventLabel_{0}
{
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint);

        mainLayout_ = new QVBoxLayout(this);
        mainLayout_->setMargin(0);
        mainLayout_->setSpacing(0);

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(0);
        topLayout_->setContentsMargins(13, 1, 13, 0);

        userItem_ = new UserItem(this);
        connect(userItem_, &UserItem::clicked, this, &ReplyPopup::userSelected);
        topLayout_->addWidget(userItem_);

        buttonLayout_ = new QHBoxLayout();
        buttonLayout_->setSpacing(0);
        buttonLayout_->setMargin(0);

        topLayout_->addLayout(buttonLayout_);
        QFont f;
        f.setPointSizeF(f.pointSizeF());
        const int fontHeight = QFontMetrics(f).height();
        buttonSize_          = std::min(fontHeight, 20);

        closeBtn_ = new FlatButton(this);
        closeBtn_->setToolTip(tr("Logout"));
        closeBtn_->setCornerRadius(buttonSize_ / 4);
        closeBtn_->setText("X");

        QIcon icon;
        icon.addFile(":/icons/icons/ui/remove-symbol.png");

        closeBtn_->setIcon(icon);
        closeBtn_->setIconSize(QSize(buttonSize_, buttonSize_));
        connect(closeBtn_, &FlatButton::clicked, this, [this]() { emit cancel(); });

        buttonLayout_->addWidget(closeBtn_);

        topLayout_->addLayout(buttonLayout_);

        mainLayout_->addLayout(topLayout_);
        msgLabel_ = new TextLabel(this);
        msgLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextBrowserInteraction);
        mainLayout_->addWidget(msgLabel_);
        eventLabel_ = new QLabel(this);
        mainLayout_->addWidget(eventLabel_);

        setLayout(mainLayout_);
}

void
ReplyPopup::setReplyContent(const RelatedInfo &related)
{
        // Update the current widget with the new data.
        userItem_->updateItem(related.quoted_user);

        msgLabel_->setText(utils::getFormattedQuoteBody(related, "")
                             .replace("<mx-reply>", "")
                             .replace("</mx-reply>", ""));

        // eventLabel_->setText(srcEvent);

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

void
ReplyPopup::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton) {
                emit clicked(eventLabel_->text());
        }

        QWidget::mousePressEvent(event);
}
