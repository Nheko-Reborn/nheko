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

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

#include <variant.hpp>

#include "Config.h"
#include "Menu.h"
#include "Ripple.h"
#include "RippleOverlay.h"
#include "RoomInfoListItem.h"
#include "RoomSettings.h"
#include "Theme.h"
#include "Utils.h"

constexpr int Padding   = 9;
constexpr int IconSize  = 44;
constexpr int MaxHeight = IconSize + 2 * Padding;

constexpr int InviteBtnX = IconSize + 2 * Padding;
constexpr int InviteBtnY = IconSize / 2 + Padding;

void
RoomInfoListItem::init(QWidget *parent)
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        setFixedHeight(MaxHeight);

        QPainterPath path;
        path.addRect(0, 0, parent->width(), height());

        ripple_overlay_ = new RippleOverlay(this);
        ripple_overlay_->setClipPath(path);
        ripple_overlay_->setClipping(true);

        font_.setPixelSize(conf::fontSize - 1);

        usernameFont_ = font_;

        bubbleFont_ = font_;
        bubbleFont_.setPixelSize(conf::roomlist::fonts::bubble);

        unreadCountFont_.setPixelSize(conf::roomlist::fonts::badge);
        unreadCountFont_.setBold(true);

        timestampFont_ = font_;
        timestampFont_.setPixelSize(conf::roomlist::fonts::timestamp);
        timestampFont_.setBold(false);

        headingFont_ = font_;
        headingFont_.setPixelSize(conf::roomlist::fonts::heading - 1);
        headingFont_.setWeight(60);
}

RoomInfoListItem::RoomInfoListItem(QString room_id,
                                   mtx::responses::InvitedRoom room,
                                   QWidget *parent)
  : QWidget(parent)
  , roomType_{RoomType::Invited}
  , invitedRoom_{std::move(room)}
  , roomId_{std::move(room_id)}
{
        init(parent);

        roomAvatar_ = QString::fromStdString(invitedRoom_.avatar());
        roomName_   = QString::fromStdString(invitedRoom_.name());
}

