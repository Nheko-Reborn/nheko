/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMouseEvent>
#include <QPainter>

#include "Config.h"
#include "Menu.h"
#include "Ripple.h"
#include "RippleOverlay.h"
#include "RoomInfoListItem.h"
#include "RoomSettings.h"
#include "Theme.h"

RoomInfoListItem::RoomInfoListItem(QSharedPointer<RoomSettings> settings,
                                   RoomState state,
                                   QString room_id,
                                   QWidget *parent)
  : QWidget(parent)
  , state_(state)
  , roomId_(room_id)
  , roomSettings_{settings}
  , isPressed_(false)
  , maxHeight_(IconSize + 2 * Padding)
  , unreadMsgCount_(0)
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        setFixedHeight(maxHeight_);

        QPainterPath path;
        path.addRect(0, 0, parent->width(), height());

        ripple_overlay_ = new RippleOverlay(this);
        ripple_overlay_->setClipPath(path);
        ripple_overlay_->setClipping(true);

        menu_ = new Menu(this);

        toggleNotifications_ = new QAction(notificationText(), this);
        connect(toggleNotifications_, &QAction::triggered, this, [=]() {
                roomSettings_->toggleNotifications();
        });

        leaveRoom_ = new QAction(tr("Leave room"), this);
        connect(leaveRoom_, &QAction::triggered, this, [=]() { emit leaveRoom(room_id); });

        menu_->addAction(toggleNotifications_);
        menu_->addAction(leaveRoom_);
}

QString
RoomInfoListItem::notificationText()
{
        if (roomSettings_.isNull() || roomSettings_->isNotificationsEnabled())
                return QString(tr("Disable notifications"));

        return tr("Enable notifications");
}

void
RoomInfoListItem::resizeEvent(QResizeEvent *)
{
        // Update ripple's clipping path.
        QPainterPath path;
        path.addRect(0, 0, width(), height());

        ripple_overlay_->setClipPath(path);
        ripple_overlay_->setClipping(true);
}

