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

#pragma once

#include <QWidget>

#include "RippleOverlay.h"
#include "RoomState.h"

class RoomInfoListItem : public QWidget
{
	Q_OBJECT

public:
	RoomInfoListItem(RoomState state, QString room_id, QWidget *parent = 0);
	~RoomInfoListItem();

	void updateUnreadMessageCount(int count);
	void clearUnreadMessageCount();
	void setState(const RoomState &state);

	inline bool isPressed() const;
	inline RoomState state() const;
	inline void setAvatar(const QImage &avatar_image);
	inline int unreadMessageCount() const;

signals:
	void clicked(const QString &room_id);

public slots:
	void setPressedState(bool state);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

private:
	const int Padding = 10;
	const int IconSize = 45;

	RippleOverlay *ripple_overlay_;

	RoomState state_;

	QString roomId_;
	QString roomName_;
	QString lastMessage_;
	QString lastTimestamp_;

	QPixmap roomAvatar_;

	bool isPressed_;

	int maxHeight_ = 60;
	int unreadMsgCount_ = 0;
};

inline int RoomInfoListItem::unreadMessageCount() const
{
	return unreadMsgCount_;
}

inline bool RoomInfoListItem::isPressed() const
{
	return isPressed_;
}

inline RoomState RoomInfoListItem::state() const
{
	return state_;
}

inline void RoomInfoListItem::setAvatar(const QImage &img)
{
	roomAvatar_ = QPixmap::fromImage(img.scaled(IconSize, IconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
