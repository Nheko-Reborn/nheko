// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommunitiesListItem.h"

#include <QMenu>
#include <QMouseEvent>

#include "Utils.h"
#include "ui/Painter.h"
#include "ui/Ripple.h"
#include "ui/RippleOverlay.h"

CommunitiesListItem::CommunitiesListItem(QString group_id, QWidget *parent)
  : QWidget(parent)
  , groupId_(group_id)
{
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        QPainterPath path;
        path.addRect(0, 0, parent->width(), height());
        rippleOverlay_ = new RippleOverlay(this);
        rippleOverlay_->setClipPath(path);
        rippleOverlay_->setClipping(true);

        menu_ = new QMenu(this);
        hideRoomsWithTagAction_ =
          new QAction(tr("Hide rooms with this tag or from this community"), this);
        hideRoomsWithTagAction_->setCheckable(true);
        menu_->addAction(hideRoomsWithTagAction_);
        connect(menu_, &QMenu::aboutToShow, this, [this]() {
                hideRoomsWithTagAction_->setChecked(isDisabled_);
        });

        connect(hideRoomsWithTagAction_, &QAction::triggered, this, [this](bool checked) {
                this->setDisabled(checked);
        });

        updateTooltip();
}

void
CommunitiesListItem::contextMenuEvent(QContextMenuEvent *event)
{
        menu_->popup(event->globalPos());
}

void
CommunitiesListItem::setName(QString name)
{
        name_ = name;
        updateTooltip();
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
CommunitiesListItem::setDisabled(bool state)
{
        if (isDisabled_ != state) {
                isDisabled_ = state;
                update();
                emit isDisabledChanged();
        }
}

void
CommunitiesListItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit clicked(groupId_);

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
        else if (isDisabled_)
                p.fillRect(rect(), disabledBackgroundColor_);
        else if (underMouse())
                p.fillRect(rect(), hoverBackgroundColor_);
        else
                p.fillRect(rect(), backgroundColor_);

        if (avatar_.isNull()) {
                QPixmap source;
                if (groupId_ == "world")
                        source = QPixmap(":/icons/icons/ui/world.png");
                else if (groupId_ == "tag:m.favourite")
                        source = QPixmap(":/icons/icons/ui/star.png");
                else if (groupId_ == "tag:m.lowpriority")
                        source = QPixmap(":/icons/icons/ui/lowprio.png");
                else if (groupId_.startsWith("tag:"))
                        source = QPixmap(":/icons/icons/ui/tag.png");

                if (source.isNull()) {
                        QFont font;
                        font.setPointSizeF(font.pointSizeF() * 1.3);
                        p.setFont(font);

                        p.drawLetterAvatar(utils::firstChar(resolveName()),
                                           avatarFgColor_,
                                           avatarBgColor_,
                                           width(),
                                           height(),
                                           IconSize);
                } else {
                        QPainter painter(&source);
                        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                        painter.fillRect(source.rect(), avatarFgColor_);
                        painter.end();

                        const int imageSz = 32;
                        p.drawPixmap(
                          QRect(
                            (width() - imageSz) / 2, (height() - imageSz) / 2, imageSz, imageSz),
                          source);
                }
        } else {
                p.save();

                p.drawAvatar(avatar_, width(), height(), IconSize);
                p.restore();
        }
}

void
CommunitiesListItem::setAvatar(const QImage &img)
{
        avatar_ = utils::scaleImageToPixmap(img, IconSize);
        update();
}

QString
CommunitiesListItem::resolveName() const
{
        if (!name_.isEmpty())
                return name_;
        if (groupId_.startsWith("tag:"))
                return groupId_.right(static_cast<int>(groupId_.size() - strlen("tag:")));
        if (!groupId_.startsWith("+"))
                return QString("Group"); // Group with no name or id.

        // Extract the localpart of the group.
        auto firstPart = groupId_.split(':').at(0);
        return firstPart.right(firstPart.size() - 1);
}

void
CommunitiesListItem::updateTooltip()
{
        if (groupId_ == "world")
                setToolTip(tr("All rooms"));
        else if (is_tag()) {
                QStringRef tag =
                  groupId_.rightRef(static_cast<int>(groupId_.size() - strlen("tag:")));
                if (tag == "m.favourite")
                        setToolTip(tr("Favourite rooms"));
                else if (tag == "m.lowpriority")
                        setToolTip(tr("Low priority rooms"));
                else if (tag == "m.server_notice")
                        setToolTip(tr("Server Notices", "Tag translation for m.server_notice"));
                else if (tag.startsWith("u."))
                        setToolTip(tag.right(tag.size() - 2) + tr(" (tag)"));
                else
                        setToolTip(tag + tr(" (tag)"));
        } else {
                QString name = resolveName();
                setToolTip(name + tr(" (community)"));
        }
}