void
RoomInfoListItem::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);

        QFont font;
        font.setPixelSize(conf::fontSize);
        QFontMetrics metrics(font);

        if (isPressed_) {
                p.fillRect(rect(), highlightedBackgroundColor_);
        } else if (underMouse()) {
                p.fillRect(rect(), hoverBackgroundColor_);
        } else {
                p.fillRect(rect(), backgroundColor_);
        }

        // p.setPen(QColor("#333"));

        QRect avatarRegion(Padding, Padding, IconSize, IconSize);

        // Description line with the default font.
        int bottom_y = maxHeight_ - Padding - Padding / 3 - metrics.ascent() / 2;

        if (width() > ui::sidebar::SmallSize) {
                if (isPressed_) {
                        QPen pen(highlightedTitleColor_);
                        p.setPen(pen);
                } else {
                        QPen pen(titleColor_);
                        p.setPen(pen);
                }
                font.setPixelSize(conf::roomlist::fonts::heading);
                p.setFont(font);

                // Name line.
                QFontMetrics fontNameMetrics(font);
                int top_y = 2 * Padding + fontNameMetrics.ascent() / 2;

                auto name = metrics.elidedText(
                  state_.getName(), Qt::ElideRight, (width() - IconSize - 2 * Padding) * 0.8);
                p.drawText(QPoint(2 * Padding + IconSize, top_y), name);

                if (isPressed_) {
                        QPen pen(highlightedSubtitleColor_);
                        p.setPen(pen);
                } else {
                        QPen pen(subtitleColor_);
                        p.setPen(pen);
                }

                font.setPixelSize(conf::fontSize);
                p.setFont(font);

                auto msgStampWidth = QFontMetrics(font).width(lastMsgInfo_.timestamp) + 5;

                // The limit is the space between the end of the avatar and the start of the
                // timestamp.
                int usernameLimit =
                  std::max(0, width() - 3 * Padding - msgStampWidth - IconSize - 20);
                auto userName =
                  metrics.elidedText(lastMsgInfo_.username, Qt::ElideRight, usernameLimit);

                font.setBold(true);
                p.setFont(font);
                p.drawText(QPoint(2 * Padding + IconSize, bottom_y), userName);

                int nameWidth = QFontMetrics(font).width(userName);

                font.setBold(false);
                p.setFont(font);

                // The limit is the space between the end of the username and the start of
                // the timestamp.
                int descriptionLimit =
                  std::max(0, width() - 3 * Padding - msgStampWidth - IconSize - nameWidth - 5);
                auto description =
                  metrics.elidedText(lastMsgInfo_.body, Qt::ElideRight, descriptionLimit);
                p.drawText(QPoint(2 * Padding + IconSize + nameWidth, bottom_y), description);

                // We either show the bubble or the last message timestamp.
                if (unreadMsgCount_ == 0) {
                        font.setBold(true);
                        p.drawText(QPoint(width() - Padding - msgStampWidth, bottom_y),
                                   lastMsgInfo_.timestamp);
                }
        }

        font.setBold(false);
        p.setPen(Qt::NoPen);

        // We using the first letter of room's name.
        if (roomAvatar_.isNull()) {
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor("#eee");

                p.setPen(Qt::NoPen);
                p.setBrush(brush);

                p.drawEllipse(avatarRegion.center(), IconSize / 2, IconSize / 2);

                font.setPixelSize(conf::roomlist::fonts::bubble);
                p.setFont(font);
                p.setPen(QColor("#333"));
                p.setBrush(Qt::NoBrush);
                p.drawText(
                  avatarRegion.translated(0, -1), Qt::AlignCenter, QChar(state_.getName()[0]));
        } else {
                p.save();

                QPainterPath path;
                path.addEllipse(Padding, Padding, IconSize, IconSize);
                p.setClipPath(path);

                p.drawPixmap(avatarRegion, roomAvatar_);
                p.restore();
        }

        if (unreadMsgCount_ > 0) {
                QColor textColor("white");
                QColor backgroundColor("#38A3D8");

                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(backgroundColor);

                if (isPressed_)
                        brush.setColor(textColor);

                QFont unreadCountFont;
                unreadCountFont.setPixelSize(conf::roomlist::fonts::badge);
                unreadCountFont.setBold(true);

                p.setBrush(brush);
                p.setPen(Qt::NoPen);
                p.setFont(unreadCountFont);

                int diameter = 20;

                QRectF r(
                  width() - diameter - Padding, bottom_y - diameter / 2 - 5, diameter, diameter);

                if (width() == ui::sidebar::SmallSize)
                        r = QRectF(
                          width() - diameter - 5, height() - diameter - 5, diameter, diameter);

                p.setPen(Qt::NoPen);
                p.drawEllipse(r);

                p.setPen(QPen(textColor));

                if (isPressed_)
                        p.setPen(QPen(backgroundColor));

                p.setBrush(Qt::NoBrush);
                p.drawText(
                  r.translated(0, -0.5), Qt::AlignCenter, QString::number(unreadMsgCount_));
        }
}

void
RoomInfoListItem::updateUnreadMessageCount(int count)
{
        unreadMsgCount_ += count;
        update();
}

void
RoomInfoListItem::clearUnreadMessageCount()
{
        unreadMsgCount_ = 0;
        update();
}

void
RoomInfoListItem::setPressedState(bool state)
{
        if (!isPressed_ && state) {
                isPressed_ = state;
                update();
        } else if (isPressed_ && !state) {
                isPressed_ = state;
                update();
        }
}

void
RoomInfoListItem::setState(const RoomState &new_state)
{
        state_ = new_state;
        update();
}

void
RoomInfoListItem::contextMenuEvent(QContextMenuEvent *event)
{
        Q_UNUSED(event);

        toggleNotifications_->setText(notificationText());
        menu_->popup(event->globalPos());
}

void
RoomInfoListItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() == Qt::RightButton) {
                QWidget::mousePressEvent(event);
                return;
        }

        emit clicked(roomId_);

        setPressedState(true);

        // Ripple on mouse position by default.
        QPoint pos           = event->pos();
        qreal radiusEndValue = static_cast<qreal>(width()) / 3;

        Ripple *ripple = new Ripple(pos);

        ripple->setRadiusEndValue(radiusEndValue);
        ripple->setOpacityStartValue(0.15);
        ripple->setColor(QColor("white"));
        ripple->radiusAnimation()->setDuration(200);
        ripple->opacityAnimation()->setDuration(400);

        ripple_overlay_->addRipple(ripple);
}

void
RoomInfoListItem::setAvatar(const QImage &img)
{
        roomAvatar_ = QPixmap::fromImage(
          img.scaled(IconSize, IconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        update();
}

void
RoomInfoListItem::setDescriptionMessage(const DescInfo &info)
{
        lastMsgInfo_ = info;
        update();
}

RoomInfoListItem::~RoomInfoListItem() {}