RoomInfoListItem::RoomInfoListItem(QSharedPointer<RoomSettings> settings,
                                   QSharedPointer<RoomState> state,
                                   QString room_id,
                                   QWidget *parent)
  : QWidget(parent)
  , state_(state)
  , roomId_(room_id)
  , roomSettings_{settings}
  , isPressed_(false)
  , unreadMsgCount_(0)
{
        init(parent);

        menu_ = new Menu(this);

        toggleNotifications_ = new QAction(notificationText(), this);
        connect(toggleNotifications_, &QAction::triggered, this, [this]() {
                roomSettings_->toggleNotifications();
        });

        leaveRoom_ = new QAction(tr("Leave room"), this);
        connect(leaveRoom_, &QAction::triggered, this, [this]() { emit leaveRoom(roomId_); });

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

        QFontMetrics metrics(font_);

        QPen titlePen(titleColor_);
        QPen subtitlePen(subtitleColor_);

        if (isPressed_) {
                p.fillRect(rect(), highlightedBackgroundColor_);
                titlePen.setColor(highlightedTitleColor_);
                subtitlePen.setColor(highlightedSubtitleColor_);
        } else if (underMouse()) {
                p.fillRect(rect(), hoverBackgroundColor_);
        } else {
                p.fillRect(rect(), backgroundColor_);
        }

        QRect avatarRegion(Padding, Padding, IconSize, IconSize);

        // Description line with the default font.
        int bottom_y = MaxHeight - Padding - metrics.ascent() / 2;

        if (width() > ui::sidebar::SmallSize) {
                p.setFont(headingFont_);
                p.setPen(titlePen);

                const int msgStampWidth =
                  QFontMetrics(timestampFont_).width(lastMsgInfo_.timestamp) + 4;

                // We use the full width of the widget if there is no unread msg bubble.
                const int bottomLineWidthLimit = (unreadMsgCount_ > 0) ? msgStampWidth : 0;

                // Name line.
                QFontMetrics fontNameMetrics(headingFont_);
                int top_y = 2 * Padding + fontNameMetrics.ascent() / 2;

                const auto name =
                  metrics.elidedText(roomName(),
                                     Qt::ElideRight,
                                     (width() - IconSize - 2 * Padding - msgStampWidth) * 0.8);
                p.drawText(QPoint(2 * Padding + IconSize, top_y), name);

                if (roomType_ == RoomType::Joined) {
                        p.setFont(font_);
                        p.setPen(subtitlePen);

                        // The limit is the space between the end of the avatar and the start of the
                        // timestamp.
                        int usernameLimit =
                          std::max(0, width() - 3 * Padding - msgStampWidth - IconSize - 20);
                        auto userName =
                          metrics.elidedText(lastMsgInfo_.username, Qt::ElideRight, usernameLimit);

                        p.setFont(usernameFont_);
                        p.drawText(QPoint(2 * Padding + IconSize, bottom_y), userName);

                        int nameWidth = QFontMetrics(usernameFont_).width(userName);

                        p.setFont(font_);

                        // The limit is the space between the end of the username and the start of
                        // the timestamp.
                        int descriptionLimit = std::max(
                          0,
                          width() - 3 * Padding - bottomLineWidthLimit - IconSize - nameWidth - 5);
                        auto description =
                          metrics.elidedText(lastMsgInfo_.body, Qt::ElideRight, descriptionLimit);
                        p.drawText(QPoint(2 * Padding + IconSize + nameWidth, bottom_y),
                                   description);

                        // We show the last message timestamp.
                        p.save();
                        if (isPressed_)
                                p.setPen(QPen(highlightedTimestampColor_));
                        else
                                p.setPen(QPen(timestampColor_));

                        p.setFont(timestampFont_);
                        p.drawText(QPoint(width() - Padding - msgStampWidth, top_y),
                                   lastMsgInfo_.timestamp);
                        p.restore();
                } else {
                        int btnWidth = (width() - IconSize - 6 * Padding) / 2;

                        acceptBtnRegion_ = QRectF(InviteBtnX, InviteBtnY, btnWidth, 20);
                        declineBtnRegion_ =
                          QRectF(InviteBtnX + btnWidth + 2 * Padding, InviteBtnY, btnWidth, 20);

                        QPainterPath acceptPath;
                        acceptPath.addRoundedRect(acceptBtnRegion_, 10, 10);

                        p.setPen(Qt::NoPen);
                        p.fillPath(acceptPath, btnColor_);
                        p.drawPath(acceptPath);

                        QPainterPath declinePath;
                        declinePath.addRoundedRect(declineBtnRegion_, 10, 10);

                        p.setPen(Qt::NoPen);
                        p.fillPath(declinePath, btnColor_);
                        p.drawPath(declinePath);

                        p.setPen(QPen(btnTextColor_));
                        p.setFont(font_);
                        p.drawText(acceptBtnRegion_, Qt::AlignCenter, tr("Accept"));
                        p.drawText(declineBtnRegion_, Qt::AlignCenter, tr("Decline"));
                }
        }

        p.setPen(Qt::NoPen);

        // We using the first letter of room's name.
        if (roomAvatar_.isNull()) {
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(avatarBgColor());

                p.setPen(Qt::NoPen);
                p.setBrush(brush);

                p.drawEllipse(avatarRegion.center(), IconSize / 2, IconSize / 2);

                p.setFont(bubbleFont_);
                p.setPen(avatarFgColor());
                p.setBrush(Qt::NoBrush);
                p.drawText(
                  avatarRegion.translated(0, -1), Qt::AlignCenter, utils::firstChar(roomName()));
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

                p.setBrush(brush);
                p.setPen(Qt::NoPen);
                p.setFont(unreadCountFont_);

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
        unreadMsgCount_ = count;
        update();
}

void
RoomInfoListItem::setPressedState(bool state)
{
        if (isPressed_ != state) {
                isPressed_ = state;
                update();
        }
}

void
RoomInfoListItem::contextMenuEvent(QContextMenuEvent *event)
{
        Q_UNUSED(event);

        if (roomType_ == RoomType::Invited)
                return;

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

        if (roomType_ == RoomType::Invited) {
                const auto point = event->pos();

                if (acceptBtnRegion_.contains(point))
                        emit acceptInvite(roomId_);

                if (declineBtnRegion_.contains(point))
                        emit declineInvite(roomId_);

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
