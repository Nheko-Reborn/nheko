#include "CommunitiesListItem.h"
#include "Painter.h"
#include "Ripple.h"
#include "RippleOverlay.h"
#include "Utils.h"

CommunitiesListItem::CommunitiesListItem(QSharedPointer<Community> community,
                                         QString community_id,
                                         QWidget *parent)
  : QWidget(parent)
  , community_(community)
  , communityId_(community_id)
{
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        QPainterPath path;
        path.addRect(0, 0, parent->width(), height());
        rippleOverlay_ = new RippleOverlay(this);
        rippleOverlay_->setClipPath(path);
        rippleOverlay_->setClipping(true);

        if (communityId_ == "world")
                communityAvatar_ = QPixmap(":/icons/icons/ui/world.svg");
}

void
CommunitiesListItem::setPressedState(bool state)
{
        if (isPressed_ != state) {
                isPressed_ = state;
                update();
        }
}

void
CommunitiesListItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit clicked(communityId_);

        setPressedState(true);

        QPoint pos           = event->pos();
        qreal radiusEndValue = static_cast<qreal>(width()) / 3;

        auto ripple = new Ripple(pos);
        ripple->setRadiusEndValue(radiusEndValue);
        ripple->setOpacityStartValue(0.15);
        ripple->setColor("white");
        ripple->radiusAnimation()->setDuration(200);
        ripple->opacityAnimation()->setDuration(400);
        rippleOverlay_->addRipple(ripple);
}

void
CommunitiesListItem::paintEvent(QPaintEvent *)
{
        Painter p(this);
        PainterHighQualityEnabler hq(p);

        if (isPressed_)
                p.fillRect(rect(), highlightedBackgroundColor_);
        else if (underMouse())
                p.fillRect(rect(), hoverBackgroundColor_);
        else
                p.fillRect(rect(), backgroundColor_);

        if (communityAvatar_.isNull()) {
                QFont font;
                font.setPixelSize(conf::roomlist::fonts::communityBubble);
                p.setFont(font);

                p.drawLetterAvatar(utils::firstChar(community_->getName()),
                                   avatarFgColor_,
                                   avatarBgColor_,
                                   width(),
                                   height(),
                                   IconSize);
        } else {
                p.save();

                p.drawAvatar(communityAvatar_, width(), height(), IconSize);
                p.restore();
        }
}

void
CommunitiesListItem::setAvatar(const QImage &img)
{
        communityAvatar_ = utils::scaleImageToPixmap(img, IconSize);
        update();
}
